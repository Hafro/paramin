#ifndef linesearch_h
#define linesearch_h

#include "paramin.h"
#include "netinterface.h"

class NetInterface;
class condition;

class linesearch {
protected:
  int NUMALPHA;
public:
  linesearch();
  virtual ~linesearch();
  double f;           // best function value found in linesearch.
  vector x;           // best point found in linesearch.
  int fail;
  double getBestF();
  vector getBestX();
  virtual double getAlpha() = 0;
  virtual int computeConditionFunction() = 0;
};

class armijo : public linesearch {
private:
  vector inixvec;     // Stores initial alpha at beginning of linesearch computation
  vector x;           // Linesearch done from x
  double fcn;         // The function value at x.
  double df;          // The derivative of g(alpha)=f(x + alpha * h).
  double beta;
  vector  hvec;
  double s;
  double sigma;
  NetInterface* net;
  condition* cond;
  int numberOfVariables;
public:
  struct optimal {
    double value;
    int power;
    double beta;
  };
  optimal opt;
  armijo();
  virtual ~armijo();
  double getAlpha();
  int conditionSatisfied(double y);
  void doLinesearch();
  void doArmijo(const vector& v1, double fx, double dery,
    const vector& h, NetInterface* netI, double s);
  void doArmijoCondor(const vector & v1, double fx, double dery, const vector & h, NetInterface* netI, double s);
  vector getXvector(const vector& x, const vector& inixvec);
  void prepareNewLineSearch();
  void initiateAlphas();
  int setData();
  double getBeta();
  int getPower();
  int outstandingRequests();
  int computeConditionFunction();
};

class wolfe : public linesearch {
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
  condition* con;
public:
  wolfe();
  virtual ~wolfe();
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

class lineseeker : public wolfe {
private:
  NetInterface* net;
public:
  lineseeker();
  virtual ~lineseeker();
  void doLineseek(const vector& xold, const vector& xnew,
    double fnew, NetInterface* netI);
  void doLineseekCondor(const vector & xold, const vector & xnew, double fnew, NetInterface *netI);
  int outstandingRequests();
};

#endif
