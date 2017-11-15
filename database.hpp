#pragma once
#include "firnlibs/sqlite/sqlite.hpp"
#include <json/value.h>

namespace FirnPlayer
{
  class Database
  {
  public:
    Database();
    bool Initialize(const std::string &path);
    bool AddTrack(const Json::Value &trackInfo);

    Json::Value GetSettings();
    void SetSetting(const std::string &key, const std::string &value);

    Json::Value GetTrack(const std::string &path);

    Json::Value GetTracksMatchingMetadata(const std::string &metadata);

  private:
    sqlite3_stmt *AddTrackStmt();
    sqlite3_stmt *addTrackStmt;
    sqlite3_stmt *AddMetadataStmt();
    sqlite3_stmt *addMetadataStmt;
    sqlite3_stmt *GetMetadataMatchStmt();
    sqlite3_stmt *getMetadataMatchStmt;
    sqlite3_stmt *GetSettingsStmt();
    sqlite3_stmt *getSettingsStmt;
    sqlite3_stmt *SetSettingStmt();
    sqlite3_stmt *setSettingStmt;
    sqlite3_stmt *GetTrackidFromPathStmt();
    sqlite3_stmt *getTrackidFromPathStmt;
    sqlite3_stmt *GetMetadataStmt();
    sqlite3_stmt *getMetadataStmt;
    sqlite3_stmt *GetPathFromIdStmt();
    sqlite3_stmt *getPathFromIdStmt;

    Json::Value GetTrack(const int64_t &trackid, const std::string &path = "");

    void PrepMetadataStmts();
    FirnLibs::SQLite db;
  };
}
