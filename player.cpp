#include "player.hpp"
#include "firnlibs/files/files.hpp"
#include "firnlibs/string/string.hpp"
#include "firnlibs/crypto/general.hpp"
#include "firnlibs/mp3/metadata.hpp"
#include <algorithm>
#include <iostream>

namespace FirnPlayer {

void Player::Start()
{
  historyPos = history.end();
  std::cout << "Starting database.\n";
  std::string dataPath = FirnLibs::Files::CleanPath(FirnLibs::Files::HomePath() + "/.firnplayer/");
  FirnLibs::Files::CreateFolder(dataPath);
  std::string dbPath = dataPath + "/database.sqlite";

  auto nextLambda = [this] (const std::string &current) -> std::string
  {
    return this->NextGetter(current);
  };

  stream.SetNextGetter(nextLambda);

  {
    auto tok = db.Get("Start Get");
    tok->Initialize(dbPath);
  }

  std::cout << "Mapping commands.\n";
  MapCmds();

  std::cout << "Starting Networking.\n";
  FirnLibs::Networking::Init();
  auto lambda = [this] (const std::shared_ptr<FirnLibs::Networking::Client> &newClient) -> void
  {
    this->AddClient(newClient);
  };

  std::cout << "Starting listener.\n";
  clientListener.Listen(6666,
                        std::function<void (const std::shared_ptr<FirnLibs::Networking::Client> &newClient)>(lambda),
                        std::function<void (const int &)>(nullptr));
}


void Player::AddClient(const std::shared_ptr<FirnLibs::Networking::Client> &client)
{
  auto lambda = [this, client] (const std::vector<unsigned char> &data) -> void
  {
    this->ClientCallback(client, data);
  };
  auto errorLambda = [this, client] (const int &error) -> void
  {
    this->ClientErrorCallback(client, error);
  };
  clients.push_back(client);
  client->Commence(lambda, errorLambda);
}


void Player::ClientErrorCallback(const std::shared_ptr<FirnLibs::Networking::Client> &client, const int &error)
{
  clients.erase(std::find(clients.begin(), clients.end(), client));
}

void Player::MapCmds()
{
  cmds.description = "Commands available:";
  
  // Help function
  const std::string helpCmd = "help";
  cmds[helpCmd].func = 
    [this](const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command) -> bool 
      {return this->HandleHelp(client, command);};
  cmds[helpCmd].description = "Display this menu.";

  // Settings function.  Lots of dummy subcommands just to clarify settings.
  const std::string settingsCmd = "settings";
  cmds[settingsCmd].description = "Settings command.  See settings options below.  Blank lists current settings.";
  cmds[settingsCmd].arguments = "[<setting> <value>]";
  cmds[settingsCmd].func = 
    [this](const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command) -> bool 
      {this->HandleSettings(client, command); return true;};
  const std::string scopeCmd = "scope";
  cmds[settingsCmd][scopeCmd].description = "Sets the playback scope for the player.";
  cmds[settingsCmd][scopeCmd].arguments = "<all | artist | album | track>";
  const std::string repeatCmd = "repeat";
  cmds[settingsCmd][repeatCmd].description = "Toggle repeat of playback of the current scope.";
  cmds[settingsCmd][repeatCmd].arguments = "<yes | no>";
  const std::string shuffleCmd = "shuffle";
  cmds[settingsCmd][shuffleCmd].description = "Toggle shuffle of tracks within the current scope.";
  cmds[settingsCmd][shuffleCmd].arguments = "<yes | no>";

  // Scan command
  const std::string scanCmd = "scan";
  cmds[scanCmd].description = "Scan server directory for mp3 files for the library.";
  cmds[scanCmd].arguments = "<path to scan>";
  cmds[scanCmd].func = 
    [this](const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command) -> bool 
      {this->HandleScan(client, command); return true;};

  // Search command
  const std::string searchCmd = "search";
  cmds[searchCmd].description = "Search the database for tracks matching a string in any property field.\nEmpty string lists entire database.\nResults are in the format <trackid>: <album artist> - <album> - <track no> <track name>";
  cmds[searchCmd].arguments = "<string>";
  cmds[searchCmd].func = 
    [this](const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command) -> bool 
      {this->HandleSearch(client, command); return true;};

  // Play command
  const std::string playCmd = "play";
  cmds[playCmd].description = "Skip to and play a specific track.\nUse search to find trackids.";
  cmds[playCmd].arguments = "<trackid>";
  cmds[playCmd].func = 
    [this](const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command) -> bool 
      {this->HandlePlay(client, command); return true;};

  // Stop command
  const std::string stopCmd = "stop";
  cmds[stopCmd].description = "Pause playback.";
  cmds[stopCmd].func =
    [this](const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command) -> bool
      {this->HandleStop(client, command); return true;};

  // Resume command
  const std::string resumeCmd = "resume";
  cmds[resumeCmd].description = "Resume playback.";
  cmds[resumeCmd].func =
    [this](const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command) -> bool
      {this->HandleResume(client, command); return true;};

  // Next command
  const std::string nextCmd = "next";
  cmds[nextCmd].description = "Skip to next track";
  cmds[nextCmd].func =
    [this](const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command) -> bool
      {this->HandleNext(client, command); return true;};

  // Prev command
  const std::string prevCmd = "prev";
  cmds[prevCmd].description = "Skip to next track";
  cmds[prevCmd].func =
    [this](const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command) -> bool
      {this->HandlePrev(client, command); return true;};

  // Queue command
  const std::string queueCmd = "queue";
  cmds[queueCmd].description = "Add a track to the end of the queue.\nUse search to find trackids.";
  cmds[queueCmd].arguments = "<trackid>";
  cmds[queueCmd].func =
    [this](const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command) -> bool
      {this->HandleQueue(client, command); return true;};
  
  // Info command
  const std::string infoCmd = "info";
  cmds[infoCmd].description = "Get detailed info about one or more tracks.\nUse search to find trackids.";
  cmds[infoCmd].arguments = "<trackid> [<trackid> ...]";
  cmds[infoCmd].func =
    [this](const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command) -> bool
      {this->HandleInfo(client, command); return true;};
}

void Player::ClientCallback(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::vector<unsigned char> &data)
{
  std::string commandStr((const char *)&data[0], data.size());

  cmds.Execute(client, commandStr);
}


void Player::HandlePrev(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command)
{
  std::string prev = PrevGetter();
  if(prev != "")
  {
    stream.PlayTrack(prev);
  }
}


void Player::HandleNext(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command)
{
  std::string next = NextGetter(stream.GetCurrent());
  if(next != "")
  {
    stream.PlayTrack(next);
  }
}


void Player::HandleResume(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command)
{
  stream.play();
}


void Player::HandleStop(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command)
{
  stream.pause();
}


void Player::HandleScan(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command)
{
  auto lambda = [this, client, command] () -> void
  {
    this->DoScan(client, command);
  };
  scanPool.Push(lambda);

  std::vector<unsigned char> reply;
  FirnLibs::Crypto::StringToVector(reply, "Started scan\n");
  client->Send(reply);
}


void Player::DoScan(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command)
{
  std::string scanPath = FirnLibs::String::Trim(command, " \r\n");

  auto lambda = [this] (const std::string &path) -> void
  {
    if(FirnLibs::Files::HasExtension(path, "mp3", false))
    {
      auto tok = this->db.Get("Player::DoScan Get");
      time_t fileTime = FirnLibs::Files::FileModifiedTime(path);
      Json::Value tmpie = tok->GetTrack(path);
      time_t recordedFileTime = std::stoll(tmpie.get("MTIM", "0").asString());
      if(recordedFileTime != fileTime)
      {
        tmpie = Json::Value();
        FirnLibs::Mp3::GetMetadata(tmpie, path, {"TPE1", "TPE2", "TALB", "TIT2"}, "Unknown");
        tok->AddTrack(tmpie);
      }
    }
  };
  
  FirnLibs::Files::ForEachFile(scanPath, lambda, true);
  PreparePlaylist(stream.GetCurrent(), true);
}


void Player::HandleSettings(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &commandStr)
{
  std::vector<std::string> command = FirnLibs::String::Split(commandStr, " \r\n");
  auto tok = db.Get("HandleSettings Get");
  if(command.size() == 0)
  {
    Json::Value settings = tok->GetSettings();
    std::vector<unsigned char> replyData;
    FirnLibs::Crypto::StringToVector(replyData, settings.toStyledString());
    client->Send(replyData);
    return;
  }
  else if(command.size() == 2)
  {
    auto setItr = settingsMap.find(command[0]);
    if(setItr != settingsMap.end())
    {
      if(std::find(setItr->second.begin(), setItr->second.end(), command[1]) != setItr->second.end())
      {
        tok->SetSetting(command[0], command[1]);
        std::vector<unsigned char> reply;
        Json::Value settings = tok->GetSettings();
        FirnLibs::Crypto::StringToVector(reply, "Settings succesfully updated.\n" + settings.toStyledString());
        client->Send(reply);
        return;
      }
    }
  }
  std::vector<unsigned char> reply;
  FirnLibs::Crypto::StringToVector(reply, "Invalid setting.");
  client->Send(reply);
}


void Player::HandleSearch(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command)
{
  std::string searchTerm = FirnLibs::String::Trim(command, " \n\r");

  Json::Value tracks;
  {
    auto tok = db.Get("Player::HandleSearch Get");
    tracks = tok->GetTracksMatchingMetadata(searchTerm);
  }

  std::vector<int64_t> trid;
  std::vector<int> trno;
  std::vector<std::string> artist, album, trackss;
  int artLen = 0, albLen = 0, trackLen = 0;
  std::string tmp;
  for(const Json::Value &track: tracks)
  {
    trid.push_back(std::stoll(track.get("TRID", "0").asString()));
    artist.push_back(track.get("TPE1", "").asString().c_str());
    album.push_back(track.get("TALB", "").asString().c_str());
    trackss.push_back(track.get("TIT2", "").asString().c_str());
    tmp = track.get("TRCK", "0").asString();
    if(tmp.find_first_not_of("0123456789") == std::string::npos && tmp.size() != 0)
      trno.push_back(std::stoi(tmp));
    else
    {
      trno.push_back(0);
    }
    if(artLen < artist.back().size())
      artLen = artist.back().size();
    if(albLen < album.back().size())
      albLen = album.back().size();
  }
  std::vector<unsigned char> reply;
  std::string replyStr;
  for(size_t i = 0; i < album.size(); i++)
  {
    replyStr = FirnLibs::String::StringPrintf("%6ld: %-*s - %-*s - %3d %s\n",
                (int)trid[i],
                artLen, artist[i].c_str(),
                albLen, album[i].c_str(),
                trno[i],
                trackss[i].c_str());
    FirnLibs::Crypto::StringToVector(reply, replyStr);
    client->Send(reply);
  }
  replyStr = FirnLibs::String::StringPrintf("%d tracks total.\n", tracks.size());
  FirnLibs::Crypto::StringToVector(reply, replyStr);
  client->Send(reply);
}


void Player::HandlePlay(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command)
{
  std::vector<unsigned char> reply;
  std::vector<std::string> cmdVec = FirnLibs::String::Split(command, " \r\n");
  if(cmdVec.size() != 1)
  {
    FirnLibs::Crypto::StringToVector(reply, "Invalid usage.  Usage:  play <trackid>\n");
    client->Send(reply);
    return;
  }

  if(cmdVec[0].find_first_not_of("0123456789") != std::string::npos)
  {
    FirnLibs::Crypto::StringToVector(reply, "Invalid usage.  trackid must be all numbers.\n");
    client->Send(reply);
    return;
  }
  
  int64_t trackid = std::stoll(cmdVec[0]);

  std::string trackPath;
  {
    auto tok = db.Get("Player::HandlePlay Get");
    trackPath = tok->GetTrackPath(trackid);
    if(trackPath == "")
    {
      FirnLibs::Crypto::StringToVector(reply, "Track does not exist.\n");
      client->Send(reply);
      return;
    }
    PreparePlaylist(trackPath, true);
    history.erase(historyPos, history.end());
    history.push_back(trackPath);
    historyPos = history.end();
  }
  stream.PlayTrack(trackPath);
}


void Player::HandleQueue(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command)
{
  std::string trimCmd = FirnLibs::String::Trim(command, " \n\r");
  std::vector<unsigned char> reply;
  if(trimCmd == "")
  {
    FirnLibs::Crypto::StringToVector(reply, "Invalid usage.  Usage:  play <trackid>\n");
    client->Send(reply);
    return;
  }

  if(trimCmd.find_first_not_of("0123456789") != std::string::npos)
  {
    FirnLibs::Crypto::StringToVector(reply, "Invalid usage.  trackid must be all numbers.\n");
    client->Send(reply);
    return;
  }
  
  int64_t trackid = std::stoll(trimCmd);

  std::string trackPath;
  {
    auto tok = db.Get("Player::HandleQueue Get");
    trackPath = tok->GetTrackPath(trackid);
    if(trackPath == "")
    {
      FirnLibs::Crypto::StringToVector(reply, "Track does not exist.\n");
      client->Send(reply);
      return;
    }
  }
  queue.Push(trackPath);
}


void Player::HandleInfo(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command)
{
  std::vector<std::string> cmdVec = FirnLibs::String::Split(command, " \r\n");
  std::vector<unsigned char> reply;
  bool invalidCommand = false;
  if(cmdVec.size() == 0)
  {
    invalidCommand = true;
  }
  else
  {
    for(auto cItr = cmdVec.begin(), cEnd = cmdVec.end(); cItr != cEnd; cItr++)
    {
      if(cItr->find_first_not_of("0123456789") != std::string::npos)
      {
        invalidCommand = true;
        break;
      }
    }
  }

  if(invalidCommand)
  {
    FirnLibs::Crypto::StringToVector(reply, "Invalid usage.  Correct usage:  info <trackid> [<trackid> ...]\ntrackids must be numbers");
    client->Send(reply);
    return;
  }

  auto tok = db.Get("Player::HandleInfo Get");
  Json::Value trackInfos;
  for(auto cItr = cmdVec.begin(), cEnd = cmdVec.end(); cItr != cEnd; cItr++)
  {
    int64_t trackid = std::stoll(*cItr);
    {
      trackInfos[*cItr] = tok->GetTrack(trackid);
    }
  }

  FirnLibs::Crypto::StringToVector(reply, trackInfos.toStyledString());
  client->Send(reply);
  return;
}


bool Player::HandleHelp(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command)
{
  std::vector<unsigned char> reply;
  FirnLibs::Crypto::StringToVector(reply, FirnLibs::String::Trim(cmds.ToString(), " "));
  client->Send(reply);
  return true;
}



void Player::PreparePlaylist(const std::string &current, const bool &shuffleCurrentFirst)
{
  auto tok = db.Get("PreparePlaylist Get");
  Json::Value settings = tok->GetSettings();
  std::string scope = settings.get("scope", "all").asString();
  playlist.clear();
  if(scope == "track")
  {
    playlist.push_back(current);
  }
  else
  {
    Json::Value curTrack = tok->GetTrack(current);
    std::string metaKey;
    if(scope == "artist" || scope == "all")
    {
      metaKey = "TPE2";
    }
    else if(scope == "album")
    {
      metaKey = "TALB";
    }
    std::string metaValue = curTrack.get(metaKey, "").asString();

    Json::Value tracks = tok->GetTracksMatchingMetadata(metaValue, metaKey);
    std::map<std::string, std::map<int, std::vector<std::string> > > sortingMap;
    std::string tmp;
    int trackNo = 0;
    for(const Json::Value &track: tracks)
    {
      tmp = track.get("TRCK", "0").asString();
      if(tmp.find_first_not_of("0123456789") == std::string::npos && tmp.size() != 0)
      {
        trackNo = std::stoi(tmp);
      }
      else
      {
        trackNo = 0;
      }
      sortingMap[track.get("TALB", "").asString()][trackNo].push_back(track.get("FILE", "").asString());
    }
    for(auto s1Itr = sortingMap.begin(), s1End = sortingMap.end(); s1Itr != s1End; s1Itr++)
    {
      for(auto s2Itr = s1Itr->second.begin(), s2End = s1Itr->second.end(); s2Itr != s2End; s2Itr++)
      {
        for(auto s3Itr = s2Itr->second.begin(), s3End = s2Itr->second.end(); s3Itr != s3End; s3Itr++)
        {
          playlist.push_back(*s3Itr);
        }
      }
    }
  }
  if(settings.get("shuffle", "no").asString() == "yes")
  {
    ShufflePlaylist(shuffleCurrentFirst ? current : "");
  }
}


void Player::ShufflePlaylist(const std::string &current)
{
  std::random_shuffle(playlist.begin(), playlist.end());
  if(current != "")
  {
    std::string tmp = playlist.front();
    auto curItr = std::find(playlist.begin(), playlist.end(), current);
    if(curItr == playlist.end())
      return;
    playlist.front() = current;
    *curItr = tmp;
  }
}


std::string Player::NextGetter(const std::string &current)
{
  if(current == "")
    return "";

  if(historyPos == history.end())
  {
    history.push_back(Advance(current));
    while(history.size() > 100)
      history.erase(history.begin());
    historyPos = history.end();
    return history.back();
  }
  else
  {
    return *historyPos++;
  }
}


std::string Player::PrevGetter()
{
  if(history.size() == 0)
    return "";

  auto tmpie = historyPos;
  if(--tmpie  == history.begin())
    return "";

  --historyPos;
  return *--tmpie;
}


std::string Player::Advance(const std::string &current)
{
  Json::Value settings;
  auto tok = db.Get("Advance Get");
  settings = tok->GetSettings();
  bool repeat = settings.get("repeat", "no").asString() == "yes";
  bool shuffle = settings.get("shuffle", "no").asString() == "yes";
  std::string scope = settings.get("scope", "all").asString();

  std::string fromQueue;
  bool reShuffle = GetFromQueue(fromQueue);
  if(reShuffle)
    PreparePlaylist(fromQueue, true);

  if(fromQueue != "")
    return fromQueue;

  // Special treatment of shuffle all.
  if(scope == "all" && shuffle)
  {
    return tok->GetRandomTrack();
  }
    
  std::vector<std::string>::const_iterator playItr = std::find(playlist.begin(), playlist.end(), current);

  if(playItr == playlist.end())
    return "";

  ++playItr;


  if(playItr == playlist.end())
  {
    if(!repeat && scope != "all")
      return "";
    else if(scope != "all")
    {
      if(shuffle)
        ShufflePlaylist("");
      playItr = playlist.begin();
    }
    else // All
    {
      Json::Value track = tok->GetTrack(current);
      std::string nextArtist = tok->GetNextMetaOfKey("TPE2", track.get("TPE2","").asString());
      std::string nextArtistTrack = tok->GetATrackWithMetadata("TPE2", nextArtist);
      PreparePlaylist(nextArtistTrack, false);
      return *playlist.begin();
    }
  }

  return *playItr;
}


bool Player::GetFromQueue(std::string &output)
{
  output = queue.Pop();
  if(output == "")
    return false;

  return queue.Size() == 0;
}


}

