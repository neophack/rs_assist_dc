#!/bin/bash

my_path=$(dirname $0)
my_path=`cd $my_path;pwd`

cd $my_path

net_file=$1

if [ -z "$net_file"  ]
then
	echo "USED net.txt file"
        exit
fi

function upload_config(){
    serverip="23"
    ntpserverip="23"
    port="12345"
    ip=$1
    cameraid=$2
    
    if [ -z $ip ] || [ -z $cameraid ] 
    then
        echo "not right ip cameraid"
	exit 0
    fi
    
    echo "##" >config
    echo "ip=\"$ip\"" >>config
    echo "serverip=\"192.168.254.$serverip\"" >>config
    echo "ntpip=\"192.168.254.$ntpserverip\"" >>config
    echo "serverport=\"$port\"" >>config
    echo "cameraid=\"$cameraid\"" >>config
    echo "frametype=\"0\"" >>config
    echo "##end" >>config
    cat config
    sleep 1
    if [ "x"$ip == "x"  ]
    then
       echo "not find ip :$ip" 
       exit
    fi
}

function echo_cmd(){
    echo $1
    #sleep 1
}

function run_cmd(){
   cmd=$2
    while read ip cameraid
    do
        {
        echo "#########################${ip}####${cameraid}######################################"
        (
            echo_cmd xeye010OK
            echo_cmd "$cmd"  
            echo_cmd "rtwaittime 200"  
            echo_cmd "exit"  
        ) | ./xeyeclient "${ip}"
        }
    done < $net_file

}


function reset_xeye(){
    while read ip cameraid
    do
        echo "#########################${ip}####${cameraid}######################################"
        upload_config $ip $cameraid
        (
	    if [ -f "config" ]
            then
                echo_cmd xeye010OK
                echo_cmd "upload 0 config"
                echo_cmd reset
                echo_cmd "serverip"  
            fi 
        ) | ./xeyeclient "${ip}"
    done < $net_file
}

if [ "x$2" == "xreset"  ]
then
    reset_xeye $1 $2
else
    run_cmd $1 $2
fi

#reset_xeye
