#!/bin/bash
proxymgr=$1

parallel -j300 curl -L --proxy "socks5://$proxymgr:1080 -U {1}-{2}-1080:{3} {4}" :::: /tmp/usr.txt :::: ~/BlackH/proxy/prx.txt :::: /tmp/passwd.txt ::: google.com yandex.ru youtube.com facebook.com gnu.org
