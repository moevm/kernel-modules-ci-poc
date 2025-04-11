#!/bin/bash

# Expected result - comp_exec should contain a lot of test\n entries

./client_emulator/client_emul.py -c ./tst/integration/task_related/7069_output_of_task_15_killed_by_timeout/solution.c  -m ./tst/integration/task_related/7069_output_of_task_15_killed_by_timeout/Makefile -n 15 -u http://mooc-linux-programming/
