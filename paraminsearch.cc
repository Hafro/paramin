#include "paraminsearch.h"

// ********************************************************
// functions for base class ParaminSearch
// ********************************************************
ParaminSearch::ParaminSearch(NetInterface* netInt) {
  net = netInt;
  lowerbound = net->getLowerScaleConstant();
  upperbound = net->getUpperScaleConstant();
  numvar = net->getNumVarsInDataGroup();
  maxiterations = 2000;
}

ParaminSearch::~ParaminSearch() {
}

const vector& ParaminSearch::getBestX() {
  return net->unscaleX(bestx);
}

double ParaminSearch::getBestF() {
  return bestf;
}

void ParaminSearch::randomOrder(int* vec) {
  int i, j, k, change;
  i = j = k = change = 0;
  while (change < numvar) {
    j = rand() % numvar;
    k = 1;
    for (i = 0; i < change; i++)
      if (vec[i] == j)
        k = 0;

    if (k) {
      vec[change] = j;
      change++;
    }
  }
}

double ParaminSearch::expRep(double n) {
  double exprep = 0.0;
  if (n > verysmall)
    exprep = 1.0;
  else if (n < -25.0)
    exprep = rathersmall;
  else
    exprep = exp(n);
  return exprep;
}

double ParaminSearch::randomNumber() {
  int r = rand();
  double k = r % 32767;
  return k / 32767.0;
}
