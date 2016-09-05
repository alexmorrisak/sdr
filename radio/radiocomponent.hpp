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

#include "registry.hpp"
#include "include/sockman.hpp"
#include "../../cJSON/cJSON.h"

#define CHUNK_SIZE 256
#define BUFFER_SIZE 1024*CHUNK_SIZE // size of buffer in bytes
#define HIGH_WATER 1024

struct dataMsg {
  int32_t size;
  int32_t location;
  int32_t id;
};

std::vector<int>  clients;
void connection_handler(int port);
std::mutex dqmtx;

class radiocomponent {
  public: 
    std::queue<dataMsg> dq;
    radiocomponent ( std::string inBufferFile, std::string outBufferFile ){
      int verbose = 0;

      //
      //Open input buffer (mmap'd file) and output buffer (mmap'd file)
      //

      // Open the input file for reading
      if (inBufferFile != "") {
        if ((fd3 = open (inBufferFile.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0) {
          throw "Error on file open()";
        }
        if (ftruncate(fd3, BUFFER_SIZE*sizeof(char))) {
          throw "Error on file ftruncate()";
        }
        inBuffer = (int32_t*) mmap((caddr_t)0, BUFFER_SIZE*sizeof(char), PROT_READ|PROT_WRITE,
            MAP_SHARED, fd3, 0);
      }

      // Open the output file for writing
      if (outBufferFile != "") {
        if ((fd4 = open (outBufferFile.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0) {
          throw "Error on file open()";
        }
        if (ftruncate(fd4, BUFFER_SIZE*sizeof(char))) {
          throw "Error on file ftruncate()";
        }
        outBuffer = (int32_t*) mmap((caddr_t)0, BUFFER_SIZE*sizeof(char), PROT_READ|PROT_WRITE,
            MAP_SHARED, fd4, 0);
      }

      printf("inbuffer mapPtr: %x\n", (unsigned int) inBuffer);
      std::cout << "inbuffer filename: " << inBufferFile << std::endl;
      printf("outbuffer mapPtr: %x\n", (unsigned int) outBuffer);
      std::cout << "outbuffer filename: " << outBufferFile << std::endl;
      // End open in/out buffers

      //sleep(10);
    }

    ~radiocomponent () {
    }
    int post(int port) {
      printf("launching connection handler thread\n");
      t1 = std::thread(connection_handler, port);
      printf("successfully launched connection handler thread\n");
    }

    int subscribe(int port) {
      subscriptionPort = port;
      sockfd = 0;
      if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
          printf("\n Error : Could not create socket \n");
          return -1;
      } 
      printf("Subscribing to port number %i\n", port);
      memset(&serv_addr, '0', sizeof(serv_addr)); 
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons(port); 
      if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
          printf("\n inet_pton error occured\n");
          return -1;
      } 

      printf("connecting to socket %i\n", ntohs(serv_addr.sin_port));
      if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
         printf("\n Error : Connect Failed \n");
         perror("connect");
         return -1;
      } 

      printf("Registering client\n");
      sockManMsg msg;
      msg.type = REGISTER;
      msg.length = sizeof(pid_t);
      pid_t pid = getpid();
      int rv;
      rv = write(sockfd, &msg, sizeof(sockManMsg));
      rv = write(sockfd, &pid, sizeof(pid));
      printf("Reading from socket\n");
      rv = read(sockfd, &msg, sizeof(sockManMsg));
      printf("bytes returned: %i\n", rv);
      connected = true;
      // make the socket non-blocking
      int status = fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);
      //printf("connection made to file descriptor %i\n", sockfd);
      return 0;
    }

    void notify(char* msg, size_t len) {
      sockManMsg smsg;
      smsg.type = NOTIFY;
      smsg.length = 3*sizeof(int32_t);
      std::vector<int> dead_clients;

      // Notify all clients that we are keeping track of
      for (int i=0; i<clients.size(); i++){
        int rv = write(clients[i], &smsg, sizeof(sockManMsg));
        // If the write() failed, then good chance the client
        // disconnected.  Remove this client from the client list.
        // TODO: add more logic here.  Is it really dead or some other error?
        if (rv < 0) dead_clients.push_back(i);

        // Write the contents of the outReg to client
        rv = write(clients[i], msg, len);
      }

      //Remove all clients that were discovered to be dead
      for (int i=0; i<dead_clients.size(); i++){
        printf("Deleting client %i\n", i);
        clients[dead_clients[i]] = clients.back();
        clients.pop_back();
      }

    } //End function notify()

    void unregister(){
      printf("unregistering\n");
      sockManMsg msg;
      msg.type = UNREGISTER;
      pid_t pid = getpid();
      msg.length = sizeof(pid);
      int rv = write(sockfd, &msg, sizeof(sockManMsg));
      rv = write(sockfd, &pid, sizeof(pid));
      printf("wrote messages to fd %i\n", sockfd);
      close(sockfd);
    }

    int rclisten() {
      cJSON *buff;
      if (!connected) {
        while (subscribe(subscriptionPort)) {
          printf("can't subscribe.  retrying..\n");
          sleep(1);
        }
      }

      struct dataMsg dmLocal;
      int32_t buf[8];
      //char buf[1024];
      sockManMsg msg;

      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(sockfd, &rfds);
      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 0;

      int rv = select(sockfd+1, &rfds, NULL, NULL, NULL);
      int nloops = 0;
      while (rv > 0) {
        nloops++;
        rv = read(sockfd, &msg, sizeof(sockManMsg));
        if (rv <= 0) {
          if (errno == EAGAIN){
            continue;
          }
          else {
            perror("read(): ");
            connected = false;
            return rv;
          }
        }
        int rv = select(sockfd+1, &rfds, NULL, NULL, NULL);
        if (msg.type == NOTIFY) {
          rv = read(sockfd, buf, msg.length);
            // Check if the queue length is less than the
            // high-water mark.  If it isn't, then in means we're not
            // keeping up with incoming samples.  If that's
            // the case, then we won't add anything to the queue.
            if (dq.size() < HIGH_WATER) {

              dmLocal.location = buf[0];
              dmLocal.size = buf[1];
              dmLocal.id = buf[2];
              dqmtx.lock();
              dq.push(dmLocal);
              dqmtx.unlock();
            }
        }
        //else if (msg.type == UNREGISTER) {
        //  printf("unregistering..\n");
        //}
        else {
          printf("unknown message: %i\n", (int) msg.type);
          rv = -1;
        }
      }
      //printf("nloops: %i\n", nloops);
      return rv;
    }

  protected:
    char sendBuff[1025];
    int fd1, fd2, fd3, fd4;
    int i, j;
    int32_t* outBuffer;
    int32_t* inBuffer;
    int32_t localReg[8];
    int32_t inReg[8];
    int32_t outReg[8];
    int verbose;
  private:
    std::thread t1;
    struct sockaddr_in serv_addr; 
    int sockfd;
    bool connected;
    int subscriptionPort;
  //  //std::vector<std::vector<pid_t> > subscribers;
  //  std::vector<int>  clients;
};

/*
 * This will handle connection for each client
 * */
void connection_handler(int port) {
  signal(SIGPIPE, SIG_IGN);
  // Create socket for listening to downstream clients
  int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr; 
  char sendBuff[1025];
  //while(1) {
  //  printf("here i am\n");
  //  sleep(1);
  //}

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&serv_addr, '0', sizeof(serv_addr));
  memset(sendBuff, '0', sizeof(sendBuff)); 

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  int yes = 1;
  if ( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) {
        perror("setsockopt");
  }
  if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
    perror("bind"); 
  }

  printf("Listening on socket %i\n", ntohs(serv_addr.sin_port));
  listen(listenfd, 10); 
  while(1) {
    connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 
    printf("connection accepted\n");
    sockManMsg msg;
    int rv = read(connfd, &msg, sizeof(sockManMsg)); 
    printf("bytes read: %i\n", rv);
    switch(msg.type) {
      case REGISTER:
        clients.push_back(connfd);
        msg.type = ACK;
        msg.length = 0;
        write(connfd, &msg, sizeof(msg));
        printf("Registering client\n");
        break;
      default:
        printf("Unknown message type\n");
        break;
      }
      //usleep(10000);
      printf("end of while loop\n");
    }
    printf("after while loop\n");
    return;

};



