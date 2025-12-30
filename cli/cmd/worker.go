package cmd

import (
	"encoding/json"
	"errors"
	"fmt"
	"net"
	"os"
	"strings"
	"time"

	"github.com/spf13/cobra"
)

type Task struct {
	Name string `json:"name"`
}

type Job struct {
	ID       int    `json:"id,omitempty"`
	Tasks    []Task `json:"tasks,omitempty"`
	Parallel bool   `json:"parallel,omitempty"`
}

type CommandPayload struct {
	Command  string `json:"command"`
	Token    string `json:"token"`
	WorkerID int    `json:"worker_id,omitempty"`
	Job      *Job   `json:"job,omitempty"`
}

var (
	workerID int
	token    string
	sockPath string
)

func init() {
	workerCmd := &cobra.Command{
		Use:   "worker",
		Short: "Interact with a worker",
	}

	workerCmd.PersistentFlags().IntVar(&workerID, "id", 0, "Worker ID (required)")
	workerCmd.PersistentFlags().StringVar(&token, "token", "", "API token (required)")
	workerCmd.PersistentFlags().StringVar(&sockPath, "sock", defaultSockPath(), "Path to worker Unix socket")

	// Mark persistent flags required
	_ = workerCmd.MarkPersistentFlagRequired("id")
	_ = workerCmd.MarkPersistentFlagRequired("token")

	workerCmd.AddCommand(
		newRunCmd(),
		newAssignCmd(),
		newSimpleCmd("start", "Start a worker", "start"),
		newSimpleCmd("stop", "Stop a worker", "stop"),
		newSimpleCmd("get", "Get worker status", "get_status"),
	)

	rootCmd.AddCommand(workerCmd)
}

// ---- Command constructors ----

func newRunCmd() *cobra.Command {
	var (
		jobID    int
		tasksStr string
		parallel bool
	)

	cmd := &cobra.Command{
		Use:   "run",
		Short: "Run a job on a worker",
		RunE: func(cmd *cobra.Command, args []string) error {
			if jobID <= 0 {
				return errors.New("job id must be > 0")
			}

			payload := newPayload("run")
			payload.Job = &Job{
				ID:       jobID,
				Tasks:    parseTasks(tasksStr),
				Parallel: parallel,
			}

			return sendPayload(cmd, payload)
		},
	}

	cmd.Flags().IntVar(&jobID, "job", 0, "Job ID (required)")
	cmd.Flags().StringVar(&tasksStr, "tasks", "", "Comma-separated tasks")
	cmd.Flags().BoolVar(&parallel, "parallel", false, "Parallel mode")
	_ = cmd.MarkFlagRequired("job")

	return cmd
}

func newAssignCmd() *cobra.Command {
	var (
		jobID    int
		tasksStr string
	)

	cmd := &cobra.Command{
		Use:   "assign",
		Short: "Assign tasks to a job",
		RunE: func(cmd *cobra.Command, args []string) error {
			if jobID <= 0 {
				return errors.New("job id must be > 0")
			}

			payload := newPayload("assign")
			payload.Job = &Job{
				ID:    jobID,
				Tasks: parseTasks(tasksStr),
			}

			return sendPayload(cmd, payload)
		},
	}

	cmd.Flags().IntVar(&jobID, "job", 0, "Job ID (required)")
	cmd.Flags().StringVar(&tasksStr, "tasks", "", "Comma-separated tasks")
	_ = cmd.MarkFlagRequired("job")

	return cmd
}

func newSimpleCmd(use, short, command string) *cobra.Command {
	return &cobra.Command{
		Use:   use,
		Short: short,
		RunE: func(cmd *cobra.Command, args []string) error {
			return sendPayload(cmd, newPayload(command))
		},
	}
}

// ---- Payload + helpers ----

func newPayload(command string) CommandPayload {
	return CommandPayload{
		Command:  command,
		Token:    token,
		WorkerID: workerID,
	}
}

func parseTasks(taskStr string) []Task {
	taskStr = strings.TrimSpace(taskStr)
	if taskStr == "" {
		return nil
	}

	parts := strings.Split(taskStr, ",")
	out := make([]Task, 0, len(parts))

	for _, p := range parts {
		name := strings.TrimSpace(p)
		if name == "" {
			continue
		}
		out = append(out, Task{Name: name})
	}

	if len(out) == 0 {
		return nil
	}
	return out
}

func sendPayload(cmd *cobra.Command, payload CommandPayload) error {
	b, err := json.Marshal(payload)
	if err != nil {
		return fmt.Errorf("marshal payload: %w", err)
	}

	conn, err := net.DialTimeout("unix", sockPath, 2*time.Second)
	if err != nil {
		return fmt.Errorf("dial socket %q: %w", sockPath, err)
	}
	defer conn.Close()

	if _, err := conn.Write(append(b, '\n')); err != nil {
		return fmt.Errorf("write payload: %w", err)
	}

	// read response (best-effort)
	buf := make([]byte, 4096)
	_ = conn.SetReadDeadline(time.Now().Add(2 * time.Second))

	n, err := conn.Read(buf)
	if err != nil {
		return fmt.Errorf("read response: %w", err)
	}

	cmd.Println("Worker response:", strings.TrimSpace(string(buf[:n])))
	return nil
}

func defaultSockPath() string {
	if v := os.Getenv("WORKER_SOCK"); v != "" {
		return v
	}
	return "/Users/leejahsprock/cs/pyoneer/etc/worker.sock"
}
