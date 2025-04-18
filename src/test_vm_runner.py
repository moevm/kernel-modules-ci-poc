#!/usr/bin/env python3

import subprocess
import time
import sys

QEMU_COMMAND = [
    "qemu-system-x86_64", "-drive", "file=nixos.qcow2", "-nographic", "-nic",
    "user,hostfwd=tcp::8000-:8000", "-m", "2G", "-smp", "2", "-enable-kvm"
]


def run_vm_loop():
    """
    Runs the QEMU VM in an infinite loop, restarting it after it exits.
    """
    while True:
        print("Starting QEMU VM...")
        try:
            process = subprocess.run(QEMU_COMMAND, check=False)
            print(f"QEMU VM exited with code {process.returncode}.")

        except FileNotFoundError:
            print(f"Error: 'qemu-system-x86_64' not found")
            sys.exit(1)
        except Exception as e:
            print(f"An unexpected error occurred: {e}")

        print("Restarting VM in 5 seconds...")
        time.sleep(5)


if __name__ == "__main__":
    try:
        run_vm_loop()
    except KeyboardInterrupt:
        print("\nStopped by user.")
        sys.exit(0)
