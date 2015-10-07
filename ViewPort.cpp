// ViewPort.cpp

#include "includes/ViewPort.hpp"
#include "includes/utils.h"
#include <vector>
#include <cstring>


// Library initialization
bool ViewPort::Initialized = false;
void ViewPort::Initialize()
{
  if(Initialized)
    return;
  initscr();
  cbreak();
  noecho();
  start_color();
  keypad(stdscr, TRUE);
  Initialized = true;
}

void ViewPort::Uninitialize()
{
  endwin();
  Initialized = false;
}



// Constructor

ViewPort::ViewPort(const int &x0, const int &y0, const int &x1, const int &y1, 
                   const int &Attribute, const int &FrameColPair,
                   const int &ColPair)
{
  this->x0           = x0;
  this->y0           = y0;
  this->x1           = x1;
  this->y1           = y1;
  this->Attributes   = Attribute;
  this->ColPair      = ColPair;
  this->FrameColPair = FrameColPair;
  DivEnd = Divisor(this);
}

// Attribute operations

void ViewPort::SetAttr(const int &Attribute)
{
  this->Attributes = Attribute;
}

void ViewPort::AddAttr(const int &Attribute)
{
  this->Attributes |= Attribute;
}

void ViewPort::RemAttr(const int &Attribute)
{
  this->Attributes &= ~Attribute;
}

int ViewPort::GetAttr()
{
  return this->Attributes;
}

void ViewPort::SetColPair(const int &ColPair)
{
  this->ColPair = ColPair;
}

int ViewPort::GetColPair()
{
  return this->ColPair;
}

void ViewPort::DefineColPair(const int &Pair, const int &FG, const int &BG)
{
  if(!Initialized)
    return;
  init_pair(Pair, FG, BG);
}

void ViewPort::SetCorners(const int &x0, const int &y0, const int &x1, const int &y1)
{
  this->x0           = x0;
  this->y0           = y0;
  this->x1           = x1;
  this->y1           = y1;
}

ViewPort::Divisor * ViewPort::GetFinalDivisor()
{
  return &DivEnd;
}

int ViewPort::MaxLineLength()
{
  if(!Initialized)
    return 0;
  int Height, Width;
  getmaxyx(stdscr, Height, Width);
  return Saturate(x1 - x0 - 3, 0, Width - 3);
}

int ViewPort::GetHeight()
{
  return y1 - y0 - 1;
}
void ViewPort::SetTopText(const std::string &TopText)
{
  this->TopText = TopText;
}

void ViewPort::SetBottomText(const std::string &BottomText)
{
  this->BottomText = BottomText;
}

void ViewPort::SetFrameColPair(const int &ColPair)
{
  this->FrameColPair = ColPair;
}

// Do the printing

int ViewPort::Write(const int &x, const int &y, const int &MaxLength, const int &Attribute, 
        const int &ColPair, const char * fmt)
{
  // The main function for writing stuff in the viewport.
  // x, y denominate where to start the line.
  // MaxLength defines how many characters may be written.
  // MaxLength = 0 will let the line write until the edge of the window.
  // MaxLength = -1 will let the line write until the edge of the window and will blank the
  // MaxLength = -2 is like MaxLength = -1 except that the gap to the edge of the screen will fill as well
  // line apart from what it is writing (Spaces all over the place)
  // Attribute and ColPair set the Attribute and Color Pair for the text.
  // Sorry about the messy codeUNDERLINE.

  if(!Initialized || (x < 0) || (y < 0) || (x > x1 - x0 - 3) || (y > y1 - y0 - 1))
    return -1;
  bool CleanLine = MaxLength < 0;
  bool EdgeToEdge = MaxLength < -1;
  int LengthMax = MaxLength;
  char DefaultVal = MaxLength < 0 ? ' ' : '\0'; // Find out if we overwrite line with spaces.
  if(LengthMax <= 0)
  // If we blank the line, we want max length no matter what.  Otherwise, we only print between x and end of frame.
  LengthMax = MaxLineLength() - (CleanLine ? 0 : x); 
  std::vector<char> WriteBuffer(LengthMax + (EdgeToEdge ? 3 : 1), DefaultVal);
  // Write the text to our buffer.  If we are blanking the line, we start writing at x and leave the blanks before.
  snprintf(&WriteBuffer[0] + (CleanLine ? x : 0) + (EdgeToEdge ? 1 : 0),
           WriteBuffer.size() - (CleanLine ? x : 0) - (EdgeToEdge ? 1 : 0), fmt, strlen(fmt));
  // If we are blanking, we don't want to stop writing at the end of the string.
  WriteBuffer[strlen(&WriteBuffer[CleanLine ? x : 0]) + (CleanLine ? x : 0)] = DefaultVal; 
  WriteBuffer.back() = '\0'; // Null terminate the full buffer in case we are blanking.
  wmove(stdscr, y + y0 + 1, x0 + (EdgeToEdge ? 1 : 2) + (CleanLine ? 0 : x));
  wattron(stdscr, COLOR_PAIR(ColPair) | Attribute);
  int RetVal = waddstr(stdscr, &WriteBuffer[0]);
  return RetVal;
}

int ViewPort::Write(const int &x, const int &y, const int &MaxLength, const char * fmt)
{
  // This just passes on to the main version of this function with the window default values.
  int RetVal = Write(x, y, MaxLength, this->Attributes, this->ColPair, fmt);
  return RetVal;
}

void ViewPort::DrawFrame(const int &ColPair)
{
  if(!Initialized)
    return;
  // Draw the frame, possibly with a specific color.
  // First, assing a vector to hold the top and bottom data in and zero terminate it.
  std::vector<char> TopBottom(x1 - x0 + 1, '=');
  TopBottom.push_back('\0');
  // Print the top text in the buffer and undo the zero termination.
  snprintf(&TopBottom[2], TopBottom.size() - 6, "%s", TopText.c_str());
  TopBottom[strlen(&TopBottom[0])] = '=';
  // Move and print
  wmove(stdscr, y0, x0);
  wattron(stdscr, COLOR_PAIR((ColPair >= 0) ? ColPair : FrameColPair));
  waddstr(stdscr, &TopBottom[0]);
  // Rince and repeat for the bottom.
  TopBottom.assign(TopBottom.size() - 1, '-');
  TopBottom.push_back('\0');
  snprintf(&TopBottom[2], TopBottom.size() - 6, "%s", BottomText.c_str());
  TopBottom[strlen(&TopBottom[0])] = '-';
  wmove(stdscr, y1, x0);
  waddstr(stdscr, &TopBottom[0]);
  // Draw the vertical bars.
  for(int i = y0 + 1; i < y1; i++)
  {
    wmove(stdscr, i, x0);
    waddch(stdscr, '|');
    wmove(stdscr, i, x1);
    waddch(stdscr, '|');
  }
}

void ViewPort::Update()
{
  if(!Initialized)
    return;
  wrefresh(stdscr);
}
