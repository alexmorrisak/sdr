/**
 * @file radiocomponent.hpp
 *
 * @brief Header file for the radiocomponent class
 *
 * @author Alex Morris
 * Contact: alexmorrisak@gmail.com
 *
 */

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
#include <cstdint>

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
#include "../cJSON/cJSON.h"
#include "./cxxopts.hpp"

#define CHUNK_SIZE 1024
#define BUFFER_SIZE 1024*CHUNK_SIZE // size of buffer in bytes
#define HIGH_WATER 1024

class radiocomponent {
  public:

    /**
    * Class constructor.
    *
    * @param inBufferFile input buffer file name.  This file has a region of mapped memory associated with it
    * @param outBufferFile output buffer file name.  This file has a region of mapped memory associated with it
    */
    radiocomponent(std::string inBufferFile, std::string outBufferFile);

    ~radiocomponent();

    /**
    * 
    *
    * @param inBufferFile input buffer file name.  This file has a region of mapped memory associated with it
    * @param outBufferFile output buffer file name.  This file has a region of mapped memory associated with it
    */
    int subscribe(int port);

    int post(int port);
  protected:
    void notify(notifyMsg* msg, size_t len);

    void unregister();

    // This is a blocking read on messages from the up-stream process
    int rclisten();

    void connection_handler(int port);

    int sockfd;
    int fd1, fd2;
    int subscriptionPort;
    int32_t *inBuffer;
    int32_t *outBuffer;
    struct sockaddr_in serv_addr;
    bool connected;

    std::vector<int> clients;
    std::queue<notifyMsg> dq;
    std::mutex dqmtx;
  private:
};

#endif
