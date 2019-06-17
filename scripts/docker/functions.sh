#!/bin/bash
#
# OpenJK server utility functions.
#

# Send and receive UDP datagrams
function sendrecv
{
	local COMMAND="$*"
	local HEADER="\0377\0377\0377\0377"
	echo -e "${HEADER}${COMMAND}" | socat -t3 -T3 - UDP:127.0.0.1:29070 | tr -d "${HEADER}" 2>/dev/null
}

# Parse a config file and get the value for a cvar
function parseconfig
{
	local FILE="$1"
	local KEY="$2"
	if [ -e "$FILE" ]; then
		grep -E -i -m 1 "^set[usa]?\s+${KEY}\s+(.*)$" "$FILE" | sed -r 's#^set[usa]?\s+[^ ]+\s+"?([^"]*)"?\s*$#\1#g'
	fi
}

# Parse an info string and get the value for a key
function parseinfo
{
	local INFO="$1"
	local KEY="$2"
	echo "$INFO" | grep -o "\\\\${KEY}\\\\[^\\\\]*\\\\" | cut -d"\\" -f 3
}

# Get live server info
function getinfo
{
	sendrecv getinfo | tail -n +2
}

# Get live server status
function getstatus
{
	sendrecv getstatus | tail -n +2
}

# Get live server serverinfo
function getserverinfo
{
	getstatus | head -n 1
}

# Get live server players
function getplayers
{
	getstatus | tail -n +2
}

# Execute a rcon command
function rcon
{
	local COMMAND="$*"
	local CONFIG
	for CONFIG in "$OJK_HOMEPATH/$OJK_MOD/server.cfg" "$OJK_BASEPATH/$OJK_MOD/server.cfg" "$OJK_CDPATH/$OJK_MOD/server.cfg"; do
		local RCON_PASSWORD=`parseconfig "$CONFIG" rconpassword`
		if [ -n "$RCON_PASSWORD" ]; then
			sendrecv "rcon $RCON_PASSWORD $COMMAND"
			break
		fi
	done
}
