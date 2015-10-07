#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <signal.h>
#include <ncurses.h>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <list>
#include <map>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <dirent.h>
#include <algorithm>
#include <boost/filesystem.hpp>



#include "includes/json/value.h"
#include "includes/json/reader.h"
#include "includes/utils.h"

#include "AccessQueue.hpp"
#include "ViewPort.hpp"
#include "MetaData.hpp"
#include "Playlist.hpp"
#include "Player.hpp"
