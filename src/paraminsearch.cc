#include "paraminsearch.h"

// ********************************************************
// functions for base class ParaminSearch
// ********************************************************
ParaminSearch::ParaminSearch(NetInterface* netInt) {
    // Must make sure the optinfo is called and constructor initialized..
  net = netInt;
  lowerbound = net->getLowerScaleConstant();
  upperbound = net->getUpperScaleConstant();
  numvar = net->getNumVarsInDataGroup();
  maxiterations = 2000;
}

ParaminSearch::~ParaminSearch() {
}

const DoubleVector& ParaminSearch::getBestX() {
  return net->unscaleX(bestx);
}

double ParaminSearch::getBestF() {
  return bestf;
}

void ParaminSearch::randomOrder(IntVector& vec) {
  int i, j, k, change;
  /*
    i = j = k = change = 0;
  if (vec.Size() != numvar)
      // error
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
  */
}

double ParaminSearch::expRep(double x) {
  double exprep = 0.0;
  if (x > verysmall)
    exprep = 1.0;
  else if (x < -25.0)
    exprep = rathersmall;
  else
    exprep = exp(x);
  return exprep;
}

double ParaminSearch::randomNumber() {
  int r = rand();
  double k = r % 32767;
  return k / 32767.0;

    // return 0.4;
}
void ParaminSearch::printX(const DoubleVector& vec) {
    int i;
    for (i = 0; i < vec.Size(); i++)
	cout << vec[i] << sep;
    cout << endl;
}
void ParaminSearch::printX(ofstream& output, const DoubleVector& vec) {
   int i;
    for (i = 0; i < vec.Size(); i++)
	output << vec[i] << sep;
    output << endl;   
}
