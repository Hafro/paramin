#include "linesearch.h"

// ********************************************************
//  Functions for class linesearch
// ********************************************************
linesearch::linesearch() {
  NUMALPHA = 200;
}

linesearch::~linesearch() {
}

double linesearch::GetBestF() {
  return f;
}

vector linesearch::GetBestX() {
  return x;
}

struct linecon {
  double BETA;
  double SIGMA;
};

linecon readlineConstants() {
  char letter;
  int i = 0;
  char text[MaxStrLength];
  linecon tmp;

  ifstream bfgsfile( "bfgsconstants", ios::in|ios::binary|ios::nocreate );
  if (!bfgsfile) {
    cerr << "Unable to open inputfile bfgsconstants\n";
    exit(EXIT_FAILURE);
  }

  while (!bfgsfile.eof()) {
    bfgsfile >> text;
    if (strcasecmp(text, ";") == 0) {
      bfgsfile.get(letter);
      while ((letter != '\n') && !bfgsfile.eof())
        bfgsfile.get(letter);

    } else if (strcasecmp(text, "//") == 0) {
      bfgsfile.get(letter);
      while ((letter != '\n') && !bfgsfile.eof())
        bfgsfile.get(letter);

    } else {
      if (strcasecmp(text, "BETA") == 0) {
        bfgsfile >> tmp.BETA;
        i++;
      }
      if (strcasecmp(text, "SIGMA") == 0) {
        bfgsfile >> tmp.SIGMA;
        i++;
      }
    }
  }

  bfgsfile.close();
  if (i != 2) {
    cerr << "Error in reading from input file bfgsconstants\n";
    exit(EXIT_FAILURE);
  }
  return(tmp);
}

// ********************************************************
// Functions for class armijo
// ********************************************************
armijo::armijo() {
  cond = new lineSeekerCondition(this);
  vector tempVec(50);
  inixvec = tempVec;
}

armijo::~armijo() {
  delete cond;
}

double armijo::GetAlpha() {
  return pow(opt.beta, opt.power) * s;
}

int armijo::CondSatisfied( double y ) {
  int returnId = net->getReceiveId();
  if ((fcn-y) >= -sigma * pow(beta, returnId) * s * df)
    return(1);
  else
    return(0);
}

void armijo::DoArmijo(const vector& v1, double fx, double dery,
  const vector& h, netInterface *netI, double s1) {

  int cond_satisfied;
  linecon par;
  par = readlineConstants();

  beta = par.BETA;
  sigma = par.SIGMA;

  opt.value = fx;
  opt.power = -1;
  opt.beta = beta;

  numberOfVariables = netI->getNumOfVarsInDataGroup();

  x = v1;
  s = s1;
  hvec = h;
  net = netI;
  fcn = fx;
  df = dery;
  fail = -1;

  prepareNewLineSearch();
  InitateAlphas();
  cond_satisfied = net->send_receive_setData(cond);
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

int armijo::computeConditionFunction() {
  int i, cond_satisfied = 0;
  int counter = net->getNumDataItemsSet();
  int newreturns = net->getNumDataItemsAnswered();
  int returnId;
  double y;

  returnId = net->getReceiveId();
  if (returnId >= 0) {
    y = net->getY(returnId);
    cond_satisfied = ((CondSatisfied(y) == 1) && (opt.value > y));
    if (cond_satisfied) {
      cout << "New optimum value f(x) = " << y << endl;
      opt.value = y;
      opt.power = returnId - 1;
      opt.beta = beta;
    }
  }

  if (opt.power == -1) {
    if (net->dataGroupFull()) {
      if ((0.8 * counter) <= newreturns) {
        // cannot set more data and have received 8/10 of all data - bailing out
        opt.power = 1;
        opt.beta = 0;
        return 1;

      } else
        return 0;

    } else {
      // dataGroup not full
      i = SetData();
      return cond_satisfied;
    }

  } else if (opt.power >= 0) {
    // not setting any more data, have already found optimal value
    if ((0.8 * counter) <= newreturns) {
      // have found optimal value and received 8/10 of all data set
      return 1;
    } else
      return 0;

  } else
    return 1;
}

vector armijo::getXvector(const vector& x1, const vector& inixvec) {
  vector x(numberOfVariables);
  x = x1 + inixvec;
  return x;
}

void armijo::prepareNewLineSearch() {
  net->startNewDataGroup(NUMALPHA, x, hvec);
  if (df > 0) {
    cerr << "Error in linesearch - bad derivative\n";
    net->stopUsingDataGroup();
  }
  vector tempx(1);
  tempx[0] = 0.0;
  net->setDataPair(tempx, fcn);
}

void armijo::InitateAlphas() {
  int i = net->getTotalNumProc();
  int j;
  vector tempx(1);
  assert(beta > 0.0);
  assert(beta <= 0.5);
    for (j = 0; j < i ; j++) {
    inixvec[j] = pow(beta, j) * s;
    tempx[0] = inixvec[j];
    net->setX(tempx);
  }
}

double armijo::GetBeta() {
  return opt.beta;
}

int armijo::GetPower() {
  return opt.power;
}

int armijo::OutstandingRequests() {
  int out;
  int pending = net->getNumNotAns();
  out = (pending > 0);
  return out;
}

int armijo::SetData() {

  vector tempx(1);
  int counter = net->getNumDataItemsSet();
  double a = pow(beta,counter) * s;
  tempx[0] = a;
  int ok = -1;
  if (net->dataGroupFull())
    ok = 0;
  else {
    net->setXFirstToSend(tempx);
    ok = 1;
  }
  return ok;
}

// ********************************************************
// Functions for class wolfe
// ********************************************************
wolfe::wolfe() {
  // this has to change..
  con = new lineSeekerCondition(this);
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

wolfe::~wolfe() {
  delete con;
}

void wolfe::DoWolfe(const vector& v1, double fx, double dery, const vector& hess, netInterface* netInt) {
  int i = 0;

  if (v1.dimension() != hess.dimension()) {
    cout << "Error in linesearch - different number of parameters\n";
    exit(EXIT_FAILURE);
  }
  if (v1.dimension() <= 0) {
    cout << "Error in linesearch - no parameters\n";
    exit(EXIT_FAILURE);
  }

  vector tempVector(50);
  inixvec = tempVector;
  inixvec[0] = 1.0;
  newreturns = 0;
  askedfor = 0;
  net = netInt;
  net->startNewDataGroup(NUMALPHA, v1, hess);
  prepareNewLineSearch(v1, fx, dery);

  while (!linesearchCondSatisfied()) {
    seekNewLimits(i);
    wolfecond = net->send_receiveTillCondition(con);
    if (wolfecond == -1) {
      cout << "Error in linesearch - netcommunication has been halted\n";
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

int wolfe::computeConditionFunction() {
  int returnId = net->getReceiveId();
  assert (returnId != -1);
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

double wolfe::getLastAnsweredAlpha() {
  int returnId = net->getReceiveId();
  assert(returnId != -1);
  vector x;
  x = net->getX(returnId);
  return (x[0]);
}

void wolfe::prepareNewLineSearch(const vector& v1, double fx, double dery) {
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
    cout << "Error in linesearch - wrong sign of derivative\n";
    resultAlpha = -2.0;
  }

  vector tempx(1);
  tempx[0] = 0.0;
  net->setDataPair(tempx, fx);
  initiateAlphas();
}

int wolfe::linesearchCondSatisfied() {
  /* The criteria for exiting are crucial - would like to exit when the
   * wolfe condition is satisfied - still OK if get xl>0 and tight bound
   * Must give up and decide on convergence to zero if xu<MACHEPS */

  int notFinished;
  notFinished = ((xu < 0 || !(xl > 0 && xu - xl < acc)
    || (xl == 0 && xu > MACHEPS)) && wolfecond == 0);
  return !notFinished;
}

void wolfe::seekNewLimits(int i) {
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

void wolfe::setNewLinesearchResults(double fx) {

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

  if (xl == 0.0 && xu - xl < MACHEPS && yu > yl && (yc > yl || xc < 0.0) && wolfecond == 0) {
    xc = -1.0;
    cout << "Error in linesearch - algorithm converges to zero\n";
  }

  if (xc < 0 && xwolfe > 0 && xwolfe <= xu && xwolfe >= xl)
    xc = xwolfe;
  else if (xc < 0 && xl > 0 && yl < fx) {
    xc = xl;
    cout << "Error in linesearch - no center so selected lower point\n";
  } else if (xc < 0 && xu > 0 && yu < yl) {
    xc = xu;
    cout << "Error in linesearch - no center so selected upper point\n";
  }
  resultAlpha = xc;
}

double wolfe::GetAlpha() {
  return resultAlpha;
}

void wolfe::initiateAlphas() {
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
      inixvec[i] = 2.0 * rand() / 32767.;

    tempx[0] = inixvec[i];
    net->setX(tempx);
    askedfor++;
  }
}

void wolfe::increaseLowerbound() {
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

void wolfe::seekUpperbound(int itnum) {
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

void wolfe::seekNewCenter() {
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

void wolfe::findRandomAlpha() {
  vector xtmp(1);
  xtmp[0] = xl + (xu - xl) * (rand() / 32767.);
  net->setX(xtmp);
  askedfor++;
}

void wolfe::interpolate() {
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

void wolfe::finishSweep() {
  vector xtmp(1);
  double tmpdelta;
  int i;
  int maxreq = max(numberOfProcesses - askedfor, 3);

  if (xu < 0.0 && xl == 0.0)
    tmpdelta = 2.0 / maxreq;
  else if (xu > 0.0)
    tmpdelta = (xu - xl) / maxreq;
  else if (xl < 0.001)
    tmpdelta = (0.01 - xl) / maxreq;
  else if (xl < 0.01)
    tmpdelta = (0.1 - xl) / maxreq;
  else
    tmpdelta = (1.0 - xl) / maxreq;

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

int wolfe::OutstandingRequests() {
  int pending = net->getNumNotAns();
  return pending;
}

int wolfe::wolfeCondFalse() {
  return !wolfecond;
}

void wolfe::checkBoundary() {
  double ytmp;
  vector xtmp;
  int returnId = net->getReceiveId();
  assert(returnId != -1);
  ytmp = net->getY(returnId);
  xtmp = net->getX(returnId);
}

void wolfe::computeWolfe() {
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

void wolfe::updateCenterAndBounds() {
  vector xtmp;
  int returnId = net->getReceiveId();
  double ytmp = net->getY(returnId);
  xtmp = net->getX(returnId);
  swapstuff(xtmp[0], ytmp);
}

int wolfe::tabu(double x) {
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

int wolfe::wolf(double xtmp, double ytmp) {
  //xtmp is the step length to be tested.
  //ytmp is the function value at f(x+h*xtmp)

  int i;
  vector a;
  double fa;
  int numAnswered = net->getNumDataItemsAnswered();
  if (ytmp > y0 + xtmp * rho * dy0)
    return(0);

  net->setFirstAnsweredData();
  for (i = 0; i < numAnswered; i++) {
    a = net->getNextAnswerX();
    if (xtmp > a[0]) {
      fa = net->getNextAnswerY();
      if (ytmp >= fa + (xtmp - a[0]) * sigma * dy0)
        return(1);
    }
  }
  return(0);
}

double wolfe::maximum() {
  int i;
  double tmp = -1.0;
  vector tempx;
  int numSetX = net->getNumDataItemsSet();

  for (i = 0; i < numSetX; i++) {
    tempx = net->getX(i);
    if (tmp < tempx[0])
      tmp = tempx[0];
  }
  return(tmp);
}

void wolfe::swapstuff(double xtmp, double ytmp) {

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
      cout << "Error in linesearch - unimodality case\n";
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
        cout << "Error in linesearch - unimodality case\n";
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
        cout << "Error in linesearch - unimodality case\n";
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
        cout << "Error in linesearch - unimodality case\n";
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

// ********************************************************
// Functions for class lineseeker
// ********************************************************
lineseeker::lineseeker() {
}

lineseeker::~lineseeker() {
}

int lineseeker::OutstandingRequests() {
  int pending = net->getNumNotAns();
  return pending;
}

/* In the initial version of Hooke and Jeeves, the point xnew+d is      */
/* checked after each run of best_nearby, where d=xnew-xold. That is,   */
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
void lineseeker::DoLineseek(const vector& xold, const vector& xnew,
  double fnew, netInterface *netI) {

  int i, j, k, l;
  int send_receive, numDataItems;

  net = netI;
  int numvar = net->getNumOfVarsInDataGroup();
  int numberOfHosts = net->getNumFreeProcesses();
  vector d;
  vector z(numvar);
  d = xnew - xold;
  vector upper(numvar);
  vector lower(numvar);
  upper = netI->getUpperScaleConstant();
  lower = netI->getLowerScaleConstant();
  f = fnew;
  x = xnew;

  net->startNewDataGroup(numberOfHosts);
  for (i = 0; i < numvar; i++)
    d[i] = xnew[i] - xold[i];

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
        if (( z[i] < lower[i]) || (z[i] > upper[i]))
          l = 0;
      }
      if (l)
        net->setX(z);
      else
        break;
    }

  } else {
    l = 1;
    for (i = 0; i < numvar; i++)
      z[i] = xnew[i] - 1 / 2 * d[i];

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

  send_receive = net->send_receiveAllData();
  if (send_receive == net->NET_ERROR()) {
    cout << "Error in lineseeker - could not receive data\n";
    net->stopUsingDataGroup();
    exit(EXIT_FAILURE);

  } else if (send_receive == net->NET_SUCCESS()) {
    numDataItems = net->getNumDataItemsAnswered();
    for (i = 0; i < numDataItems; i++) {
      if (net->getY(i) < f) {
        f = net->getY(i);
        x = net->getX(i);
      }
    }
    net->stopUsingDataGroup();

  } else {
    cout << "Error in lineseeker - could not receive data\n";
    net->stopNetComm();
    net->stopUsingDataGroup();
    exit(EXIT_FAILURE);
  }
}
