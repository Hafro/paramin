#ifndef netgradient_h
#define netgradient_h

#include "netinterface.h"
#include "paramin.h"

/* class gradient is an abstract class containing only
 * pure virtual functions except for the destructor. */

class gradient {
public:
  virtual ~gradient();
  virtual void setXVectors(const vector& x, NetInterface* netInt) = 0;
  virtual int computeGradient(NetInterface* net, const vector& x, int linesprob) = 0;
#ifdef CONDOR
  virtual int computeGradientCondor(NetInterface* net, const vector & x, int linesprob) = 0;
#endif
  virtual vector getDiagonalHessian() = 0;
  virtual double getNormGrad() = 0;
  virtual vector getGradient() = 0;
  virtual int getDifficultGrad() = 0;
  virtual double getBaseFX() = 0;
  virtual void initializeDiagonalHessian() = 0;
};

/* class NetGradient is a derived class of gradient and implements gradient
 * computation which uses net communication to send/receive data. */

class NetGradient : public gradient {
private:
  int numberOfVariables;
  double delta0;           // Upper bound on percentage h in f(x+h).
  double delta1;           // Lower bound on percentage h in f(x+h).
  double delta2;           // AJ - NEW PARAMTER
  double delta;            // Central values of percentage h and h.
  vector deltavec;
  int difficultgrad;       // 0 if linear approx, 1 if symmetric approx,
                           // and >=2 if approx. using four points
  int difficult;           // 1 if values around x are all bigger, 0 else.
  vector diagHess;
  double normgrad;
  vector grad;
  double fx0;              // f(x0) where x0 is the base data vector
public:
  NetGradient(int numVar);
  virtual ~NetGradient();
  void setXVectors(const vector& x, NetInterface* netInt);
  int computeGradient(NetInterface* net, const vector& x, int symgrad);
  int computeGradientCondor(NetInterface* net, const vector & x, int symgrad);
  vector getDiagonalHessian();
  double getNormGrad();
  vector getGradient();
  int getDifficultGrad();
  double getBaseFX();
  void initializeDiagonalHessian();
};

#endif
