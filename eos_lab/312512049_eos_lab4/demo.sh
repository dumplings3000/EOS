#!/bin/sh

set -x #prints each command and its arguments to the terminal before executing it
# set -e #Exit immediately if a command exits with a non-zero status

rmmod -f mydev.ko
insmod mydev.ko

./writer sam & #run in subshell
./reader 192.168.222.100 7000 /dev/my_dev
