#include "metadata.hpp"
#include <boost/filesystem.hpp>
#include <dirent.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include "firnlibs/files/files.hpp"

Json::Value Metadata::ScanDirectory(const std::string &root)
{
  // Initializations
  DIR *dir;
  struct dirent *ent;
  std::list<std::string> directoryQueue;
  Json::Value allJson;

  int total = 0;
  // Push the root to the queue and start the scan loop
  directoryQueue.push_back(root);
  while(directoryQueue.size()){
    // Grap the front of the queue and erase it from the queue
    std::string thisDirectory = *directoryQueue.begin();
    directoryQueue.erase(directoryQueue.begin());
    if((dir = opendir(thisDirectory.c_str())) != NULL) {  // Try to open the directory
      while ((ent = readdir(dir)) != NULL) {
        // First, see if it is a directory.  If so, we add id to the queue for later scans.
        if(ent->d_type == 4 && *ent->d_name != '.')
          directoryQueue.push_back(FirnLibs::Files::CleanPath(thisDirectory + "/" + ent->d_name));
        else{
          // If it is not a queue, we look closer at the file.
          // First, we want to see if it has the .mp3 ending.
          std::string fileName = FirnLibs::Files::CleanPath(thisDirectory + "/" + ent->d_name);
          std::string lastfour("    ");
          std::transform(fileName.begin()+(fileName.size()-4), fileName.end(), lastfour.begin(), ::toupper);
          if(lastfour == ".MP3")
          {
            // Ok, it calls itself an mp3.  Scan it!
            Json::Value &trackJson = (allJson[fileName] = Json::Value());
            TagLib::MPEG::File file(fileName.c_str());
            // If it has an ID3v2Tag, we use that.
            if(file.hasID3v2Tag())
            {
              total ++;
              TagLib::ID3v2::Tag *tag = file.ID3v2Tag(true);
              for(TagLib::List<TagLib::ID3v2::Frame *>::ConstIterator theitr = tag->frameList().begin(); theitr != tag->frameList().end(); theitr++)
              {
                std::string framevalue = (*theitr)->toString().to8Bit();
                std::string trigger((*theitr)->frameID().data());
                trigger = std::string(trigger.begin(), trigger.begin()+4);
                trackJson[trigger] = framevalue;
              }
            }
            // Now save the file path, the bitrate and the track length.
            trackJson["FILE"] = fileName;
            TagLib::MPEG::Properties *properties = file.audioProperties();
            trackJson["LENG"] = properties->length();
            trackJson["BITR"] = properties->bitrate();
            std::cout << trackJson.toStyledString();
          }
        }
      }
    }
  }
  std::cout << "Total files: " << total << std::endl;
  return allJson;
}
