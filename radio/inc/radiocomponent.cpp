#include "radiocomponent.hpp"

//class radiocomponent; 

void radiocomponent::connection_handler(int port) {
  printf("created connection handler\n");
  signal(SIGPIPE, SIG_IGN);
  // Create socket for listening to downstream clients
  int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr; 
  char sendBuff[1025];

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
    clients.push_back(connfd);
    //sockManMsg msg;
    //int rv = read(connfd, &msg, sizeof(sockManMsg)); 
    //printf("bytes read: %i\n", rv);
    //switch(msg.type) {
    //  case REGISTER:
    //    clients.push_back(connfd);
    //    msg.type = ACK;
    //    msg.length = 0;
    //    write(connfd, &msg, sizeof(msg));
    //    printf("Registering client\n");
    //    break;
    //  default:
    //    printf("Unknown message type\n");
    //    break;
    //  }
    //  //usleep(10000);
    //  printf("end of while loop\n");
    }
    printf("after while loop\n");
    return;

};

radiocomponent::radiocomponent ( std::string inBufferFile, std::string outBufferFile ){
      int verbose = 0;

      //
      //Open input buffer (mmap'd file) and output buffer (mmap'd file)
      //

      // Open the input file for reading
      if (inBufferFile != "") {
        if ((fd1 = open (inBufferFile.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0) {
          throw "Error on file open()";
        }
        if (ftruncate(fd1, BUFFER_SIZE*sizeof(char))) {
          throw "Error on file ftruncate()";
        }
        inBuffer = (int32_t*) mmap((caddr_t)0, 4*BUFFER_SIZE*sizeof(char), PROT_READ|PROT_WRITE,
            MAP_SHARED, fd1, 0);
      }

      // Open the output file for writing
      if (outBufferFile != "") {
        if ((fd2 = open (outBufferFile.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0) {
          throw "Error on file open()";
        }
        if (ftruncate(fd2, BUFFER_SIZE*sizeof(char))) {
          throw "Error on file ftruncate()";
        }
        outBuffer = (int32_t*) mmap((caddr_t)0, 4*BUFFER_SIZE*sizeof(char), PROT_READ|PROT_WRITE,
            MAP_SHARED, fd2, 0);
      }

      printf("inbuffer mapPtr: %lu\n", (intptr_t) inBuffer);
      std::cout << "inbuffer filename: " << inBufferFile << std::endl;
      printf("outbuffer mapPtr: %lu\n", (intptr_t) outBuffer);
      std::cout << "outbuffer filename: " << outBufferFile << std::endl;
      // End open in/out buffers

      //sleep(10);
    }

radiocomponent::~radiocomponent () {
}

int radiocomponent::post(int port) {
  printf("launching connection handler thread\n");
  std::thread t1(&radiocomponent::connection_handler,this, port);
  t1.detach();
  printf("successfully launched connection handler thread\n");
}

int radiocomponent::subscribe(int port) {
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
  //msg.type = REGISTER;
  //msg.length = sizeof(pid_t);
  //pid_t pid = getpid();
  int rv;
  //rv = write(sockfd, &msg, sizeof(sockManMsg));
  //rv = write(sockfd, &pid, sizeof(pid));
  //printf("Reading from socket\n");
  //rv = read(sockfd, &msg, sizeof(sockManMsg));
  //printf("bytes returned: %i\n", rv);
  //if (msg.type == ACK) printf("Connection confirmed\n");
  connected = true;
  // make the socket non-blocking
  int status = fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);
  printf("connection made to file descriptor %i\n", sockfd);
  return 0;
}

void radiocomponent::notify(notifyMsg* msg, size_t len) {
  sockManMsg smsg;
  smsg.type = NOTIFY;
  smsg.length = len;
  std::vector<int> dead_clients;

  // Notify all clients that we are keeping track of
  //printf("Sending to %u clients:\n", clients.size());
  //printf("msg.size: %i\n", msg->size);
  //printf("msg.location: %i\n", msg->location);
  //printf("msg.id: %i\n", msg->id);
  for (int i=0; i<clients.size(); i++){
    //int rv = write(clients[i], &smsg, sizeof(smsg));
    int rv = write(clients[i], msg, sizeof(notifyMsg));
    //int rv = 0;
    // If the write() failed, then good chance the client
    // disconnected.  Remove this client from the client list.
    // TODO: add more logic here.  Is it really dead or some other error?
    if (rv < 0) dead_clients.push_back(i);
  }

  //Remove all clients that were discovered to be dead
  for (int i=0; i<dead_clients.size(); i++){
    printf("Deleting client %i\n", i);
    clients[dead_clients[i]] = clients.back();
    clients.pop_back();
  }

} //End function notify()

void radiocomponent::unregister(){
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

// This is a blocking read on messages from the up-stream process
int radiocomponent::rclisten() {
  //cJSON *buff;
  if (!connected) {
    while (subscribe(subscriptionPort)) {
      printf("can't subscribe.  retrying..\n");
      sleep(1);
    }
  }
  //printf("connected, listening..\n");

  notifyMsg nm;
  sockManMsg msg;

  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(sockfd, &rfds);
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  int rv = select(sockfd+1, &rfds, NULL, NULL, NULL);
  //printf("got something (%i)!!\n", rv);
  int nloops = 0;
  while (rv > 0) {
    nloops++;
    rv = read(sockfd, &nm, sizeof(notifyMsg));
    //printf("read rv: %i\n", rv);
    if (rv == 0) {
      printf("Disconnected..\n");
      connected = false;
      break;
    }
    else if (rv < 0) {
      return rv;
    }
    if (rv == sizeof(notifyMsg)) {
      //FD_ZERO(&rfds);
      //FD_SET(sockfd, &rfds);
      //struct timeval tv;
      //tv.tv_sec = 0;
      //tv.tv_usec = 0;
      //int rv = select(sockfd+1, &rfds, NULL, NULL, NULL); //blocking read
      //rv = read(sockfd, &nm, sizeof(notifyMsg));
      //printf("rv: %i\n", rv);
      //printf("nm.size: %i\n", nm.size);
      //printf("nm.location: %i\n", nm.location);
      //printf("nm.id: %i\n", nm.id);
        // Check if the queue length is less than the
        // high-water mark.  If it isn't, then in means we're not
        // keeping up with incoming samples.  If that's
        // the case, then we won't add anything to the queue.
        if (dq.size() < HIGH_WATER && rv > 0) {
          dqmtx.lock();
          dq.push(nm);
          dqmtx.unlock();
        }
    }
    else {
      printf("unknown message: %i\n", (int) msg.type);
      rv = -1;
    }
  }
  return rv;
}

