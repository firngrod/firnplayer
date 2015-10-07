#include <thread>
#include "includes/ViewPort.hpp"
#include "includes/AccessQueue.hpp"
#include "includes/MetaData.hpp"
#include "includes/json/value.h"
#include "includes/Playlist.hpp"
#include "includes/Player.hpp"
#include "includes/utils.h"


int main(int argc, char ** argv)
{
  Player ThePlayer;
  ThePlayer.DoRun();
  fsleep(1);

  return 0;
}
