#include "netgradient.h"
#include "mathfunc.h"

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

void NetGradient::setXVectors(const vector& x, double f, NetInterface* netInt) {
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
  netInt->setDataPair(x, f);
  //netInt->setX(x);

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

int NetGradient::computeGradient(NetInterface* net, const vector& x, double f, int difficultgradient) {

  difficult = 0;
  difficultgrad = difficultgradient;

  int i;
  double fx1, fx2, fx3, fx4;
  int SEND_RECEIVE = 0;
  setXVectors(x, f, net);
  SEND_RECEIVE = net->sendAndReceiveAllData();
  //SEND_RECEIVE = net->sendAndReceiveAllDataCondor(); //JMB
  if (SEND_RECEIVE == -1) {
    cerr << "Error in netgradient - could not send or receive data\n";
    exit(EXIT_FAILURE);
  }

  int numfx = 1;
  fx0 = f;
  double deltai;
  normgrad = 0.0;

  //Calculate f(x + hi * ei) for all i
  for (i = 0;  i < numberOfVariables; i++) {
    deltai = (1.0 + absolute(x[i])) * deltavec[i];
    fx1 = net->getY(numfx);
    numfx++;

    if (difficultgrad == 1) {
      fx4 = net->getY(numfx);
      numfx++;
      grad[i] = (fx4 - fx1) / (2.0 * deltai);
      diagHess[i] = (fx4 - 2.0 * fx0 + fx1) / (deltai * deltai);

      //Check for difficultgrad
      if ((fx4 > fx0) && (fx1 > fx0))
        difficult = 1;

      if (absolute(fx4 - fx1) / fx0 < rathersmall) {  // may be running into roundoff
        deltavec[i] = min(delta0, deltavec[i] * 5.0);
        cerr << "Warning in netgradient - possible roundoff errors in gradient\n";
      }

    } else if (difficultgrad >= 2) {
      fx2 = net->getY(numfx);
      numfx++;
      fx3 = net->getY(numfx);
      numfx++;
      fx4 = net->getY(numfx);
      numfx++;
      grad[i] = (fx1 - fx4 + 8.0 * fx3 - 8.0 * fx2) / (6.0 * deltai);
      diagHess[i] = (fx4 - 2.0 * fx0 + fx1) / (deltai * deltai);

      if (absolute(fx4 - fx1) / fx0 < rathersmall) {  // may be running into roundoff
        deltavec[i] = min(delta0, deltavec[i] * 5.0);
        cerr << "Warning in netgradient - possible roundoff errors in gradient\n";
      }

    } else {
      grad[i] = (fx0 - fx1) / deltai;
      if (absolute(fx0 - fx1) / fx0 < rathersmall) {  // may be running into roundoff
        deltavec[i] = min(delta0, deltavec[i] * 5.0);
        cerr << "Warning in netgradient - possible roundoff errors in gradient\n";
      }
    }

    if (grad[i] > verybig) { // seem to be running outside the boundary
      deltavec[i] = max(delta1, deltavec[i] / 5.0);
      cerr << "Warning in netgradient - possible boundary errors in gradient\n";
    }
    normgrad += grad[i] * grad[i];
  }

  normgrad = sqrt(normgrad);
  difficultgrad += difficult;

  // finished computing gradient do not need datagroup anymore
  net->stopUsingDataGroup();
  return 1;
}

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
