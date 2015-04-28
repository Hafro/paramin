#include "lineseeker.h"

// ********************************************************
// Functions for class lineseeker
// ********************************************************
LineSeeker::LineSeeker() {
}

LineSeeker::~LineSeeker() {
}

int LineSeeker::outstandingRequests() {
  int pending = net->getNumNotAns();
  return pending;
}

/* In the initial version of Hooke and Jeeves, the point xnew+d is      */
/* checked after each run of bestNearby, where d=xnew-xold. That is,    */
/* it's tried to go a bit further in the direction that improves the    */
/* function value.  Here, some more points on the line through xnew and */
/* xold are tried as most of the computers would just be waiting during */
/* this time anyway. These tries depend on the number of free computers */
/* Nr. of computers:     Tries made:                                    */
/* 1.....................xnew+d                                         */
/* 2.....................xnew+d, xnew+2*d                               */
/* 3.....................xnew+d, xnew+3/2*d, xnew+2*d                   */
/* 4.....................xnew+1/2*d, xnew+d, xnew+3/2*d, xnew+2*d       */
/* 5.....................xnew-1/2*d, xnew+1/2*d, xnew+d, xnew+3/2*d,    */
/*                       xnew+2*d                                       */
/* 6.....................xnew-1/2*d, xnew+1/2*d, xnew+d, xnew+3/2*d,    */
/*                       xnew+2*d, xnew+4*d                             */
/* 7.....................xnew-1/2*d, xnew+1/2*d, xnew+d, xnew+3/2*d,    */
/*                       xnew+2*d, xnew+4*d, xnew+8*d                   */
/* and so further. (THLTH 29.08.01)                                     */
int LineSeeker::doLineseek(const DoubleVector& xold, const DoubleVector& xnew,
  double fnew, NetInterface *netI) {

  int i, j, k, l;
  int numSendReceive, numDataItems;

  net = netI;
  int numvar = net->getNumVarsInDataGroup();
  int numberOfHosts = net->getNumFreeProcesses();
  if (numberOfHosts == 0)
    numberOfHosts = 1;

  DoubleVector d(numvar);
  DoubleVector z(numvar);
  for (i = 0; i < numvar; i++)
      d[i] = xnew[i] - xold[i];
  DoubleVector upper(netI->getUpperScaleConstant());
  DoubleVector lower(netI->getLowerScaleConstant());
  f = fnew;
  x = xnew;

  net->startNewDataGroup(numberOfHosts);
  if (numberOfHosts < 5) {
    l = 1;
    for (i = 0; i < numvar; i++) {
      z[i] = xnew[i] + d[i];
      if ((z[i] < lower[i]) || (z[i] > upper[i]))
        l = 0;
    }
    if (l)
      net->setX(z);

    for (j = 1; j < numberOfHosts; j++) {
      for (i = 0; i < numvar; i++) {
        z[i] = xnew[i] + (2 - 1 / 2 * (j - 1) - 1 / 4 * (j - 1) * (j - 2)) * d[i];
        if ((z[i] < lower[i]) || (z[i] > upper[i]))
          l = 0;
      }
      if (l)
        net->setX(z);
      else
        break;
    }

  } else {
    l = 1;
    for (i = 0; i < numvar; i++) {
      z[i] = xnew[i] - 1 / 2 * d[i];
      if ((z[i] < lower[i]) || (z[i] > upper[i]))
        l = 0;
    }
    if (l)
      net->setX(z);

    for (j = 1; j < 4; j++) {
      for (i = 0; i < numvar; i++) {
        z[i] = xnew[i] + 1 / 2 * j * d[i];
        if ((z[i] < lower[i]) || (z[i] > upper[i]))
          l = 0;
      }
      if (l)
        net->setX(z);
      else
        break;
    }

    k = 2;
    for (j = 4; j < numberOfHosts; j++) {
      for (i = 0; i < numvar; i++) {
        z[i] = xnew[i] + k * d[i];
        if ((z[i] < lower[i]) || (z[i] > upper[i]))
          l = 0;
      }
      if (l)
        net->setX(z);
      else
        break;
      k = 2 * k;
    }
  }

  numSendReceive = net->sendAndReceiveAllData();
  if (numSendReceive == net->netError()) {
    cerr << "Error in lineseeker - could not receive data\n";
    net->stopUsingDataGroup();
    exit(EXIT_FAILURE);

  } else if (numSendReceive == net->netSuccess()) {
    numDataItems = net->getNumDataItemsAnswered();
    for (i = 0; i < numDataItems; i++) {
      if (net->getY(i) < f) {
        f = net->getY(i);
        x = net->getX(i);
      }
    }
    net->stopUsingDataGroup();

  } else {
    cerr << "Error in lineseeker - could not receive data\n";
    net->stopNetComm();
    net->stopUsingDataGroup();
    exit(EXIT_FAILURE);
  }
  return numDataItems;
}
