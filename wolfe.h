#ifndef wolfe_h
#define wolfe_h

#include "linesearch.h"

class Wolfe : public LineSearch {
private:
  double resultAlpha;    // alpha found as a result of latest linesearch,
  int newreturns;        // number of funtions values returned per one linesearch
  int askedfor;          // number of requests made per one linesearch round
  int wolfecond;         // wolfecond = 1 if wolfe condition is satisfied else 0
  double xl, xc, xu;     // to be xvalues surrounding central x
  double yl, yc, yu;     // and corresponding y values
  double xwolfe, ywolfe; // (alpha,f(x+h*alhpa) which satisfy wolfe condition,
  double acc;            // accuracy requirement in max(1, abs(x))
  double delta;          // used for stepwise linear searches
  double newretprop;     // fraction demanded back after query round
  double sigma, rho;     // used for Wolfe's criterion
  double oldxc;          // stores previous center from last linesearch
  vector inixvec;        // stores initial alpha-s at beginning of linesearch
  double y0;             // function value at 0
  int numberOfProcesses;
  double dy0;
  NetInterface* net;
  Condition* con;
public:
  Wolfe();
  virtual ~Wolfe();
  void doWolfe(const vector& x, double y, double dery,
    const vector& invH, NetInterface* netInt);
  void prepareNewLineSearch(const vector& v1, double fx, double dery);
  void seekNewLimits(int i);
  void initiateAlphas();
  void increaseLowerbound();
  void seekUpperbound(int itnum);
  void seekNewCenter();
  void findRandomAlpha();
  void interpolate();
  void finishSweep();
  void checkBoundary();
  void computeWolfe();
  int computeConditionFunction();
  void updateCenterAndBounds();
  void setNewLinesearchResults(double fx);
  int linesearchCondSatisfied();
  int wolfeCondFalse();
  double getLastAnsweredAlpha();
  double getAlpha();
  int tabu(double xtmp);
  int wolf(double xtmp, double ytmp);
  double maximum();
  void swapStuff(double x, double y);
  int outstandingRequests();
};

#endif
