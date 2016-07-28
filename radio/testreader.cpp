#include "radiocomponent.hpp"

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <sys/file.h>

#include <fftw.h>
#include <cmath>

//#define CHUNK_SIZE 128
//#define BUFFER_SIZE 128*CHUNK_SIZE // size of buffer in bytes

void send_data_php(int conn, std::string out, size_t nbytes) {
  printf("This function will send data to the PHP server...\n");
  write(conn, &out.front(), out.size());
}

class testreader : public radiocomponent {
  public: 
    testreader ( std::string readFile )
      : radiocomponent( readFile, "") {
    };
    ~testreader () {
    }

    void run () {

      /* Create the client connection to the php server.  Note that this 
         following bit of code will execute once and then continue past.  So 
         the PHP server should already be running at the time of exectution.
         This should be in a re-try loop or something similar to keep looking
         for an available PHP server
      */
      int phpconn = -1;
      int port = 1337;
      int connected = -1;
      while (connected < 0) {
        connected = 0;
        if((phpconn = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\n Error : Could not create socket \n");
            connected = -1;
        }
        printf("Subscribing to port number %i\n", port);
        struct sockaddr_in serv_addr;
        memset(&serv_addr, '0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
            printf("\n inet_pton error occured\n");
            connected = -1;
            //return -1;
        }

        printf("connecting to socket %i\n", ntohs(serv_addr.sin_port));
        if( connect(phpconn, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
           printf("\n Error : Connect Failed \n");
           perror("connect");
           connected = -1;
           //return -1;
        }
        if (phpconn < 0) connected = -1;
        if (connected == -1) {
          printf("retrying..\n");
          sleep(1);
        }
        else {
          printf("success!\n");
        }
      }

      while (1) {
        rclisten(inReg);
        //printf("Got something\n");
        fflush(stdout);
        //printf("Got a message. size: %i, location: %u\n", localReg[1], localReg[0]);
        size_t ndigits = inReg[1] / (2*sizeof(int32_t));
        int addr = inReg[0];
        fftw_complex in[2*ndigits], out[2*ndigits];
        fftw_plan p;
        for(int i=0; i<ndigits; i++){
            in[i].re = float(inBuffer[addr+2*i]);
            in[i].im = float(inBuffer[addr+2*i+1]);
        }
        for(int i=ndigits; i<2*ndigits; i++){
            in[i].re = 0;
            in[i].im = 0;
        }
        p = fftw_create_plan(2*ndigits, FFTW_FORWARD, FFTW_ESTIMATE);
        fftw_one(p, in, out);
        
        for(int i=0; i<2*ndigits; i++){
          //printf("ndigits: %i\n", ndigits);
          //printf("%3.0f ", 10*std::log10(float(in[i].re*out[i].re) + float(in[i].im*out[i].im)));
          printf("%3.0f ", 10*std::log10(float(out[i].re*out[i].re) + float(out[i].im*out[i].im)));
        }
        printf("\n");


        std::string data_str;
        std::string tmp_str;
        for (int i=0; i<2*ndigits; i++) {
          //std::string tmp_str = sprintf("%3.0f ", 10*std::log10(float(out[i].re*out[i].re) + float(out[i].im*out[i].im)));
          tmp_str = std::to_string(int(10*std::log10(float(out[i].re*out[i].re) + float(out[i].im*out[i].im))));
          tmp_str.push_back(' ');
          data_str = data_str + tmp_str;
        }
        data_str.push_back('\n');
        printf("Phpconn: %i\n", phpconn);
        send_data_php(phpconn, data_str, data_str.size());

        fftw_destroy_plan(p);  
      }
      return;
    }

  private:
};

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Please supply TCP port to subscribe to\n");
    return -1;
  }
  try{
    testreader test( "write.tmp");
    test.subscribe(atoi(argv[1]));
    //test.post(5001);
    test.run();
    //test.rclisten();
    
    return 0;
  }
  catch (const char* msg) {
    std::cerr << msg << std::endl;
    return -1;
  }
};
