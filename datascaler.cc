#include "datascaler.h"

dataScaler::dataScaler() {
  numVar = -1;
}

void dataScaler::setInitialData(const vector& l, const vector& u) {
  assert(l.dimension() > 0);
  assert(l.dimension() == u.dimension());
  numVar = l.dimension();
  lbd = l;
  ubd = u;
}

dataScaler::~dataScaler() {
}

double dataScaler::scaleResult(double y, int id, const vector& v1) {
  return y;
}

vector dataScaler::scaleX(const vector& v1) {
  assert(numVar > 0);
  assert(numVar == v1.dimension());
  int i;
  vector vec(numVar);
  for (i = 0; i < numVar; i++)
    vec[i] = scale(v1[i], i);
  return vec;
}

double dataScaler::scale(double p, int i) {
  assert(numVar > 0);
  assert(i > 0 && i <= numVar);
  return(p - ((ubd[i] + lbd[i]) * 0.5)) / ((ubd[i] - lbd[i]) * 0.5);
}

vector dataScaler::unscaleX(const vector& v1) {
  assert(numVar > 0);
  assert(v1.dimension() == numVar);
  int i;
  vector vec(numVar);
  for (i = 0; i < numVar; i++)
    vec[i]= unscale(v1[i], i);
  return vec;
}

double dataScaler::unscale(double p, int i) {
  assert(numVar > 0);
  assert(i > 0 && i <= numVar);
  return(p * (ubd[i] - lbd[i]) * 0.5 + (lbd[i] + ubd[i]) * 0.5);
}

void dataScaler::setPenalty(int numIndexes) {
  vector temp(numIndexes);
  penalty = temp;
  penalty.setValue(0.0);
}

vector dataScaler::getLower() {
  return lbd;
}

vector dataScaler::getUpper() {
  return ubd;
}

int dataScaler::getNumberOfVariables() {
  return numVar;
}
