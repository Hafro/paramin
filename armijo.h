#ifndef armijo_h
#define armijo_h

#include "linesearch.h"


class Armijo : public LineSearch {
private:
  vector inixvec;     // Stores initial alpha at beginning of linesearch computation
  vector initialx;    // Linesearch done from x
  double initialf;    // The function value at x.
  double df;          // The derivative of g(alpha)=f(x + alpha * h).
  vector hvec;
  double s;
  double sigma;
  NetInterface* net;
  Condition* cond;
  int numberOfVariables;
  double alpha;
  int power;
  double beta;
public:
  Armijo();
  virtual ~Armijo();
  double getAlpha();
  int conditionSatisfied(double y);
  void doLinesearch();
  void doArmijo(const vector& v1, double fx, double dery, const vector& h, NetInterface* netI, double s);
  void prepareNewLineSearch();
  void initiateAlphas();
  int setData();
  void setSigma(double s);
  void setBeta(double b);
  double getBeta();
  int getPower();
  int outstandingRequests();
  int computeConditionFunction();
};

#endif
