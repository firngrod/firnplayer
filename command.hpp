#pragma once
#include <functional>
#include <memory>
#include "firnlibs/networking/networking.hpp"

namespace FirnPlayer
{
  class Command
  {
  public:
    // Functions
    std::string ToString(const std::string &arngTemplate = "%s") const;
    bool Execute(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command) const;
    Command &operator[](const std::string &subCmd) { return subCmds[subCmd]; }

    // Members
    std::function<bool (const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command)> func;
    std::map<std::string, Command> subCmds;
    std::string description;
    std::string arguments;
  };
}

