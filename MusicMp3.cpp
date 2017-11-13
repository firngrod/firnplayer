#include "MusicMp3.hpp"
#include <cstdio>


MusicMp3::MusicMp3() : sf::Music::Music()
{
  mh = NULL;
  done = 0;
  channels = encoding = 0;
  rate = 0;
  err = MPG123_OK;
  samples = 0;
  err = mpg123_init();
  if(err != MPG123_OK || (mh = mpg123_new(NULL, &err)) == NULL)
  {
    fprintf(stderr, "Basic setup goes wrong: %s", mpg123_plain_strerror(err));
    mpg123_ok = false;
    return;
  }
  mpg123_ok = true;
}

MusicMp3::~MusicMp3()
{
  mpg123_close(mh);
  mpg123_delete(mh);
  mpg123_exit();
}

void MusicMp3::setQueue(const std::string &nextTrack)
{
  this->nextTrack = nextTrack;
}

bool MusicMp3::openFromFile(const std::string &filename)
{
  // First off, stop playback
  stop();
  // Kill the queue;
  nextTrack = ""
  ;
  // Then open the new file.
  if (mpg123_open(mh, filename.c_str()) != MPG123_OK
      || mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK)
  {
    mpg123_ok = false;
    fprintf( stderr, "Trouble with mpg123: %s\n", mpg123_strerror(mh) );
    return false;
  }
  
  initialize();
  
  mpg123_ok = true;
  return true;
}


bool MusicMp3::onGetData(SoundStream::Chunk& data)
{
  sf::Lock lock(m_mutex);

  data.samples = &m_samples[0];
  err = mpg123_read(mh, (unsigned char *)&m_samples[0], m_samples.size()*2, &done);
  data.sampleCount = done/2;

  if((data.sampleCount == m_samples.size()) || !nextTrack.size())
  return data.sampleCount == m_samples.size();

  int oldchannels = channels, oldencoding = encoding;
  long oldrate = rate;

  if (mpg123_open(mh, nextTrack.c_str()) != MPG123_OK
      || mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK)
  {
    mpg123_ok == false;
    fprintf(stderr, "Trouble with mpg123: %s\n", mpg123_strerror(mh) );
    return false;
  }
  
  if(oldchannels != channels || oldencoding != encoding || oldrate != rate)
  {
    fprintf(stderr, "New file rate, encoding or channel count does not match old.\n");
    return false;
  }

  err = mpg123_read(mh, (unsigned char *)&m_samples[data.sampleCount], (m_samples.size() - data.sampleCount) * 2, &done);
  data.sampleCount += done/2;
  nextTrack = "";
  m_duration = sf::seconds((float)mpg123_length(mh) / rate);

  return data.sampleCount == m_samples.size();
}


void MusicMp3::initialize()
{
  m_duration =  sf::seconds((float)mpg123_length(mh) / rate);

  m_samples.resize(rate);

  SoundStream::initialize(channels, rate);
}
