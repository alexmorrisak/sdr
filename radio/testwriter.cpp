#include "radiocomponent.hpp"

#define LOOKUP_SIZE 4096
//#define CHUNK_SIZE 1024
//#define BUFFER_SIZE 128*CHUNK_SIZE // size of buffer in bytes

void help() {
  std::cout << "testwriter [args]\n";
  std::cout << "Arguments:\n";
  std::cout << "[freq]    center frequency of fake data:\n";
  std::cout << "[rate]    data rate fake data:\n";
}


class testwriter : public radiocomponent {
  public: 
    testwriter ( std::string writeFile )
      : radiocomponent("", writeFile) {
    };
    ~testwriter () {
    }

    void run (float amp, float freq, float rate) {
      int yi, yq, oldj;
      struct timeval time;
      size_t start_usec, elapsed_usec;
      i = 0; //Increment for complex numbers within the entire buffer(2 x int32_t)
      j = 0; //Increment for chunk number
      oldj =j;

      gettimeofday(&time, NULL);
      start_usec = 1e6*time.tv_sec + time.tv_usec;
      size_t old_sec = time.tv_sec;
      size_t targetSamps = 0;
      size_t actualSamps = 0;
      int32_t lookup_i [LOOKUP_SIZE];
      int32_t lookup_q [LOOKUP_SIZE];
      for (int inx=0; inx<LOOKUP_SIZE; inx++){
        lookup_i[inx] = amp*(cos(freq*float(inx)) + 1e-4*(rand() % 1000 - 500));
        lookup_q[inx] = amp*(sin(freq*float(inx)) + 1e-4*(rand() % 1000 - 500));
      }
      flock(fd2, LOCK_EX);
      while (1) {
        while (actualSamps < targetSamps){
          i = (i+1) % (BUFFER_SIZE / (2*sizeof(int32_t)));
          j = i*2*sizeof(int32_t) / CHUNK_SIZE;
          outBuffer[2*i] = lookup_i[i % LOOKUP_SIZE];
          outBuffer[2*i+1] = lookup_q[i % LOOKUP_SIZE];
          if (j != oldj) {
            outReg[0] = (j*CHUNK_SIZE/(2*sizeof(int32_t))); //address of new data in shared memory
            outReg[1] = (CHUNK_SIZE); //size of new data in shared memory
            outReg[2] = j;
            //printf("ndigits %i @ %i\n", outReg[1], j);
            notify(outReg);
            //if (verbose) printf("Notifying client\n");
            if (0) {
              for (int inx=0; inx<CHUNK_SIZE; inx++){
                printf("%.1f ", float(outBuffer[inx]));
              }
            }
            oldj = j;
          }
          actualSamps++;
        }
        while (actualSamps > targetSamps - 0.1*rate) {
          //if (verbose) printf("sleeping\n");
          usleep(1000);
          gettimeofday(&time, NULL);
          elapsed_usec = 1e6*time.tv_sec + time.tv_usec - start_usec;
          if (time.tv_sec != old_sec) {
            printf("Completed %u samples (%u desired)\n", actualSamps, targetSamps);
            old_sec = time.tv_sec;
          }
          targetSamps = elapsed_usec * 1e-6*rate;
        }
      }
    }
  private:
};
int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Error!\nPlease supply command line argumentsn");
    help();
    return 0;
  }
  try{
    testwriter test("write.tmp");
    test.post(5000);
    //printf("done posting\n");
    test.run(10000, atof(argv[1]), atof(argv[2]));
    return 0;
  }
  catch (const char* msg) {
    std::cerr << msg << std::endl;
    return -1;
  }
};
