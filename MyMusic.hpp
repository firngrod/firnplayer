#include <SFML/Audio.hpp>
#include <map>
#include <string>
#include "utils.h"
#include <cstdio>

class MusicWrapper : public sf::Music
{
public:
  MusicWrapper(int ParentIdentifier, void (*FuncPtr)(int)){
    this->ParentIdentifier = ParentIdentifier;
    CallbackFunctionPtr = FuncPtr;
  }

protected:
  bool onGetData(SoundStream::Chunk& data)
  {
    bool running = sf::Music::onGetData(data);
    if(!running){
      usleep(data.sampleCount * 1000000 / 2 / 44100);
      std::printf("Data count %d, slept %d\n", data.sampleCount, data.sampleCount * 1000000 / 2 / 44100);
      OnMusicEnd();
    }
    return running;
  }

  void OnMusicEnd()
  {
    if(CallbackFunctionPtr)
      CallbackFunctionPtr(ParentIdentifier);
  }

  void (*CallbackFunctionPtr)(int);
  int ParentIdentifier;
};

class MyMusic {
public:
  MyMusic();
  ~MyMusic();

  bool openFromFile(const std::string &filename);
  bool enqueueTrack(const std::string &filename);
  void setVolume(float volume);
  void play();

protected:
  // Variables
  MusicWrapper *CurrentTrack, *QueuedTrack;
  static std::map<int, MyMusic *> ActiveInstances;
  int InstanceIdentifier;
  bool HasQueue;
  
  // Functions
  static void CallbackFunction(int Instance);

};
