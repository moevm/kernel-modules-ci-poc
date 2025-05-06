#!/bin/bash

HOST="http://mooc-linux-programming/"
REPEAT=1
PARTICULAR_FLAG=""
TASK_IDS=""

usage() {
    echo "./tst/integration/heavy_check.tst" \
        "[options] [task_id1] [task_id2] ..."
    echo "Options:"
    echo "  -u|--url            - Host url"
    echo "  -n|--repeat         - Number of times to repeat"
    echo "  -p|--particular     - Particular id to check"
    echo "  -h|--help           - Show this message"
}

while [ $# -gt 0 ]; do
    case $1 in
        -u|--url)
            HOST=$2
            shift
            shift
            ;;
        -n|--repeat)
            REPEAT=$2
            shift
            shift
            ;;
        -p|--particular)
            PARTICULAR_FLAG=$2
            shift
            shift
            ;;
        -h|--help)
            usage
            exit
            ;;
        *)
            TASK_IDS="$1 ${TASK_IDS}"
            shift
            ;;
    esac
done

fail_file="./fail"
failed_list_file="./failed_list"
test_single_sample="tst/integration/check_single\
_example_and_wait_for_result.sh"
test_data_path="./tst/integration/data"

rm -f ${fail_file} ${failed_list_file}

echo "0" > ${fail_file}
echo "Failed tests:" > ${failed_list_file}

tests_to_check="${TASK_IDS}"
if [ "${tests_to_check}" = "" ]; then
    tests_to_check=$(ls ${test_data_path})
fi

echo "Existing task_ids which will be tested:"
echo "${tests_to_check}"

for repeat in $(seq 1 "${REPEAT}"); do
    echo "Attempt ${repeat}/${REPEAT}"
    for i in ${tests_to_check}; do
        echo "task_id=${i}"
        PARTICULAR=${PARTICULAR_FLAG}
        if [ "${PARTICULAR}" = "" ]; then
            PARTICULAR=$( ls "${test_data_path}/${i}" )
        fi
        echo "Particular tasks: $PARTICULAR"
        for j in ${PARTICULAR}; do
            echo "Checking ${i}/${j}"
            echo "${test_single_sample} ${test_data_path} ${i} ${j}" \
                "${HOST} ${fail_file} ${failed_list_file}"
            ${test_single_sample} ${test_data_path} "${i}" "${j}" \
                "${HOST}" ${fail_file} ${failed_list_file}
            rm -rf /app/task_folder/*
            echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
        done
        echo "////////////////////////////////////////////////////"
    done
done

fail=$(cat ${fail_file})
if [ "${fail}" -eq 1 ]; then
    echo "[FAIL] Some tests failed"
    cat "${failed_list_file}"
    exit 1
fi
echo "[SUCCESS] All tests successfuly completed!"
