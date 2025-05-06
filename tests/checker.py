#!/usr/bin/env python3
import subprocess
import time
import socket
import sys
import os
import json

VM_IMAGE = "/var/lib/images/nixos.qcow2"
VM_PORT = 8000
VM_TIMEOUT = 300
VM_HOST = "127.0.0.1"


def wait_for_port(host: str, port: int, timeout: int = 300) -> bool:
    start = time.time()
    while time.time() - start < timeout:
        try:
            with socket.create_connection((host, port), timeout=5):
                return True
        except (socket.timeout, ConnectionRefusedError):
            time.sleep(1)
    return False


def launch_vm(image_path: str) -> subprocess.Popen:
    qemu_cmd = [
        "qemu-system-x86_64",
        "-snapshot",
        "-drive", f"file={image_path},format=qcow2",
        "-nographic",
        "-nic", f"user,hostfwd=tcp::{VM_PORT}-:{VM_PORT}",
        "-m", "2G",
        "-smp", "2",
        "-enable-kvm",
    ]
    return subprocess.Popen(qemu_cmd)


def run_test(test_path: str, solution_path: str) -> subprocess.CompletedProcess:
    return subprocess.run(
        ["python3", test_path, solution_path],
        capture_output=True
    )


def write_result(result_path: str, data: dict):
    os.makedirs(result_path, exist_ok=True)
    with open(os.path.join(result_path, "solve.json"), "w") as f:
        json.dump(data, f)


def main(id_: str, task_id: str, sol_path: str, _: str) -> int:
    out_dir = f"/app/task_folder/{id_}/"
    test_path = f"/app/tests/{task_id}/test.py"

    if not os.path.isfile(test_path):
        print(f"Test file not found: {test_path}", file=sys.stderr)
        return 1

    vm = launch_vm(VM_IMAGE)

    if not wait_for_port(VM_HOST, VM_PORT, timeout=VM_TIMEOUT):
        vm.terminate()
        print("VM startup timed out", file=sys.stderr)
        return 1

    proc = run_test(test_path, sol_path)

    result = {
        "solve_status": 0 if proc.returncode == 0 else 1,
        "comp_log": proc.stdout.decode('utf-8', errors='replace'),
        "comp_exec": proc.stderr.decode('utf-8', errors='replace'),
        "error_message": "",
        "status": "VERDICT"
    }

    if proc.returncode != 0:
        result["error_message"] = f"Module test failed (exit code {proc.returncode})"

    write_result(out_dir, result)
    vm.terminate()
    return proc.returncode


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print(f"Usage: {sys.argv[0]} <id> <task_id> <solution.c> <makefile>", file=sys.stderr)
        sys.exit(1)

    exit_code = main(*sys.argv[1:5])
    sys.exit(exit_code)

