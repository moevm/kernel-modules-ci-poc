#!/bin/bash


TIMEOUT=220
TICK=1

base_path=$1
task_id=$2
sample_path=$3
host=$4
fail_file=$5
failed_list_file=$6
fullpath="${base_path}/${task_id}/${sample_path}"
correct_result=$(echo "${sample_path}" | grep -o '^[01]')
client="client_emulator/client_emul.py"

upload() {
    ${client} \
        -c "${fullpath}/solution.c" \
        -m "${fullpath}/Makefile" \
        -n "${task_id}" \
        -u "${host}"
}

get_status() {
    id=$1

    ${client} \
        -a "check" \
        -i "${id}" \
        -u "${host}"
}

fail() {
    reason=$1

    echo 1 > "${fail_file}"
    echo "${task_id}/${sample_path} ($id) - ${reason}" >> "${failed_list_file}"
}

fail_timeout() {
    fail "timeout"
    echo "[FAIL] Failed due to timeout"
    exit
}

fail_incorrect() {
    fail "incorrect_result"
    echo "[FAIL] $1 != $2"
    exit
}

exit_success() {
    echo "[SUCCESS]"
    exit 0
}

compare_and_exit() {
    result=$1
    status_code=${2%.*}

    echo "from request ${status_code}, valid result ${correct_result}"
    fail_reason=$(get_json_value "${result}" "fail_reason")

    if [ "${status_code}" = "${correct_result}" ]; then
        exit_success
    fi

    if [ "${fail_reason}" = "TIMEOUT" ]; then
        fail_timeout
    else
        fail_incorrect "${status_code}" "${correct_result}"
    fi
}

pretty_print() {
    label=$1
    text=$2

    echo "----${label}-----"
    echo "${text}"
    echo "-End of ${label}-"
}

print_result() {
    result=$1

    echo "Results: ${result}"
    comp_exec=$(get_json_value "${result}" "comp_exec")
    comp_log=$(get_json_value "${result}" "comp_log")
    solve_status=$(get_json_value "${result}" "solve_status")

    pretty_print "Compilation log" "${comp_log}"
    pretty_print "Execution log" "${comp_exec}"
    pretty_print "Score" "${solve_status}"
}

get_json_value() {
    str=$1
    key=$2
    tmpf=/tmp/check_single.tmp

    echo "${str}" > "${tmpf}"
    if value=$("${client}" "decode" "${tmpf}" "$key"); then
        echo "$value"
    fi
    rm -f "${tmpf}"
}

result=$(upload)
id=$(get_json_value "${result}" "id")

echo "Solution id: ${id}"
if [ -z "$id" ]; then
    fail_timeout
fi

t="1"
while [ $t -lt $TIMEOUT ]; do
    result=$(get_status "${id}")
    status_text=$(get_json_value "${result}" "status")
    if [ "${status_text}" = "VERDICT" ]; then
        echo "Sample checking is finished, analysing results"
        print_result "${result}"

        status_code=$(get_json_value "${result}" "solve_status")
        compare_and_exit "${result}" "${status_code}"
    else
        printf '\rSample is still not checked (%ss)                      ' "$t"
        t=$((t + TICK))
        sleep $TICK
    fi
done

