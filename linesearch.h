#ifndef linesearch_h
#define linesearch_h

#include "paramin.h"
#include "netinterface.h"

class NetInterface;
class Condition;

class LineSearch {
protected:
  int NUMALPHA;
public:
  LineSearch();
  virtual ~LineSearch();
  double f;           // best function value found in linesearch.
  vector x;           // best point found in linesearch.
  //int fail;
  double getBestF();
  vector getBestX();
  virtual double getAlpha() = 0;
  virtual int computeConditionFunction() = 0;
};


#endif
