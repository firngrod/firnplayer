// utils.h

#ifndef _utils_h_
#define _utils_h_

#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <list>
#include <sys/stat.h>
#include <cstdarg>
#include <cstdio>
#include <fstream>

inline std::string HomePath()
{
  return std::string(getenv("HOME"));
}
inline int msleep(unsigned long mseconds)
{
  return usleep(mseconds * 1000);
}

inline int fsleep(double seconds)
{
  return usleep((unsigned long)(seconds * 1000000));
}

static std::string cleanpath(const std::string &input)
{
  char temp[0x1000];
  char * ptr = temp;
  for(std::string::const_iterator itr = input.begin(), end = input.end(); itr != end && *itr != '\0'; itr++)
  {
    if(*itr == '/' && (*(itr + 1) == '/' || *(itr + 1) == '\0')) // Remove double and trailing /
      itr++;
    else
      *ptr++ = *itr;
  }
  *ptr = '\0';
  realpath(std::string(temp).c_str(), temp);
  return std::string(temp);
}

inline bool FileExists(const std::string FilePath)
{
  struct stat buffer;
  return (stat (FilePath.c_str(), &buffer) == 0);
}

template<typename T>
inline T Saturate(const T &Number, const T &Lower, const T &Upper)
{
  return (Number > Upper) ? Upper : ((Number < Lower) ? Lower : Number);
}

template<typename T>
inline T Max(const T &Number1, const T &Number2)
{
  return (Number1 > Number2) ? Number1 : Number2;
}

template<typename T>
inline T Min(const T &Number1, const T &Number2)
{
  return (Number1 < Number2) ? Number1 : Number2;
}

static std::string StringPrintf(const std::string &Format, ...)
{
  std::string Format2 = /*std::string("2") +*/ Format;
  // Format a string into an std::string with the same syntax as the old-school vsprintf(const char * fmt, ...)
  va_list FormatArguments, FormatArgs;
  va_start(FormatArgs, Format);
  // First, find the length of the formatted string.  This partly to limit RAM use, but mostly to make absolutely
  // sure that no albeit huge predefined buffer will ever overflow.
  // I read somewhere that vsnprintf can modify the arglist, so we do a copy for it to play with.
  va_copy(FormatArguments, FormatArgs);
  int TotalLength = vsnprintf(NULL, 0, Format2.c_str(), FormatArguments);
  va_end(FormatArguments);
  // Now use this info to make a buffer and vsprintf into it, convert it into an std::string, clean up and return
  // the string.
  char * Buffer = new char[TotalLength + 1];
  vsprintf(Buffer, Format2.c_str(), FormatArgs);
  std::string ReturnString = Buffer;
  va_end(FormatArgs);
  delete[] Buffer;
  return ReturnString;
}


static void DebugOut(const std::string &FileName, const std::string &Output)
{
  std::ofstream OutputFile;
  OutputFile.open(FileName.c_str(), std::ofstream::out | std::ofstream::app);
  if(OutputFile.is_open())
  {
    OutputFile << Output;
    OutputFile.close();
  }
}


static long SafeStoi(const std::string &NumberString, const int &Base = 10)
{
  if(NumberString == "")
    return 0;
  else
    return std::stoi(NumberString, NULL, Base);
}


template<typename T>
static T Saturate(T &Number, const T &Lower, const T &Upper)
{
  Number = Number < Lower ? Lower : (Number > Upper ? Upper : Number);
  return Number;
}

#endif
