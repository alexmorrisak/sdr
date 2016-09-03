#include "radiocomponent.hpp"

#define LOOKUP_SIZE 256*4096
//#define CHUNK_SIZE 1024
//#define BUFFER_SIZE 128*CHUNK_SIZE // size of buffer in bytes

void help() {
  std::cout << "testwriter [args]\n";
  std::cout << "Arguments:\n";
  std::cout << "[freq]    center frequency of fake data:\n";
  std::cout << "[rate]    data rate fake data:\n";
}
int grab_audio(FILE * fd, int16_t** ptr);

//char * fname = "../Her_Majesty.wav";
char * fname;// = "../SheCameInThruTheBathroomWindow.wav";
FILE * fd;
int rv,i,j;
int16_t * ptr;

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
      size_t lookup_size;
      int32_t * lookup_i;
      int32_t * lookup_q;
      lookup_i = (int32_t *) malloc(LOOKUP_SIZE*sizeof(int32_t));
      lookup_q = (int32_t *) malloc(LOOKUP_SIZE*sizeof(int32_t));
      lookup_size = grab_audio(fd, &ptr);
      for (int inx=0; inx<LOOKUP_SIZE; inx++){
        //lookup_i[inx] = amp*(cos(freq*float(inx)) + 1e-4*(rand() % 1000 - 500));
        //lookup_q[inx] = amp*(sin(freq*float(inx)) + 1e-4*(rand() % 1000 - 500));
        lookup_i[inx] = (int32_t)1.*amp*ptr[2*inx];
        //lookup_q[inx] = (int32_t)1.*amp*ptr[2*inx+1];
        lookup_q[inx] = 0;
      }
      flock(fd2, LOCK_EX);
      size_t audioinx;
      while (1) {
        while (actualSamps < targetSamps){
          i = (i+1) % (BUFFER_SIZE / (2*sizeof(int32_t)));
          //j = i/(2*sizeof(int32_t)) / CHUNK_SIZE;
          j = i / CHUNK_SIZE;
          audioinx++;
          //outBuffer[2*i] = lookup_i[i % LOOKUP_SIZE];
          //outBuffer[2*i+1] = lookup_q[i % LOOKUP_SIZE];
          outBuffer[2*i] = ptr[2*audioinx % lookup_size+0];
          outBuffer[2*i+1] = ptr[2*audioinx % lookup_size+1];
          //if (i%100 == 0) printf("%i: %i\n", i, ptr[audioinx % 1000000]);
          if (j != oldj) {
            outReg[0] = (j*CHUNK_SIZE/(2*sizeof(int32_t))); //address of new data in shared memory
            outReg[1] = (CHUNK_SIZE); //size of new data in shared memory
            outReg[2] = j;
            //printf("ndigits %i @ %i\n", outReg[1], j);
            notify(outReg);
            //printf("Notify. %i %i\n", j, actualSamps);
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
    fname = argv[1];
    fd = fopen(fname, "rb");
    printf("File: %s\n", fname);
    test.run(1, 0, atof(argv[2]));
    return 0;
  }
  catch (const char* msg) {
    std::cerr << msg << std::endl;
    return -1;
  }
};
//#include <stdio.h>
//#include <stdint.h>
//#include <malloc.h>

#define BUF_SIZE 4096
int grab_audio(FILE * fd, int16_t** ptr) {
  fseek(fd, 0L, SEEK_END);
  size_t sz;
  sz = ftell(fd);
  printf("grab audio sz: %i\n", sz);
  *ptr = (int16_t*) malloc(sz);
  fseek(fd, 0L, SEEK_SET);
  //  rv = fread(ptr+j*BUF_SIZE, sizeof(int16_t),nsamps,fd);
  rv = fread(*ptr, sizeof(int16_t), sz / sizeof(int16_t), fd);
  printf("grab audio rv: %i\n", rv);
  return rv;
}
