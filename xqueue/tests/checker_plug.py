#!/usr/bin/env python3

import json
import sys
import os

submission_id = sys.argv[1]
task_id = sys.argv[2]
solution_path = sys.argv[3]
makefile_path = sys.argv[4]
task_dir = os.path.dirname(solution_path)


def print_file_info(path, label):
    print(f"\n{label} path: {path}")
    if os.path.isfile(path):
        try:
            with open(path, "r") as f:
                content = f.read()
            print(f"{label} content:\n{content}")
        except Exception as e:
            print(f"Could not read {label}: {e}")
    else:
        print(f"{label} not found.")


solve_status = 1 if os.path.isfile(solution_path) and os.path.isfile(makefile_path) else 0
if solve_status == 1:
    print("\n--- File Info ---")
    print_file_info(solution_path, "Solution")
    print_file_info(makefile_path, "Makefile")


data = {
    "submission_id": submission_id,
    "solve_status": solve_status,
    "status": "VERDICT",
    "comp_log": f"Solution path - {solution_path}, Makefile_path - {makefile_path}.",
    "comp_exec": "no-op",
    "error_message": "" if solve_status else "Missing solution.c or Makefile"
}

json_path = os.path.join(task_dir, "solve.json")
with open(json_path, "w") as f:
    json.dump(data, f)

print(json.dumps(data, indent=2))

