#ifndef netgradient_h
#define netgradient_h

#include "netinterface.h"
#include "paramin.h"

/* class gradient is an abstract class containing only
 * pure virtual functions except for the destructor. */

class gradient {
public:
  virtual ~gradient();
  virtual void setXVectors(const vector& x, netInterface* netInt) = 0;
  virtual int computeGradient(netInterface* net, const vector& x, int linesprob) = 0;
  virtual vector getDiagonalHess() = 0;
  virtual double getNormGrad() = 0;
  virtual vector getGradient() = 0;
  virtual int getDifficultGrad() = 0;
  virtual double getBaseF_x() = 0;
  virtual void initializeDiaghess() = 0;
};

/* class netGradient is a derived class of gradient and implements gradient
 * computation which uses net communication to send/receive data. */

class netGradient : public gradient {
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
  netGradient(int numVar);
  virtual ~netGradient();
  void setXVectors(const vector& x, netInterface* netInt);
  int computeGradient(netInterface* net, const vector& x, int symgrad);
  vector getDiagonalHess();
  double getNormGrad();
  vector getGradient();
  int getDifficultGrad();
  double getBaseF_x();
  void initializeDiaghess();
};

#endif
