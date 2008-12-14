#include "paraminbfgs.h"
// Check compute direction vector, how use invhess[i][j], i, j OK???

// ********************************************************
// functions for class ParaminBFGS
// ********************************************************
ParaminBFGS::ParaminBFGS(NetInterface* netInt) : ParaminSearch(netInt) {
    // Vector temp(numvar);
  lineS = new Armijo();   // use lineS to do linesearch
  grad = new NetGradient(numvar);            // use grad to compute gradient
  // not sure I need to resize here, need to check...
  deltax.resize(numvar, 0.0);                          // xi-xim1
  h.resize(numvar, 0.0);                               // the line search vector
  gim1.resize(numvar, 0.0);                            // store previous gradient
  invhess.AddRows(numvar, numvar, 0.0);
      /*invhess = new double*[numvar];
  int i;
  for (i = 0; i < numvar; i++)
    invhess[i] = new double[numvar];
      */
  // xopt.resize(numvar, 1);
  // iter = 0;
  // default values
  shannonScaling = 0;
  bfgs_constant = 1;
  armijoproblem = 0;
  to_print = 0;
  errortol = 0.01;
  xtol = 0.000001;
  maxrounds = 5;
  initial_difficultgrad = 1;
  // If want to set default value separately
  maxiterations = 200;
  // converged = 0;
}

ParaminBFGS::~ParaminBFGS() {
  int i;
  /*
  for (i = 0; i < numvar; i++)
    delete[] invhess[i];
  delete[] invhess;
  */
  delete grad;
  delete lineS;
}

void ParaminBFGS::read(CommentStream& infile, char* text) {
  int i = 0;
  double temp;

  while (!infile.eof() && strcasecmp(text, "seed") && strcasecmp(text, "[hooke]") && strcasecmp(text, "[simann]")) {
    infile >> ws;
    if (strcasecmp(text, "shannonscaling") == 0) {
      infile >> shannonScaling;

    } else if (strcasecmp(text, "difficultgrad") == 0) {
      infile >> initial_difficultgrad;

    } else if (strcasecmp(text, "bfgspar") == 0) {
      infile >> bfgs_constant;

    } else if ((strcasecmp(text, "maxiterations") == 0) || (strcasecmp(text, "maxiter") == 0) || (strcasecmp(text, "bfgsiter") == 0))  {
      infile >> maxiterations;

    } else if ((strcasecmp(text, "eps") == 0) || (strcasecmp(text, "bfgseps") == 0) || (strcasecmp(text, "errortol") == 0)) {
      infile >> errortol;

    } else if (strcasecmp(text, "xtol") == 0) {
      infile >> xtol;

    } else if ((strcasecmp(text, "maxrounds") == 0) || (strcasecmp(text, "bfgsrounds") == 0)) {
      infile >> maxrounds;

    } else if (strcasecmp(text, "printing") == 0) {
      infile >> to_print;

    } else if (strcasecmp(text, "sigma") == 0) {
      infile >> temp;
      lineS->setSigma(temp);

    } else if (strcasecmp(text, "beta") == 0) {
      infile >> temp;
      lineS->setBeta(temp);

    } else if ((strcasecmp(text, "gradacc") == 0) || (strcasecmp(text, "gradstep") == 0) || (strcasecmp(text, "st") == 0) || (strcasecmp(text, "step") == 0) || (strcasecmp(text, "scale") == 0)) {
      cout << "BFGS - " << text << " is not used in paramin" << endl;
      infile >> ws;

    } else {
      cerr << "Error while reading optinfo for bfgs - unknown option " << text << endl;
      exit(EXIT_FAILURE);
    }
    i++;
    infile >> text;
  }

  if (i == 0)
    cerr << "Warning - no optinfo give for bfgs in file - using default parameters" << endl;
}

void ParaminBFGS::iteration() {
  double alpha;
  doLineseek();

  //JMB - changed to check if a double is very close to zero
  alpha = lineS->getAlpha();
  if (isZero(alpha)) {
    armijoproblem++;
    difficultgrad++;
    bfgsFail = -2;
    if (armijoproblem > 1)
      bfgsFail = 6;
  }

  if (alpha < 0.0) {
    // negative alpha indicates wrong derivative - Hessian wrong
    bfgsFail = 1;
  } else {
    UpdateXandGrad();
    // prevy = y;
    ComputeGradient();

    normgrad = grad->getNormGrad();
    //cout << "got normgrad: " << normgrad << endl;
    gi = grad->getGradient();

    if (difficultgrad >= 1)
      diaghess = grad->getDiagonalHessian();

    error = normgrad / (1.0 + fabs(bestf));
    if (bfgsFail != 6 && bfgsFail != -2) {
      if (bfgs_constant == 1) {
        int update = bfgsUpdate();
        if (update == 5)
          bfgsFail = update;
      }
    }
    normx = sqrt(normx);
    // relchng = (prevy - y) / (1.0 + ABSOFNUM(prevy));
  }

  if (bfgsFail <= 0) {
    if (error <= errortol) {
      bfgsFail = 0;
    } else if ((normdeltax < xtol) && (bfgsFail != -2)) {
      bfgsFail = 2;
    } else
      bfgsFail = -1;
  }
}

void ParaminBFGS::ComputeGradient() {
  int i;

  i = grad->computeGradient(net, bestx, bestf, difficultgrad);
  int tmp = grad->getDifficultGrad();
  difficultgrad = tmp;
  if (i == net->netError()) {
    cerr << "Error in BFGS - did not receive gradient data\n";
    this->printX(net->unscaleX(bestx));
    net->stopNetComm();
    exit(EXIT_FAILURE);
  }
}

void ParaminBFGS::doLineseek() {
  s = -1e10;
  double low = GetS(0);
  double upp = GetS(1);
  if (upp <= 0.0) {
    cerr << "Warning in BFGS - upperbound for alpha is negative\n";
    upp = 1.0;
  }
  s = upp;
  if (shannonScaling == 1) {
    if (iters == 0 || upp < 1.0)
      s = upp;
    else
      s = 1.0;
  }
  lineS->doArmijo(bestx, bestf, dery, h, net, min(s, 1.0));
}

void ParaminBFGS::ScaleDirectionVector() {
  int i;
  dery *= normdeltax / normh;
  for (i = 0; i < numvar; i++)
    h[i] *= normdeltax / normh;
}

void ParaminBFGS::ComputeDirectionVector() {
  int i, j;
  for (i = 0; i < numvar; i++) {
    h[i] = 0;
    for (j = 0; j < numvar; j++)
      h[i] -= invhess[i][j] * gi[j];
  }
}

void ParaminBFGS::DefineHessian() {
  int i, j;
  for (i = 0; i < numvar; i++) {
    // start with the identity matrix
    for (j = 0; j < numvar; j++)
      invhess[i][j] = 0.0;

    // set diagonal only if didn't get from grad
    if ((difficultgrad >= 1) && (bfgs_constant == 1)) {
      if (diaghess[i] > 0)
        invhess[i][i] = 1 / diaghess[i];
      else
        invhess[i][i] = 1.0;

    } else
      invhess[i][i] = 1.0;

    if (invhess[i][i] < 0.0) {
      cerr << "Error in BFGS - negative inverse hessian\n";
      break;
    }
  }
}

void ParaminBFGS::UpdateXandGrad() {
  int i;
  double xi;
  normdeltax = 0.0;
  DoubleVector temp(lineS->getBestX());
  for (i = 0; i < numvar; i++) 
      deltax[i] = temp[i] - bestx[i];
  bestx = temp;
  bestf = lineS->getBestF();
  // temp = gi;
  // gim1 = temp;
  gim1 = gi;
  // must check if this is still behaving correctly with double vector..
  normdeltax = (deltax * deltax);
}

int ParaminBFGS::bfgsUpdate() {
  int i, j;

   /* prepare the BFGS update */
  double deltaxg = 0.0;
  double deltaghg = 0.0;
  DoubleVector hg(numvar);
  DoubleVector deltag(numvar);
  normx = 0.0;

  for (i = 0; i < numvar; i++) {
    hg[i] = 0.0;
    deltag[i] = gi[i] - gim1[i];
    deltaxg += deltax[i] * deltag[i];
    normx += bestx[i] * bestx[i];
  }

  if (deltaxg <= 0.0)
    cerr << "Warning in BFGS - instability error\n";
  // Must check that invhess is used correctly [i][j] or [j][i]
  for (i = 0; i < numvar; i++) {
    for (j = 0; j < numvar; j++)
      hg[i] += invhess[i][j] * deltag[j];

    deltaghg += deltag[i] * hg[i];
  }

  /* do the BFGS update */
  for (i = 0; i < numvar; i++) {
    for (j = 0; j < numvar; j++) {
      invhess[i][j] += (deltax[i] * deltax[j] / deltaxg) - (hg[i] * hg[j] /
        deltaghg) + deltaghg * (deltax[i] / deltaxg - hg[i] / deltaghg) *
        (deltax[j] / deltaxg - hg[j] / deltaghg);
    }
  }

  if (deltaxg <= 0)
    return 5;
  else
    return -999;
}

double ParaminBFGS::norm() {
  int i;
  double normx;
  normx = 0.0;
  /*
  for (i = 0; i < numvar; i++)
    normx += bestx[i] * bestx[i];
  */
  normx = bestx*bestx;
  normx = sqrt(normx);
  return normx;
}

void ParaminBFGS::printGradient() {
    int i;
  ofstream outputfile;
  outputfile.open("gradient");
  if (!outputfile) {
    cerr << "Error in BFGS - could not print gradient data\n";
    printX(gi);
    net->stopNetComm();
    exit(EXIT_FAILURE);
  }
  printX(outputfile, gi);
  
  outputfile.close();
}

void ParaminBFGS::printInverseHessian() {
  int i, j;
  ofstream outputfile;
  outputfile.open("hessian");
  if (!outputfile) {
    cerr << "Error in BFGS - could not print hessian\n";
    exit(EXIT_FAILURE);
  }
  invhess.Print(outputfile);
  /*
  for (i = 0; i < numvar; i++) {
    for (j = 0; j < numvar; j++)
      outputfile << invhess[i][j] << " ";
    outputfile << endl;
  }
  */
  outputfile.close();
}

double ParaminBFGS::GetS(int get) {
  double b = 1.e69;
  double a = -1.e69;

  DoubleVector alpha_l(numvar);
  DoubleVector alpha_u(numvar);

  int i;
  for (i = 0; i < numvar; i++) {
    if (h[i] > 0) {
      alpha_l[i] = (lowerbound[i] - bestx[i]) / h[i];
      alpha_u[i] = (upperbound[i] - bestx[i]) / h[i];
    } else if (h[i] < 0) {
      alpha_u[i] = (lowerbound[i] - bestx[i]) / h[i];
      alpha_l[i] = (upperbound[i] - bestx[i]) / h[i];
    } else {
      alpha_u[i] = b;
      alpha_l[i] = a;
    }
    a = max(a, alpha_l[i]);
    b = min(b, alpha_u[i]);
  }

  if (get == 0)
    return a;
  else if (get == 1)
    return b;
  else {
    cerr << "Error in BFGS - unrecognised return value\n";
    exit(EXIT_FAILURE);
  }
}


void ParaminBFGS::SetInitialValues() {
  // For the first iteration, define the parameters
  // and the Hessian matrix, the gradient and the direction vector
  armijoproblem = 0;
  if (difficultgrad != initial_difficultgrad) {
    difficultgrad = initial_difficultgrad;
    grad->initializeDiagonalHessian();
    ComputeGradient();
    normgrad = grad->getNormGrad();

    if (difficultgrad >= 1)
      diaghess = grad->getDiagonalHessian();

    gi = grad->getGradient();
    normx = norm();
  }
  DefineHessian();
}

void ParaminBFGS::UpdateValues() {
  bfgsFail = 0;
  int i;

  normdeltax = 1.0;
  ComputeDirectionVector();
  dery = 0.0;
  normh = 0.0;
  for (i = 0; i < numvar; i++) {
    normh += h[i] * h[i];
    dery += gi[i] * h[i];
  }

  // if scaling of direction vector is to be done, then do it here.
  if (iters <= numvar && shannonScaling == 1)
    ScaleDirectionVector();
  if (dery > 0) {
    cerr << "Error in BFGS - the derivative is positive\n";
    bfgsFail = 4;
  }
  error = normgrad / (1.0 + fabs(bestf));
}

void ParaminBFGS::OptimiseLikelihood() {
  int rounds = 0;
  int numrounds = 0;

  bestx = net->getInitialX();
  bestf = net->getScore();
  difficultgrad = -1;

  // Loop over BFGS iterations
  // continue until tolerance criterion is satisfied
  // or give up because x's do not move -- only valid if function does not
  // change either and must do one iteration

  for (rounds = 0; rounds < maxrounds; rounds++) {
    bfgsFail = -1;
    iters = 0;
    SetInitialValues();
    while ((iters < maxiterations) && (bfgsFail < 0)) {
      this->UpdateValues();
      this->iteration();
      
      net->setInitialScore(bestx, bestf);
      iters++;
    }

    if ((iters == maxiterations) && (bfgsFail != 0))
      cout << "During BFGS - Quit this round because have reached maxiterations." << endl;

    if (bfgsFail == 0) {
      numrounds = rounds;
      rounds = maxrounds - 1;
    }
    if (bfgsFail == 1)
      cerr << "BFGS - failed because alpha < 0" << endl;;
    if (bfgsFail == 2)
      cerr << "BFGS - failed because normdeltax < xtol" << endl;
    if (bfgsFail == 4)
      cerr << "BFGS - failed because dery > 0" << endl;
    if (bfgsFail == 5)
      cerr << "BFGS - failed because of instability error" << endl;
    if (bfgsFail == 6)
      cerr << "BFGS - failed because could not get alpha from linesearch, alpha == 0" << endl;
    if (bfgsFail == -2)
      cerr << "BFGS - Could not get alpha from linesearch but will continue and increase difficultgrad" << endl;

    if (rounds < (maxrounds - 1))
      cout << "Starting a new round. Still have " << maxrounds - rounds - 1 << " to go." << endl;
  }

  if ((rounds >= maxrounds) &&  (bfgsFail != 0)) {
    cout << "\nBFGS optimisation completed after " << rounds << " rounds (max " << maxrounds << ") and " << iters << " iterations (max " << maxiterations << ")\nThe model terminated because the maximum number of rounds was reached\n";
  } else if (bfgsFail == 0)  {
    cout << "\nStopping BFGS \n\nThe optimisation stopped after " << numrounds << " rounds (max " << maxrounds << ") and " << iters << " iterations  (max " << maxiterations << ")\nThe optimisation stopped because an optimum was found for this run\n";
    converge = 1;
  }
  score = bestf;
  if (to_print) {
    this->printGradient();
    this->printInverseHessian();
  }
}
void ParaminBFGS::Print(ofstream& outfile, int prec) {

}
