#pragma once
#include "firnlibs/networking/networking.hpp"
#include "database.hpp"
#include "firnlibs/mp3/mp3stream.hpp"


namespace FirnPlayer
{
  class Player
  {
  public:
    Player() : db(), scanPool(1) {}
    void Start();

  protected:
    FirnLibs::Threading::GuardedVar<Database> db;
    FirnLibs::Networking::Listener clientListener;
    FirnLibs::Mp3::Mp3Stream stream;

    std::string Advance(const std::string &current);
    std::string NextGetter(const std::string &current);
    std::string PrevGetter();

    std::vector<std::shared_ptr<FirnLibs::Networking::Client> > clients;
    void AddClient(const std::shared_ptr<FirnLibs::Networking::Client> &client);
    void ClientCallback(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::vector<unsigned char> &data);
    void ClientErrorCallback(const std::shared_ptr<FirnLibs::Networking::Client> &client, const int &error);

    void DoScan(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::vector<std::string> command);
    void HandleSearch(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::vector<std::string> command);
    FirnLibs::Threading::Threadpool scanPool;
    void HandlePlay(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::vector<std::string> command);

    void PreparePlaylist(const std::string &current, const bool &shuffleCurrentFirst);
    void ShufflePlaylist(const std::string &current);
    std::vector<std::string> playlist;
    std::list<std::string> history;
    std::list<std::string>::iterator historyPos;

    const std::map<std::string, std::vector<std::string> > settingsMap = 
    {
      {"scope", {"all", "artist", "album", "track"}},
      {"repeat", {"yes", "no"}},
      {"shuffle", {"yes", "no"}},
    };
    void HandleSettings(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::vector<std::string> command);
  };
}
