// ViewPort.cpp
#ifndef _VIEWPORT_HPP_
#define _VIEWPORT_HPP_


class ViewPort
{
public:
  // Constructor
  ViewPort(const int &x0 = 0, const int &y0 = 0, const int &x1 = 0, const int &y1 = 0, 
           const int &Attribute = A_NORMAL, const int &FrameColPair = 1, 
           const int &ColPair = 1);
  
  // Settings
  void SetCorners(const int &x0, const int &y0, const int &x1, const int &y2);
  void SetAttr(const int &Attribute);
  void AddAttr(const int &Attribute);
  void RemAttr(const int &Attribute);
  int  GetAttr();
  void SetColPair(const int &ColPair);
  int  GetColPair();
  int  MaxLineLength();
  int  GetHeight();
  void SetTopText(const std::string &TopText);
  void SetBottomText(const std::string &BottomText);
  void SetFrameColPair(const int &ColPair);

  // Do output
  int  Write(const int &x, const int &y, const int &MaxLength, const int &Attribute, 
        const int &ColPair, const char * fmt);
  int  Write(const int &x, const int &y, const int &MaxLength, const char * fmt);
  void DrawFrame(const int &ColPair = -1);


protected:
  int Attributes, ColPair, FrameColPair;
  int x0, y0, x1, y1;
  std::string TopText, BottomText;


  // Static stuff
public:
  static void DefineColPair(const int &Pair, const int &FG, const int &BG);
  static void Initialize();
  static void Uninitialize();
  static void Update();
protected:
  static bool Initialized;

  // Helper class
public:
  class Divisor
  {
  public:
    Divisor(ViewPort * FinalView = NULL);
    // Divide : Add a new subwindow with a size.
    bool Divide(Divisor *Divisor, const double &Size);
    bool Divide(Divisor &Divisor, const double &Size);
    bool Divide(ViewPort &Divisor, const double &Size);
    bool SetSize(const int &Number, const double &Size);
    // Switch between vertical and horizontal views.
    bool SetHorizontal(bool Horizontal = true);
    // Set all sub-divisors final sizes recursively.
    bool Resolve(const int &x0, const int &x1, const int &y0, const int &y1);
  
  private:
    std::vector<Divisor *> SubDivisors;
    std::vector<double> Sizes;
    bool Horizontal;
    ViewPort * FinalViewPort;
    int CurrentMode();
  };
  Divisor *GetFinalDivisor();
protected:
  Divisor DivEnd;
};

#endif // _VIEWPORT_HPP_
