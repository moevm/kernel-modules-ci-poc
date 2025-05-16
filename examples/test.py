#!/usr/bin/env python3

import checker_client
import sys
import os
import subprocess

if __name__ == "__main__":
    module_source_file = sys.argv[1]

    checker = checker_client.Checker()

    module_source = open(module_source_file, "rb").read()
    module_name = os.path.splitext(os.path.basename(module_source_file))[0]

    try:
        result = checker.compile_module(module_source, module_name)
        checker.load_module(module_name)
        checker.unload_module(module_name)

        build_log = result.stdout.decode().strip()
        kernel_log = "\n".join(checker.get_kernel_log().splitlines()[-9:])
        print(build_log)
        print(kernel_log)
    except subprocess.CalledProcessError as e:
        print(e.stderr.decode().strip())
