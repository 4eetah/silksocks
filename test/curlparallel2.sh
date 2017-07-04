#!/bin/bash
proxymgr=$1
user=$2
passwd=$3

parallel -j300 curl -L --proxy socks5://$proxymgr:1080 -U $user-{1}-1080:$passwd {2} :::: ~/BlackH/proxy/prx.txt ::: google.com yandex.ru youtube.com facebook.com gnu.org
