#ifndef dataconverter_h
#define dataconverter_h

#include "paramin.h"
#include "vector.h"

/**
 *
 * \brief class DataConverter makes a new vector from a given vector v[0..numVar-1], using the formula:
 *   xNew[i] = xfullraw[i] if xind[i] = 0 else xNew[i] = v[j].
 *   0 <= i < numVarTotal. 0 <= j < numVar. 
 */

class DataConverter {
private:
  /**
   * \brief initial values for vector
   */
  Vector xfullraw;  
  int* xind;
  int numVarTotal;
public:
  DataConverter();
  ~DataConverter();
  void setInitialData(int* xi, const Vector& xr);
  Vector getXFull();
  int getNumVarTotal();
  Vector convertX(const Vector& v1);
  Vector getOptInfo();
};

#endif
