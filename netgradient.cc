#include "netgradient.h"

gradient::~gradient() {
}

netGradient::netGradient(int numVar) {
  difficultgrad = 1;
  delta0 = 0.01;
  delta1 = 0.0001;
  delta2 = 0.00001;
  delta = 0.0001;
  numberOfVariables = numVar;
  normgrad = -1.0;
  vector tempVec(numVar);
  grad = tempVec;
  diagHess = tempVec;
  deltavec = tempVec;
  deltavec.setValue(delta);
}

netGradient::~netGradient() {
}

void netGradient::initializeDiaghess() {
  int i;
  for(i = 0; i < numberOfVariables; i++){
    diagHess[i]= -1.0;
  }
}

void netGradient::setXVectors(const vector& x, netInterface* netInt) {
  int i;
  double deltai;
  int numberOfx = 0;

  if (difficultgrad == 0)
    numberOfx = numberOfVariables + 1;
  else if(difficultgrad == 1)
    numberOfx = numberOfVariables * 2 + 1;
  else if (difficultgrad >= 2)
    numberOfx = numberOfVariables * 4 + 1;

  // initialize new datagroup for gradient computing and set current point
  netInt->startNewDataGroup(numberOfx);
  netInt->setX(x);

  // compute x + hi * ei for all i
  vector tempVec;
  tempVec = x;
  for (i = 0; i < numberOfVariables; i++) {
    deltai = deltavec[i] * (1.0 + absolute(tempVec[i]));
    tempVec[i] -= deltai;
    netInt->setX(tempVec);
    if (difficultgrad == 1) {
      tempVec[i] += 2 * deltai;
      netInt->setX(tempVec);
      tempVec[i] -= deltai;
    } else if (difficultgrad >= 2) {
      tempVec[i] += 0.5 * deltai;
      netInt->setX(tempVec);
      tempVec[i] += deltai;
      netInt->setX(tempVec);
      tempVec[i] += 0.5 * deltai;
      netInt->setX(tempVec);
      tempVec[i] -= deltai;
    }
  }
}

int netGradient::computeGradient(netInterface* net, const vector& x, int difficultgradient) {

  difficult = 0;
  difficultgrad = difficultgradient;

  int i;
  double fx1, fx2, fx3, fx4;
  int SEND_RECEIVE = 0;
  setXVectors(x, net);
  SEND_RECEIVE = net->send_receiveAllData();
  if (SEND_RECEIVE == -1) {
    cout << "Error in netgradient - Could not send or receive data\n";
    exit(EXIT_FAILURE);
  }

  int numfx = 0;
  double fx = net->getY(numfx);
  numfx++;
  double deltai;
  normgrad = 0.0;

  //Calculate f(x + hi * ei) for all i
  for (i = 0;  i < numberOfVariables; i++) {
    deltai=(1. + absolute(x[i])) * deltavec[i];
    fx1 = net->getY(numfx);
    numfx++;

    if (difficultgrad == 1) {
      fx4 = net->getY(numfx);
      numfx++;
      grad[i] = (fx4 - fx1)/(2*deltai);
      diagHess[i] = (fx4 - 2*fx + fx1)/(deltai*deltai);

      //Check for difficultgrad
      if ((fx4 > fx) && (fx1 > fx))
        difficult = 1;

      if (absolute(fx4 - fx1) / fx < ROUNDOFF) {  // may be running into roundoff
        deltavec[i] = min(delta0, deltavec[i] * 5.0);
        cout << "Warning - roundoff errors in gradient\n";
      }

    } else if (difficultgrad >= 2) {
      fx2 = net->getY(numfx);
      numfx++;
      fx3 = net->getY(numfx);
      numfx++;
      fx4 = net->getY(numfx);
      numfx++;
      grad[i] = (fx1 - fx4 + 8 * fx3 - 8 * fx2) / (6 * deltai);
      diagHess[i] = (fx4 - 2 * fx + fx1) / (deltai * deltai);

      if (absolute(fx4 - fx1) / fx < ROUNDOFF) {  // may be running into roundoff
        deltavec[i] = min(delta0, deltavec[i] * 5.0);
        cout << "Warning - roundoff errors in gradient\n";
      }

    } else {
      grad[i] = (fx - fx1) / deltai;
      if (absolute(fx - fx1) / fx < ROUNDOFF) {  // may be running into roundoff
        deltavec[i] = min(delta0, deltavec[i] * 5.0);
        cout << "Warning - roundoff errors in gradient\n";
      }
    }

    if (grad[i] > BIG) { // seem to be running outside the boundary
      deltavec[i] = max(delta1, deltavec[i] / 5.0);
      cout << "Warning - roundoff errors in gradient\n";
    }
    normgrad += grad[i] * grad[i];
  }

  normgrad = sqrt(normgrad);
  fx0 = fx;
  difficultgrad += difficult;

  // finished computing gradient do not need datagroup anymore
  net->stopUsingDataGroup();
  return(1);
}

double netGradient::getBaseF_x() {
  return fx0;
}

// If symgrad == 0, this returns the initialization, that is crap.
vector netGradient::getDiagonalHess() {
  return diagHess;
}

double netGradient::getNormGrad() {
  return normgrad;
}

vector netGradient::getGradient() {
  return grad;
}

int netGradient::getDifficultGrad() {
  return difficultgrad;
}
