#include <SFML/Audio.hpp>
#include <map>
#include <string>
#include <cstdio>
#include <mpg123.h>

class MusicMp3: public sf::Music
{
public:
  void setQueue(const std::string &nextTrack);
  MusicMp3();
  ~MusicMp3();
  bool mpg123_ok;
  bool openFromFile(const std::string& filename);
protected:
  std::string nextTrack;
  bool onGetData(SoundStream::Chunk& data);
  void initialize();
  mpg123_handle *mh;
  size_t done;
  int channels, encoding;
  long rate;
  int err;
  off_t samples;
};
