#include "dataconverter.h"

DataConverter::DataConverter() {
    // xind = NULL;
  numVarTotal = -1;
}

void DataConverter::setInitialData(const IntVector& xi, const DoubleVector& xr) {
  assert(xr.Size() > 0);
  xind = xi;
  xfullraw = xr;
  numVarTotal = xr.Size();
  xConverted.resize(numVarTotal, 0.0);
}

DataConverter::~DataConverter() {
}

const DoubleVector& DataConverter::convertX(const DoubleVector& v1) {
  assert(numVarTotal > 0);
  int i, j = 0;
  int numVar = v1.Size();
  // DoubleVector vec(numVarTotal);
  for (i = 0; i < numVarTotal; i++) {
    if (xind[i] == 1) {
      if (j >= numVar) {
        cerr << "Error in dataconverter - trying to access invalid parameter\n";
        exit(EXIT_FAILURE);
      }
      xConverted[i] = v1[j];
      j++;
    } else
      xConverted[i] = xfullraw[i];
  }
  return xConverted;
}

int DataConverter::getNumVarTotal() {
  return numVarTotal;
}

const DoubleVector& DataConverter::getXFull() {
  return xfullraw;
}
// AJ **********
// Changing from doublevector to intvector
const IntVector& DataConverter::getOptInfo() {
    return xind;
    /*
    int i;
  DoubleVector vec(numVarTotal);
  if (numVarTotal > 0) {
    for (i = 0; i < numVarTotal; i++)
      vec[i] = xind[i];

  }
  return vec;
    */
}
