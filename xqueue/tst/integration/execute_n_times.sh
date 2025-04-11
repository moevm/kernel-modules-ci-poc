#!/bin/bash

# This scripts performs N executions of given test (task_id/sample_path) and prints all fails
# Usage
# ./tst/integration/execute_n_times.sh 15 0_wrong_name http://example.org/ 100


task_id=$1
sample_path=$2
host=$3
N=$4

test_data_path="./tst/integration/data"
fail_file="./fail"
failed_list_file="./failed_list"

rm ${failed_list_file}
touch ${failed_list_file}

for i in `seq 1 $N`;
do
	echo "attempt=$i"
	echo '0' > ${fail_file}
	./tst/integration/check_single_example_and_wait_for_result.sh ${test_data_path} ${task_id} ${sample_path} ${host} ${fail_file} ${failed_list_file}
	
done
echo "Fails=`cat ${failed_list_file}| wc -l`"
cat ${failed_list_file}
