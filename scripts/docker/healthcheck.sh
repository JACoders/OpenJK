#!/bin/bash
#
# OpenJK server health check script.
#

# Set variables
OJK_DIR="/opt/openjk"
RET=0

# Load functions
. "$OJK_DIR/functions.sh"

# Check server status
INFO=`getinfo`
if [ -z "$INFO" ]; then
	# Server didn't respond
	RET=1
else
	# Server is running
	MAPNAME=`parseinfo "$INFO" mapname`
	CUR_CLIENTS=`parseinfo "$INFO" clients`
	MAX_CLIENTS=`parseinfo "$INFO" sv_maxclients`
	echo "Connected players: $CUR_CLIENTS/$MAX_CLIENTS on $MAPNAME"
fi

exit $RET
