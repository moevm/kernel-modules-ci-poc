#!/bin/bash

# Expected result - comp_exec and comp_log should not be empty

./client_emulator/client_emul.py -c ./tst/integration/task_related/7068_explicit_time_out_message/solution.c  -m ./tst/integration/task_related/7068_explicit_time_out_message/Makefile -n 3 -u http://mooc-linux-programming/
