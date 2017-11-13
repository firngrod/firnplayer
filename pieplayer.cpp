#include <iostream>
#include <SFML/Audio.hpp>
#include "firnlibs/crypto/aes.hpp"
#include "firnlibs/crypto/general.hpp"
#include "firnlibs/threading/threadpool.hpp"
#include "firnlibs/threading/guardedvar.hpp"
#include "firnlibs/networking/networking.hpp"
#include "firnlibs/files/files.hpp"
#include "firnlibs/mp3/metadata.hpp"
#include "firnlibs/sqlite/sqlite.hpp"
#include "firnlibs/mp3/mp3stream.hpp"
#include "firnlibs/string/string.hpp"
#include <unistd.h>
#include "database.hpp"
#include "player.hpp"

int main(int argc, char ** argv)
{
  //std::string tmp = "settings\r\n";
  //std::vector<std::string> split = FirnLibs::String::Split(tmp, " \n");
  //std::cout << split.size() << std::endl;
  //return 0;
  FirnPlayer::Player player;
  player.Start();

  sleep(500);
  //FirnLibs::Mp3::Mp3Stream stream;
  //stream.SetNextGetter(std::function<std::string (const std::string &)>(NextGetter));
  //stream.play();

  //sleep(10);
  //stream.stop();
  //stream.SetNextGetter(std::function<std::string (const std::string &)>(NextGetter2));
  //stream.play();

  //sleep(10);
  
  //stream.stop();

  //auto lambda = [&db] (const std::string &path) -> void
  //{
    //if(FirnLibs::Files::HasExtension(path, "mp3", false))
    //{
      //Json::Value tmpie;
      //FirnLibs::Mp3::GetMetadata(tmpie, path);
      //db.AddTrack(tmpie);
      //std::cout << tmpie.toStyledString();
    //}
  //};
  //FirnLibs::Files::ForEachFile("/home/christian/NASMusic", lambda, true);
  
  std::cout << "Exiting program.\n";
  return 0;
}
