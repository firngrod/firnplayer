#include "firnlibs/string/string.hpp"
#include "command.hpp"

namespace FirnPlayer
{
  bool Command::Execute(const std::shared_ptr<FirnLibs::Networking::Client> &client, const std::string &command) const
  {
    for(const auto &cItr: subCmds)
    {
      if(command == cItr.first || (command.find(cItr.first) == 0 && command.find_first_of(" \r\n") == cItr.first.size()))
      {
        return cItr.second.Execute(client, command.substr(cItr.first.size(), command.size() - cItr.first.size()));
      }
    }

    if(func != nullptr)
      return func(client, command);

    return false;
  }


  std::string Command::ToString(const std::string &argTemplate) const
  {
    std::string outputStr = FirnLibs::String::StringPrintf(argTemplate, arguments.c_str());
    outputStr += FirnLibs::String::Replace(description, "\n", "\n" + FirnLibs::String::StringPrintf(argTemplate, ""));
    outputStr += "\n";

    int maxLenCmd = 0;
    int maxLenArg = 0;
    for(const auto &cItr: subCmds)
    {
      if(cItr.first.size() > maxLenCmd)
        maxLenCmd = cItr.first.size();
      if(cItr.second.arguments.size() > maxLenArg)
        maxLenArg = cItr.second.arguments.size();
    }

    std::string strTemplateCmd = FirnLibs::String::StringPrintf("%%-%ds ", maxLenCmd);
    std::string strTemplateArg = FirnLibs::String::StringPrintf("%%-%ds  ", maxLenArg);
    std::string spacingStr = FirnLibs::String::StringPrintf(strTemplateCmd.c_str(), "");

    for(const auto &cItr: subCmds)
    {
      std::string subDesc = cItr.second.ToString(strTemplateArg);
      subDesc = FirnLibs::String::Replace(subDesc, "\n", "\n" + spacingStr);
      subDesc = "\n" + FirnLibs::String::StringPrintf(strTemplateCmd.c_str(), cItr.first.c_str()) + subDesc;
      outputStr += subDesc;
    }
    return outputStr;
  }
}
