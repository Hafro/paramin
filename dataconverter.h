#ifndef dataconverter_h
#define dataconverter_h

#include "paramin.h"
#include "vector.h"

/* class dataConverter makes a new vector from a given vector v[0..numVar-1],
 * using the formula:
 *   xNew[i] = xfullraw[i] if xind[i] = 0 else xNew[i] = v[j].
 *   0 <= i < numVarTotal. 0 <= j < numVar. */

class dataConverter {
private:
  vector xfullraw;  //initial values for vector
  int* xind;
  int numVarTotal;
public:
  dataConverter();
  ~dataConverter();
  void setInitialData(int* xi, const vector& xr);
  vector getXFull();
  int getNumVarTotal();
  vector convertX(const vector& v1);
  vector getOptInfo();
};

#endif
