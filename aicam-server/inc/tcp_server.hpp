#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "mysql/mysql.h"
#define MAXSIZE 1024*1024
using namespace std;
struct image_t;
bool filter_frame(vector<image_t> v_image);
enum Table_type {
  goods = 0,
  human = 1
};
class tcp_server
{
private:
        int socket_fd,accept_fd;
        sockaddr_in myserver;
        sockaddr_in remote_addr;

public:
        tcp_server(int listen_port, int type);
        int recv_msg();
};
struct image_t {
    char frame[1024*1024];
    uint16_t image_size;
    image_t () {
        memset(frame, 0, sizeof(frame));
	image_size = 0;
	}
};
struct filter_t {
    vector<image_t> v_image;
    uint8_t is_end;
    uint64_t event_id;
    void reset(uint64_t event) {
       v_image.clear();
       is_end = 0;
       event_id = event;
    }
};
struct comm_args_t {
        int fd;
        int port;
	pthread_t thread_id;
        void *rec_args;
};
