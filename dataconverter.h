#ifndef dataconverter_h
#define dataconverter_h

#include "paramin.h"
#include "intvector.h"
#include "doublevector.h"

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
  DoubleVector xfullraw;
  DoubleVector xConverted;
  IntVector xind;
  int numVarTotal;
public:
  DataConverter();
  ~DataConverter();
  void setInitialData(const IntVector& xi, const DoubleVector& xr);
  const DoubleVector& getXFull();
  int getNumVarTotal();
  const DoubleVector& convertX(const DoubleVector& v1);
  const IntVector& getOptInfo();
};

#endif
