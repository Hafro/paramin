#include "wolfe.h"

// ********************************************************
// Functions for class wolfe
// ********************************************************
Wolfe::Wolfe() {
  // this has to change..
  con = new LineSeekerCondition(this);
  newreturns = 0;       // number of function values returned
  askedfor = 0;         // number of function values requested
  wolfecond = 0;        // --> =1 when Wolfe condition is satisfied
  acc = LINACC;         // accuracy requirement in max(1, abs(x))
  delta = 0.001;        // used for stepwise linear searches
  newretprop = 0.7;     // fraction demanded back after query
  sigma = 0.9;          // used for Wolfe's criterion
  rho = 0.001;          // used for Wolfe's criterion
  oldxc = 0.000001;     // stores previous center
  vector tempVec(50);
  inixvec = tempVec;
  inixvec[0] = 1.0;
}

Wolfe::~Wolfe() {
  delete con;
}

void Wolfe::doWolfe(const vector& v1, double fx, double dery, const vector& hess, NetInterface* netInt) {
  int i = 0;

  if (v1.dimension() != hess.dimension()) {
    cerr << "Error in linesearch - different number of parameters\n";
    exit(EXIT_FAILURE);
  }
  if (v1.dimension() <= 0) {
    cerr << "Error in linesearch - no parameters\n";
    exit(EXIT_FAILURE);
  }

  vector tempVector(50);
  inixvec = tempVector;
  inixvec[0] = 1.0;
  newreturns = 0;
  askedfor = 0;
  net = netInt;
  net->startNewDataGroup(numAlpha, v1, hess);
  prepareNewLineSearch(v1, fx, dery);

  while (!linesearchCondSatisfied()) {
    seekNewLimits(i);
    wolfecond = net->sendAndReceiveTillCondition(con);
    if (wolfecond == -1) {
      cerr << "Error in linesearch - netcommunication has been halted\n";
      net->stopUsingDataGroup();
      exit(EXIT_FAILURE);
    }
    askedfor = 0;
    newreturns = 0;
    //i++ - JMB why has this been taken out??
  }
  setNewLinesearchResults(fx);
  net->stopUsingDataGroup();
}

int Wolfe::computeConditionFunction() {
  int returnId = net->getReceiveId();
  assert(returnId != -1);
  vector xtmp;
  double ytmp = net->getY(returnId);
  xtmp = net->getX(returnId);
  if (wolfecond == 0 || ytmp < ywolfe) {
    wolfecond = wolf(xtmp[0], ytmp);
    if (wolfecond == 1) {
      xwolfe = xtmp[0];
      ywolfe = ytmp;
    }
  }
  updateCenterAndBounds();
  return wolfecond;
}

double Wolfe::getLastAnsweredAlpha() {
  int returnId = net->getReceiveId();
  assert(returnId != -1);
  vector x;
  x = net->getX(returnId);
  return (x[0]);
}

void Wolfe::prepareNewLineSearch(const vector& v1, double fx, double dery) {
  numberOfProcesses = net->getNumFreeProcesses();

  y0 = fx;
  yl = fx;
  dy0 = dery;
  xwolfe = -1.0;
  ywolfe = -1.0;
  wolfecond = 0;
  x = v1;
  xl = 0.0;
  xc = -1.0;
  xu = -1.0;

  if (dy0 > 0) {
    cerr << "Warning in linesearch - wrong sign of derivative\n";
    resultAlpha = -2.0;
  }

  vector tempx(1);
  tempx[0] = 0.0;
  net->setDataPair(tempx, fx);
  initiateAlphas();
}

int Wolfe::linesearchCondSatisfied() {
  /* The criteria for exiting are crucial - would like to exit when the
   * wolfe condition is satisfied - still OK if get xl>0 and tight bound
   * Must give up and decide on convergence to zero if xu<rathersmall */

  int notFinished;
  notFinished = ((xu < 0 || !(xl > 0 && xu - xl < acc)
    || (xl == 0 && xu > rathersmall)) && wolfecond == 0);
  return !notFinished;
}

void Wolfe::seekNewLimits(int i) {
  increaseLowerbound();
  if (xu < 0)
    seekUpperbound(i);
  else if (xc > 0.0 && yc > yl)
    seekNewCenter();
  else if (xc < 0.0)
    findRandomAlpha();
  else
    interpolate();
  finishSweep();
}

void Wolfe::setNewLinesearchResults(double fx) {

  if (xc > 0) {
    y0 = yc;
    oldxc = xc;
  } else if (xl > 0.0 && (yl < yu || xu < 0)) {
    y0 = yl;
    oldxc = xl;
  } else {
    y0 = yu;
    oldxc = xu;
  }

  if (xl == 0.0 && xu - xl < rathersmall && yu > yl && (yc > yl || xc < 0.0) && wolfecond == 0) {
    xc = -1.0;
    cerr << "Warning in linesearch - algorithm converges to zero\n";
  }

  if (xc < 0 && xwolfe > 0 && xwolfe <= xu && xwolfe >= xl)
    xc = xwolfe;
  else if (xc < 0 && xl > 0 && yl < fx) {
    xc = xl;
    cerr << "Warning in linesearch - no center so selected lower point\n";
  } else if (xc < 0 && xu > 0 && yu < yl) {
    xc = xu;
    cerr << "Warning in linesearch - no center so selected upper point\n";
  }
  resultAlpha = xc;
}

double Wolfe::getAlpha() {
  return resultAlpha;
}

void Wolfe::initiateAlphas() {
  assert(dy0 != 0.0);
  assert(rho != 0.0);
  assert(sigma != 0.0);

  double tmpM = (1.0 + absolute(yl)) / dy0;
  inixvec[1] = -0.00001 * tmpM;
  inixvec[2] = 2.0;
  inixvec[3] = 0.5;
  inixvec[4] = -0.0001 * tmpM;
  inixvec[5] = -0.000000001 * tmpM;
  inixvec[6] = -0.000001 * tmpM;
  inixvec[7] = -0.01 * tmpM;
  inixvec[8] = -0.5 * tmpM;
  inixvec[9] = -tmpM;
  inixvec[10] = -yl / (rho * dy0);
  inixvec[11] = -yl / (sigma * dy0);
  inixvec[12] = 0.0;

  int i;
  vector tempx(1);
  for (i = 0; inixvec[i] > 0.0; i++) {
    if (inixvec[i] > 2.0)
      inixvec[i] = 2.0 * rand() / 32767.0;

    tempx[0] = inixvec[i];
    net->setX(tempx);
    askedfor++;
  }
}

void Wolfe::increaseLowerbound() {
  int maxreq = (int)floor(numberOfProcesses / 8.0);
  assert(numberOfProcesses != 0);
  if (delta > 0.0)
    delta /= numberOfProcesses;

  if (delta > xu - xl && xu > 0 && xl > 0)
    delta = (xu - xl) / numberOfProcesses;

  vector xtmp(1);
  if (xc < 0.0 && xu < 0.0)
    xtmp[0] = oldxc;
  else
    xtmp[0] = xl + delta;

  int i;
  for (i = 0; i < maxreq; i++) {
    while (tabu(xtmp[0]))
      xtmp[0] += acc / 100.0;

    if (xtmp[0] < xu || xu < 0.0) {  // only look within bounds
      net->setX(xtmp);
      askedfor++;
    }
    xtmp[0] += delta;
  }
}

void Wolfe::seekUpperbound(int itnum) {
  vector xtmp(1);
  int i;
  int maxreq = (int)floor(numberOfProcesses / 8.0);
  if (itnum > 0)
    maxreq = numberOfProcesses;

  xtmp[0] = maximum();
  for (i = 0; i < maxreq;i++) {
    xtmp[0] *= 2.0;
    net->setX(xtmp);
    askedfor++;
  }
}

void Wolfe::seekNewCenter() {
  int i;
  vector xtmp(1);
  double tmpdelta;
  tmpdelta= (xc - xl) / 2.0;

  for (i = askedfor + 1; i < numberOfProcesses - 1; i++) {
    if (i == askedfor + 1)
      xtmp[0]= -dy0 * (xc * xc) / (2.0 * (yc - y0 - dy0 * xc));
    else {
      xtmp[0] = xl + tmpdelta;
      tmpdelta /= 2.0;
    }

    while (tabu(xtmp[0]))
      xtmp[0] -= acc / 100.0;

    if (xtmp[0] > xl && xtmp[0] < xu) {
      net->setX(xtmp);
      askedfor++;
    }
  }

  assert(yc > yl && xc > 0);
  xu = xc;
  yu = yc;
  xc = -1.0;
}

void Wolfe::findRandomAlpha() {
  vector xtmp(1);
  xtmp[0] = xl + (xu - xl) * (rand() / 32767.0);
  net->setX(xtmp);
  askedfor++;
}

void Wolfe::interpolate() {
  vector xtmp(1);
  double a, b, c;            /* terms in parabola when fit through pts */

  assert(xc > 0 && xu > 0);
  assert((((xu * xu - xc * xc) * (xc - xl)) - ((xc * xc - xl * xl) * (xu - xc))) != 0);

  a = ((yu - yc) * (xc - xl) - (yc - yl) * (xu - xc)) /
    ((xu * xu - xc * xc) * (xc - xl) - (xc * xc - xl * xl) * (xu - xc));

  if (a < 0) {         /* Abnormal situation - parabola is upside down */
    xtmp[0] = xl + (xu - xl) * (rand() / 32767.);
    net->setX(xtmp);
    askedfor++;

  } else if (a > 0.0) {    /* Normal situation - parabola with minumum */
    b = ((yu - yc) - a * (xu * xu - xc * xc)) / (xu - xc);
    c = yu - a * xu * xu - b * xu;
    xtmp[0] = -b / (2 * a);
    net->setX(xtmp);
    askedfor++;
  }
}

void Wolfe::finishSweep() {
  vector xtmp(1);
  double tmpdelta;
  int i;

  if (xu < 0.0 && xl == 0.0)
    tmpdelta = 2.0 / max(numberOfProcesses - askedfor, 3);
  else if (xu > 0.0)
    tmpdelta = (xu - xl) / max(numberOfProcesses - askedfor, 3);
  else if (xl < 0.001)
    tmpdelta = (0.01 - xl) / max(numberOfProcesses - askedfor, 3);
  else if (xl < 0.01)
    tmpdelta = (0.1 - xl) / max(numberOfProcesses - askedfor, 3);
  else
    tmpdelta = (1.0 - xl) / max(numberOfProcesses - askedfor, 3);

  for (i = askedfor, xtmp[0] = xl + tmpdelta;
      i < numberOfProcesses; xtmp[0] += tmpdelta, i++) {

    while (tabu(xtmp[0]))
      xtmp[0] += acc / 100.0;

    if ((xtmp[0] < xu || xu < 0.0) && xtmp[0] > xl) {
      net->setX(xtmp);
      askedfor++;
    }
  }
}

int Wolfe::outstandingRequests() {
  int pending = net->getNumNotAns();
  return pending;
}

int Wolfe::wolfeCondFalse() {
  return !wolfecond;
}

void Wolfe::checkBoundary() {
  double ytmp;
  vector xtmp;
  int returnId = net->getReceiveId();
  assert(returnId != -1);
  ytmp = net->getY(returnId);
  xtmp = net->getX(returnId);
}

void Wolfe::computeWolfe() {
  int returnId = net->getReceiveId();
  assert(returnId != -1);
  vector xtmp;
  double ytmp = net->getY(returnId);
  xtmp = net->getX(returnId);
  if (wolfecond == 0 || ytmp < ywolfe) {
    wolfecond = wolf(xtmp[0], ytmp);
    if (wolfecond == 1) {
      xwolfe = xtmp[0];
      ywolfe = ytmp;
    }
  }
}

void Wolfe::updateCenterAndBounds() {
  vector xtmp;
  int returnId = net->getReceiveId();
  double ytmp = net->getY(returnId);
  xtmp = net->getX(returnId);
  swapStuff(xtmp[0], ytmp);
}

int Wolfe::tabu(double x) {
  int i;
  int n = net->getNumDataItemsSet();
  vector alpha;

  for (i=0;i<n;i++) {
    alpha = net->getX(i);
    if ((absolute(x - alpha[0])) < (acc / 100.0)) {
      return 1;
    }
  }
  return 0;
}

int Wolfe::wolf(double xtmp, double ytmp) {
  //xtmp is the step length to be tested.
  //ytmp is the function value at f(x+h*xtmp)

  int i;
  vector a;
  double fa;
  int numAnswered = net->getNumDataItemsAnswered();
  if (ytmp > y0 + xtmp * rho * dy0)
    return 0;

  net->setFirstAnsweredData();
  for (i = 0; i < numAnswered; i++) {
    a = net->getNextAnswerX();
    if (xtmp > a[0]) {
      fa = net->getNextAnswerY();
      if (ytmp >= fa + (xtmp - a[0]) * sigma * dy0)
        return 1;
    }
  }
  return 0;
}

double Wolfe::maximum() {
  int i;
  double tmp = -1.0;
  vector tempx;
  int numSetX = net->getNumDataItemsSet();

  for (i = 0; i < numSetX; i++) {
    tempx = net->getX(i);
    if (tmp < tempx[0])
      tmp = tempx[0];
  }
  return tmp;
}

void Wolfe::swapStuff(double xtmp, double ytmp) {

  if (xtmp <= xl || (xtmp >= xu && xu > 0.0)) {       /* outside bounds - can't use this */
    if (xtmp <= xl && ytmp * (1.0 + BORMACC) < yl) {  /* whoa - outside bounds but bounds wrong! */
      if (ytmp > y0) {
        xu = xtmp;
        yu = ytmp;
        xc = -1;
      } else if (yl > ytmp) {
        xu = xl;
        yu = yl;
        xc = xtmp;
        yc = ytmp;
      } else {
        xu = -1;
        xc = xtmp;
        yc = ytmp;
      }
      xl = 0.0;
      yl = y0;
      cerr << "Warning in linesearch - unimodality case\n";
      return;
    }
    return;
  }

  if (xc < 0.0 && xu < 0.0) {            /* special case: first value seen */
    if (ytmp > yl * (1.0 + BORMACC)) {   /* make sure we don't just have a roundoff problem */
      xu = xtmp;
      yu = ytmp;
    } else {
      xc = xtmp;
      yc = ytmp;
    }

  } else if (xu < 0.0) {                 /* special case: don't have upper bound but have xc */
    if (xtmp > xc) {
      if (ytmp > yc) {                   /* new pt is first upper bound */
        xu = xtmp;
        yu = ytmp;
      } else if (ytmp < yc && yc < yl) { /* center is new lower bound */
        xl = xc;
        yl = yc;
        xc = xtmp;
        yc = ytmp;
      } else if (ytmp * (1 + BORMACC) < yc && yc > yl * (1 + BORMACC)) { /* unimodality problem */
        xu = xc;
        yu = yc;
        xc = -1.;
        cerr << "Warning in linesearch - unimodality case\n";
      }
    } else {                      /* within (*xu)<0 take all cases of xtmp < (*xc) */
      if (ytmp < yc) {            /* center is upper bd */
        xu = xc;
        yu = yc;
        xc = xtmp;
        yc = ytmp;
      } else if (ytmp > yc && yc < yl) {  /* this is a new lb */
        xl = xtmp;
        yl = ytmp;
      } else if (ytmp > yc * (1 + BORMACC) && yc > yl) {  /* unimodality problem */
        cerr << "Warning in linesearch - unimodality case\n";
      }
    }

  } else if (xc < 0.0) {               /* no center yet - but have (*xl), (*xu)*/
    if (ytmp > yl * (1 + BORMACC)) {   /* New value still to the right of min since ytmp>yl */
      xu = xtmp;                       /* leave center undefined */
      yu = ytmp;                       /* NB Only do this if we're sure it's not roundoff*/
      delta = (xu - xl) / numberOfProcesses;
    } else if (ytmp < yu) {            /* normal new center */
      xc = xtmp;
      yc = ytmp;
    }

  } else {                             /* normal case - found an intermediate x-value */
    if (xtmp < xc) {                   /* new x is in ((*xl),(*xc)) - replace either */
      if (ytmp > yl * (1 + BORMACC)) { /* unimodality problem */
        xu = xtmp;
        yu = ytmp;
        xc = -1.0;
        delta=(xu - xl) / numberOfProcesses;
        cerr << "Warning in linesearch - unimodality case\n";
      } else if (ytmp < yc) {          /* new c to left of (*xc) */
        xu = xc;
        yu = yc;
        xc = xtmp;
        yc = ytmp;
      } else {                         /* new left: (*yl)>ytmp>(*yc) */
        xl = xtmp;
        yl = ytmp;
      }

    } else if (xtmp > xc) {            /* to the right of center */
      if (ytmp < yc && ytmp < yu) {    /* new c to right of (*xc) */
        xl = xc;
        yl = yc;
        xc = xtmp;
        yc = ytmp;
      } else {                         /* new right */
        xu = xtmp;
        yu = ytmp;
      }
    }
  }
}
