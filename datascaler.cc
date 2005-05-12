#include "datascaler.h"

DataScaler::DataScaler() {
  numVar = -1;
}

void DataScaler::setInitialData(const Vector& l, const Vector& u) {
  assert(l.dimension() > 0);
  assert(l.dimension() == u.dimension());
  numVar = l.dimension();
  lbd = l;
  ubd = u;
}

DataScaler::~DataScaler() {
}

double DataScaler::scaleResult(double y, int id, const Vector& v1) {
  return y;
}

Vector DataScaler::scaleX(const Vector& v1) {
  assert(numVar > 0);
  assert(numVar == v1.dimension());
  int i;
  Vector vec(numVar);
  for (i = 0; i < numVar; i++)
    vec[i] = scale(v1[i], i);
  return vec;
}

double DataScaler::scale(double p, int i) {
  assert(numVar > 0);
  assert(i > 0 && i <= numVar);
  return (p - ((ubd[i] + lbd[i]) * 0.5)) / ((ubd[i] - lbd[i]) * 0.5);
}

Vector DataScaler::unscaleX(const Vector& v1) {
  assert(numVar > 0);
  assert(v1.dimension() == numVar);
  int i;
  Vector vec(numVar);
  for (i = 0; i < numVar; i++)
    vec[i]= unscale(v1[i], i);
  return vec;
}

double DataScaler::unscale(double p, int i) {
  assert(numVar > 0);
  assert(i > 0 && i <= numVar);
  return (p * (ubd[i] - lbd[i]) * 0.5 + (lbd[i] + ubd[i]) * 0.5);
}

void DataScaler::setPenalty(int numIndexes) {
  Vector temp(numIndexes);
  penalty = temp;
  penalty.setValue(0.0);
}

Vector DataScaler::getLower() {
  return lbd;
}

Vector DataScaler::getUpper() {
  return ubd;
}

int DataScaler::getNumVariables() {
  return numVar;
}
