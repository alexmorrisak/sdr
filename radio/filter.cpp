#include "radiocomponent.hpp"
#include <string.h>

#define LOOKUP_SIZE 1024
//#define CHUNK_SIZE 1024
//#define BUFFER_SIZE 128*CHUNK_SIZE // size of buffer in bytes

class filter : public radiocomponent {
  public: 
    filter ( std::string inBuffFile, std::string outBuffFile )
      : radiocomponent(inBuffFile, outBuffFile) {
    };
    ~filter () {
    }

    void run (float freq) {
      int32_t* inPtr;
      int32_t* outPtr;
      unsigned int oldReg = -1;
      while(1) {
        rclisten(inReg);
        unsigned int addr = inReg[0];
        size_t buffSize = inReg[1];
        //printf("Got a message. size: %i, location: %u\n", localReg[1], localReg[0]);
        printf("%i %i %i\n", inReg[0], inReg[1], inReg[2]);
        memcpy(outBuffer+inReg[0], inBuffer+inReg[0], buffSize);
        outPtr= outBuffer + inReg[0];
        inPtr= inBuffer + inReg[0];
        size_t numSamps = buffSize / (2*sizeof(int32_t));
        printf("Num samps: %i\n", numSamps);
        for (int i=0; i<numSamps-1; i++) {
          //outPtr[2*i] = cos(2.14*i) *(inPtr[2*i] + inPtr[2*i+2]);
          //outPtr[2*i+1] = sin(2.14*i) *(inPtr[2*i+1] + inPtr[2*i+3]);
          outPtr[2*i] = cos(freq*i) * inPtr[2*i] - sin(freq*i) * inPtr[2*i+1];
          outPtr[2*i+1] = cos(freq*i) * inPtr[2*i+1] + sin(freq*i) * inPtr[2*i];
        }
        memcpy(outReg, inReg, 8*sizeof(int32_t));
        notify(outReg);
      }
      return;
    }
  private:
};
int main(int argc, char *argv[]) {
  if (argc < 4) {
    printf("Please supply cmd line arguments, input port # and output port #, mixer frequency\n");
    return 0;
  }
  try{
    filter test("write.tmp", "filterout.tmp");
    test.post(atoi(argv[2]));
    test.subscribe(atoi(argv[1]));
    test.run(atof(argv[3]));
    return 0;
  }
  catch (const char* msg) {
    std::cerr << msg << std::endl;
    return -1;
  }
};
