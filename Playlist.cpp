// Playlist.cpp
#include "includes/json/value.h"
#include <list>
#include <utility>
#include "includes/Playlist.hpp"


Firnplayer::Playlist::Playlist()
{
  CurrentTrack = Tracks.begin();
}


bool Firnplayer::Playlist::Up(const bool &WrapAround)
{
  if(CurrentTrack == Tracks.begin())
  {
    if(WrapAround)
    {
      CurrentTrack = Tracks.end();
      return true;
    }
    else
      return false;
  }
  else
  {
    CurrentTrack--;
    return true;
  }
}


bool Firnplayer::Playlist::Down(const bool &WrapAround)
{
  CurrentTrack++;
  if(CurrentTrack == Tracks.end())
  {
    if(WrapAround)
    {
      CurrentTrack = Tracks.begin();
      return true;
    }
    else
    {
      CurrentTrack--;
      return false;
    }
  }
  else
  {
    return true;
  }
}


bool Firnplayer::Playlist::MoveUp()
{
  if(CurrentTrack == Tracks.begin())
    return false;

  auto PreviousTrack = CurrentTrack;
  PreviousTrack--;
  
  std::swap(PreviousTrack, CurrentTrack);

  return true;
}


bool Firnplayer::Playlist::MoveDown()
{
  auto NextTrack = CurrentTrack;
  NextTrack++;
  
  if(NextTrack == Tracks.end())
    return false;
    
  std::swap(CurrentTrack, NextTrack);
  
  return true;
}


bool Firnplayer::Playlist::Append(Json::Value * NewTrack)
{
  if(NewTrack == NULL)
    return false;
  
  Tracks.push_back(NewTrack);
  return true;
}


bool Firnplayer::Playlist::Insert(Json::Value * NewTrack)
{
  if(NewTrack == NULL)
    return false;

  // Insert *after* current track.
  auto InsertPoint = CurrentTrack;
  InsertPoint++;

  Tracks.insert(InsertPoint, NewTrack);
  return true;
}


bool Firnplayer::Playlist::MoveToStart()
{
  CurrentTrack = Tracks.begin();
  return true;
}


bool Firnplayer::Playlist::MoveToEnd()
{
  CurrentTrack = Tracks.end();
  return true;
}


bool Firnplayer::Playlist::RemoveCurrent()
{
  if(Tracks.size() == 0)
    return false;

  auto ToBeDeleted = CurrentTrack;
  CurrentTrack++;

  Tracks.erase(ToBeDeleted);
  
  if(CurrentTrack == Tracks.end())
    CurrentTrack--;

  return true;
}
