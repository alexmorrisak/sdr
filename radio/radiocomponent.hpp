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

#include <vector>
#include <thread>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */

#include <exception>
#include <time.h>
#include <sys/time.h>

#include "registry.hpp"
#include "include/sockman.hpp"

#define CHUNK_SIZE 128
#define BUFFER_SIZE 128*CHUNK_SIZE // size of buffer in bytes


std::vector<int>  clients;
void connection_handler(int port);

class radiocomponent {
  public: 
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
      return 0;
    }

    void notify(int32_t* outReg) {
      sockManMsg msg;
      printf("Notifying client\n");
      msg.type = NOTIFY;
      msg.length = 3*sizeof(int32_t);
      for (int i=0; i<clients.size(); i++){
        printf("clients[%i]: %i\n", i, clients[i]);
        int rv = write(clients[i], &msg, sizeof(sockManMsg));
        rv = write(clients[i], outReg, 3*sizeof(int32_t));
      }
    }

    int rclisten(int32_t * inReg) {
      if (!connected) {
        while (subscribe(subscriptionPort)) {
          printf("can't subscribe.  retrying..\n");
          sleep(1);
        }
      }
      sockManMsg msg;
      int rv = read(sockfd, &msg, sizeof(sockManMsg));
      if (rv <= 0) {
        perror("read(): ");
        connected = false;
        return rv;
      }
      if (msg.type == NOTIFY) {
        rv = read(sockfd, inReg, msg.length);
        //printf("got NOTIFY from publisher\n");
      }
      else {
        printf("unknown message: %i\n", (int) msg.type);
        rv = -1;
      }
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
      //serv_addr.sin_port = htons(atoi(argv[1])); 
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
            //pid_t newpid;
            //rv = read(connfd, &newpid, msg.length);
            clients.push_back(connfd);
            //subscribers.push_back(std::vector<pid_t> (0));
            //clientThreads.push_back(std::thread(connection_handler, connfd, newpid));

            msg.type = ACK;
            msg.length = 0;
            write(connfd, &msg, sizeof(msg));
            printf("Registering client\n");
            break;
          default:
            printf("Unknown message type\n");
            break;
         }
         usleep(1000);
         printf("end of while loop\n");
       }
       printf("after while loop\n");
       return;

};



//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <errno.h>
//#include <string.h>
//#include <sys/types.h>
//#include <time.h> 
//#include <thread>
//#include <vector>
//#include <signal.h>
//
//#include "sockman.hpp"
//
//std::vector<std::vector<pid_t> > subscribers;
//std::vector<pid_t>  clients;
/*

int main(int argc, char *argv[])
{
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr; 

    char sendBuff[1025];
    time_t ticks; 

    void connection_handler(int fd, pid_t pid);
    signal(SIGPIPE, SIG_IGN);

    if (argc < 2) {
      printf("Please supply socket number for listening\n");
      return 0;
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1])); 

    int yes = 1;
    if ( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 )
    {
          perror("setsockopt");
    }
    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
      perror("bind"); 
    }

    printf("Listening on socket %i\n", ntohs(serv_addr.sin_port));
    listen(listenfd, 10); 

    std::vector<std::thread> clientThreads;
    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 
        printf("connection accepted\n");
        sockManMsg msg;
        int rv = read(connfd, &msg, sizeof(sockManMsg)); 
        printf("bytes read: %i\n", rv);
        switch(msg.type) {
          case REGISTER:
            pid_t newpid;
            rv = read(connfd, &newpid, msg.length);
            clients.push_back(newpid);
            subscribers.push_back(std::vector<pid_t> (0));
            clientThreads.push_back(std::thread(connection_handler, connfd, newpid));

            msg.type = ACK;
            msg.length = 0;
            write(connfd, &msg, sizeof(msg));
            printf("Registering client\n");
            break;
          default:
            printf("Unknown message type\n");
            break;
        }
        usleep(1000);
        printf("end of while loop\n");

     }
        printf("after while loop\n");
}

//void connection_handler(int sockfd, pid_t pid)
//{
//    int rv;
//    char buffer[1024];
//    //signal(SIGPIPE, SIG_IGN);
//    pid_t mypid = pid;
//    sockManMsg msg;
//    printf("Created handler for socket.  Waiting for requests now..\n");
//    printf("fd: %i\n", sockfd);
//     
//    while(1) {
//      if (read(sockfd, &msg, sizeof(sockManMsg)) < 0) {; 
//        perror("read(): ");
//      }
//      printf("got message of length %i\n", rv);
//
//      switch(msg.type) {
//        case UNREGISTER: {
//          printf("unregistering client\n");
//          return;
//          break;
//        }
//        case SUBSCRIBE: {
//          pid_t newpid;
//          rv = read(sockfd, &newpid, msg.length);
//          printf("Adding pid: %i\n", newpid);
//
//          /* Add PID to source's list of subscribers*/
//          for (int i=0; i < clients.size(); i++){
//            if (clients[i] == newpid) {
//              subscribers[i].push_back(mypid);
//            }
//          }
//
//          sockManMsg replymsg;
//          replymsg.type = ACK;
//          replymsg.length = 0;
//          write(sockfd, &replymsg, sizeof(replymsg));
//          break;
//        }
//        case UNSUBSCRIBE: {
//          pid_t newpid;
//          rv = read(sockfd, &newpid, msg.length);
//          printf("Removing pid: %i\n", newpid);
//
//          /* Remove PID from source's list of subscribers*/
//          for (int i=0; i < clients.size(); i++){
//            if (clients[i] == newpid) {
//              for (int j=0; j<subscribers[i].size(); j++) {
//                if (subscribers[i][j] == mypid) {
//                  subscribers[i].erase(subscribers[i].begin() + j);
//                }
//              }
//            }
//          }
//
//          sockManMsg replymsg;
//          replymsg.type = ACK;
//          replymsg.length = 0;
//          write(sockfd, &replymsg, sizeof(replymsg));
//          break;
//        }
//        case NOTIFY: {
//          printf("Notifying all subscribers to pid: %i\n", mypid);
//
//          /* Add PID to source's list of subscribers*/
//          for (int i=0; i < clients.size(); i++){
//            if (clients[i] == mypid) {
//              printf("Notifying client pid's: ");
//              for (int j=0; j<subscribers[i].size(); j++) {
//                printf("%u ", subscribers[i][j]);
//              }
//              printf("\n");
//            }
//          }
//
//          sockManMsg replymsg;
//          replymsg.type = ACK;
//          replymsg.length = 0;
//          printf("About to write\n");
//          if (write(sockfd, &replymsg, sizeof(replymsg)) < 0) {
//            printf("Write to bad socket\n");
//            close(sockfd);
//            return;
//          }
//          break;
//        }
//        default: {
//          printf("Unknown message type\n");
//          return;
//          break;
//        }
//      }
//    }
//    return;
//} 
//#include <sys/types.h>
//#include <netinet/in.h>
//#include <netdb.h>
//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <errno.h>
//#include <arpa/inet.h> 
//
//#include "sockman.hpp"
//
//int main(int argc, char *argv[])
//{
//    int sockfd = 0, n = 0;
//    char recvBuff[1024];
//    struct sockaddr_in serv_addr; 
//
//    if(argc < 2)
//    {
//        printf("\n Usage: %s <ip of server> \n",argv[0]);
//        return 1;
//    } 
//
//    memset(recvBuff, '0',sizeof(recvBuff));
//    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
//    {
//        printf("\n Error : Could not create socket \n");
//        return 1;
//    } 
//
//    memset(&serv_addr, '0', sizeof(serv_addr)); 
//
//    serv_addr.sin_family = AF_INET;
//    serv_addr.sin_port = htons(5000); 
//
//    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
//    {
//        printf("\n inet_pton error occured\n");
//        return 1;
//    } 
//
//    printf("connecting to socket %i\n", ntohs(serv_addr.sin_port));
//    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
//    {
//       printf("\n Error : Connect Failed \n");
//       perror("connect");
//       return 1;
//    } 
//
//    printf("Registering client\n");
//    sockManMsg msg;
//    msg.type = REGISTER;
//    msg.length = sizeof(pid_t);
//    pid_t pid = getpid();
//    int rv;
//    rv = write(sockfd, &msg, sizeof(sockManMsg));
//    rv = write(sockfd, &pid, sizeof(pid));
//    printf("Reading from socket\n");
//    rv = read(sockfd, &msg, sizeof(sockManMsg));
//
//    printf("Adding input\n");
//    msg.type = SUBSCRIBE;
//    msg.length = sizeof(int);
//    pid = atoi(argv[2]);
//    rv = write(sockfd, &msg, sizeof(sockManMsg));
//    rv = write(sockfd, &pid, sizeof(pid));
//    rv = read(sockfd, &msg, sizeof(sockManMsg));
//
//    sleep(5);
//    printf("Removing input\n");
//    msg.type = UNSUBSCRIBE;
//    msg.length = sizeof(int);
//    pid = atoi(argv[2]);
//    rv = write(sockfd, &msg, sizeof(sockManMsg));
//    rv = write(sockfd, &pid, sizeof(pid));
//    rv = read(sockfd, &msg, sizeof(sockManMsg));
//
//    msg.type = UNREGISTER;
//    msg.length = 0;
//    rv = write(sockfd, &msg, sizeof(sockManMsg));
//
//
//    close(sockfd);
//    return 0;
//    while ( (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0)
//    {
//      printf("bytes read: %i\n", n);
//      recvBuff[n] = 0;
//      struct sockaddr_in new_addr;
//      new_addr.sin_port = htons(5100);
//      memcpy(&new_addr, recvBuff, sizeof(struct sockaddr));
//      printf("server address: %u\n", htons(new_addr.sin_port));
//      //if(fputs(recvBuff, stdout) == EOF)
//      //{
//      //    printf("\n Error : Fputs error\n");
//      //}
//    } 
//
//    if(n < 0)
//    {
//        printf("\n Read error \n");
//    } 
//
//    return 0;
//}
