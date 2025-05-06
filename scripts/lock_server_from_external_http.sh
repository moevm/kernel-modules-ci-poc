#!/bin/bash

config="$1"

# Creating backup
cp ${config} ${config}_0

# Replacing hostname to $local_host_name
local_host_name="mooc-linux-programming"
host_name=`/var/www/mooc-linux-programming/scripts/extract_server_name_from_conf.sh ${config}`

sed -i "s/${host_name}/${local_host_name}/g" ${config}

# Restarting apache
/etc/init.d/httpd restart
