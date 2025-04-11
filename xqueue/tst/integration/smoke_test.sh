#!/bin/bash

HOST_TO_CHECK=${1:-`hostname`}
HAS_ERROR=0;

function check_url(){
	url="$HOST_TO_CHECK$1"
	curl -f -t 30 ${url} 
	if [ "0" -ne "$?" ]
	then
		HAS_ERROR=1
                echo
	        echo "${url} check failed"
	fi
}

urls='./tst/integration/urls'

for i in $( cat ${urls} ); do
	echo 
	echo "Checking url: $i"
	check_url $i
done


if [ "${HAS_ERROR}" -ne "0" ]
then
	echo "Errors were found. See log above for details."
	exit 1
fi
