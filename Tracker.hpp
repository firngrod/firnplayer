// Tracker.hpp


namespace Firnplayer
{
  class Tracker
  {
  public:
    Tracker(ArtistList &TheList);
    
    void SetShuffle(const bool &Shuffle);
    void SetRepeat(const bool &Repeat);
    void SetPlaymode(const int &Playmode);

    enum
    {
      PLAYMODE_ALL,
      PLAYMODE_ARTIST,
      PLAYMODE_ALBUM,
      PLAYMODE_PLAYLIS,
    };
  private:
    int Playmode;
    bool Shuffle;
    
  };
}
