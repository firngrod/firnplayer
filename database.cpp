#include "database.hpp"
#include "firnlibs/files/files.hpp"
#include <iostream>


namespace FirnPlayer
{


Database::Database()
{
}


bool Database::Initialize(const std::string &path)
{
  FirnLibs::SQLite::Error rc = db.LoadDB(path);
  
  if(rc != FirnLibs::SQLite::Error::None)
    return false;

  Json::Value settings = GetSettings();
  int64_t dbVer = std::stoi(settings.get("dbver", "0").asString());

  if(dbVer < 1)
  {
    rc = db.UnpreparedExecute("CREATE TABLE settings ( key TEXT NOT NULL, value TEXT NOT NULL, PRIMARY KEY (key));", nullptr);
    if(rc != FirnLibs::SQLite::Error::None)
      return false;

    rc = db.UnpreparedExecute("CREATE TABLE tracks ( trackid INTEGER NOT NULL PRIMARY KEY, path TEXT NOT NULL UNIQUE ON CONFLICT FAIL);", nullptr);
    if(rc != FirnLibs::SQLite::Error::None)
      return false;

    rc = db.UnpreparedExecute("CREATE TABLE metadata ( trackid INTEGER NOT NULL, key TEXT NOT NULL, value TEXT NOT NULL, PRIMARY KEY (trackid, key), FOREIGN KEY(trackid) REFERENCES tracks(trackid) ON UPDATE CASCADE ON DELETE CASCADE);", nullptr);
    if(rc != FirnLibs::SQLite::Error::None)
      return false;

    rc = db.UnpreparedExecute("INSERT INTO settings (key, value) VALUES ('dbver', '1')", nullptr);
    if(rc != FirnLibs::SQLite::Error::None)
      return false;
  }

  return true;
}


sqlite3_stmt *Database::AddTrackStmt()
{
  if(statementMap.find("addTrackStmt") == statementMap.end())
    statementMap["addTrackStmt"] = db.Prepare("INSERT INTO tracks (path) VALUES(?);");
  return statementMap["addTrackStmt"];
}


sqlite3_stmt *Database::AddMetadataStmt()
{
  if(statementMap.find("addMetadataStmt") == statementMap.end())
    statementMap["addMetadataStmt"] = db.Prepare("INSERT INTO metadata (trackid, key, value) VALUES ((SELECT trackid FROM tracks WHERE path=?), ?, ?);");
  return statementMap["addMetadataStmt"];
}


sqlite3_stmt *Database::GetMetadataMatchStmt()
{
  if(statementMap.find("getMetadataMatchStmt") == statementMap.end())
    statementMap["getMetadataMatchStmt"] = db.Prepare("SELECT DISTINCT trackid FROM metadata WHERE value LIKE '%' || ? || '%' COLLATE NOCASE;");
  return statementMap["getMetadataMatchStmt"];
}


sqlite3_stmt *Database::GetMetadataMatchKeyStmt()
{
  if(statementMap.find("getMetadataMatchKeyStmt") == statementMap.end())
    statementMap["getMetadataMatchKeyStmt"] = db.Prepare("SELECT DISTINCT trackid FROM metadata WHERE key = ? AND value = ? COLLATE NOCASE;");
  return statementMap["getMetadataMatchKeyStmt"];
}


sqlite3_stmt *Database::GetSettingsStmt()
{
  if(statementMap.find("getSettingsStmt") == statementMap.end())
    statementMap["getSettingsStmt"] = db.Prepare("SELECT * FROM settings;");
  return statementMap["getSettingsStmt"];
}


sqlite3_stmt *Database::SetSettingStmt()
{
  if(statementMap.find("setSettingStmt") == statementMap.end())
    statementMap["setSettingStmt"] = db.Prepare("INSERT OR REPLACE INTO settings(key, value) VALUES( ? , ? );");
  return statementMap["setSettingStmt"];
}


sqlite3_stmt *Database::GetTrackidFromPathStmt()
{
  if(statementMap.find("getTrackidFromPathStmt") == statementMap.end())
    statementMap["getTrackidFromPathStmt"] = db.Prepare("SELECT trackid FROM tracks WHERE path=?;");
  return statementMap["getTrackidFromPathStmt"];
}


sqlite3_stmt *Database::GetMetadataStmt()
{
  if(statementMap.find("getMetadataStmt") == statementMap.end())
    statementMap["getMetadataStmt"] = db.Prepare("SELECT key, value FROM metadata WHERE trackid=?;");
  return statementMap["getMetadataStmt"];
}


sqlite3_stmt *Database::GetPathFromIdStmt()
{
  if(statementMap.find("getPathFromIdStmt") == statementMap.end())
    statementMap["getPathFromIdStmt"] = db.Prepare("SELECT path FROM tracks WHERE trackid=?;");
  return statementMap["getPathFromIdStmt"];
}


sqlite3_stmt *Database::GetAllOfAMetadataTagStmt()
{
  if(statementMap.find("getAllOfAMetadataTagStmt") == statementMap.end())
    statementMap["getAllOfAMetadataTagStmt"] = db.Prepare("SELECT DISTINCT value FROM metadata WHERE key=?;");
  return statementMap["getAllOfAMetadataTagStmt"];
}


sqlite3_stmt *Database::GetATrackMatchingMetadataTagStmt()
{
  if(statementMap.find("getATrackMatchingMetadataTagStmt") == statementMap.end())
    statementMap["getATrackMatchingMetadataTagStmt"] = 
      db.Prepare("SELECT path FROM tracks WHERE trackid IN (SELECT trackid FROM metadata WHERE key=? AND value=? LIMIT 1);");
  return statementMap["getATrackMatchingMetadataTagStmt"];
}


sqlite3_stmt *Database::GetRandomTrackStmt()
{
  if(statementMap.find("getRandomTrackStmt") == statementMap.end())
    statementMap["getRandomTrackStmt"] = 
      db.Prepare("SELECT path FROM tracks LIMIT 1 OFFSET (SELECT ABS(RANDOM() % (SELECT COUNT(trackid) FROM tracks)));");
  return statementMap["getRandomTrackStmt"];
}


Json::Value Database::GetTrack(const std::string &path)
{
  int64_t trackid = 0;
  auto lambda1 = [&trackid] (const FirnLibs::SQLite::PrepVector &vals, const std::vector<std::string> &colNames) -> void
  {
    vals[0].GetValue(trackid);
  };
  db.PreparedExecute(GetTrackidFromPathStmt(), {FirnLibs::SQLite::Prepvar(path)}, lambda1);

  if(trackid == 0)
    return Json::Value();

  return GetTrack(trackid, path);
}


Json::Value Database::GetTrack(const int64_t &trackid, const std::string &path)
{
  std::string pathInHere = path;
  if(pathInHere == "")
  {
    pathInHere = GetTrackPath(trackid);
  }
  Json::Value trackInfo;
  trackInfo["FILE"] = pathInHere;
  trackInfo["TRID"] = trackid;
  auto lambda2 = [&trackInfo] (const FirnLibs::SQLite::PrepVector &vals, const std::vector<std::string> &colNames) -> void
  {
    std::string tmpie1, tmpie2;
    vals[0].GetValue(tmpie1);
    vals[1].GetValue(tmpie2);
    trackInfo[tmpie1] = tmpie2;
  };
  db.PreparedExecute(GetMetadataStmt(), {FirnLibs::SQLite::Prepvar(trackid)}, lambda2);
  return trackInfo;
}


std::string Database::GetTrackPath(const int64_t &trackid)
{
  std::string path;
  auto lambda1 = [&path] (const FirnLibs::SQLite::PrepVector &vals, const std::vector<std::string> &colNames) -> void
  {
    vals[0].GetValue(path);
  };
  db.PreparedExecute(GetPathFromIdStmt(), {FirnLibs::SQLite::Prepvar(trackid)}, lambda1);
  return path;
}


void Database::SetSetting(const std::string &key, const std::string &value)
{
  FirnLibs::SQLite::PrepVector tmpie = {
                                        FirnLibs::SQLite::Prepvar(key),
                                        FirnLibs::SQLite::Prepvar(value)
                                        };

  db.PreparedExecute(SetSettingStmt(), tmpie, FirnLibs::SQLite::PrepCallback(nullptr));
}


bool Database::AddTrack(const Json::Value &trackInfo)
{
  std::string path = trackInfo.get("FILE", "").asString();
  if(path == "")
    return false;

  std::vector<FirnLibs::SQLite::Prepvar> vars;
  vars.push_back(FirnLibs::SQLite::Prepvar(path));
  FirnLibs::SQLite::Error rc = db.PreparedExecute(AddTrackStmt(), vars, FirnLibs::SQLite::PrepCallback(nullptr));

  if(rc != FirnLibs::SQLite::Error::None)
    return false;

  vars.resize(3);

  std::vector<std::string> keys = trackInfo.getMemberNames();
  for(std::vector<std::string>::const_iterator keyItr = keys.begin(), keyEnd = keys.end(); keyItr != keyEnd; keyItr++)
  {
    if(*keyItr == "FILE")
      continue;

    vars[1] = FirnLibs::SQLite::Prepvar(*keyItr);
    vars[2] = FirnLibs::SQLite::Prepvar(trackInfo.get(*keyItr, "").asString());

    rc = db.PreparedExecute(AddMetadataStmt(), vars, FirnLibs::SQLite::PrepCallback(nullptr));

    if(rc != FirnLibs::SQLite::Error::None)
      continue;
  }

  return true;
}


Json::Value Database::GetSettings()
{
  Json::Value settings;
  auto lambda = [&settings] (const std::vector<FirnLibs::SQLite::Prepvar> &vals, const std::vector<std::string> &colNames) -> void 
  {
    std::string tmpie, tmpie2;
    vals[0].GetValue(tmpie);
    vals[1].GetValue(tmpie2);
    settings[tmpie] = tmpie2;
  };

  db.PreparedExecute(GetSettingsStmt(), FirnLibs::SQLite::PrepVector(), FirnLibs::SQLite::PrepCallback(lambda));

  return settings;
}


Json::Value Database::GetTracksMatchingMetadata(const std::string &metaValue, const std::string &metaKey)
{
  std::vector<int64_t> matchingTracks;
  auto lambda = [&matchingTracks] (const std::vector<FirnLibs::SQLite::Prepvar> &vals, const std::vector<std::string> &colNames) -> void
  {
    matchingTracks.push_back(0);
    vals[0].GetValue(matchingTracks.back());
  };
  if(metaKey == "")
  {
    db.PreparedExecute(GetMetadataMatchStmt(), {FirnLibs::SQLite::Prepvar(metaValue)}, lambda);
  }
  else
  {
    db.PreparedExecute(GetMetadataMatchKeyStmt(), {FirnLibs::SQLite::Prepvar(metaKey), FirnLibs::SQLite::Prepvar(metaValue)}, lambda);
  }

  Json::Value tracks;

  for(auto trackid: matchingTracks)
  {
    tracks[tracks.size()] = GetTrack(trackid);
  }

  return tracks;  
}


std::string Database::GetNextMetaOfKey(const std::string &metaKey, const std::string &metaValue)
{
  std::string nextValue = "";
  std::string firstValue = "";
  auto lambda = [&nextValue, &metaValue, &firstValue] (const std::vector<FirnLibs::SQLite::Prepvar> &vals, const std::vector<std::string> &colNames) -> void
  {
    std::string tmpie;
    vals[0].GetValue(tmpie);
    if(firstValue == "" || tmpie < firstValue)
      firstValue = tmpie;

    if(tmpie > metaValue && (nextValue == "" || tmpie < nextValue))
      nextValue = tmpie;
  };
  db.PreparedExecute(GetAllOfAMetadataTagStmt(), {metaKey}, lambda);
  
  if(nextValue == "")
    return firstValue;

  return nextValue;
}


std::string Database::GetATrackWithMetadata(const std::string &key, const std::string &value)
{
  std::string track;
  auto lambda = [&track] (const std::vector<FirnLibs::SQLite::Prepvar> &vals, const std::vector<std::string> &colNames) -> void
  {
    vals[0].GetValue(track);
  };
  db.PreparedExecute(GetATrackMatchingMetadataTagStmt(), {key, value}, lambda);
  return track;
}


std::string Database::GetRandomTrack()
{
  std::string track;
  auto lambda = [&track] (const std::vector<FirnLibs::SQLite::Prepvar> &vals, const std::vector<std::string> &colNames) -> void
  {
    vals[0].GetValue(track);
  };
  db.PreparedExecute(GetRandomTrackStmt(), {}, lambda);
  return track;
}




}
