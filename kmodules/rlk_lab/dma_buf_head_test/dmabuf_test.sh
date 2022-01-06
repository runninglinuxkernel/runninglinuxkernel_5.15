#!/bin/bash

TCID="dmabuf_test.sh"
errcode=0

run_test()
{
	heaptype=$1
	./dmabuf_export &
	sleep 1
	./dmabuf_import
	if [ $? -ne 0 ]; then
		errcode=1
	fi
	sleep 1
	echo ""
}

check_root()
{
	uid=$(id -u)
	if [ $uid -ne 0 ]; then
		echo $TCID: must be run as root >&2
		exit -1
	fi
}

check_device()
{
	DEVICE=/dev/dma_heap/
	if [ ! -e $DEVICE ]; then
		echo $TCID: No $DEVICE device found >&2
		exit -1
	fi
}

main_function()
{
	check_device
	check_root

	run_test 0
}

main_function
echo "$TCID: done"
exit $errcode
