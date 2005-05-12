#include "dataconverter.h"

DataConverter::DataConverter() {
  xind = NULL;
  numVarTotal = -1;
}

void DataConverter::setInitialData(int* xi, const Vector& xr) {
  assert(xr.dimension() > 0);
  int i;
  numVarTotal = xr.dimension();

  if (xind != NULL) {
    delete[] xind;
    xind = NULL;
  }
  xind = new int[numVarTotal];
  for (i = 0; i < numVarTotal; i++)
    xind[i] = xi[i];
  xfullraw = xr;
}

DataConverter::~DataConverter() {
  if (xind != NULL) {
    delete[] xind;
    xind = NULL;
  }
}

Vector DataConverter::convertX(const Vector& v1) {
  assert(numVarTotal > 0);
  int i, j = 0;
  int numVar = v1.dimension();
  Vector vec(numVarTotal);
  for (i = 0; i < numVarTotal; i++) {
    if (xind[i] == 1) {
      if (j >= numVar) {
        cerr << "Error in dataconverter - trying to access invalid parameter\n";
        exit(EXIT_FAILURE);
      }
      vec[i] = v1[j];
      j++;
    } else
      vec[i] = xfullraw[i];
  }
  return vec;
}

int DataConverter::getNumVarTotal() {
  return numVarTotal;
}

Vector DataConverter::getXFull() {
  return xfullraw;
}

Vector DataConverter::getOptInfo() {
  int i;
  Vector vec(numVarTotal);
  if (numVarTotal > 0) {
    for (i = 0; i < numVarTotal; i++)
      vec[i] = xind[i];

  }
  return vec;
}
