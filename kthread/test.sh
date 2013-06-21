#!/bin/sh

if [ ! -c tdrv ]; then
	mknod tdrv c 111 0
fi

make clean
make

lsmod | grep kthread_test
if [ $? -ne 0 ]; then
	insmod ./kthread_test.ko
fi

echo "####test begin####"
./usr
echo "####test finish####"

rmmod kthread_test
