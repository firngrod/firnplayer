// Playlist.hpp

namespace Firnplayer
{
  class Playlist
  {
  public:
    Playlist();


    bool Up(const bool &WrapAround = false);
    bool Down(const bool &WrapAround = false);
    bool MoveUp();
    bool MoveDown();
    bool Append(Json::Value * NewTrack);
    bool Insert(Json::Value * NewTrack);
    bool MoveToStart();
    bool MoveToEnd();
    bool RemoveCurrent();

  private:
    std::list<Json::Value *> Tracks;
    std::list<Json::Value *>::iterator CurrentTrack;
  };
}
