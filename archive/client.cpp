#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

#include "sockman.hpp"

int main(int argc, char *argv[])
{
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr; 

    if(argc < 2)
    {
        printf("\n Usage: %s <ip of server> \n",argv[0]);
        return 1;
    } 

    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); 

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    printf("connecting to socket %i\n", ntohs(serv_addr.sin_port));
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       perror("connect");
       return 1;
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

    printf("Adding input\n");
    msg.type = SUBSCRIBE;
    msg.length = sizeof(int);
    pid = atoi(argv[2]);
    rv = write(sockfd, &msg, sizeof(sockManMsg));
    rv = write(sockfd, &pid, sizeof(pid));
    rv = read(sockfd, &msg, sizeof(sockManMsg));

    sleep(5);
    printf("Removing input\n");
    msg.type = UNSUBSCRIBE;
    msg.length = sizeof(int);
    pid = atoi(argv[2]);
    rv = write(sockfd, &msg, sizeof(sockManMsg));
    rv = write(sockfd, &pid, sizeof(pid));
    rv = read(sockfd, &msg, sizeof(sockManMsg));

    msg.type = UNREGISTER;
    msg.length = 0;
    rv = write(sockfd, &msg, sizeof(sockManMsg));


    close(sockfd);
    return 0;
    while ( (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0)
    {
      printf("bytes read: %i\n", n);
      recvBuff[n] = 0;
      struct sockaddr_in new_addr;
      new_addr.sin_port = htons(5100);
      memcpy(&new_addr, recvBuff, sizeof(struct sockaddr));
      printf("server address: %u\n", htons(new_addr.sin_port));
      //if(fputs(recvBuff, stdout) == EOF)
      //{
      //    printf("\n Error : Fputs error\n");
      //}
    } 

    if(n < 0)
    {
        printf("\n Read error \n");
    } 

    return 0;
}
