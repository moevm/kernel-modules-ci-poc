#!/bin/bash

# Usage:
#     ./scripts/local_deploy.sh -s apache2  # Ubuntu
#     ./scripts/local_deploy.sh -s httpd    # Amazon Linux
#     ./scripts/local_deploy.sh             # Amazon Linux


DEPLOY_DIR="/var/www/mooc-linux-programming"
WEB_SERVER="httpd"
USER="ec2-user"
CONFIG_FILE="mooc.conf"
LOCAL_HOST_NAME="mooc-linux-programming"
HOSTS_STRING="127.0.0.1 ${LOCAL_HOST_NAME}"
TASK_FILE="/var/www/mooc-linux-programming/status/deploy_task"
GRADER_CONF="xqueue_watcher/conf.d/local.json"

while getopts ":s:c:p" opt ;
do
    case $opt in
        s) WEB_SERVER=$OPTARG;
           USER="www-data";
           CONFIG_FILE="mooc_apache2.conf"
            ;;
        c) CUSTOM_CONFIG=$OPTARG;
            ;;
        p) GRADER_CONF="xqueue_watcher/conf.d/prod.json";
            ;;
        *) echo "the option is incorrect";
            exit 1
            ;;
        esac
done

if [ -n "${CUSTOM_CONFIG}" ]
then
	CONFIG_FILE=${CUSTOM_CONFIG}
fi

if ! grep -Fxq "$HOSTS_STRING" /etc/hosts
then
        echo "$HOSTS_STRING" >> /etc/hosts
fi

mkdir -p ${DEPLOY_DIR}

echo "Performing file copying ..."
#./scripts/install_backend_sources.sh ${DEPLOY_DIR}
cp -r ./ ${DEPLOY_DIR}

mkdir -p ${DEPLOY_DIR}/status 
mkdir -p ${DEPLOY_DIR}/task_folder

echo "Checking ${DEPLOY_DIR}/status/state"
STATUS_FILE="${DEPLOY_DIR}/status/state";
if [ ! -f "${STATUS_FILE}" ];
then
	echo "IDDLE" > ${STATUS_FILE};
fi

touch ${DEPLOY_DIR}/tests/solve_status.txt

echo "GRADER_CONF = ${GRADER_CONF}"
if [ ! -z ${GRADER_CONF} ]
then
	echo "Setting up GRADER_CONF"
	mkdir -p ${DEPLOY_DIR}/xqueue_watcher/conf.d/conf.d/
	rm ${DEPLOY_DIR}/xqueue_watcher/conf.d/conf.d/*
	cp ${GRADER_CONF} ${DEPLOY_DIR}/xqueue_watcher/conf.d/conf.d/config.json
else
	echo "GRADER_CONF will be empty."
fi

chown -R ${USER}:${USER} ${DEPLOY_DIR}

echo "Updating apache2 configuration"

cp "./config/${CONFIG_FILE}" "/etc/${WEB_SERVER}/sites-available/mooc.conf"
cp "./config/mooc_status_stub.conf" "/etc/${WEB_SERVER}/sites-available/"

ln -s /etc/${WEB_SERVER}/sites-available/mooc.conf /etc/${WEB_SERVER}/sites-enabled/mooc.conf 2>/dev/null || true
ln -s /etc/${WEB_SERVER}/sites-available/mooc_status_stub.conf /etc/${WEB_SERVER}/sites-enabled/mooc_status_stub.conf 2>/dev/null || true

echo "Disabling outgoing connections for docker containers"
#./scripts/block_docker_outgoing_connections.sh

# Enabling daemon
echo "[Service]
User=${USER}
Group=${USER}
ExecStart=${DEPLOY_DIR}/scripts/pdaemon.sh
Restart=always
RestartSec=1

[Install]
WantedBy=multi-user.target" > /lib/systemd/system/pdaemon.service
systemctl daemon-reload
systemctl enable pdaemon
systemctl start pdaemon

a2ensite mooc #${CONFIG_FILE}

service ${WEB_SERVER} restart

DEPLOY_TASK=`cat ${TASK_FILE} 2>/dev/null`

echo ${DEPLOY_TASK}

touch ${DEPLOY_DIR}/task_folder/${DEPLOY_TASK}/deploy_done
rm ${TASK_FILE} 2>/dev/null

echo "Deploy completed"
