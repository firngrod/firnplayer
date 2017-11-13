#pragma once
#include <json/value.h>

namespace Metadata
{
  Json::Value ScanDirectory(const std::string &root);
}
