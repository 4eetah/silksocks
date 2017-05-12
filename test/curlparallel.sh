#/bin/sh

if [ $# -lt 4 ]; then
    echo "Usage: $0 <proxylist.txt> <proxymgrhost> <user> <passwd> [<host>]"
    exit 1
fi

which parallel > /dev/null
if [ $? -ne 0 ]; then
    echo "Please install \"GNU parallel\" for running simultaneous bash jobs"
    exit 1
fi

proxylist=$1
curljobs=`mktemp`
proxymgr=$2
user=$3
passwd=$4

request="google.com"
if [ $# -eq 5 ]; then
    request=$5
fi


for host in `cat $proxylist`; do 
    echo "curl -L --proxy socks5://$proxymgr:1080 -U $user-$host-1080:$passwd $request";
done > $curljobs

cat $curljobs | parallel #>/dev/null 2>&1
unlink $curljobs
