#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <thread>
#include <vector>
#include <signal.h>

#include "sockman.hpp"

std::vector<std::vector<pid_t> > subscribers;
std::vector<pid_t>  clients;

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

/*
 * This will handle connection for each client
 * */
void connection_handler(int sockfd, pid_t pid)
{
    int rv;
    char buffer[1024];
    signal(SIGPIPE, SIG_IGN);
    pid_t mypid = pid;
    sockManMsg msg;
    printf("Created handler for socket.  Waiting for requests now..\n");
    printf("fd: %i\n", sockfd);
     
    while(1) {
      if (read(sockfd, &msg, sizeof(sockManMsg)) < 0) {; 
        perror("read(): ");
      }
      printf("got message of length %i\n", rv);

      switch(msg.type) {
        case UNREGISTER: {
          printf("unregistering client\n");
          return;
          break;
        }
        case SUBSCRIBE: {
          pid_t newpid;
          rv = read(sockfd, &newpid, msg.length);
          printf("Adding pid: %i\n", newpid);

          /* Add PID to source's list of subscribers*/
          for (int i=0; i < clients.size(); i++){
            if (clients[i] == newpid) {
              subscribers[i].push_back(mypid);
            }
          }

          sockManMsg replymsg;
          replymsg.type = ACK;
          replymsg.length = 0;
          write(sockfd, &replymsg, sizeof(replymsg));
          break;
        }
        case UNSUBSCRIBE: {
          pid_t newpid;
          rv = read(sockfd, &newpid, msg.length);
          printf("Removing pid: %i\n", newpid);

          /* Remove PID from source's list of subscribers*/
          for (int i=0; i < clients.size(); i++){
            if (clients[i] == newpid) {
              for (int j=0; j<subscribers[i].size(); j++) {
                if (subscribers[i][j] == mypid) {
                  subscribers[i].erase(subscribers[i].begin() + j);
                }
              }
            }
          }

          sockManMsg replymsg;
          replymsg.type = ACK;
          replymsg.length = 0;
          write(sockfd, &replymsg, sizeof(replymsg));
          break;
        }
        case NOTIFY: {
          printf("Notifying all subscribers to pid: %i\n", mypid);

          /* Add PID to source's list of subscribers*/
          for (int i=0; i < clients.size(); i++){
            if (clients[i] == mypid) {
              printf("Notifying client pid's: ");
              for (int j=0; j<subscribers[i].size(); j++) {
                printf("%u ", subscribers[i][j]);
              }
              printf("\n");
            }
          }

          sockManMsg replymsg;
          replymsg.type = ACK;
          replymsg.length = 0;
          printf("About to write\n");
          if (write(sockfd, &replymsg, sizeof(replymsg)) < 0) {
            printf("Write to bad socket\n");
            close(sockfd);
            return;
          }
          break;
        }
        default: {
          printf("Unknown message type\n");
          return;
          break;
        }
      }
    }
    return;
} 
