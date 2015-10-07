// Player.hpp
#ifndef _PLAYER_HPP_
#define _PLAYER_HPP_
class Player
{
public:
  Player();
  std::string GetFatalError();
  void DoRun();
  void SignalShutdown();

private:
  std::string ConfigDir, ConfigFile, TrackInfoFile;
  Json::Value TrackList;
  std::vector<Json::Value *> TracksForShow; 
  std::vector<std::string> TracksForShowPrintable;
  Json::Value Options;
  ViewPort ArtistView, TrackView, QueueView, PlaylistView;
  ViewPort::Divisor WholeScreen, RightPart, QueuePlaylist;
  int ActiveViewPort;
  enum
  {
    ACTIVE_ARTISTS = 0,
    ACTIVE_TRACKS,
    ACTIVE_QUEUE,
    ACTIVE_PLAYLIST,
  };
  enum
  {
    COLPAIR_LISTLINE = 1,
    COLPAIR_NORMAL,
    COLPAIR_ACTLINE,
    COLPAIR_FRAME,
  };
  //MusicMp3 Audio;
  AccessQueue TrackListQueue, ArtistsQueue, ViewPortsQueue;
  std::string FatalErrorString;
  std::map<std::string, std::thread> Threads;
  std::mutex MutexForShutdown;
  std::condition_variable CVForShutdown;
  ArtistList Artists;
  unsigned int PositionInArtists, PositionInTracks, PositionInPlaylist, PositionInQueue, TracksActualPos;
  int Running;
  void SanitizeConfig();
  void RedefineDivisors();
  void DrawArtists();
  void DrawTracks();
  void DrawList(Firnplayer::Playlist &ForDrawing, ViewPort &ThePort);
  void ResetShownTracks();
  void DrawPlaylist();
  void DrawQueue();
  void MoveUp();
  void MoveDown();
  void MoveLeft();
  void MoveRight();
  bool LoadJson(const std::string &FilePath, Json::Value &Container);
  void Shutdown();
  static void Sortfunction(ArtistList &Artists, AccessQueue &ArtistsQueue, 
                           Json::Value &TrackList, AccessQueue &TrackListQueue);
  void ToggleExpanded();


public:
  void Redraw();
  static Player * ThePlayer;
  static void InputHandler(int Stuff);
  static void ResizeHandler(int Signal);
};
#endif //_PLAYER_HPP_
