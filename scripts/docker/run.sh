#!/bin/bash -ex
#
# OpenJK server run script.
#

# Set variables
OJK_DIR="/opt/openjk"
OJK_MOD="${OJK_MOD:-base}"
OJK_ARCH="${OJK_ARCH:-i386}"
OJK_CDPATH="$OJK_DIR/cdpath"
OJK_BASEPATH="$OJK_DIR/basepath"
OJK_HOMEPATH="$OJK_DIR/homepath"
OJK_OPTS="+set dedicated 2 +set net_port 29070 +set fs_cdpath $OJK_CDPATH +set fs_basepath $OJK_BASEPATH +set fs_homepath $OJK_HOMEPATH +set fs_game $OJK_MOD $OJK_OPTS"
OJK_BIN="$OJK_DIR/openjkded.$OJK_ARCH"
OJK_LOG="$OJK_HOMEPATH/$OJK_MOD/openjk_server.log"

# Load functions
. "$OJK_DIR/functions.sh"

# Remove nav files
find "$OJK_DIR" -name '*.nav' -delete

# Register signal handler
trap 'rcon quit' SIGTERM

# Launch OpenJK
mkdir -p `dirname "$OJK_LOG"`
export HOME="$OJK_HOMEPATH"
umask 0002
$OJK_BIN $OJK_OPTS 2>&1 | tee -a "$OJK_LOG" &

# Wait for it while listening to signals
wait $!
