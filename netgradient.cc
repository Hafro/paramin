#include "netgradient.h"

gradient::~gradient() {
}

NetGradient::NetGradient(int numVar) {
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

NetGradient::~NetGradient() {
}

void NetGradient::initializeDiagonalHessian() {
  int i;
  for (i = 0; i < numberOfVariables; i++)
    diagHess[i]= -1.0;
}

void NetGradient::setXVectors(const vector& x, NetInterface* netInt) {
  int i;
  double deltai;
  int numberOfx = 0;

  if (difficultgrad == 0)
    numberOfx = numberOfVariables + 1;
  else if (difficultgrad == 1)
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
    deltai = deltavec[i] * (1.0 + ABS(tempVec[i]));
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

int NetGradient::computeGradient(NetInterface* net, const vector& x, int difficultgradient) {

  difficult = 0;
  difficultgrad = difficultgradient;

  int i;
  double fx1, fx2, fx3, fx4;
  int SEND_RECEIVE = 0;
  setXVectors(x, net);
  SEND_RECEIVE = net->sendAndReceiveAllData();
  if (SEND_RECEIVE == -1) {
    cerr << "Error in netgradient - could not send or receive data\n";
    exit(EXIT_FAILURE);
  }
  int numfx = 0;
  double fx = net->getY(numfx);
  numfx++;
  double deltai;
  normgrad = 0.0;

  //Calculate f(x + hi * ei) for all i
  for (i = 0;  i < numberOfVariables; i++) {
    deltai=(1. + ABS(x[i])) * deltavec[i];
    fx1 = net->getY(numfx);
    numfx++;

    if (difficultgrad == 1) {
      fx4 = net->getY(numfx);
      numfx++;
      grad[i] = (fx4 - fx1) / (2 * deltai);
      diagHess[i] = (fx4 - 2 * fx + fx1) / (deltai * deltai);

      //Check for difficultgrad
      if ((fx4 > fx) && (fx1 > fx))
        difficult = 1;

      if (ABS(fx4 - fx1) / fx < ROUNDOFF) {  // may be running into roundoff
        deltavec[i] = MIN(delta0, deltavec[i] * 5.0);
        cout << "Warning in netgradient - possible roundoff errors in gradient\n";
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

      if (ABS(fx4 - fx1) / fx < ROUNDOFF) {  // may be running into roundoff
        deltavec[i] = MIN(delta0, deltavec[i] * 5.0);
        cout << "Warning in netgradient - possible roundoff errors in gradient\n";
      }

    } else {
      grad[i] = (fx - fx1) / deltai;
      if (ABS(fx - fx1) / fx < ROUNDOFF) {  // may be running into roundoff
        deltavec[i] = MIN(delta0, deltavec[i] * 5.0);
        cout << "Warning in netgradient - possible roundoff errors in gradient\n";
      }
    }

    if (grad[i] > BIG) { // seem to be running outside the boundary
      deltavec[i] = MAX(delta1, deltavec[i] / 5.0);
      cout << "Warning in netgradient - possible boundary errors in gradient\n";
    }
    normgrad += grad[i] * grad[i];
  }

  normgrad = sqrt(normgrad);
  fx0 = fx;
  difficultgrad += difficult;

  // finished computing gradient do not need datagroup anymore
  net->stopUsingDataGroup();
  return 1;
}
#ifdef CONDOR
int NetGradient::computeGradientCondor(NetInterface* net, const vector & x, int difficultgradient) {

  difficult = 0;
  difficultgrad = difficultgradient;

  int i;
  double fx1, fx2, fx3, fx4;
  int SEND_RECEIVE = 0;
  // initilize x-s for gradient compuation and store them in gradData
  setXVectors(x, net);
  // send/receive all x/f(x)s in datagroup to/from slave processes
  SEND_RECEIVE = net->sendAndReceiveAllDataCondor();
  if (SEND_RECEIVE == -1) {
    cerr << "Error in netgradient - could not send or receive data\n";
    exit(EXIT_FAILURE);
  }
  int numfx = 0;
  double fx = net->getY(numfx);
  numfx++;
  double deltai;
  normgrad = 0.0;

  /* Phase 3 - get back f(x+hi*ei) for all i */
  for (i = 0;  i < numberOfVariables; i++) {
    deltai = (1.0 + ABS(x[i])) * deltavec[i];
    fx1 = net->getY(numfx);
    numfx++;
    if (difficultgrad == 1) {
      fx4 = net->getY(numfx);
      numfx++;

      //calculate gradient.  Using parabolic approximation.
      grad[i] = (fx4 - fx1)/(2 * deltai);
      diagHess[i] = (fx4 - 2 * fx + fx1) / (deltai * deltai);

      //Check for difficultgrad
      if ((fx4 > fx) && (fx1 > fx))
        difficult = 1;

      if (ABS(fx4 - fx1) / fx < ROUNDOFF) { /* may be running into roundoff */
        deltavec[i] = MIN(delta0, deltavec[i] * 5.0);
        cout << "Warning in netgradient - possible roundoff errors in gradient\n";
      }

    } else if (difficultgrad >= 2) {
      fx2 = net->getY(numfx);
      numfx++;
      fx3 = net->getY(numfx);
      numfx++;
      fx4 = net->getY(numfx);
      numfx++;

      //calculate gradient. Using polynomial of degree 4.
      grad[i] = (fx1 - fx4 + 8 * fx3 - 8 * fx2)/(6 * deltai);
      diagHess[i] = (fx4 - 2 * fx + fx1) / (deltai * deltai);

      if (ABS(fx4 - fx1) / fx < ROUNDOFF) { /* may be running into roundoff */
        deltavec[i] = MIN(delta0, deltavec[i] * 5.0);
        cout << "Warning in netgradient - possible roundoff errors in gradient\n";
      }

    } else {
      grad[i] = (fx - fx1) / deltai;
      if (ABS(fx - fx1) / fx < ROUNDOFF) { /* may be running into roundoff */
        deltavec[i] = MIN(delta0, deltavec[i] * 5.0);
        cout << "Warning in netgradient - possible roundoff errors in gradient\n";
      }
    }

    if (grad[i] > BIG) {
      // seem to be running outside the boundary or some such
      deltavec[i] = MAX(delta1, deltavec[i] / 5.0);
      cout << "Warning in netgradient - possible boundary errors in gradient\n";
    }
    normgrad += grad[i] * grad[i];
  }

  normgrad=sqrt(normgrad);
  fx0 = fx;
  difficultgrad += difficult;

  // finished computing gradient do not need datagroup anymore
  net->stopUsingDataGroup();
  return 1;
}
#endif

double NetGradient::getBaseFX() {
  return fx0;
}

vector NetGradient::getDiagonalHessian() {
  return diagHess;
}

double NetGradient::getNormGrad() {
  return normgrad;
}

vector NetGradient::getGradient() {
  return grad;
}

int NetGradient::getDifficultGrad() {
  return difficultgrad;
}
