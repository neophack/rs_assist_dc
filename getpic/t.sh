#! /usr/bin/expect
#spawn ./tes.sh
#./xeyeclient 192.168.104.174
#echo [lindex $argv 0]
spawn ./xeyeclient [lindex $argv 0]

send "xeye010OK\r\n"
expect "XEYE>" {send "get jpg [lindex $argv 1].jpg\r\n"}
expect "XEYE>" {send "status\r\n"}
expect "XEYE>" {send "status\r\n"}
expect "XEYE>" {send "quit\r\n"}
#eog [lindex $argv 1].jpg
#interact
expect off
