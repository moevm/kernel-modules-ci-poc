#!/bin/bash

NAME="pdaemon";
LOG="/var/www/mooc-linux-programming/${NAME}.log";

BASE_DIR="/app/"
CURRENT_TASK_FILE="${BASE_DIR}status/task";
HTTP_EXT_LOCK="${BASE_DIR}http_ext_lock";
HTTP_EXT_LOCK_ID="HTTP_EXT_LOCK"
XQUEUE_WATCHER_DIR="${BASE_DIR}xqueue_watcher/"

function log {
	echo "[$(date --rfc-3339=ns)][$$]: ${*}" 2>&1 | tee -a "${LOG}"
}

log "${NAME} started"
while :
do
    from_xq=0
    if [ ! -f "$CURRENT_TASK_FILE" ]; then
        cd ${XQUEUE_WATCHER_DIR}
        grader_get_log=$(python3 -m xqueue_watcher -d conf.d/ --get 2>&1)
        result=$?
        log "Grader get log"
        log "${grader_get_log}"
        if [ "$result" == "0" ]; then
            from_xq=1
        else
            sleep 20
            continue
        fi
        cd -
    fi

    id=$(cat $CURRENT_TASK_FILE)
    task_dir="${BASE_DIR}task_folder/${id}"
    task_id=$(cat ${task_dir}/task_id.txt)
    solution="${task_dir}/solution.c"
    makefile="${task_dir}/Makefile"
    tests_dir="${BASE_DIR}tasks/"

    log "$CURRENT_TASK_FILE exists: ${id}, ${task_id}"

    if [ -f "$HTTP_EXT_LOCK" ] && [ "$task_id" == "$HTTP_EXT_LOCK_ID" ]; then
        log "$HTTP_EXT_LOCK found, locking external connections to apache"
        sudo ${BASE_DIR}scripts/lock_server_from_external_http.sh $(cat $HTTP_EXT_LOCK)
        rm $HTTP_EXT_LOCK
    fi

    old_pwd=$(pwd)
    cd ${tests_dir}
    log "Calling checker.py: id=${id}, task_id=${task_id}"
    python3 checker.py ${id} ${task_id} ${solution} ${makefile} | \
        while IFS= read -r line; do log "$line"; done
    check_result=$?
    log "${id} check finished, exit_code = ${check_result}"
    cd ${old_pwd}

    if [ "$from_xq" == "1" ]; then
        log "trying to send result"
        cd ${XQUEUE_WATCHER_DIR}
        grader_send_log=$(python3 -m xqueue_watcher -d conf.d/ --send --submission_id ${id} -q $(cat "${task_dir}/queue") 2>&1)
        result=$?
        log "Grader send log"
        log "${grader_send_log}"
        if [ "$result" != "0" ]; then
            log "${id} not sent"
        else
            log "${id} successfully sent"
        fi
        cd -
    fi

    rm -rf $CURRENT_TASK_FILE
    rm -rf /var/www/mooc-linux-programming/local-lc/*
    echo "IDDLE" > "${BASE_DIR}status/state"
done
