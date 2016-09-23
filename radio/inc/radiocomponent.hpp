#ifndef RADIOCOMPONENT_H
#define RADIOCOMPONENT_H

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/file.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */

#include <exception>
#include <time.h>
#include <sys/time.h>

#include "sockman.hpp"
#include "../../../cJSON/cJSON.h"

#define CHUNK_SIZE 1024
#define BUFFER_SIZE 1024*CHUNK_SIZE // size of buffer in bytes
#define HIGH_WATER 1024

//extern std::vector<int> clients;
//extern std::queue<notifyMsg> dq;
//extern void connection_handler(int port);
//extern std::mutex dqmtx;

class radiocomponent {
  public:
    radiocomponent ( std::string inBufferFile, std::string outBufferFile );
    ~radiocomponent ();
    int subscribe(int port);
    int post(int port);
  protected:
    
    void notify(notifyMsg* msg, size_t len);
    
    void unregister();

    // This is a blocking read on messages from the up-stream process
    int rclisten();

    int sockfd;
    int fd1, fd2;
    int subscriptionPort;
    int32_t *inBuffer;
    int32_t *outBuffer;
    struct sockaddr_in serv_addr;
    bool connected;

    std::vector<int> clients;
    std::queue<notifyMsg> dq;
    void connection_handler(int port);
    std::mutex dqmtx;
  private:
};

#endif
