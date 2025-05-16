#!/usr/bin/env python3
import sys
import os
import random
import subprocess
import checker_client

def main(source_path: str) -> int:
    module_name = os.path.splitext(os.path.basename(source_path))[0]
    source_bytes = open(source_path, "rb").read()
    checker = checker_client.Checker()

    try:
        build_proc = checker.compile_module(source_bytes, module_name)
    except subprocess.CalledProcessError as e:
        print("Compilation failed:", file=sys.stderr)
        print(e.stderr.decode().strip(), file=sys.stderr)
        return 0

    build_log = build_proc.stdout.decode().strip()
    print("=== Build log ===")
    print(build_log)

    var_a = random.randint(1, 10)
    var_b = random.randint(1, 10)
    var_c = [random.randint(1, 10) for _ in range(5)]
    print(f"Params: a={var_a}, b={var_b}, c={','.join(map(str, var_c))}")

    insmod_cmd = ["insmod", f"/tmp/{module_name}.ko",
                  f"a={var_a}", f"b={var_b}", "c=" + ",".join(map(str, var_c))]
    try:
        checker.client.run_command(insmod_cmd).check_returncode()
    except subprocess.CalledProcessError as e:
        print("Error during insmod:", file=sys.stderr)
        print(e.stderr.decode().strip(), file=sys.stderr)
        return 0

    sysfs_path = "/sys/kernel/my_kobject/my_sys"
    try:
        cat_proc = checker.client.run_command(["cat", sysfs_path])
        cat_proc.check_returncode()
        actual = int(cat_proc.stdout.decode().strip())
    except Exception as e:
        print(f"Error reading {sysfs_path}:", e, file=sys.stderr)
        try:
            checker.unload_module(module_name)
        except subprocess.CalledProcessError as unload_err:
            print("Error during cleanup (rmmod):", unload_err, file=sys.stderr)
        return 0

    expected = var_a + var_b + sum(var_c)
    print(f"Expected sum: {expected}")
    print(f"Value from module: {actual}")

    if actual != expected:
        print("Incorrect.")
        checker.unload_module(module_name)
        return 0

    print("Correct.")

    try:
        checker.unload_module(module_name)
    except subprocess.CalledProcessError as e:
        print("Error during rmmod:", file=sys.stderr)
        print(e.stderr.decode().strip(), file=sys.stderr)
        return 0

    try:
        dmesg = checker.get_kernel_log().splitlines()
        print("=== dmesg (last 9) ===")
        print("\n".join(dmesg[-9:]))
    except subprocess.CalledProcessError as e:
        print("Failed to retrieve kernel log:", e, file=sys.stderr)

    return 1

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <module_source.c>", file=sys.stderr)
        sys.exit(0)
    sys.exit(main(sys.argv[1]))
