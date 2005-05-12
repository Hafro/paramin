#ifndef datascaler_h
#define datascaler_h

#include "paramin.h"
#include "mathfunc.h"
#include "vector.h"

/**
 * \class DataScaler
 * \brief Class DataScaler can be used to scale/unscale x[0..numVar-1] where x is a vector of numVar parameters. Vectors are scaled using the formula: 
 * x[i].scaled  = (x[i] - ((ubd[i] + lbd[i])/2))/((ubd[i]-lbd[i])/2); 
 * And unscaled using the formula:
 *  x[i].unscaled =  (x[i]*(ubd[i]-lbd[i])/2.0 + (lbd[i] + ubd[i])/2.0);
 * Upper bound (ubd) and lower bound (lbd) must have same number of parameters as vector to be scaled/unscaled. DataScaler can also be used to scale variable y using the formula:
 *  for all parameters in vector x:
 *   outside = (x[i]<0.) ?(-x[i]): ((x[i]>1.) ? (x[i]-1.):(0.)) ;
 *   penalty[id] = MAX(outside,penalty[id]);
 *   y *= (1. + sqrt(penalty[id]));
 *  return p;
 * where outside is based on a given vector and penalty is a vector[0...maxNum-1] which identifies the datapair (x[i..numVar-1], y)
 *
 * Lower and upperbound must be set before trying to scale/unscale vectors and must have number of variables > 0. and penalty must have been set before trying to scale variable y, and must have length > 0. DataScaler also has functions to get information about number of variables in vector to be scaled and setting/getting lower/upper bound and penalty. 
*/

class DataScaler {
private:
  int numVar;
  Vector lbd;
  Vector ubd;
  Vector penalty;
public:
  DataScaler();
  ~DataScaler();
  void setInitialData(const Vector& l, const Vector& u);
  double scaleResult(double y, int id, const Vector& v1);
  double scale(double p, int i);
  Vector scaleX(const Vector& v1);
  double unscale(double p, int i);
  Vector unscaleX(const Vector& v1);
  void setPenalty(int numIndexes);
  Vector getLower();
  Vector getUpper();
  int getNumVariables();
};

#endif
