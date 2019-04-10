#!/bin/bash

function upload_config(){
    serverip="197"
    ntpserverip="1"
    port="8700"
    ip=$1
    cameraid=$2
    
    if [ -z $ip ] || [ -z $cameraid ] 
    then
        echo "not right ip cameraid"
	exit 0
    fi
    
    echo "##" >config
    echo "ip=\"$ip\"" >>config
    echo "serverip=\"192.168.0.$serverip\"" >>config
    echo "ntpip=\"192.168.0.$ntpserverip\"" >>config
    echo "serverport=\"$port\"" >>config
    echo "cameraid=\"$cameraid\"" >>config
    echo "gsensorserverip=\"192.168.0.197\"" >> config
    echo "gsensorserverport=\"8700\"" >> config
    echo "frametype=\"1\"" >>config
    echo "##end" >>config
    cat config
    sleep 5
    if [ "x"$ip == "x"  ]
    then
       echo "not find ip :$ip" 
       exit
    fi
}

function echo_cmd(){
    echo $1
    sleep 1
}

function echo_cmd2(){
    echo $1
    sleep 0.1
}
function run_cmd(){
   cmd=$1
    while read ip cameraid
    do
        {
        echo "#########################${ip}####${cameraid}######################################"
        (
            echo_cmd xeye010OK
            echo_cmd "$cmd"  
            echo_cmd "exit"  
        ) | ./xeyeclient "${ip}"
        }
    done < net.txt

}

function reset_xeye(){
    while read ip cameraid
    do
        {
        echo "#########################${ip}####${cameraid}######################################" > ttt
        tm=$(date +%s%3N)
        filename=${tm}_${cameraid}
        (
            echo_cmd2 xeye010OK
            echo_cmd "get jpg ${filename}.jpg"
	    echo_cmd2 "quit"
        ) | ./xeyeclient "${ip}" >> ttt &
	}  
    done < net.txt
}

wait


#run_cmd $1
reset_xeye
