CC=g++ -c
CL=g++
CFLAGS=-std=c++11 -pthread
#MP3LIBS=-lmpg123
#SFMLLIBS=-lsfml-system -lsfml-network -lsfml-audio
#FMODLIBS=-lfmod

# Libs for tag reading
TAGLIBS=-ltag -lz
# NCurses Lib
NCURSESLIBS=-lncurses
# Boost
BOOSTLIBS=-lboost_filesystem -lboost_system
LDFLAGS=$(CFLAGS) $(NCURSESLIBS) $(BOOSTLIBS) $(TAGLIBS)
SOURCES=AccessQueue.cpp MetaData.cpp Playlist.cpp Tracker.cpp ViewPortDivisor.cpp firnplayer.cpp Player.cpp QueueToken.cpp ViewPort.cpp 
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=firnplayer

all: pch.h.gch $(SOURCES) $(EXECUTABLE)

pch.h.gch: pch.h
	$(CC) -o $@ $< $(CFLAGS)

$(EXECUTABLE): $(OBJECTS)
	$(CL) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:  pch.h.gch
	$(CC) $(CFLAGS) $< -o $@


clean:
	rm *.o firnplayer pch.h.gch
