#ifndef linesearch_h
#define linesearch_h

#include "paramin.h"
#include "netinterface.h"

class NetInterface;
class Condition;

class LineSearch {
protected:
  int numAlpha;
public:
  LineSearch();
  virtual ~LineSearch();
  /**
   * \brief best function value found in linesearch.
   */
  double f;           
  /**
   * \brief best point found in linesearch.
   */
  Vector x;           
  //int fail;
  double getBestF();
  Vector getBestX();
  virtual double getAlpha() = 0;
  virtual int computeConditionFunction() = 0;
};


#endif
