#include "dataconverter.h"

dataConverter::dataConverter() {
  xind = NULL;
  numVarTotal = -1;
}

void dataConverter::setInitialData(int* xi, const vector& xr) {
  assert(xr.dimension() > 0);
  int i;
  numVarTotal = xr.dimension();

  if (xind != NULL) {
    delete [] xind;
    xind = NULL;
  }
  xind = new int[numVarTotal];
  for (i = 0; i < numVarTotal; i++)
    xind[i] = xi[i];
  xfullraw = xr;
}

dataConverter::~dataConverter() {
  if (xind != NULL) {
    delete [] xind;
    xind = NULL;
  }
}

vector dataConverter::convertX(const vector& v1) {
  assert(numVarTotal > 0);
  int i, j = 0;
  int numVar = v1.dimension();
  vector vec(numVarTotal);
  for (i = 0; i < numVarTotal; i++) {
    if (xind[i] == 1) {
      if (j >= numVar) {
        cerr << "Error in dataConverter - trying to access v1[" << j
          << "], but v1 only has " << numVar << " variables\n";
        exit(EXIT_FAILURE);
      }
      vec[i] = v1[j];
      j++;
    } else
      vec[i] = xfullraw[i];
  }
  return vec;
}

int dataConverter::getNumVarTotal() {
  return numVarTotal;
}

vector dataConverter::getXFull() {
  return xfullraw;
}

vector dataConverter::getOptInfo() {
  int i;
  vector vec(numVarTotal);
  if (numVarTotal > 0) {
    for (i = 0; i < numVarTotal; i++)
      vec[i] = xind[i];

  }
  return vec;
}
