#include "armijo.h"

// ********************************************************
// Functions for class Armijo
// ********************************************************
Armijo::Armijo() {
  cond = new LineSeekerCondition(this);
  vector tempVec(50);
  inixvec = tempVec;
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
  int returnId = net->getReceiveId();
  vector temp(1);
  temp = (net->getX(returnId));
  if ((initialf - y) >= (-sigma * temp[0] * df))
    return 1;
  else
    return 0;
}

void Armijo::doArmijo(const vector& v1, double fx, double dery,
  const vector& h, NetInterface *netI, double s1) {

  int cond_satisfied;
  alpha = 0.0;
  power = -1;
  numberOfVariables = netI->getNumVarsInDataGroup();
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
  int returnId, i, cond_satisfied = 0;
  int counter = net->getNumDataItemsSet();
  int newreturns = net->getNumDataItemsAnswered();
  double y;
  vector temp;

  returnId = net->getReceiveId();
  if (returnId >= 0) {
    temp = net->getX(returnId);
    y = net->getY(returnId);
    cond_satisfied = ((conditionSatisfied(y) == 1) && (f > y));
    if (cond_satisfied) {
      cout << "New optimum value f(x) = " << y << endl;
      f = y;
      power = returnId - 1;
      alpha = temp[0];
      x = net->makeVector(temp);
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
  vector tempx(1);
  tempx[0] = 0.0;
  net->setDataPair(tempx, initialf);
}

void Armijo::initiateAlphas() {
  int i = net->getTotalNumProc();
  int j;
  vector tempx(1);
  assert(beta > 0.0);
  assert(beta <= 0.5);
  for (j = 0; j < i; j++) {
    inixvec[j] = pow(beta, j) * s;
    tempx[0] = inixvec[j];
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
  vector tempx(1);
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

