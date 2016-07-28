#include "radiocomponent.hpp"
#include <string.h>

#define LOOKUP_SIZE 1024
//#define CHUNK_SIZE 1024
//#define BUFFER_SIZE 128*CHUNK_SIZE // size of buffer in bytes

class passthru : public radiocomponent {
  public: 
    passthru ( std::string inBuffFile, std::string outBuffFile )
      : radiocomponent(inBuffFile, outBuffFile) {
    };
    ~passthru () {
    }

    void run () {
      unsigned int oldReg = -1;
      flock(fd2, LOCK_EX);
      while(1) {
        rclisten(inReg);
        unsigned int addr = inReg[0];
        size_t buffSize = inReg[1];
        //printf("Got a message. size: %i, location: %u\n", localReg[1], localReg[0]);
        printf("%i %i %i\n", inReg[0], inReg[1], inReg[2]);
        memcpy(outBuffer+inReg[0], inBuffer+inReg[0], buffSize);
        memcpy(outReg, inReg, 8*sizeof(int32_t));
        notify(outReg);
      }
      return;
    }
  private:
};
int main(int argc, char *argv[]) {
  try{
    passthru test("write.tmp", "write2.tmp");
    test.post(5100);
    test.subscribe(5000);
    test.run();
    return 0;
  }
  catch (const char* msg) {
    std::cerr << msg << std::endl;
    return -1;
  }
};
