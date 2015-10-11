#include "pch.h"

// Using namespace out of laziness
//using namespace Firnplayer;

Json::Value Firnplayer::ScanDirectory(const std::string &Root)
{
  // Initializations
  DIR *dir;
  struct dirent *ent;
  std::list<std::string> DirectoryQueue;
  Json::Value AllJson;

  // Push the root to the queue and start the scan loop
  DirectoryQueue.push_back(Root);
  while(DirectoryQueue.size()){
    // Grap the front of the queue and erase it from the queue
    std::string ThisDirectory = *DirectoryQueue.begin();
    DirectoryQueue.erase(DirectoryQueue.begin());
    if((dir = opendir(ThisDirectory.c_str())) != NULL) {  // Try to open the directory
      while ((ent = readdir(dir)) != NULL) {
        // First, see if it is a directory.  If so, we add id to the queue for later scans.
        if(ent->d_type == 4 && *ent->d_name != '.')
          DirectoryQueue.push_back(cleanpath(ThisDirectory + "/" + ent->d_name));
        else{
          // If it is not a queue, we look closer at the file.
          // First, we want to see if it has the .mp3 ending.
          std::string FileName = cleanpath(ThisDirectory + "/" + ent->d_name);
          std::string LastFour("    ");
          std::transform(FileName.begin()+(FileName.size()-4), FileName.end(), LastFour.begin(), ::toupper);
          if(LastFour == ".MP3")
          {
            // Ok, it calls itself an mp3.  Scan it!
            Json::Value &TrackJson = (AllJson[FileName] = Json::Value());
            TagLib::MPEG::File file(FileName.c_str());
            // If it has an ID3v2Tag, we use that.
            if(file.hasID3v2Tag())
            {
              TagLib::ID3v2::Tag *tag = file.ID3v2Tag(true);
              for(TagLib::List<TagLib::ID3v2::Frame *>::ConstIterator theitr = tag->frameList().begin(); theitr != tag->frameList().end(); theitr++)
              {
                std::string framevalue = (*theitr)->toString().to8Bit();
                std::string Trigger((*theitr)->frameID().data());
                Trigger = std::string(Trigger.begin(), Trigger.begin()+4);
                TrackJson[Trigger] = framevalue;
              }
            }
            // Now save the file path, the bitrate and the track length.
            TrackJson["FILE"] = FileName;
            TagLib::MPEG::Properties *Properties = file.audioProperties();
            TrackJson["LENG"] = Properties->length();
            TrackJson["BITR"] = Properties->bitrate();
          }
        }
      }
    }
  }
  return AllJson;
}


////////////////////////////////////////////////////////////////////////////////////////////////
//
// ListAlbum Class functions
//

Firnplayer::ListAlbum::ListAlbum(const std::string &Name)
{
  this->Name = Name;
}

Firnplayer::ListAlbum::ListAlbum(Json::Value * TrackJson)
{
  this->Name = (*TrackJson)["TALB"].asString();
  AddTrack(TrackJson);
}

std::string Firnplayer::ListAlbum::GetName()
{
  return Name;
}

Json::Value * Firnplayer::ListAlbum::operator[](const unsigned int &TrackNumber)
{
  if(TrackNumber < Tracks.size())
    return Tracks[TrackNumber];
  else
    return NULL;
}

int Firnplayer::ListAlbum::GetTrackCount()
{
  return Tracks.size();
}

void Firnplayer::ListAlbum::AddTrack(Json::Value * TrackJson)
{
  std::vector<Json::Value *>::iterator TrackItr = Tracks.begin(), TrackEnd = Tracks.end(),
                                             InsertPoint = Tracks.end();
  // Since this is sorting stuff, we look through the current library
  for(; TrackItr != TrackEnd; TrackItr++)
  {
    // If we notice the file already in the list, we don't have to do anything.
    if(*TrackItr == TrackJson)
    {
      return;
    }
    
    // When the track number of the iterator exceeds the track number of the new track, that is where we want
    // to insert the new track.
    if(SafeStoi((**TrackItr).get("TRCK", "0").asString()) > SafeStoi((*TrackJson).get("TRCK", "0").asString()))
    {
      InsertPoint = TrackItr;
      break;
    }
  }
  Tracks.insert(InsertPoint, TrackJson);
}

void Firnplayer::ListAlbum::GetTrackList(std::vector<Json::Value *> &TrackListOut, std::vector<std::string> &TrackListOutPretty)
{
  std::vector<std::pair<int, std::string> > TrackInfoForPrint;
  int LargestTrackNo = 0;
  for(auto TrackPtr: Tracks)
  {
    // First, let's get the data
    TrackListOut.push_back(TrackPtr);
  
    // Then, let's get info for the pwetty pwintable list.
    int ThisTrackNo = SafeStoi(TrackPtr->get("TRCK", "0").asString());
    std::string ThisTrackName = TrackPtr->get("TIT2", "Unknown Track").asString();
    TrackInfoForPrint.push_back(std::pair<int, std::string>(ThisTrackNo, ThisTrackName));
    if(ThisTrackNo > LargestTrackNo)
      LargestTrackNo = ThisTrackNo;
  }
  
  // Get the width of the track number column;
  int NumberWidth = 2;
  do
  {
    LargestTrackNo /= 10;
    NumberWidth ++;
  } while(LargestTrackNo > 0);

  // Now print the actual list;
  for(auto Track: TrackInfoForPrint)
  {
    TrackListOutPretty.push_back(StringPrintf("%*d - %s", NumberWidth, Track.first, Track.second.c_str()));
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////
//
// ListArtist Class functions
//

Firnplayer::ListArtist::ListArtist(const std::string &Name)
{
  this->Name = Name;
  Expanded = false;
}

Firnplayer::ListArtist::ListArtist(Json::Value * TrackJson)
{
  this->Name = (*TrackJson)["TPE2"].asString();
  Expanded = false;
  AddTrack(TrackJson);
}

std::string Firnplayer::ListArtist::GetName()
{
  return Name;
}

Firnplayer::ListAlbum * Firnplayer::ListArtist::operator[](const unsigned int &AlbumNumber)
{
  if(AlbumNumber < Albums.size())
    return &Albums[AlbumNumber];
  else
    return NULL;
}

Firnplayer::ListAlbum * Firnplayer::ListArtist::operator[](const std::string &AlbumName)
{
  std::vector<ListAlbum>::iterator AlbumItr = std::find(Albums.begin(), Albums.end(), AlbumName);
  if(AlbumItr != Albums.end())
    return &(*AlbumItr);
  else
    return NULL;
}

void Firnplayer::ListArtist::AddTrack(Json::Value * TrackJson)
{
  if(TrackJson->get("TALB", "").asString() == "")
    (*TrackJson)["TALB"] = "Unknown Album";
  int i;
  for(i = 0; (i < Albums.size()) && !(Albums[i] == (*TrackJson)["TALB"].asString()); i++);
  if(i < Albums.size())
    Albums[i].AddTrack(TrackJson);
  else{
    std::vector<ListAlbum>::const_iterator AlbItr = Albums.begin(), AlbEnd = Albums.end();
    for(; AlbItr != AlbEnd && (*AlbItr <= (*TrackJson)["TALB"].asString()); AlbItr++);
    Albums.insert(AlbItr, ListAlbum(TrackJson));
  }
}

int Firnplayer::ListArtist::GetAlbumCount()
{
  return Albums.size();
}

int Firnplayer::ListArtist::GetLineUsage()
{
  return 1 + (Expanded ? Albums.size() : 0);
}

int Firnplayer::ListArtist::GetAlbumPosition(const std::string &AlbumName)
{
  if(!AlbumName.size())
    return -1;

  std::vector<ListAlbum>::iterator AlbumItr = std::find(Albums.begin(), Albums.end(), AlbumName);
  if(AlbumItr == Albums.end())
    return -1;
  else
    return std::distance(Albums.begin(), AlbumItr);
}

void Firnplayer::ListArtist::GetTrackList(std::vector<Json::Value *> &TrackListOut,
                              std::vector<std::string> &TrackListOutPretty, std::string AlbumName)
{
  ListAlbum * ActiveAlbum = operator[](AlbumName);

  if(ActiveAlbum != NULL)
  {
    ActiveAlbum->GetTrackList(TrackListOut, TrackListOutPretty);
    return;
  }
  else
  {
    int AlbumCount = GetAlbumCount();
    for(int i = 0; i < AlbumCount; i++)
    {
      TrackListOut.push_back(NULL);
      TrackListOutPretty.push_back(operator[](i)->GetName());
      operator[](i)->GetTrackList(TrackListOut, TrackListOutPretty);
    }
    return;
  }
}
  
  
////////////////////////////////////////////////////////////////////////////////////////////////
//
// ArtistList Class functions
//

void Firnplayer::ArtistList::AddTrack(Json::Value * TrackJson)
{
  if(TrackJson->get("TPE2", "").asString() == "")
    (*TrackJson)["TPE2"] = "Unknown Artist";
  int i;
  for(i = 0; (i < Artists.size()) && !(Artists[i] == (*TrackJson)["TPE2"].asString()); i++);
  if(i < Artists.size())
    Artists[i].AddTrack(TrackJson);
  else
  {
    std::vector<ListArtist>::const_iterator ArtistItr = Artists.begin(), ArtistEnd = Artists.end();
    for(; ArtistItr != ArtistEnd && (*ArtistItr <= (*TrackJson)["TPE2"].asString()); ArtistItr++);
    Artists.insert(ArtistItr, ListArtist(TrackJson));
  }
}

bool Firnplayer::ArtistList::MoveUp(std::pair<std::string, std::string> * PositionPtr)
{
  std::pair<std::string, std::string> &Position = PositionPtr ? *PositionPtr : CurrentPosition;

  // If we have no artists, screw this shit.
  if(!Artists.size())
    return false;

  // If position is blank, initiate it.
  if(!Position.first.size())
  {
    Position.first = Artists.front().GetName();
    Position.second = "";
  }

  // Now find the artist's iterator.
  std::vector<ListArtist>::iterator CurrentArtist = std::find(Artists.begin(), Artists.end(), Position.first);

  // If the artist was not in the list, blank position;
  if(CurrentArtist == Artists.end())
  {
    CurrentArtist = Artists.begin();
    Position.first = CurrentArtist->GetName();
    Position.second = "";
  }
  
  // If the artist is not expanded, clear the album.
  if(!CurrentArtist->IsExpanded())
    Position.second = "";

  // Find the album position in the artist.
  int AlbumPos = CurrentArtist->GetAlbumPosition(Position.second);

  // Now, if the topmost artist was selected, nothing to do, let's get out.
  if((std::distance(Artists.begin(), CurrentArtist) == 0) && AlbumPos < 0)
    return false;

  // If an album was selected and there were album(s) in the artist before the selected, we move one up in albums.
  if(AlbumPos > 0)
  {
    Position.second = (*CurrentArtist)[AlbumPos - 1]->GetName();
    return true;
  }

  // If the first album of the artist was selected, select the artist instead.
  if(AlbumPos == 0)
  {
    Position.second = "";
    return true;
  }

  // If we are here, an artist was selected which was not the first in the list and no album was selected.
  // We move up one selection on the artists.  If the artist was expanded, the last album of the artist is selected.
  CurrentArtist--;
  Position.first = CurrentArtist->GetName();
  if(CurrentArtist->IsExpanded())
  {
    Position.second = (*CurrentArtist)[CurrentArtist->GetAlbumCount() - 1]->GetName();
  }
  else
  {
    Position.second = "";
  }
  return true;
}

bool Firnplayer::ArtistList::MoveUp(std::pair<std::string, std::string> &Position)
{
  return MoveUp(&Position);
}

std::string Firnplayer::ArtistList::PosToString(std::pair<std::string, std::string> PositionToPrint)
{
  if(PositionToPrint.second.size())
  {
    return "  " + PositionToPrint.second;
  }
  else
  {
    return PositionToPrint.first;
  }
}

void Firnplayer::ArtistList::ToggleExpanded()
{
  std::vector<ListArtist>::iterator CurrentArtist = std::find(Artists.begin(), Artists.end(), CurrentPosition.first);
  if(CurrentArtist != Artists.end())
  {
    CurrentArtist->ToggleExpanded();
    CurrentPosition.second = "";
  }
}
  
bool Firnplayer::ArtistList::MoveDown(std::pair<std::string, std::string> * PositionPtr)
{
  std::pair<std::string, std::string> &Position = PositionPtr ? *PositionPtr : CurrentPosition;

  // If we have no artists, screw this shit.
  if(!Artists.size())
    return false;

  // If position is blank, initiate it.
  if(!Position.first.size())
  {
    Position.first = Artists.front().GetName();
    Position.second = "";
  }

  // Find artist iterator.
  std::vector<ListArtist>::iterator CurrentArtist = std::find(Artists.begin(), Artists.end(), Position.first);
  
  // If the artist was not found, blank position again.
  if(CurrentArtist == Artists.end())
  {
    CurrentArtist = Artists.begin();
    Position.first = CurrentArtist->GetName();
    Position.second = "";
  }

  // If the current artist is not expanded, clear the album
  if(!CurrentArtist->IsExpanded())
    Position.second = "";

  // Find the album position in the artist.
  int AlbumPos = CurrentArtist->GetAlbumPosition(Position.second);

  // If the current artist not expanded or the last album of an artist was selected
  if(!CurrentArtist->IsExpanded() || (AlbumPos + 1 >= CurrentArtist->GetAlbumCount()))
  {
    // If the last artist was selected, do nothing.
    if(*CurrentArtist == Artists.back())
    {
      return false;
    }
    // Otherwise, increment artist and clear album.
    Position.first = (++CurrentArtist)->GetName();
    Position.second = "";
    return true;
  }
  
  // If we are not moving down artist, move down album.
  Position.second = (*CurrentArtist)[AlbumPos + 1]->GetName();
  return true;
}

void Firnplayer::ArtistList::GetTrackList(std::vector<Json::Value *> &TrackListOut,
                              std::vector<std::string> &TrackListOutPretty)
{
  auto CurrentArtist = std::find(Artists.begin(), Artists.end(), CurrentPosition.first);
  if(CurrentArtist == Artists.end())
    return;
  CurrentArtist->GetTrackList(TrackListOut, TrackListOutPretty, CurrentPosition.second);
}

bool Firnplayer::ArtistList::MoveDown(std::pair<std::string, std::string> &Position)
{
  return MoveDown(&Position);
}

std::pair<std::string, std::string> Firnplayer::ArtistList::GetCurrentPosition()
{
  if(!CurrentPosition.first.size() && Artists.size())
  {
    CurrentPosition.first = Artists.front().GetName();
    CurrentPosition.second = "";
  }
  return CurrentPosition;
}
