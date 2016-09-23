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
      char * myfifo = (char*) "/tmp/myfifo";
      float buf[16*CHUNK_SIZE];

      /* create the FIFO (named pipe) */
      mkfifo(myfifo, 0666);

      int connected = -1;
      fd = open(myfifo, O_WRONLY);
      printf("fd value: %i\n", fd);
      buf[0] = 0;

      float hamming512[512];
      float hamming1024[1024];
      for (int i=0; i<512; i++){
        hamming512[i] = 0.54 - 0.46*(cos(2*3.14*(i)/513));
      }
      for (int i=0; i<1024; i++){
        hamming1024[i] = 0.54 - 0.46*(cos(2*3.14*(i)/1023));
      }
      size_t fftsize = 1*CHUNK_SIZE;
      size_t ndigits = 0;
      fftw_complex in[fftsize], out[fftsize];
      for(int i=0; i<fftsize; i++){
          in[i].re = 0;
          in[i].im = 0;
      }
      fftw_plan p;
      notifyMsg nm, nm2;
      p = fftw_create_plan(fftsize, FFTW_FORWARD, FFTW_ESTIMATE);
      while (1) {
        rclisten();
        printf("dq size: %i\n", dq.size());

        // Service all items in the queue
        totalPackets += dq.size();
        printf("total packets: %i\n", totalPackets);
        while (dq.size() > 1) {
          printf("Queue size (before): %i\n", dq.size());
          dqmtx.lock();
          nm = dq.front();
          ndigits = nm.size;
          printf("ndigits: %i\n", ndigits);
          int addr = nm.location;
          int id = nm.id;
          dq.pop();

          nm2 = dq.front();
          ndigits = nm2.size;
          printf("ndigits: %i\n", ndigits);
          int addr2 = nm2.location;
          int id2 = nm2.id;

          printf("Queue size (after): %i\n", dq.size());
          printf("addr: %i\n", addr);
          dqmtx.unlock();


          for(int i=0; i<ndigits; i++){
              in[i].re = (float) (inBuffer[addr+2*i]) * hamming1024[i];
              in[i].im = (float) (inBuffer[addr+2*i+1]) * hamming1024[i];
            //printf("%i: %.1f\n", addr+2*i, (float) inBuffer[addr/2+2*i]);
          }
          fftw_one(p, in, out);
          // System calls are expensive.  Only do this
          // if we have time!
          buf[0]++;
          for (int i=0; i<fftsize; i++){
            buf[i] = 10*std::log10(float(out[i].re*out[i].re) + float(out[i].im*out[i].im));
          }
          write(fd, buf, fftsize*sizeof(float));


          for(int i=0; i<ndigits/2; i++){
              in[i].re = (float) (inBuffer[ndigits+addr+2*i]) * hamming1024[i];
              in[i].im = (float) (inBuffer[ndigits+addr+2*i+1]) * hamming1024[i];
            //printf("%i: %.1f\n", addr+2*i, (float) inBuffer[addr/2+2*i]);
          }
          for(int i=ndigits/2; i<ndigits; i++){
              in[i].re = (float) (inBuffer[addr2+2*i]) * hamming1024[i];
              in[i].im = (float) (inBuffer[addr2+2*i+1]) * hamming1024[i];
            //printf("%i: %.1f\n", addr+2*i, (float) inBuffer[addr/2+2*i]);
          }
          fftw_one(p, in, out);
          

          // System calls are expensive.  Only do this
          // if we have time!
          buf[0]++;
          for (int i=0; i<fftsize; i++){
            buf[i] = 10*std::log10(float(out[i].re*out[i].re) + float(out[i].im*out[i].im));
          }
          write(fd, buf, fftsize*sizeof(float));
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
