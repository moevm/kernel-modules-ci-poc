#!/bin/bash

# Expected result - comp_log should not contain successful ping 

./client_emulator/client_emul.py -c ./tst/integration/task_related/7056_block_outgoing_connections/solution.c  -m ./tst/integration/task_related/7056_block_outgoing_connections/Makefile -n 1 -u http://mooc-linux-programming/
