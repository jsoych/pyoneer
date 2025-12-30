import os
import json
import socket
import unittest
import time
from concurrent.futures import ThreadPoolExecutor, as_completed, TimeoutError

SOCKET_PATH = os.getenv("PYONEER_SOCKET_PATH")
PORT = int(os.getenv("PYONEER_PORT", "8080"))
HOST = os.getenv("PYONEER_HOST", "127.0.0.1")
TOKEN = os.getenv("PYONEER_API_TOKEN")
BUFFSIZE = 4096

worker_status = ["not_assigned", "assigned", "working", "not_working"]

def recv_all(sock, buffsize=BUFFSIZE):
    """Read until EOF; requires server to close connection. Enforced by socket timeout."""
    chunks = []
    while True:
        data = sock.recv(buffsize)  # may raise socket.timeout
        if not data:
            break
        chunks.append(data)
    return b"".join(chunks)

def send_unix_one(request, timeout=5):
    if not SOCKET_PATH:
        raise RuntimeError("PYONEER_SOCKET_PATH is not set")
    if not TOKEN:
        raise RuntimeError("PYONEER_API_TOKEN is not set")

    req = dict(request)
    req["token"] = TOKEN
    payload = json.dumps(req).encode()

    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(timeout)
    s.connect(SOCKET_PATH)
    try:
        s.sendall(payload)
        raw = recv_all(s)
        if not raw:
            return {}  # e.g. stop/start might return empty
        return json.loads(raw.decode())
    finally:
        s.close()

def send_tcp_one(request, timeout=5):
    if not TOKEN:
        raise RuntimeError("PYONEER_API_TOKEN is not set")

    req = dict(request)
    req["token"] = TOKEN
    payload = json.dumps(req).encode()

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(timeout)
    s.connect((HOST, PORT))
    try:
        s.sendall(payload)
        raw = recv_all(s)
        if not raw:
            return {}
        return json.loads(raw.decode())
    finally:
        s.close()

def stress(send_one_fn, total_requests=1000, concurrency=50, timeout_seconds=30):
    ok = 0
    bad = 0
    exceptions = 0

    start = time.time()

    def task():
        res = send_one_fn({"command": "get_status"})
        return res.get("pyoneer", {}).get("status") in worker_status

    with ThreadPoolExecutor(max_workers=concurrency) as ex:
        futs = [ex.submit(task) for _ in range(total_requests)]
        try:
            for f in as_completed(futs, timeout=timeout_seconds):
                try:
                    if f.result():
                        ok += 1
                    else:
                        bad += 1
                except Exception:
                    exceptions += 1
        except TimeoutError:
            # Not all futures finished in time
            for f in futs:
                if not f.done():
                    exceptions += 1

    elapsed = time.time() - start
    rps = total_requests / elapsed if elapsed > 0 else 0

    return {
        "ok": ok,
        "bad": bad,
        "exceptions": exceptions,
        "elapsed": elapsed,
        "rps": rps,
        "total": total_requests,
        "concurrency": concurrency,
    }

class ApiTest(unittest.TestCase):

    def test_unix_get_status(self):
        if not SOCKET_PATH:
            self.skipTest("PYONEER_SOCKET_PATH not set")
        res = send_unix_one({"command": "get_status"})
        self.assertIn(res["pyoneer"]["status"], worker_status)

    def test_tcp_get_status(self):
        res = send_tcp_one({"command": "get_status"})
        self.assertIn(res["pyoneer"]["status"], worker_status)

    def test_stress_unix_get_status(self):
        if not SOCKET_PATH:
            self.skipTest("PYONEER_SOCKET_PATH not set")

        result = stress(send_unix_one, total_requests=1000, concurrency=50, timeout_seconds=30)

        print(
            f"\nUNIX stress get_status: ok={result['ok']}, bad={result['bad']}, exceptions={result['exceptions']}, "
            f"elapsed={result['elapsed']:.2f}s, rps={result['rps']:.1f}, concurrency={result['concurrency']}"
        )

        self.assertEqual(result["exceptions"], 0, "Exceptions occurred during UNIX stress test")
        self.assertGreaterEqual(result["ok"], int(result["total"] * 0.99),
                                "Too many invalid UNIX responses (>=1% failed)")

    def test_stress_tcp_get_status(self):
        result = stress(send_tcp_one, total_requests=1000, concurrency=50, timeout_seconds=30)

        print(
            f"\nTCP stress get_status: ok={result['ok']}, bad={result['bad']}, exceptions={result['exceptions']}, "
            f"elapsed={result['elapsed']:.2f}s, rps={result['rps']:.1f}, concurrency={result['concurrency']}"
        )

        self.assertEqual(result["exceptions"], 0, "Exceptions occurred during TCP stress test")
        self.assertGreaterEqual(result["ok"], int(result["total"] * 0.99),
                                "Too many invalid TCP responses (>=1% failed)")

    def test_stress_mixed_unix_tcp(self):
        """
        Stress both endpoints at once. Half traffic goes to UNIX, half to TCP.
        This catches resource contention bugs (thread pool starvation, lock contention, etc.).
        """
        if not SOCKET_PATH:
            self.skipTest("PYONEER_SOCKET_PATH not set")

        total_requests = 2000
        concurrency = 100
        timeout_seconds = 60

        ok = 0
        bad = 0
        exceptions = 0

        start = time.time()

        def task(i):
            fn = send_unix_one if (i % 2 == 0) else send_tcp_one
            res = fn({"command": "get_status"})
            try:
                return (res["pyoneer"]["status"] in worker_status)
            except Exception:
                return False

        with ThreadPoolExecutor(max_workers=concurrency) as ex:
            futs = [ex.submit(task, i) for i in range(total_requests)]
            for f in as_completed(futs, timeout=timeout_seconds):
                try:
                    if f.result():
                        ok += 1
                    else:
                        bad += 1
                except Exception:
                    exceptions += 1

        elapsed = time.time() - start
        rps = total_requests / elapsed if elapsed > 0 else 0

        print(
            f"\nMIXED stress get_status: ok={ok}, bad={bad}, exceptions={exceptions}, "
            f"elapsed={elapsed:.2f}s, rps={rps:.1f}, concurrency={concurrency}"
        )

        self.assertEqual(exceptions, 0, "Exceptions occurred during mixed stress test")
        self.assertGreaterEqual(ok, int(total_requests * 0.99),
                                "Too many invalid mixed responses (>=1% failed)")


if __name__ == "__main__":
    unittest.main()
