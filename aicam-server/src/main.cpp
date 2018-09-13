/***************************************************************************
 *
 * Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
 *
 **************************************************************************/



/**
 * @file main.cpp
 * @author work(com@baidu.com)
 * @date 2018/04/03 14:08:48
 * @brief
 *
 **/
#include <iostream>
#include <stdlib.h>
#include "tcp_server.hpp"

int main(int argc,char* argv[])
{
	setbuf(stdout, NULL);
        //tcp_server ts(atoi(argv[1]), atoi(argv[2]));  
        tcp_server ts(12345,0);  
        ts.recv_msg();
        return 0;
}





















/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
