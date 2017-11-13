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
  std::cout << "Starting database.\n";
  std::string dataPath = FirnLibs::Files::CleanPath(FirnLibs::Files::HomePath() + "/.firnplayer/");
  FirnLibs::Files::CreateFolder(dataPath);
  std::string dbPath = dataPath + "/database.sqlite";

  {
    auto tok = db.Get();
    tok->Initialize(dbPath);
  }

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


void Player::ClientCallback(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::vector<unsigned char> &data)
{
  std::string commandStr((const char *)&data[0], data.size());
  
  std::vector<std::string> command = FirnLibs::String::Split(commandStr, " \r\n");
  if(command.size() == 0)
    return;
  
  std::string baseKey = command[0];
  if(FirnLibs::String::CmpNoCase(baseKey, "settings"))
  {
    HandleSettings(client, command);
  }
  if(FirnLibs::String::CmpNoCase(baseKey, "scan"))
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
}


void Player::DoScan(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::vector<std::string> command)
{
  std::string scanPath;
  for(unsigned int i = 1; i < command.size(); i++)
  {
    scanPath += scanPath.size() > 0 ? " " : "";
    scanPath += command[i];
  }

  auto lambda = [this] (const std::string &path) -> void
  {
    if(FirnLibs::Files::HasExtension(path, "mp3", false))
    {
      auto tok = this->db.Get();
      time_t fileTime = FirnLibs::Files::FileModifiedTime(path);
      Json::Value tmpie = tok->GetTrack(path);
      time_t recordedFileTime = std::stoll(tmpie.get("MTIM", "0").asString());
      if(recordedFileTime != fileTime)
      {
        tmpie = Json::Value();
        FirnLibs::Mp3::GetMetadata(tmpie, path);
        tok->AddTrack(tmpie);
      }
    }
  };
  
  FirnLibs::Files::ForEachFile(scanPath, lambda, true);
}


void Player::HandleSettings(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::vector<std::string> command)
{
  auto tok = db.Get();
  if(command.size() == 1)
  {
    Json::Value settings = tok->GetSettings();
    std::vector<unsigned char> replyData;
    FirnLibs::Crypto::StringToVector(replyData, settings.toStyledString());
    client->Send(replyData);
    return;
  }
  else if(command.size() == 3)
  {
    auto setItr = settingsMap.find(command[1]);
    if(setItr != settingsMap.end())
    {
      if(std::find(setItr->second.begin(), setItr->second.end(), command[2]) != setItr->second.end())
      {
        tok->SetSetting(command[1], command[2]);
        std::vector<unsigned char> reply;
        FirnLibs::Crypto::StringToVector(reply, "Settings succesfully updated.");
        client->Send(reply);
        return;
      }
    }
  }
  std::vector<unsigned char> reply;
  FirnLibs::Crypto::StringToVector(reply, "Invalid setting.");
  client->Send(reply);
}

}

