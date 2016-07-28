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

#define CHUNK_SIZE 128
#define BUFFER_SIZE 128*CHUNK_SIZE // size of buffer in bytes

class testwriter : public radiocomponent {
  public: 
    testwriter ( std::string regFile, std::string writeFile )
      : radiocomponent("", regFile, "", writeFile) {
    };
    ~testwriter () {
    }

    void run (float amp, float freq, float rate) {
      int oldReg = reg[0];
      while (1) {
          fflush(stdout);
          if (reg[0] != oldReg){
            flock(fd1, LOCK_SH);
            size_t ndigits = reg[1] / (2*sizeof(int32_t));
            int addr = reg[0];
            fftw_complex in[2*ndigits], out[2*ndigits];
            fftw_plan p;
            for(int i=0; i<ndigits; i++){
                in[i].re = float(outBuffer[addr+2*i]);
                in[i].im = float(outBuffer[addr+2*i+1]);
            }
            for(int i=ndigits; i<2*ndigits; i++){
                in[i].re = 0;
                in[i].im = 0;
            }
            p = fftw_create_plan(2*ndigits, FFTW_FORWARD, FFTW_ESTIMATE);
            fftw_one(p, in, out);
            for(int i=0; i<2*ndigits; i++){
              //printf("%8.0f ", 20*std::log10((out[i].re/10000)*(out[i].re/10000) + (out[i].im/1000)*(out[i].im/1000)));
              //printf("(%4.0f %4.0f) \n", in[i].re, in[i].im);
              printf("%3.0f ", 10*std::log10(float(out[i].re*out[i].re) + float(out[i].im*out[i].im)));
            }
            printf("\n");

            fftw_destroy_plan(p);  

            //for(int i=0; i<20; i++){
            //    printf("%09i ", buffer[reg[0]*BUFFER_SIZE+i]);
            //}
            //printf("addr: 0x%x, size: %i\n", reg[0], reg[1]);
            //printf("addr: 0x%x, ndigits: %i\n", reg[0], ndigits);
            oldReg = reg[0];
            //printf("\n\n");
            flock(fd1, LOCK_UN);
            usleep(1);
          }
          else {
            usleep(1000);
          }
      }
      return 0;
    }

  private:
};

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Please enter command line option for center freq\n");
    return 0;
  }
  try{
    testwriter test("outReg.tmp", "write.tmp");
    test.run();
    return 0;
  }
  catch (const char* msg) {
    std::cerr << msg << std::endl;
    return -1;
  }
};
