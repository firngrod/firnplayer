#include "includes/MyMusic.hpp"
#include "includes/utils.h"
#include <cstdio>

std::map<int, MyMusic *> MyMusic::ActiveInstances;
MyMusic::MyMusic()
{
  for(InstanceIdentifier = 0; ActiveInstances.find(InstanceIdentifier) != ActiveInstances.end(); InstanceIdentifier++);
  CurrentTrack = new MusicWrapper(InstanceIdentifier, MyMusic::CallbackFunction);
  QueuedTrack  = new MusicWrapper(InstanceIdentifier, MyMusic::CallbackFunction);
  HasQueue = false;
  
  ActiveInstances[InstanceIdentifier] = this;
}

MyMusic::~MyMusic()
{
  QueuedTrack->stop();
  CurrentTrack->stop();
  delete QueuedTrack;
  delete CurrentTrack;
  if(ActiveInstances.find(InstanceIdentifier) != ActiveInstances.end())
    ActiveInstances.erase(ActiveInstances.find(InstanceIdentifier));
}

bool MyMusic::openFromFile(const std::string &filename)
{
  return CurrentTrack->openFromFile(filename);
}

bool MyMusic::enqueueTrack(const std::string &filename)
{
  HasQueue = QueuedTrack->openFromFile(filename);
  return HasQueue;
}

void MyMusic::CallbackFunction(int Instance)
{
  std::printf("In the Callback.\n");
  if(ActiveInstances.find(Instance) == ActiveInstances.end())
    return;
  
  std::printf("No Identifier problems.\n");
  MyMusic &This = *ActiveInstances[Instance];
  //if(This.HasQueue){
    //This.CurrentTrack->stop();
    //This.CurrentTrack->openFromFile("02.ogg");
    //This.CurrentTrack->play();
    
  //}
  //return;
  if(This.HasQueue)
  {
    This.HasQueue = false;
    //int64_t Sleepperiod1 = This.CurrentTrack->getDuration().asMicroseconds();
    //int64_t Sleepperiod2 = This.CurrentTrack->getPlayingOffset().asMicroseconds();
    //std::printf("Need to sleep %ld - %ld = %ld microseconds\n", Sleepperiod1, Sleepperiod2, Sleepperiod1 - Sleepperiod2);
    //std::printf("Track length: %d\n", This.CurrentTrack->getDuration().asSeconds());
    //usleep(Sleepperiod1 - Sleepperiod2);
    This.QueuedTrack->play();
    MusicWrapper * tmp = This.CurrentTrack;
    This.CurrentTrack = This.QueuedTrack;
    This.QueuedTrack = tmp;
  }
}

void MyMusic::play()
{
  CurrentTrack->play();
}

void MyMusic::setVolume(float volume)
{
  CurrentTrack->setVolume(volume);
  QueuedTrack->setVolume(volume);
}
