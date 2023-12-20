#!/bin/bash
studentid=311555031
server_ip=140.113.215.197
serverport=$1
code=Qmao1003
# 连接SSH服务器
sshpass -p $code ssh $studentid@$server_ip -p $serverport


#expect{
#	"assword: "{
#	send "$code\r"
#	exp_continue
#	}
#	"yes/no"{
#	send "yes/r"
#	exp_continue
#	}
#	eof{
#	exit	
#	}

#}
#interact
