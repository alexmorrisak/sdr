#include "radiocomponent.hpp"
#include <getopt.h>

#define LOOKUP_SIZE 256*4096

void help() {
  std::cout << "testwriter [args]\n";
  std::cout << "Arguments:\n";
  std::cout << "[freq]    center frequency of fake data:\n";
  std::cout << "[rate]    data rate fake data:\n";
}
int grab_audio(FILE * fd, int16_t** ptr);

//char * fname = "../Her_Majesty.wav";
//char * fname;// = "../music/Karma_Police.wav";
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
          notifyMsg nmsg;
          int32_t * lookup_i;
          int32_t * lookup_q;
          lookup_size = grab_audio(fd, &ptr);
          lookup_i = (int32_t *) malloc(lookup_size*sizeof(int32_t));
          lookup_q = (int32_t *) malloc(lookup_size*sizeof(int32_t));
          for (int inx=0; inx<lookup_size/2; inx++){
            lookup_i[inx] = (int32_t)1.*amp*(ptr[2*inx] + ptr[2*inx+1]);
            lookup_q[inx] = 0;
          }
          flock(fd2, LOCK_EX);
          size_t audioinx = 0;
          std::cout << "Press enter to continue..\n";
          std::cin.get();
          sleep(.5);
        
          while (1) {
            while (actualSamps < targetSamps){
              // Calculate the buffer indicies.  i is fine-grained, j is coarse-grained
              i = (i+1) % (BUFFER_SIZE / (2*sizeof(int32_t)));
              j = i / CHUNK_SIZE;
        
              audioinx++;
              outBuffer[2*i] = lookup_i[audioinx % (lookup_size/2)];
              outBuffer[2*i+1] = lookup_q[audioinx % (lookup_size/2)];
        
              //Notify the client of new data chunk
              if (j != oldj) {
                nmsg.location = (size_t) (j*CHUNK_SIZE); //address of new data in shared memory
                //printf("location: %i\n", nmsg.location);
                nmsg.size = (size_t) (CHUNK_SIZE); //size of new data in shared memory
                nmsg.id = j;
                notify(&nmsg, sizeof(nmsg));
                if (0) {
                  for (int inx=0; inx<CHUNK_SIZE; inx++){
                    printf("%.1f ", float(outBuffer[inx]));
                  }
                }
                oldj = j;
              }
        
              //Increment the number of samples we have processed
              actualSamps++;
            }
        
            // If we're ahead of schedule then we sleep for a while
            while (actualSamps > targetSamps - 0.1*rate) {
              //if (verbose) printf("sleeping\n");
              usleep(1000);
              gettimeofday(&time, NULL);
              elapsed_usec = 1e6*time.tv_sec + time.tv_usec - start_usec;
              if (time.tv_sec != old_sec) {
                printf("Completed %lu samples (%lu desired)\n", actualSamps, targetSamps);
                old_sec = time.tv_sec;
              }
              targetSamps = elapsed_usec * 1e-6*rate;
            } //End sleep
          }
    }
    private:
};
int main(int argc, char *argv[]) {
    cxxopts::Options options(argv[0], "Test program that creates samples according to specified rate");
    options.add_options()
      ("f,file", "File name", cxxopts::value<std::string>())
      ("r,rate", "Sample rate", cxxopts::value<float>())
      ;
    auto result = options.parse(argc, argv);

    //std::string fname = result["file"].as<std::string>();
    //float rate = result["rate"].as<float>();
    std::string fname;
    float rate;
    try {
        std::cout << "hello\n";
        //std::cout << result << std::endl;
        std::cout << result["file"].as<std::string>() << std::endl;
        std::cout << "bye\n";
        fname = result["file"].as<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "Error! Problem parsing program options." << std::endl;
        std::cerr << e.what() << std::endl;
        std::cerr << options.help() << std::endl;
        return -1;
    }
    try {
        rate = result["rate"].as<float>();
    } catch (const std::exception& e) {
        std::cerr << "Error! Problem parsing program option rate." << std::endl;
        std::cerr << e.what() << std::endl;
        std::cerr << options.help() << std::endl;
        return -1;
    }


    std::thread t1;
    try{
        testwriter test("write.tmp");
        //std::thread t1(test.connection_handler,5000);
        test.post(5000);
        //fname = argv[1];
        fd = fopen(fname.c_str(), "rb");
        printf("filename: %s\n", fname.c_str());
        if (fd <= 0) {
            perror("fopen()");
            std::cerr << "Error opening file: " << fname << std::endl;
            return -1;
        }
        printf("File: %s\n", fname.c_str());
        //test.set_input(fname);
        test.run(1, 0, rate);
        return 0;
    }
    catch (const char* msg) {
        std::cerr << msg << std::endl;
        return -1;
    }
};

#define BUF_SIZE 4096
int grab_audio(FILE * fd, int16_t** ptr) {
  fseek(fd, 0L, SEEK_END);
  size_t sz;
  sz = ftell(fd);
  printf("grab audio sz: %lu\n", sz);
  *ptr = (int16_t*) malloc(sz);
  fseek(fd, 0L, SEEK_SET);
  //  rv = fread(ptr+j*BUF_SIZE, sizeof(int16_t),nsamps,fd);
  rv = fread(*ptr, sizeof(int16_t), sz / sizeof(int16_t), fd);
  printf("grab audio rv: %i\n", rv);
  return rv;
}
