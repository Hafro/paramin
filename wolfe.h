#ifndef wolfe_h
#define wolfe_h

#include "linesearch.h"

class Wolfe : public LineSearch {
private:
  /**
   * \brief alpha found as a result of latest linesearch,
   */
  double resultAlpha;    
  /**
   * \brief number of funtions values returned per one linesearch
   */
  int newreturns;        
  /**
   * \brief number of requests made per one linesearch round
   */
  int askedfor;          
  /**
   * \brief wolfecond = 1 if wolfe condition is satisfied else 0
   */
  int wolfecond;         
  /**
   * \brief to be xvalues surrounding central x
   */
  double xl, xc, xu;     
  /**
   * \brief and corresponding y values
   */
  double yl, yc, yu;     
  /**
   * \brief (alpha,f(x+h*alhpa) which satisfy wolfe condition,
   */
  double xwolfe, ywolfe; 
  /**
   * \brief accuracy requirement in max(1, abs(x))
   */
  double acc;            
  /**
   * \brief used for stepwise linear searches
   */
  double delta;          
  /**
   * \brief fraction demanded back after query round
   */
  double newretprop;     
  /**
   * \brief used for Wolfe's criterion
   */
  double sigma, rho;     
  /**
   * \brief stores previous center from last linesearch
   */
  double oldxc;          
  /**
   * \brief stores initial alpha-s at beginning of linesearch
   */
  vector inixvec;        
  /**
   * \brief function value at 0
   */
  double y0;             
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
