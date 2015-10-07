// Player.cpp
#include "includes/ViewPort.hpp"
#include "includes/AccessQueue.hpp"
#include "includes/MetaData.hpp"
#include <vector>
#include <thread>
#include "includes/Playlist.hpp"
#include "includes/Player.hpp"
#include <ncurses.h>
#include "includes/json/value.h"
#include "includes/json/reader.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include "includes/utils.h"
#include <signal.h>

Player * Player::ThePlayer = NULL;

// Constructor
Player::Player()
{
  ConfigDir = cleanpath(HomePath() + "/.firnplayer");
  ConfigFile = cleanpath(ConfigDir + "/Configuration.json");
  TrackInfoFile = cleanpath(ConfigDir + "/TrackInfo.json");
  
  ThePlayer = this;

  // Check if Config path exists and make if not.
  if(!FileExists(ConfigDir))
  {
    std::printf("Config folder %s does not exist, creating.\n", ConfigDir.c_str());
    boost::filesystem::create_directory(ConfigDir);
    if(!FileExists(ConfigDir))
    {
      FatalErrorString = StringPrintf("Could not create directory %s, aborting\n", ConfigDir.c_str());
    }
  }

  // Load and sanitize config file.
  if(FileExists(ConfigFile))
  {
    std::printf("Reading Config\n");
    LoadJson(ConfigFile, Options);
  }
  SanitizeConfig();

  ActiveViewPort = ACTIVE_ARTISTS;

  // Set the frame colours
  ArtistView.SetFrameColPair(COLPAIR_FRAME);
  TrackView.SetFrameColPair(COLPAIR_FRAME);
  QueueView.SetFrameColPair(COLPAIR_FRAME);
  PlaylistView.SetFrameColPair(COLPAIR_FRAME);

  // Load track info if it exists.
  std::string TrackDBPath = cleanpath(ConfigDir + "/TrackInfo.Json");
  if(FileExists(TrackDBPath))
  {
    std::printf("Loading Track Info\n");
    if(!LoadJson(TrackDBPath, TrackList))  // No queue lock.  No other threads should be running.
    {
      std::printf("Stored track info file corrupt, starting empty.\n");
    }
    else
    {
      Threads["Sorter"] = std::thread(Sortfunction, std::ref(Artists), std::ref(ArtistsQueue), 
                                      std::ref(TrackList), std::ref(TrackListQueue));
    }
  }
  Running = 1;
}

void Player::DoRun()
{
  signal(SIGWINCH, ResizeHandler);
  ViewPort::Initialize();
  ViewPort::DefineColPair(COLPAIR_LISTLINE, COLOR_GREEN, COLOR_BLUE);
  ViewPort::DefineColPair(COLPAIR_ACTLINE, COLOR_BLUE, COLOR_WHITE);
  ViewPort::DefineColPair(COLPAIR_NORMAL, COLOR_WHITE, COLOR_BLACK);
  ViewPort::DefineColPair(COLPAIR_FRAME, COLOR_GREEN, COLOR_BLUE);
  raw();
  timeout(100);
  Redraw();
  Threads["InputHandler"] = std::thread(InputHandler, 0);
  std::unique_lock<std::mutex> LockForShutdown(MutexForShutdown);
  CVForShutdown.wait(LockForShutdown, [this]{return Running == 3;});
  Shutdown();
}

void Player::ResizeHandler(int Signal)
{
  ThePlayer->Redraw();
}

void Player::InputHandler(int Stuff)
{
  while(ThePlayer->Running == 1)
  {
    int PressedKey = getch();
    if(PressedKey < 0 || ThePlayer->Running != 1)
      continue;
    if(PressedKey == 'q')
    {
      ThePlayer->SignalShutdown();
    }
    if(PressedKey == 'j')
    {
      ThePlayer->MoveDown();
    }
    if(PressedKey == 'k')
    {
      ThePlayer->MoveUp();
    }
    if(PressedKey == ' ')
    {
      ThePlayer->ToggleExpanded();
    }
    if(PressedKey == 'h')
    {
      ThePlayer->MoveLeft();
    }
    if(PressedKey == 'l')
    {
      ThePlayer->MoveRight();
    }
    else
    {
      AccessQueue::QueueToken QueueTicket = ThePlayer->ViewPortsQueue.Lock();
      //ThePlayer->ArtistView.Write(0, 0, -1, StringPrintf("Got key %d", PressedKey).c_str());
      ViewPort::Update();
    }
  }
}

void Player::ToggleExpanded()
{
  bool DoResetTracks = !!Artists.GetCurrentPosition().second.size();
  Artists.ToggleExpanded();
  DrawArtists();
  if(DoResetTracks)
  {
    ResetShownTracks();
    DrawTracks();
  }
}

void Player::MoveUp()
{
  if(ActiveViewPort == ACTIVE_ARTISTS)
  {
    auto ArtistsLock = ArtistsQueue.Lock();
    if(PositionInArtists) PositionInArtists--;
    Artists.MoveUp();
    ResetShownTracks();
    ArtistsLock.Unlock();
    DrawArtists();
    DrawTracks();
  }
  else if(ActiveViewPort == ACTIVE_TRACKS)
  {
    if(PositionInTracks) PositionInTracks--;
    if(TracksActualPos) TracksActualPos--;
    DrawTracks();
  }
}

void Player::MoveDown()
{
  if(ActiveViewPort == ACTIVE_ARTISTS)
  {
    auto ArtistsLock = ArtistsQueue.Lock();
    PositionInArtists++;
    Artists.MoveDown();
    ResetShownTracks();
    ArtistsLock.Unlock();
    DrawArtists();
    DrawTracks();
  }
  else if(ActiveViewPort == ACTIVE_TRACKS)
  {
    PositionInTracks++;
    TracksActualPos++;
    if(TracksActualPos >= TracksForShow.size())
      TracksActualPos--;
    DrawTracks();
    DrawArtists();
  }
}

void Player::MoveLeft()
{
  if(ActiveViewPort == ACTIVE_TRACKS)
  {
    ActiveViewPort = ACTIVE_ARTISTS;
    DrawTracks();
    DrawArtists();
  }
}

void Player::MoveRight()
{
  if(ActiveViewPort == ACTIVE_ARTISTS)
  {
    ActiveViewPort = ACTIVE_TRACKS;
    DrawTracks();
    DrawArtists();
  }
}

void Player::SignalShutdown()
{
  Running = 3;
  ThePlayer->CVForShutdown.notify_all();
}

void Player::Shutdown()
{
  Running = 2;
  for(std::map<std::string, std::thread>::iterator ThreadItr = Threads.begin(), ThreadEnd = Threads.end();
      ThreadItr != ThreadEnd; ThreadItr++)
  {
    ThreadItr->second.join();
  }
  ViewPortsQueue.Lock().Unlock();
  ViewPort::Uninitialize();
  Running = 3;
  CVForShutdown.notify_one();
}


void Player::Redraw()
{
  AccessQueue::QueueToken QueueTicket = ViewPortsQueue.Lock();
  endwin();
  refresh();
  clear();
  RedefineDivisors();
  int TermX, TermY;
  getmaxyx(stdscr, TermY, TermX);
  WholeScreen.Resolve(0, 0, TermX - 1, TermY - 1);
  DrawArtists();
  ResetShownTracks();
  DrawTracks();
  if(Options["Layout Settings"]["Layout"]["ShowQueue"].asInt())
  {
    DrawQueue();
    DrawPlaylist();
  }
  ViewPort::Update();
}


void Player::ResetShownTracks()
{
  TracksActualPos = 0;
  PositionInTracks = 0;
  TracksForShow.clear();
  TracksForShowPrintable.clear();
  Artists.GetTrackList(TracksForShow, TracksForShowPrintable);
}

void Player::DrawPlaylist()
{
  PlaylistView.DrawFrame();
}


void Player::DrawQueue()
{
  QueueView.DrawFrame();
}


void Player::DrawArtists()
{
  ArtistView.DrawFrame();
  int TotalHeight = ArtistView.GetHeight();
  int ActiveLine = 0;

  if(!TotalHeight)
    return;

  { double TopBottomBuffer = Options["Layout Settings"]["TopBottomBuffer"].asDouble();
    Saturate(PositionInArtists, (unsigned int)(TotalHeight * TopBottomBuffer + 1),
                               (unsigned int)(TotalHeight * (1 - TopBottomBuffer)));
  }

  AccessQueue::QueueToken ArtistsToken = ArtistsQueue.Lock(false, 50);

  std::pair<std::string, std::string> ListPosition = Artists.GetCurrentPosition();
  std::vector<std::string> StringsToPrint;
  if(ListPosition.first.size())
  {
    // First, get the lines from the top of the screen to the current selection.
    // This is done reversely.
    do
    {
      StringsToPrint.insert(StringsToPrint.begin(), Artists.PosToString(ListPosition));
    } while((StringsToPrint.size() < PositionInArtists) && Artists.MoveUp(ListPosition));
    ActiveLine = StringsToPrint.size() - 1;

    // Remember the topmost position in case we need it later (We will).
    auto TopMostPosition = ListPosition;
    
    ListPosition = Artists.GetCurrentPosition();
    if((StringsToPrint.size() < TotalHeight) && Artists.MoveDown(ListPosition))
    {
      // Get the lines below the current line.
      do
      {
        StringsToPrint.push_back(Artists.PosToString(ListPosition));
      } while((StringsToPrint.size() <= TotalHeight - 1) && Artists.MoveDown(ListPosition));
    }
    // Now that we have these strings, make sure that we are filling all the way to the bottom.  If we are not,
    // we want to move things down until we are.
    while(StringsToPrint.size() < TotalHeight && Artists.MoveUp(TopMostPosition))
    {
      StringsToPrint.insert(StringsToPrint.begin(), Artists.PosToString(TopMostPosition));
      ActiveLine++;
    }
      
    int CurrentString = 0;
    for(; CurrentString < StringsToPrint.size(); CurrentString++)
    {
      ArtistView.Write(0, CurrentString, -1, A_NORMAL,
                       (CurrentString == ActiveLine && ActiveViewPort == ACTIVE_ARTISTS) ? COLPAIR_LISTLINE:
                       (CurrentString == ActiveLine) ? COLPAIR_ACTLINE : COLPAIR_NORMAL,
                        StringsToPrint[CurrentString].c_str());
    }
    for(; CurrentString < TotalHeight; CurrentString++)
      ArtistView.Write(0, CurrentString, -1, A_NORMAL, 2, "");
  }
  else
    return;
}


void Player::DrawTracks()
{
  TrackView.DrawFrame();
  auto ArtistLock = ArtistsQueue.Lock();
  int TotalHeight = TrackView.GetHeight();
  int ActiveLine = 0;

  if(!TotalHeight)
    return;

  { double TopBottomBuffer = Options["Layout Settings"]["TopBottomBuffer"].asDouble();
    Saturate(PositionInTracks, (unsigned int)(TotalHeight * TopBottomBuffer) + 1,
                               (unsigned int)(TotalHeight * (1 - TopBottomBuffer)));
  }

  if(!TracksForShowPrintable.size())
    return;

  std::vector<std::string> StringsToPrint;
  
  // First, get the lines from the top of the screen to the current selection.
  // This is done reversely.
  int CurString = TracksActualPos;
  for(; CurString  >= 0 && PositionInTracks > StringsToPrint.size(); CurString--)
  {
    StringsToPrint.insert(StringsToPrint.begin(), TracksForShowPrintable[CurString]);
  }
  ActiveLine = StringsToPrint.size() - 1;
  
  auto TopMostPosition = CurString;

  CurString = TracksActualPos + 1;

  if((StringsToPrint.size() < TotalHeight) && CurString < TracksForShowPrintable.size())
  {
    do
    {
      StringsToPrint.push_back(TracksForShowPrintable[CurString++]);
    } while(StringsToPrint.size() < TotalHeight && CurString < TracksForShowPrintable.size());
  }

  while(StringsToPrint.size() < TotalHeight && TopMostPosition >= 0)
  {
    StringsToPrint.insert(StringsToPrint.begin(), TracksForShowPrintable[TopMostPosition--]);
    ActiveLine++;
  }

  int CurrentString = 0;
  for(; CurrentString < StringsToPrint.size(); CurrentString++)
  {
    TrackView.Write(0, CurrentString, -1, A_NORMAL,
                     (CurrentString == ActiveLine && ActiveViewPort == ACTIVE_TRACKS) ? COLPAIR_LISTLINE:
                     (CurrentString == ActiveLine) ? COLPAIR_ACTLINE : COLPAIR_NORMAL,
                      StringsToPrint[CurrentString].c_str());
  }
  for(; CurrentString < TotalHeight; CurrentString++)
    TrackView.Write(0, CurrentString, -1, A_NORMAL, 2, "");
}


//void Player::DrawList(Firnplayer::Playlist &ForDrawing, ViewPort &ThePort)
//{


void Player::RedefineDivisors()
{
  RightPart = QueuePlaylist = WholeScreen = ViewPort::Divisor();
  RightPart.Divide(TrackView, 100 - Options["Layout Settings"]["Layout"]["Queue Column Width"].asDouble());
  if(Options["Layout Settings"]["Layout"]["ShowQueue"].asInt())
  {
    QueuePlaylist.Divide(QueueView, 50);
    QueuePlaylist.Divide(PlaylistView, 50);
    QueuePlaylist.SetHorizontal();
    RightPart.Divide(QueuePlaylist, Options["Layout Settings"]["Layout"]["Queue Column Width"].asDouble());
  }
  WholeScreen.Divide(ArtistView, Options["Layout Settings"]["Layout"]["Artist Column Width"].asDouble());
  WholeScreen.Divide(RightPart, 100 - Options["Layout Settings"]["Layout"]["Artist Column Width"].asDouble());
}


std::string Player::GetFatalError()
{
  return FatalErrorString;
}

bool Player::LoadJson(const std::string &FilePath, Json::Value &Container)
{
  Json::Reader Reader;
  std::ifstream FileInput;
  FileInput.open(FilePath.c_str());
  return Reader.parse(FileInput, Container);
}

void VerifyOptionf(Json::Value &Option, const float &Default, const float &Lower, const float &Upper)
{
  if(Option.asDouble() < Lower || Option.asDouble() > Upper)
    Option = Default;
}

void VerifyOption(Json::Value &Option, const int &Default, const int &Lower, const int &Upper)
{
  if(Option.asInt() < Lower || Option.asInt() > Upper)
    Option = Default;
}

void Player::SanitizeConfig()
{
  int CurrentOptionsVersion = 1;
  // This stuff is pretty hardcoded, but it was an easy way to implement a default configuration.

  // First, check the structure.
  bool StructOK = Options.type() == 7;
  if(StructOK)
    StructOK &= (Options["Options Version"].type() == 1);
  if(StructOK && Options["Options Version"] == 1)
  {
    StructOK &= Options["Layout Settings"].type() == 7;
    if(StructOK)
    {
      StructOK &= Options["Layout"].type() == 7;
      StructOK &= Options["Colours"].type() == 7;
    }
  }

  // If the structure is not ok, we just reset to the defaults for this version of the program.
  if(!StructOK)
  {
    Options = Json::Value();
    Options["Layout Settigns"] = Json::Value();
    Options["Layout Settings"]["Layout"] = Json::Value();
  }

  // First, ViewPort size standards.
  VerifyOptionf(Options["Layout Settings"]["Layout"]["Artist Column Width"], 30.0, 1, 50);
  VerifyOptionf(Options["Layout Settings"]["Layout"]["Queue Column Width"], 50.0, 10, 90);
  VerifyOption(Options["Layout Settings"]["Layout"]["ShowQueue"], 1, 0, 1);
  Options["Layout Settings"]["Layout"]["ShowQueue"] = 1;

  // List view options
  VerifyOptionf(Options["Layout Settings"]["TopBottomBuffer"], 0.2, 0.1, 0.5);
}


void Player::Sortfunction(ArtistList &Artists, AccessQueue &ArtistsQueue, Json::Value &TrackList, 
                  AccessQueue &TrackListQueue)
{
  // Thread Function for funelling the tracklist into the ArtistList's sort function.
  // This should be thread safe.
  AccessQueue::QueueToken TracksToken = TrackListQueue.Lock(false);
  AccessQueue::QueueToken ArtistToken = ArtistsQueue.Lock(true, 200);
  for(auto TrackItr = TrackList.begin(), TrackEnd = TrackList.end(); TrackItr != TrackEnd; TrackItr++)
  {
    Artists.AddTrack(&(*TrackItr));
    //ArtistToken.Yield();
  }
}
