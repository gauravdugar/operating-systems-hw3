#!/bin/sh
set -x
lsmod
rmmod sys_xjob_mod
insmod sys_xjob_mod.ko
lsmod
