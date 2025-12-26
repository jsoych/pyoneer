package cmd

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"
)

var rootCmd = &cobra.Command{
	Use:   "pyoctl",
	Short: "Pyoctl CLI to manage Pyoneer workers",
	Long:  `CLI tool to control Pyoneer workers with run, assign, start, stop, and get status commands.`,
}

// Execute runs the root command
func Execute() {
	if err := rootCmd.Execute(); err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}
