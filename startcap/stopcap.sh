#!/bin/bash

function echo_cmd(){
    echo $1
    sleep 1
}
while read line
do
    echo "############################################${line}##########################################" 
    (
        echo_cmd xeye010OK
        echo_cmd status
        echo_cmd stopcap
        echo_cmd quit
    ) | ./xeyeclient "${line}"
done < net.txt
