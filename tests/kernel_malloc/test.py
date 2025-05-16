#!/usr/bin/env python3

import os
import sys
import subprocess
import checker_client

def main(solution_path: str) -> int:
    module_name = os.path.splitext(os.path.basename(solution_path))[0]
    solution_bytes = open(solution_path, "rb").read()
    checker = checker_client.Checker()

    # Compile and load main solution
    try:
        checker.compile_module(solution_bytes, module_name)
        checker.load_module(module_name)
    except subprocess.CalledProcessError as e:
        print("Module build/load failed:", e, file=sys.stderr)
        return 1

    # Проверка работы устройства
    try:
        dev_path = "/dev/checker"
        # Проверка: void
        with open(dev_path, "w") as f:
            f.write("void")
        with open(dev_path, "r") as f:
            output = f.read().strip()
        print("=== Output after 'void' ===")
        print(output)

        # Аналогично для int
        with open(dev_path, "w") as f:
            f.write("int")
        with open(dev_path, "r") as f:
            output = f.read().strip()
        print("=== Output after 'int' ===")
        print(output)

        # Аналогично для struct
        with open(dev_path, "w") as f:
            f.write("struct")
        with open(dev_path, "r") as f:
            output = f.read().strip()
        print("=== Output after 'struct' ===")
        print(output)

        # Очистка (kfree)
        with open(dev_path, "w") as f:
            f.write("kfree")
        with open(dev_path, "r") as f:
            output = f.read().strip()
        print("=== Output after 'kfree' ===")
        print(output)

    except Exception as e:
        print("Error communicating with /dev/checker:", e, file=sys.stderr)
        checker.unload_module(module_name)
        return 1

    # Выгрузка модуля
    try:
        checker.unload_module(module_name)
    except subprocess.CalledProcessError:
        print("Failed to unload module", file=sys.stderr)
        return 1

    print("Module test passed.")
    return 0

if name == "main":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <solution.c>", file=sys.stderr)
        sys.exit(1)
    sys.exit(main(sys.argv[1]))