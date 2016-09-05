CFLAGS+=-std=c++11 -g3
LIBS+=-lfftw -lpthread -l:libcjson.a
LIBINC+=-L../../cJSON/
all: testwriter testreader #filter passthru

testwriter: testwriter.cpp radiocomponent.hpp
	g++ testwriter.cpp $(CFLAGS) $(LIBS) $(LIBINC) -o testwriter

testreader: testreader.cpp radiocomponent.hpp
	g++ testreader.cpp $(CFLAGS) $(LIBS) $(LIBINC) -o testreader

passthru: passthru.cpp radiocomponent.hpp
	g++ passthru.cpp $(CFLAGS) $(LIBS) -o passthru

filter: filter.cpp radiocomponent.hpp
	g++ filter.cpp $(CFLAGS) $(LIBS) -o filter

clean:
	rm testwriter testreader passthru filter
  
