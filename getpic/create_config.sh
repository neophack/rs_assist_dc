

ip="150"
serverip="91"
if [ $# -gt 0 ]
then
    ip=$1
fi

if [ $# -gt 1 ]
then
    serverip=$2
fi

cameraid=`expr 4000 + $ip`

echo "##" >config
echo "ip=\"192.168.0.$ip\"" >>config
echo "serverip=\"192.168.0.$serverip\"" >>config
echo "ntpip=\"192.168.0.91\"" >>config
echo "serverport=\"12345\"" >>config
echo "cameraid=\"$cameraid\"" >>config
echo "##end" >>config
