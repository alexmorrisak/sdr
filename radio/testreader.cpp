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
#include <queue>

#include <fftw.h>
#include <cmath>

//#define CHUNK_SIZE 128
//#define BUFFER_SIZE 128*CHUNK_SIZE // size of buffer in bytes
//struct dataMsg {
//  int32_t size;
//  int32_t location;
//  int32_t id;
//};
//std::queue<dataMsg> dq;

class testreader : public radiocomponent {
  public: 
    testreader ( std::string readFile )
      : radiocomponent( readFile, "") {
    };
    ~testreader () {
    }

    void run () {
      int totalPackets = 0;
      int fd;
      char * myfifo = "/tmp/myfifo";
      float buf[2*CHUNK_SIZE];

      /* create the FIFO (named pipe) */
      mkfifo(myfifo, 0666);



      /* Create the client connection to the php server.  Note that this 
         following bit of code will execute once and then continue past.  So 
         the PHP server should already be running at the time of exectution.
         This should be in a re-try loop or something similar to keep looking
         for an available PHP server
      */
      int connected = -1;
      fd = open(myfifo, O_WRONLY);
      printf("fd value: %i\n", fd);
      buf[0] = 0;

      size_t ndigits = 512;
      fftw_complex in[2*ndigits], out[2*ndigits];
      for(int i=ndigits; i<2*ndigits; i++){
          in[i].re = 0;
          in[i].im = 0;
      }
      fftw_plan p;
      p = fftw_create_plan(512, FFTW_FORWARD, FFTW_ESTIMATE);
      dataMsg dqLocal;
      while (1) {
        rclisten();

        // Service all items in the queue
        //printf("Queue size: %i\n", dq.size());
        totalPackets += dq.size();
        printf("total packets: %i\n", totalPackets);
        while (dq.size()) {
          dqmtx.lock();
          ndigits = dq.front().size;
          int addr = dq.front().location;
          int id = dq.front().id;
          dq.pop();
          dqmtx.unlock();
          for(int i=0; i<ndigits; i++){
              in[i].re = float(inBuffer[addr+2*i]);
              in[i].im = float(inBuffer[addr+2*i+1]);
          }
          fftw_one(p, in, out);
          

          // System calls are expensive.  Only do this
          // if we have time!
          /* write data to the FIFO */
          //if ((dq.size() % 50) == 0) {
          //if (false) {
            buf[0]++;
            for (int i=1; i<2*ndigits; i++){
              buf[i] = 10*std::log10(float(out[i].re*out[i].re) + float(out[i].im*out[i].im));
            }
            write(fd, buf, 2*ndigits*sizeof(float));
          //}
        } //Done with all elements in the data queue
      } //processing loop
      fftw_destroy_plan(p);  
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
    test.run();
    
    return 0;
  }
  catch (const char* msg) {
    std::cerr << msg << std::endl;
    return -1;
  }
};
