// Divisor.cpp
// Helper class for ViewPorts to help with automating layout.

#include "includes/ViewPort.hpp"

ViewPort::Divisor::Divisor(ViewPort * FinalView)
{
  Horizontal = false;
  FinalViewPort = FinalView;
}

int ViewPort::Divisor::CurrentMode()
{
  // 0:  Undecided
  // 1:  ViewPortSplitting
  // 2:  Final ViewPort
  if(FinalViewPort)
    return 2;

  if(SubDivisors.size())
    return 1;

  return 0;
}

bool ViewPort::Divisor::SetHorizontal(bool Horizontal)
{
  // Change split mode to horozontal split
  if(CurrentMode() == 2)
    return false;
  
  this->Horizontal = Horizontal;
  return true;
}

bool ViewPort::Divisor::Divide(ViewPort &Divisor, const double &Size)
{
  // Add a new window to the split.
  return Divide(Divisor.GetFinalDivisor(), Size);
}

bool ViewPort::Divisor::Divide(Divisor &Divisor, const double &Size)
{
  // Add a new window to the split.
  return Divide(&Divisor, Size);
}

bool ViewPort::Divisor::Divide(Divisor *Divisor, const double &Size)
{
  // Add a new window to the split.
  if(CurrentMode() == 2)
    return false;
  SubDivisors.push_back(Divisor);
  Sizes.push_back(Size);
  return true;
}

bool ViewPort::Divisor::SetSize(const int &Number, const double &Size)
{
  if(Number < 0 || Number >= Sizes.size())
    return false;

  Sizes[Number] = Size;
  return true;
}

bool ViewPort::Divisor::Resolve(const int &x0, const int &y0, const int &x1, const int &y1)
{
  // If this is a final viewport, the resolve is easy
  if(FinalViewPort)
  {
    FinalViewPort->SetCorners(x0, y0, x1, y1);
    return true;
  }

  // Otherwise, this gets more interesting.
  else if(SubDivisors.size())
  {
    // First, tally up the total percentages and even it out to be 100% total
    double SizeTotal = 0;
    for(std::vector<double>::const_iterator SizeItr = Sizes.begin(), SizeEnd = Sizes.end();
        SizeItr != SizeEnd; SizeItr++)
    {
      SizeTotal += *SizeItr;
    }
    for(std::vector<double>::iterator SizeItr = Sizes.begin(), SizeEnd = Sizes.end();
        SizeItr != SizeEnd; SizeItr++)
    {
      *SizeItr = SizeTotal ? (*SizeItr / SizeTotal) : 1 / Sizes.size();
    }

    // Find the total size of the window.
    SizeTotal = Horizontal ? (y1 - y0) : (x1 - x0);
    // Making a vector of the new corners rather than sizes to make sure that the edges overlap
    // and that the full size of the main frame is used.
    std::vector<int> NewCorners(Sizes.size() + 1, Horizontal ? y0 : x0);
    for(int i = 1; i < NewCorners.size()-1; i++)
    {
      NewCorners[i] = NewCorners[i - 1] + (int)(Sizes[i - 1] * SizeTotal);
    }
    NewCorners.back() = Horizontal ? y1 : x1;

    // Now, put these new corners to use and give them to the subdivisors.
    bool SubsOK = true;
    for(int i = 0; (i < SubDivisors.size()) && SubsOK; i++)
    {
      int NewX0 = Horizontal ? x0 : NewCorners[i];
      int NewY0 = Horizontal ? NewCorners[i] : y0;
      int NewX1 = Horizontal ? x1 : NewCorners[i + 1];
      int NewY1 = Horizontal ? NewCorners[i + 1] : y1;
      SubsOK &= SubDivisors[i]->Resolve(NewX0, NewY0, NewX1, NewY1);
    }
    
    return SubsOK;
  }
  else
    return false;
}
