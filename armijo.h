#ifndef armijo_h
#define armijo_h

#include "linesearch.h"

/**
 * \class Armijo
 * \brief This class performs a linestearch along a given line without using derivatives. The method works as follows, a sequence of alpha_n = beta^n*s is generated and if f(x)-f(x+alpha_n*s) >= - sigma * alpha_n * grad(f,x)'*s otherwise the method continues
 */
class Armijo : public LineSearch {
private:
  /**
   * \brief Linesearch done from x
   */
  vector initialx;    
  /**
   * \brief The function value at x.
   */
  double initialf;    
  /**
   * \brief The derivative of g(alpha)=f(x + alpha * h).
   */
  double df;          
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
  /**
   * \brief Default constructor
   */
  Armijo();
  /**
   * \brief Default destructor
   */
  virtual ~Armijo();
  /**
   * \brief returns the best alpha
   */
  double getAlpha();
  /**
   * \brief Returns 1 if the Armijo condition is satisfied 0 otherwise
   * \param y is the functionvalue
   */
  int conditionSatisfied(double y);
  /**
   * \brief not implemented
   */
  void doLinesearch();
  /**
   * \brief doArmijo does the Armijo linesearch
   * \param v1 is the inital starting point
   * \param fx is the f-value at v1
   * \param dery is the dot product of the gradient and search direction
   * \param netI 
   * \param s is the starting alpha value
   */
  void doArmijo(const vector& v1, double fx, double dery, const vector& h, NetInterface* netI, double s);
  /**
   * \brief Prepares the linesearch by setting the first f-value as initalf (corresponds to alpha = 0)
   */
  void prepareNewLineSearch();
  /**
   * \brief Starts "number of available hosts" function evaluations 
   */
  void initiateAlphas();
  /**
   * \brief Keeps the nodes busy by sending the next alpha value
   */
  int setData();
  /**
   * \param s is the new value for sigma
   */
  void setSigma(double s);
  /**
   * \param s is the new value for sigma
   */
  void setBeta(double b);
  /**
   * \brief Returns beta 
   */
  double getBeta();
  /**
   * \brief Returns n in alpha = beta^n * s
   */
  int getPower();
  /**
   * \brief Returns the number of function evaluations that has not been finished
   */
  int outstandingRequests();
  /**
   * \brief Computes the conditionfunction and supplies nodes new jobs 
   */
  int computeConditionFunction();
};

#endif
