#!/bin/ksh -x

# resume method for transfer queue setup
# NOTE: make sure the host which this script runs on is a submit host
# for the remote cluster

# first get the remote job id
ARCH=`$SGE_ROOT/util/arch`
export PATH=$SGE_ROOT/bin/$ARCH:$PATH
#list=`qstat -j $JOB_ID | nawk '/^context/ { print $2}'`
#tmp1=${list##*REMOTE_JID=}
#remote_jid=${tmp1%%,*}
remote_jid=`cat $TMPDIR/REMOTE_JID`

# execute in subshell to avoid changing vars
(
# below should contain path to SGE_ROOT of cluster where jobs are being sent
	export COMMD_PORT=536
	export SGE_ROOT=/net/tcf27-b030/share/gridware
	export SGE_CELL=default
	ARCH=`$SGE_ROOT/util/arch`
	export PATH=$SGE_ROOT/bin/$ARCH:$PATH
	qmod -us $remote_jid
)
