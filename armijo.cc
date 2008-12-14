#include "armijo.h"

// ********************************************************
// Functions for class Armijo
// ********************************************************
Armijo::Armijo() {
  cond = new LineSeekerCondition(this);
  beta = 0.3;
  sigma = 0.01;
  alpha = 0.0;
}

Armijo::~Armijo() {
  delete cond;
}

double Armijo::getAlpha() {
  return alpha;
}

int Armijo::conditionSatisfied(double y) {
  int returnID = net->getReceiveID();
  // Vector temp(1);
  // temp = (net->getX(returnID));
  if ((initialf - y) >= (-sigma * net->getX(returnID)[0] * df))
    return 1;
  else
    return 0;
}

void Armijo::doArmijo(const DoubleVector& v1, double fx, double dery,
  const DoubleVector& h, NetInterface *netI, double s1) {

  int cond_satisfied;
  alpha = 0.0;
  power = -1;
  numVar = netI->getNumVarsInDataGroup();
  x = v1;
  s = s1;
  f = fx;
  initialx = v1;
  initialf = fx;
  hvec = h;
  net = netI;
  df = dery;

  prepareNewLineSearch();
  initiateAlphas();
  cond_satisfied = net->sendAndReceiveSetData(cond);
  //cond_satisfied = net->sendAndReceiveSetDataCondor(cond); //JMB
  if (cond_satisfied == -1) {
    cerr << "Error in linesearch - cannot receive or send data\n";
    net->stopUsingDataGroup();
    exit(EXIT_FAILURE);
  } else if (cond_satisfied == 1) {
    // check this better should be working???
  } else {
    net->stopNetComm();
    exit(EXIT_FAILURE);
  }
  net->stopUsingDataGroup();
}

int Armijo::computeConditionFunction() {
  int returnID, i, cond_satisfied = 0;
  int counter = net->getNumDataItemsSet();
  int newreturns = net->getNumDataItemsAnswered();
  double y;
  // Vector temp;

  returnID = net->getReceiveID();
  if (returnID >= 0) {
      //temp = net->getX(returnID);
    y = net->getY(returnID);
    cond_satisfied = ((conditionSatisfied(y) == 1) && (f > y));
    if (cond_satisfied) {
      cout << "New optimum value f(x) = " << y << " at \n";
      f = y;
      power = returnID - 1;
      alpha = net->getX(returnID)[0];
      x = net->makeVector(net->getX(returnID));
      for (i = 0; i < x.Size() ; i++)
	cout << x[i] << " ";
      cout << endl << endl;
    }
  }

  if (power == -1) {
    if (net->dataGroupFull()) {
      if ((0.8 * counter) <= newreturns) {
        // cannot set more data and have received 8/10 of all data - bailing out
        power = 1;
        alpha = 0.0;
        return 1;

      } else
        return 0;

    } else {
      // dataGroup not full
      i = setData();
      return cond_satisfied;
    }

  } else if (power >= 0) {
    // not setting any more data, have already found optimal value
    if ((0.8 * counter) <= newreturns) {
      // have found optimal value and received 8/10 of all data set
      return 1;
    } else
      return 0;

  } else
    return 1;
}

void Armijo::prepareNewLineSearch() {
  net->startNewDataGroup(numAlpha, x, hvec);
  if (df > 0) {
    cerr << "Error in linesearch - bad derivative\n";
    net->stopUsingDataGroup();
  }
  DoubleVector tempx(1,0.0);
  // tempx[0] = 0.0;
  net->setDataPair(tempx, initialf);
}

void Armijo::initiateAlphas() {
  int i = net->getTotalNumProc();
  int j;
  DoubleVector tempx(1);
  assert(beta > 0.0);
  assert(beta <= 0.5);
  for (j = 0; j < i; j++) {
    tempx[0] = pow(beta, j) * s;
    net->setX(tempx);
  }
}

void Armijo::setSigma(double s) {
  sigma = s;
}

void Armijo::setBeta(double b) {
  beta = b;
}

double Armijo::getBeta() {
  return beta;
}

int Armijo::getPower() {
  return power;
}

int Armijo::outstandingRequests() {
  int out;
  int pending = net->getNumNotAns();
  out = (pending > 0);
  return out;
}

int Armijo::setData() {
  DoubleVector tempx(1);
  int counter = net->getNumDataItemsSet() - 1;
  double a = pow(beta, counter) * s;
  tempx[0] = a;
  int ok = -1;
  if (net->dataGroupFull()) {
    cerr << "Error in armijo - have set too many values\n";
    ok = 0;
  } else {
    net->setXFirstToSend(tempx);
    ok = 1;
  }
  return ok;
}

