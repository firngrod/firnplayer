#include "database.hpp"
#include "firnlibs/files/files.hpp"
#include <iostream>


namespace FirnPlayer
{


Database::Database()
{
  addTrackStmt = nullptr;
  addMetadataStmt = nullptr;
  getSettingsStmt = nullptr;
  setSettingStmt = nullptr;
  getMetadataMatchStmt = nullptr;
  getTrackidFromPathStmt = nullptr;
  getMetadataStmt = nullptr;
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

    rc = db.UnpreparedExecute("CREATE TABLE metadata ( trackid INTEGER NOT NULL, key TEXT NOT NULL, value TEXT NOT NULL, PRIMARY KEY (trackid, key), FOREIGN KEY(trackid) REFERENCES tracks(trackid));", nullptr);
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
  if(addTrackStmt == nullptr)
    addTrackStmt = db.Prepare("INSERT INTO tracks (path) VALUES(?);");
  return addTrackStmt;
}


sqlite3_stmt *Database::AddMetadataStmt()
{
  if(addMetadataStmt == nullptr)
    addMetadataStmt = db.Prepare("INSERT INTO metadata (trackid, key, value) VALUES ((SELECT trackid FROM tracks WHERE path=?), ?, ?);");
  return addMetadataStmt;
}


sqlite3_stmt *Database::GetMetadataMatchStmt()
{
  if(getMetadataMatchStmt == nullptr)
    getMetadataMatchStmt = db.Prepare("SELECT path FROM tracks WHERE trackid IN (SELECT trackid FROM metadata WHERE value LIKE ?);");
  return getMetadataMatchStmt;
}


sqlite3_stmt *Database::GetSettingsStmt()
{
  if(getSettingsStmt == nullptr)
    getSettingsStmt = db.Prepare("SELECT * FROM settings;");
  return getSettingsStmt;
}


sqlite3_stmt *Database::SetSettingStmt()
{
  if(setSettingStmt == nullptr)
    setSettingStmt = db.Prepare("INSERT OR REPLACE INTO settings(key, value) VALUES( ? , ? );");
  return setSettingStmt;
}


sqlite3_stmt *Database::GetTrackidFromPathStmt()
{
  if(getTrackidFromPathStmt == nullptr)
    getTrackidFromPathStmt = db.Prepare("SELECT trackid FROM tracks WHERE path=?;");
  return getTrackidFromPathStmt;
}


sqlite3_stmt *Database::GetMetadataStmt()
{
  if(getMetadataStmt == nullptr)
    getMetadataStmt = db.Prepare("SELECT key, value FROM metadata WHERE trackid=?;");
  return getMetadataStmt;
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

  Json::Value trackInfo;
  trackInfo["FILE"] = path;
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


}
