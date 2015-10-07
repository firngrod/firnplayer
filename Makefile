CC=g++
CFLAGS=-std=c++11 -pthread
CLFLAGS=-c -std=c++11 -pthread
MP3FLAGS=
MP3LIBS=-lmpg123
SFMLLIBS=-lsfml-system -lsfml-network -lsfml-audio
BOOSTLIBS= -lboost_regex
FMODLIBS=-lfmod
TAGLIBS=-ltag -lz
NCURSESLIBS=-lncurses
BOOSTLIBS=-lboost_filesystem -lboost_system


firnplayer : firnplayer.cpp Playlist MusicMp3 MetaData Player ViewPort AccessQueue QueueToken
	$(CC) $@.cpp $(CLFLAGS) -o firnplayer.o
	$(CC) -o $@ $(CFLAGS) MusicMp3.o MetaData.o Playlist.o Player.o ViewPort.o ViewPortDivisor.o AccessQueue.o QueueToken.o firnplayer.o -lpthread -std=c++11 $(LIBS) $(MP3LIBS) $(TAGLIBS) $(NCURSESLIBS) $(BOOSTLIBS) $(SFMLLIBS)


.cpp.o:
	$(CC) $(CLFLAGS) $@.cpp -o $@.o

MusicMp3: MusicMp3.cpp
	$(CC) $(CLFLAGS) $@.cpp -o $@.o

MetaData: MetaData.cpp
	$(CC) $(CLFLAGS) $@.cpp -o $@.o

Player: Player.cpp
	$(CC) $(CLFLAGS) $@.cpp -o $@.o 

ViewPort: ViewPort.cpp ViewPortDivisor
	$(CC) $(CLFLAGS) $@.cpp -o $@.o

ViewPortDivisor: ViewPortDivisor.cpp
	$(CC) $(CLFLAGS) $@.cpp -o $@.o

QueueToken: QueueToken.cpp
	$(CC) $(CLFLAGS) $@.cpp -o $@.o

Playlist: Playlist.cpp
	$(CC) $(CLFLAGS) $@.cpp -o $@.o

AccessQueue: AccessQueue.cpp
	$(CC) $(CLFLAGS) $@.cpp -o $@.o

all: firnplayer

