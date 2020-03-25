CFLAGS+=-std=c++11 -g3
LIBS+=-lfftw3 -lpthread -l:libcjson.a
INC+=-I./inc/
LIBINC+=-L./cJSON/
all: testwriter testreader spectrogram #pulsedetector filter passthru

testreader.o: testreader.cpp inc/radiocomponent.hpp
	g++ -c testreader.cpp $(CFLAGS) $(INC) $(LIBS) $(LIBINC) -o testreader.o

testreader: testreader.o inc/radiocomponent.o
	g++ testreader.o inc/radiocomponent.o $(INC) $(LIBINC) $(LIBS) -o testreader

spectrogram.o: spectrogram.cpp inc/radiocomponent.hpp
	g++ -c spectrogram.cpp $(CFLAGS) $(INC) $(LIBS) $(LIBINC) -o spectrogram.o

spectrogram: spectrogram.o inc/radiocomponent.o
	g++ spectrogram.o inc/radiocomponent.o $(INC) $(LIBINC) $(LIBS) -o spectrogram

pulsedetector.o: pulsedetector.cpp inc/radiocomponent.hpp
	g++ -c pulsedetector.cpp $(CFLAGS) $(INC) $(LIBS) $(LIBINC) -o pulsedetector.o

pulsedetector: pulsedetector.o inc/radiocomponent.o
	g++ pulsedetector.o inc/radiocomponent.o $(INC) $(LIBINC) $(LIBS) -o pulsedetector

testwriter.o: testwriter.cpp inc/radiocomponent.hpp
	g++ -c testwriter.cpp $(CFLAGS) $(INC) $(LIBS) $(LIBINC) -o testwriter.o

testwriter: testwriter.o inc/radiocomponent.o
	g++ testwriter.o inc/radiocomponent.o $(LIBS) $(LIBINC) -o testwriter

inc/radiocomponent.o: inc/radiocomponent.cpp
	g++ -c inc/radiocomponent.cpp $(CFLAGS) $(INC) $(LIBS) $(LIBINC) -o inc/radiocomponent.o

clean:
	rm  testreader testwriter
  
