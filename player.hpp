#pragma once
#include "firnlibs/networking/networking.hpp"
#include "database.hpp"
#include "firnlibs/mp3/mp3stream.hpp"
#include "command.hpp"


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
    bool        GetFromQueue(std::string &output);

    class ClientStuff
    {
    public:
      ClientStuff(const std::shared_ptr<FirnLibs::Networking::Client> &client) { this->client = client; }
      std::shared_ptr<FirnLibs::Networking::Client> client;
      std::vector<std::string> queue;
      bool operator==(const std::shared_ptr<FirnLibs::Networking::Client> &other) { return other == client; }
      bool operator<(const std::shared_ptr<FirnLibs::Networking::Client> &other) { return client < other; }
      ClientStuff &operator=(const std::shared_ptr<FirnLibs::Networking::Client> &other) { client = other; return *this; }
    };
    std::vector<ClientStuff> clients;

    void AddClient(const std::shared_ptr<FirnLibs::Networking::Client> &client);
    void ClientCallback(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::vector<unsigned char> &data);
    void ClientErrorCallback(const std::shared_ptr<FirnLibs::Networking::Client> &client, const int &error);

    void MapCmds();
    void HandleScan(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command);
    void DoScan(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command);
    void HandleSearch(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command);
    FirnLibs::Threading::Threadpool scanPool;
    void HandlePlay(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command);
    void HandleQueue(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command);
    void HandleInfo(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command);
    bool HandleHelp(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command);
    void HandleResume(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command);
    void HandleStop(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command);
    void HandlePrev(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command);
    void HandleNext(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command);

    void PreparePlaylist(const std::string &current, const bool &shuffleCurrentFirst);
    void ShufflePlaylist(const std::string &current);
    std::vector<std::string> playlist;
    std::list<std::string> history;
    FirnLibs::Threading::Queue<std::string> queue;
    std::list<std::string>::iterator historyPos;

    FirnPlayer::Command cmds;

    const std::map<std::string, std::vector<std::string> > settingsMap = 
    {
      {"scope", {"all", "artist", "album", "track"}},
      {"repeat", {"yes", "no"}},
      {"shuffle", {"yes", "no"}},
    };
    void HandleSettings(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &commandStr);
  };
}
