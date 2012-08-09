#include "datascaler.h"

DataScaler::DataScaler() {
  numVar = -1;
}

void DataScaler::setInitialData(const DoubleVector& l, const DoubleVector& u) {
  //assert(l.Size() > 0);
  //assert(l.Size() == u.Size());
  numVar = l.Size();
  lbd = l;
  ubd = u;
}

DataScaler::~DataScaler() {
}

double DataScaler::scaleResult(double y, int id, const DoubleVector& v1) {
  return y;
}

const DoubleVector& DataScaler::scaleX(const DoubleVector& v1) {
  assert(numVar > 0);
  assert(numVar == v1.Size());
  int i;
  x_vec.Reset();
  x_vec.resize(numVar, 0.0);
  for (i = 0; i < numVar; i++)
    x_vec[i] = scale(v1[i], i);
  return x_vec;
}

double DataScaler::scale(double p, int i) {
  //assert(numVar > 0);
  //assert(i > 0 && i <= numVar);
  return (p - ((ubd[i] + lbd[i]) * 0.5)) / ((ubd[i] - lbd[i]) * 0.5);
}

const DoubleVector& DataScaler::unscaleX(const DoubleVector& v1) {
  //assert(numVar > 0);
  //assert(v1.Size() == numVar);
  int i;
  x_vec.Reset();
  x_vec.resize(numVar, 0.0);
  for (i = 0; i < numVar; i++)
    x_vec[i]= unscale(v1[i], i);
  return x_vec;
}

double DataScaler::unscale(double p, int i) {
  //assert(numVar > 0);
  //assert(i > 0 && i <= numVar);
  return (p * (ubd[i] - lbd[i]) * 0.5 + (lbd[i] + ubd[i]) * 0.5);
}

void DataScaler::setPenalty(int numIndexes) {
  penalty.resize(numVar, 0.0);
}

const DoubleVector& DataScaler::getLower() {
  return lbd;
}

const DoubleVector& DataScaler::getUpper() {
  return ubd;
}

int DataScaler::getNumVariables() {
  return numVar;
}
