#ifndef netgradient_h
#define netgradient_h

#include "netinterface.h"
#include "paramin.h"

/**
 * \class gradient
 * \brief class gradient is an abstract class containing only pure virtual functions except for the destructor. 
 */

class gradient {
public:
  virtual ~gradient();
  virtual void setXVectors(const vector& x, double fx, NetInterface* netInt) = 0;
  virtual int computeGradient(NetInterface* net, const vector& x, double fx, int linesprob) = 0;
  virtual vector getDiagonalHessian() = 0;
  virtual double getNormGrad() = 0;
  virtual vector getGradient() = 0;
  virtual int getDifficultGrad() = 0;
  virtual double getBaseFX() = 0;
  virtual void initializeDiagonalHessian() = 0;
};

/**
 * \class NetGradient
 * \brief class NetGradient is a derived class of gradient and implements gradient computation which uses net communication to send/receive data. 
 */

class NetGradient : public gradient {
private:
  int numberOfVariables;
  /**
   * \brief Upper bound on percentage h in f(x+h).
   */
  double delta0;           
  /**
   * \brief  Lower bound on percentage h in f(x+h)
   */
  double delta1;           
  /**
   * \brief AJ - NEW PARAMTER
   */
  double delta2;           
  /**
   * \brief Central values of percentage h and h.
   */
  double delta;            
  /**
   * \brief 
   */
  vector deltavec;
  /**
   * \brief 0 if linear approx, 1 if symmetric approx, and >=2 if approx. using four points
   */
  int difficultgrad;       
  /**
   * \brief 1 if values around x are all bigger, 0 else.
   */
  int difficult;           
  vector diagHess;
  double normgrad;
  vector grad;
  /**
   * \brief f(x0) where x0 is the base data vector
   */
  double fx0;              
public:
  NetGradient(int numVar);
  virtual ~NetGradient();
  void setXVectors(const vector& x, double fx, NetInterface* netInt);
  int computeGradient(NetInterface* net, const vector& x, double fx, int symgrad);
  vector getDiagonalHessian();
  double getNormGrad();
  vector getGradient();
  int getDifficultGrad();
  double getBaseFX();
  void initializeDiagonalHessian();
};

#endif
