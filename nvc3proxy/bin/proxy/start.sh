#!/bin/sh

BINPATH=/sxmobi/sxmobiserv/nvc3proxy/bin/proxy


pid=`ps axu | grep Proxy | grep -v grep | grep "183.56.160.53:9003" | awk -F ' ' '{print $2}'`
if [ "X" == "X$pid" ]; then
        echo "Proxy  not exist"
        sleep 1
        ${BINPATH}/Proxy -id 10000 -tcp 183.56.160.53:9003 -cs 127.0.0.1:9005
fi
