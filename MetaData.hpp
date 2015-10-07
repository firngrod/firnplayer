#include "json/value.h"
#include <string>
#include <utility>

Json::Value ScanDirectory(const std::string &Root);


class ListAlbum
{
public:
  ListAlbum(const std::string &Name);
  ListAlbum(Json::Value * TrackJson);
  std::string GetName();
  Json::Value * operator[](const unsigned int &TrackNo);
  void AddTrack(Json::Value * TrackJson);
  int GetTrackCount();
  void GetTrackList(std::vector<Json::Value *> &TrackListOut, std::vector<std::string> &TrackListOutPretty);

 
  inline bool operator==(const std::string &AlbumName) const {return Name == AlbumName;}
  inline bool operator<(const std::string &AlbumName) const {return Name < AlbumName;}
  inline bool operator<=(const std::string &AlbumName) const {return Name <= AlbumName;}

private:
  std::vector<Json::Value *> Tracks;
  std::string Name;
};


class ListArtist
{
public:
  ListArtist(const std::string &Name);
  ListArtist(Json::Value * TrackJson);
  std::string GetName();
  ListAlbum * operator[](const unsigned int &AlbumNumber);
  ListAlbum * operator[](const std::string &AlbumName);
  void AddTrack(Json::Value * TrackJson);
  int GetAlbumCount();
  int GetLineUsage();
  int GetAlbumPosition(const std::string &AlbumName);
  void GetTrackList(std::vector<Json::Value *> &TrackListOut, std::vector<std::string> &TrackListOutPretty,
                    std::string AlbumName);
  inline bool operator==(const std::string &ArtistName) const {return Name == ArtistName;}
  inline bool operator==(const ListArtist &OtherArtist) const {return OtherArtist.Name == this->Name;}
  inline bool operator<(const std::string &ArtistName) const {return Name < ArtistName;}
  inline bool operator<=(const std::string &ArtistName) const {return Name <= ArtistName;}
  inline bool IsExpanded() {return Expanded;}
  void ToggleExpanded() {Expanded = !Expanded;}
  //bool GetLinesToPrint(const unsigned int &NumberOfLines, std::list<std::string> &OutputList, 
                       //const bool &FromBehind = false);


protected:
  std::vector<ListAlbum> Albums;
  std::string Name;
  bool Expanded;
};


class ArtistList
{
public:
  void AddTrack(Json::Value * TrackJson);
  bool MoveUp(std::pair<std::string, std::string> &Position);
  bool MoveUp(std::pair<std::string, std::string> * PositionPtr = NULL);
  bool MoveDown(std::pair<std::string, std::string> &Position);
  bool MoveDown(std::pair<std::string, std::string> * PositionPtr = NULL);
  std::string PosToString(std::pair<std::string, std::string> PositionToPrint);
  void ToggleExpanded();
  void SetPosition(const std::string &Artist, const std::string &Album);
  void GetTrackList(std::vector<Json::Value *> &TrackListOut, std::vector<std::string> &TrackListOutPretty);
  std::pair<std::string, std::string> GetCurrentPosition();

private:
  std::pair<std::string, std::string> CurrentPosition;
  std::vector<ListArtist> Artists;
  void SanitizePosition();
};
