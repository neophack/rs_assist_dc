/***************************************************************************
 *
 * Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
 *
 **************************************************************************/



/**
 * @file tcp_server.cpp
 * @author work(com@baidu.com)
 * @date 2018/04/03 14:08:09
 * @brief
 *
 **/
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>
#include <ctime>
#include <memory.h>
#include <chrono>
#include "tcp_server.hpp"
#include "pack.hpp"
void * rec_data(void *args);
int unpack(char *buf, int pack_len, void *args);
void * init_rec_args(void *rec_args);
void destroy_thread(void *rec_args) ;
Table_type table_type;
//int tmp_flag = 0;
uint64_t getUnixTimeStamp() {
    std::chrono::time_point<std::chrono::system_clock>p1;
    p1 = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds> (p1.time_since_epoch()).count();
}
tcp_server::tcp_server(int listen_port, int type) {
    table_type = static_cast<Table_type>(type);
    if (table_type == goods) {
      std::cout << "Table type: goods" <<std::endl;
    } else if (table_type == human) {
      std::cout << "Table type: human" <<std::endl;
    }

    if(( socket_fd = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 ){
            throw "socket() failed";
    }

    memset(&myserver,0,sizeof(myserver));
    myserver.sin_family = AF_INET;
    myserver.sin_addr.s_addr = htonl(INADDR_ANY);
    myserver.sin_port = htons(listen_port);

    bool opt = true;
    setsockopt(socket_fd,SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(&opt));
    if( bind(socket_fd,(sockaddr*) &myserver,sizeof(myserver)) < 0 ) {
            throw "bind() failed";
    }
    if( listen(socket_fd,100< 0 )) {
            throw "listen() failed";
    }
}

int tcp_server::recv_msg() {
    while( 1 ) {
      socklen_t sin_size = sizeof(struct sockaddr_in);
      if(( accept_fd = accept(socket_fd,(struct sockaddr*) &remote_addr,&sin_size)) == -1 )
      {
              throw "Accept error!";
              continue;
      }
      printf("Received a connection from %s\n",(char*) inet_ntoa(remote_addr.sin_addr));
      comm_args_t *comm_args = new comm_args_t();
      comm_args->fd = accept_fd;
      comm_args->rec_args = init_rec_args(NULL);
      int ret = pthread_create(&(comm_args->thread_id), NULL, rec_data, comm_args);
      if(ret) {
          printf("Create pthread error!");
          return 1;
      }
      usleep(100000);
    }
    return 0;
}

void * rec_data(void * comm_args)
{
    //int flag = tmp_flag++;
    comm_args_t * comm_args_tmp = (comm_args_t *) comm_args;
    int fd = comm_args_tmp->fd;
    int client_sockfd = fd;
    int rec_buf_len = 0;
    int total_len = 0;
    char buffer[MAXSIZE];
    char rec_buf[1024*1024*5];
    while(1) {
        memset(buffer,0,sizeof(buffer));
        int rec_len = read(client_sockfd,buffer,1024*1024*5 - rec_buf_len);
        //printf("5555555555555 %lu rec_buf_len: %d, %p,  total_len: %d, rec_len: %d \n", pthread_self(), rec_buf_len, rec_buf + rec_buf_len, total_len, rec_len);
        if( rec_len <= 0 ) {
            close(client_sockfd);
            destroy_thread(comm_args_tmp->rec_args);
            delete comm_args_tmp;
            pthread_exit(0);
        }
        memmove(&rec_buf[rec_buf_len], buffer, rec_len);
        rec_buf_len = rec_buf_len + rec_len;
        if( rec_len > 0 && rec_buf_len + rec_len > 4) {
            if(total_len == 0 && rec_buf_len >= 4) {
                total_len = *(int32_t*) rec_buf;
            }
            if(total_len != 0 && total_len + 4 <= rec_buf_len) {
                //whole package
                //printf("66666666666666666 total_len: %d\n", total_len);
                char pack_buf[1024*1024];
                memset(pack_buf,0, sizeof(pack_buf));
                memcpy(pack_buf,rec_buf,total_len+4);
                memcpy(&rec_buf[0], &rec_buf[total_len+4], rec_buf_len - total_len-4);
                memset(&rec_buf[rec_buf_len - total_len-4], 0, sizeof(rec_buf) - rec_buf_len + total_len +4);
                rec_buf_len = rec_buf_len - total_len -4;
                //uint64_t write_time = getUnixTimeStamp() + 60 * 1000;;
                //if (flag == 0)
                //printf("%lu 1111111111111111:%lu\n",pthread_self(), write_time); 
                unpack(pack_buf, total_len, comm_args_tmp->rec_args);
                //write_time = getUnixTimeStamp() + 60 * 1000;;
                //if (flag == 0)
		//printf("%lu 2222222222222222:%lu\n",pthread_self(), write_time);
                //printf("444444444444444  rec_buf_len: %d, total_len: %d \n", rec_buf_len, total_len);
                total_len = 0;
                //rec_buf_len = 0;
            }
        }
    }
}


/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
