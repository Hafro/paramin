#include "search.h"

// ********************************************************
// functions for class search
// ********************************************************
search::~search() {
}

vector search::GetBestX(netInterface *net) {
  return net->unscaleX(x);
}

double search::GetBestF() {
  return f;
}

// ********************************************************
// functions for class simann
// ********************************************************
struct sacon {
  int NEPS;
  int NS;
  double EPS;
  double RT;
  int NT;
};

sacon readSAConstants() {
  char letter;
  int i = 0;
  char text[MaxStrLength];
  sacon tmp;

  ifstream simfile("simconstants", ios::in|ios::binary|ios::nocreate);
  if (!simfile) {
    cerr << "Unable to open inputfile simconstants\n";
    exit(EXIT_FAILURE);
  }

  while (!simfile.eof()) {
    simfile >> text;
    if (strcasecmp(text, ";") == 0) {
      simfile.get(letter);
      while ((letter !=  '\n') && !simfile.eof())
        simfile.get(letter);

    } else if (strcasecmp(text, "//") == 0) {
      simfile.get(letter);
      while ((letter != '\n') && !simfile.eof())
        simfile.get(letter);

    } else {
      if (strcasecmp(text, "NS") == 0) {
        simfile >> tmp.NS;
        i++;
      }
      if (strcasecmp(text, "NT") == 0) {
        simfile >> tmp.NT;
        i++;
      }
      if (strcasecmp(text, "NEPS") == 0) {
        simfile >> tmp.NEPS;
        i++;
      }
      if (strcasecmp(text, "RT") == 0) {
        simfile >> tmp.RT;
        i++;
      }
      if (strcasecmp(text, "EPS") == 0) {
        simfile >> tmp.EPS;
        i++;
      }
    }
  }

  simfile.close();
  if (i != 5) {
    cerr << "Error in reading from input file simconstants\n";
    exit(EXIT_FAILURE);
  }
  return(tmp);
}

simann::simann(netInterface* netInt, int m, int evl, const vector& stp,
  int tempt, const vector& stepl) {

  net = netInt;
  n = net->getNumOfVarsInDataGroup();
  maxmin = m;
  maxevl = evl;
  xstart = net->getInitialX();
  initialC = stp;
  initialT = tempt;
  initialVm = stepl;
  vector tempVec(n);
  xopt = tempVec;
  xp = tempVec;
  nfcnev = 0;
}

simann::~simann() {
}

int simann::DoSearch() {
  int i, m, j, h, q, g, quit;
  sacon par;
  par = readSAConstants();
  nacc = 0;
  nobds = 0;
  vm = initialVm;
  t = initialT;
  c = initialC;

  // getInitialX() gives x scaled if scaling is defined in net.
  x = net->getInitialX();
  xstart = net->getInitialX();
  vector tempVec(par.NEPS);
  tempVec.setValue(1.0e20);
  fstar = tempVec;
  vector upper = net->getUpperScaleConstant();
  vector lower = net->getLowerScaleConstant();
  if (xstart > upper || xstart < lower) {
    cout << "Error in Simmulated Annealing - x is not within bounds\n";
    exit(EXIT_FAILURE);
  }

  // total number of processes initiated at beginning
  NumberOfHosts = net->getTotalNumProc();

  // Find the function value at the starting point.
  net->startNewDataGroup(1);
  net->setX(xstart);
  net->send_receiveAllData();
  f = net->getY(0);
  net->stopUsingDataGroup();

  if (!maxmin)
    f = -f;

  fopt = f;
  fstar[0] = f;

  // start the main loop.  Note that it terminates if (i) the algorithm
  // succesfully otimizes the function or (ii) there are too many func. eval.
  net->startNewDataGroup(maxevl*2);
  Id = new int[n];
  int rock = 1;

  while (rock) {
    nup = 0;
    nrej = 0;
    ndown = 0;
    lnobds = 0;
    for (m = 0; m < par.NT; m++) {
      for (j = 0; j < par.NS; j++) {
        h = 0;
        if (j == 0) {
          for (i = 0; i < n; i++)
            Id[i] = -1;

          // generate xp, the trial value of x
          if (n > NumberOfHosts) {
            i = 0;
            while (i < NumberOfHosts) {
              q = random() % n;
              if (!IdContains(q)) {
                Id[h] = q;
                xp = GetXP(q);
                net->setX(xp);
                h++;
                i++;
              }
            }

            // put the remaining parameters in the Id array.
            g = 0;
            for (i = 0; i < n; i++) {
              if (!IdContains(i)) {
                Id[g + NumberOfHosts] = i;
                g++;
              }
            }

          } else {
            i = 0;
            while (i < n) {
              q = random() % n;
              if (!IdContains(q)) {
                Id[h] = q;
                xp = GetXP(q);
                net->setX(xp);
                h++;
                i++;
              }
            }
          }

          // sends all available data to all free hosts.
          net->sendToAllIdleHosts();
        }

        while (h < n) {
          // get a function value and do any update that's necessary.
          nfcnev = nfcnev + 1;
          ReceiveValue();
          // if too many function evaluations occur, terminate the algorithm
          if (nfcnev > maxevl) {
            cout << "\nSimulated Annealing optimisation completed after " << nfcnev
              << " iterations (max " << maxevl << ")\nThe model terminated "
              << "because the maximum number of iterations was reached\n";
            if (!maxmin)
              fopt = -fopt;

            f = fopt;
            i = net->receiveAll();  //WHY? this takes time
            net->stopUsingDataGroup();
            net->setBestX(x);
            delete [] Id;
            return(nfcnev);
          }

          // sending new point instead of the one just recieved.
          xp = GetXP(Id[h]);
          net->setX(xp);
          net->sendToAllIdleHosts();
          h++;
        }
      }

      while (net->getNumNotAns() > 0) {
        // get a function value and do any update that's necessary.
        nfcnev = nfcnev + 1;
        ReceiveValue();
        // if too many function evaluations occur, terminate the algorithm
        if (nfcnev > maxevl) {
          cout << "\nSimulated Annealing optimisation completed after " << nfcnev
            << " iterations (max " << maxevl << ")\nThe model terminated "
            << "because the maximum number of iterations was reached\n";
          if (!maxmin)
            fopt = -fopt;

          f = fopt;
          i = net->receiveAll();  //WHY? this takes time
          net->setBestX(x);
          net->stopUsingDataGroup();
          delete [] Id;
          return(nfcnev);
        }
      }

      // adjust vm so that approximately half of all evaluations are accepted.
      UpdateVM();
      for (i = 0; i < n; i++)
        nacp[i] = 0;
    }

    // check termination criteria.
    quit = 0;
    fstar[0] = f;
    if ((fopt - fstar[0]) <= par.EPS)
      quit = 1;

    for (i = 0; i < par.NEPS; i++)
      if (fabs(f - fstar[i]) > par.EPS)
        quit = 0;

    // terminate SA if appropriate.
    if (quit) {
      if (!maxmin)
        fopt = -fopt;
      rock = 0;
    }

    // if termination criteria is not met, prepare for another loop.
    if (rock) {
      t = par.RT * t;
      for (i = par.NEPS - 1; i > 0; i--)
        fstar[i] = fstar[i - 1];

      f = fopt;
      xstart = x;
    }
  }

  net->stopUsingDataGroup();
  delete [] Id;
  f = fopt;
  // update net with best x found so far.
  net->setBestX(x);
  return(nfcnev);
}

// generate xp, the trial value of x - note use of vm to choose xp.
vector simann::GetXP(int k) {
  //net->getUpperScaleConstant and net->getLowerScaleConstant return a
  //vector with all values equal to 1.0/-1.0 if scaling is defined else
  //return upper and lower bound give initially from initvals file.
  vector lbd = net->getLowerScaleConstant();
  vector ubd = net->getUpperScaleConstant();
  int i, param = k;
  for (i = 0; i < n; i++) {
    if (i == param)
      xp[i] = xstart[i] + (Random() * 2.0 - 1.0) * vm[i];
    else
      xp[i] = xstart[i];

    if (xp[i] < lbd[i] || xp[i] > ubd[i]) {
      xp[i] = lbd[i] + (ubd[i] - lbd[i]) * Random();
      lnobds++;
      nobds++;
    }
  }
  return xp;
}

void simann::AcceptPoint() {
  xstart = xp;
  f = fp;
  nacc++;

  // if better than any other point record as new optimum.
  if (fp > fopt) {
    x = xp;
    cout << "\nAfter " << nfcnev << " function evaluations, f(x) = " << -fp << " at\n";
    cout << net->unscaleX(x);
    fopt = fp;
    nnew++;
  }
}

void simann::UpdateVM() {
  int i;
  double ratio, na, ns;
  sacon par;
  par = readSAConstants();
  ns = par.NS;

  // adjust vm so that approximately half of all evaluations are accepted.
  for (i = 0; i < n; i++) {
    na = nacp[i];
    ratio =  na / ns;
    if (ratio > 0.6)
      vm[i] = vm[i] * (1.0 + c[i] * (ratio - 0.6) * 2.5);
    else if (ratio < 0.4)
      vm[i] = vm[i] / (1.0 + c[i] * (0.4 - ratio) * 2.5);
    if (vm[i] > 2.0)
      vm[i] = 2.0;
  }
}

double simann::ExpRep(double inn) {
  double rdum, exprep;
  rdum = inn;
  if (rdum > 174.)
    exprep = 3.69e75;
  else if (rdum < -180.)
    exprep = 0.0;
  else
    exprep = exp(rdum);
  return exprep;
}

double simann::Random() {
  int r = random();
  double k = r % 32767;
  return k/32767.;
}

int simann::IdContains(int q) {
  int i, ans = 0;
  for (i = 0; i < n; i++)
    if (Id[i] == q)
      ans = 1;

  return ans;
}

// get a function value and do any updates that are necessary.
void simann::ReceiveValue() {
  int receive = net->receiveOne();
  int returnId;
  double p, pp;

  if (receive == net->NET_SUCCESS()) {
    returnId = net->getReceiveId();
    if (returnId >= 0) {
      // received data belonging to correct datagroup
      fp = net->getY(returnId);
      xp = net->getX(returnId);

      if (!maxmin)
        fp = -fp;

      // accept the new point if the function value increases.
      if (fp >= f) {
        AcceptPoint();
        nup++;
        nacp[Id[returnId % n]] = nacp[Id[returnId % n]] + 1;

      } else {
        p = ExpRep((fp - f) / t);
        pp = Random();

        // accept the new point if Metropolis condition is satisfied.
        if (pp < p) {
          AcceptPoint();
          ndown++;
          nacp[Id[returnId % n]] = nacp[Id[returnId % n]] + 1;
        } else
          nrej++;
      }
    }

  } else
    exit(EXIT_FAILURE);
}

// ********************************************************
// functions for class hooke
// ********************************************************
struct hjcon {
  double epsilon;
  int maxiter;
  double rho;
};

hjcon readHJConstants() {
  char letter = ' ';
  int i = 0;
  char text[MaxStrLength];
  hjcon tmp;

  ifstream hjfile("hjconstants", ios::in|ios::binary|ios::nocreate);
  if (!hjfile) {
    cerr << "Unable to open inputfile hjconstants\n";
    exit(EXIT_FAILURE);
  }

  while (!hjfile.eof()) {
    hjfile >> text;
    if (strcasecmp(text, ";") == 0) {
      hjfile.get(letter);
      while ((letter !=  '\n') && !hjfile.eof())
        hjfile.get(letter);

    } else if (strcasecmp(text, "//") == 0) {
      hjfile.get(letter);
      while ((letter != '\n') && !hjfile.eof())
         hjfile.get(letter);

    } else {
      if (strcasecmp(text, "epsilon") == 0) {
        hjfile >> tmp.epsilon;
        i++;
      }
      if (strcasecmp(text, "MAXITER") == 0) {
        hjfile >> tmp.maxiter;
        i++;
      }
      if (strcasecmp(text, "rho") == 0) {
        hjfile >> tmp.rho;
        i++;
      }
    }
  }

  hjfile.close();
  if (i != 3) {
    cerr << "Error in reading from input file hjconstants\n";
    exit(EXIT_FAILURE);
  }

  if (tmp.rho > tmp.epsilon) {
    cout << "Error in Hooke - the value for rho must be greater than epsilon\n";
    exit(EXIT_FAILURE);
  }

  return(tmp);
}

hooke::hooke(netInterface* netInt) {
  hjcon val = readHJConstants();
  vector tempVec(val.maxiter + NUMVARS);
  fbefore = tempVec;
  par = new int[val.maxiter + NUMVARS];
  net = netInt;
  iters = 0;
}

hooke::~hooke() {
  delete [] par;
}

double hooke::best_nearby(const vector& delta, double prevbest) {
  int receive;
  double fopt;
  double ftmp;
  int i, j, k, h, p;
  int returnId;
  int* change;
  change = new int[numvar];

  hjcon val = readHJConstants();
  net->startNewDataGroup(10 * numvar);

  // randomize the order of the parameters once in a while, to avoid
  // the order having an influence on which changes are accepted.
  int changes = 0;
  while (changes < numvar) {
    h = rand() % numvar;
    k = 1;
    for (i = 0; i < changes; i++)
      if (param[i] == h)
        k = 0;

    if (k) {
      param[changes] = h;
      changes++;
    }
  }

  nsent = 0;
  fopt = prevbest;
  for (i = 0; i < numvar; i++)
    change[param[i]] = 0;

  h = 0;
  i = 0;
  // sending as many points as there are processors.
  while ((i < NumberOfHosts) && (h < numvar)) {
    j = SetPoint(param[h], fopt);
    if (j == 1) {
      change[param[h]]++;
      i++;
    }
    h++;
  }

  // send all available data to all free hosts and then receive them
  // back one at a time, sending new points when possible
  net->sendToAllIdleHosts();
  while (net->getNumNotAns() > 0) {
    receive = net->receiveOne();
    if (receive == net->NET_ERROR()) {
      cout << "Error in Hooke - failed to receive data in wolfe\n";
      net->stopUsingDataGroup();
      exit(EXIT_FAILURE);
    }

    returnId = net->getReceiveId();
    if (returnId >= 0) {
      p = par[returnId];
      ftmp = net->getY(returnId);
      iters++;
      if (iters % 1000 == 0)
        cout << "\nAfter " << iters << " function evaluations, f(x) = " << fopt << " at\n" << newx;

      if (iters > val.maxiter) {
        // break out of the while loop and return ...
        i = net->receiveAll();  //WHY? this takes time
        break;
      }

      // update the optimum if necessary.
      j = 0;
      if (ftmp < fopt) {
        fopt = ftmp;
        newx = net->getX(returnId);

      // AJ feb. 2002 indexing error - change use p instead of param[p].
      } else if (change[p] == 1) {
        if (ftmp < fbefore[returnId]) {
          // try the same change on the best point if improvement from last point
          j = SetPoint(p, fopt);
          if (j == 1)
            net->sendOne();

        } else {
          // worse than before, so try the opposite direction.
          delta[p] = 0.0 - delta[p];
          j = SetPoint(p, fopt);
          if (j == 1) {
            change[p]++;
            net->sendOne();
          }
        }
      }

      // try new coordinate.
      if (j == 0) {
        k = 0;
        while ((h < numvar) && (k == 0)) {
          j = SetPoint(param[h], fopt);
          if (j == 1) {
            change[param[h]]++;
            net->sendOne();
            h++;
            k = 1;
          } else
            h++;
        }
      }
    }
  }

  net->stopUsingDataGroup();
  delete [] change;
  return fopt;
}

int hooke::DoSearch() {
  hjcon val = readHJConstants();
  numvar = net->getNumOfVarsInDataGroup();
  double newf, fbefore, steplength;
  int i, keep;
  NumberOfHosts = net->getTotalNumProc();

  ubd = net->getUpperScaleConstant();
  lbd = net->getLowerScaleConstant();
  newx = net->getInitialX();
  xbefore = newx;
  vector tempVec(numvar);
  delta = tempVec;

  // The original definition of the delta array has not been changed, even
  // though we're sometimes working with scaled x-values.
  for (i = 0; i < numvar; i++) {
    delta[i] = fabs(newx[i] * val.rho);
    if (delta[i] == 0.0)
      delta[i] = val.rho;
    param[i] = i;
  }

  steplength = val.rho;
  net->startNewDataGroup(1);
  net->setX(newx);

  int sendreceive = net->send_receiveAllData();
  if (sendreceive == net->NET_ERROR()) {
    cout << "Error in Hooke - failed to send/receive data in search\n";
    net->stopUsingDataGroup();
    exit(EXIT_FAILURE);
  }

  fbefore = net->getY(0);
  newf = fbefore;
  net->stopUsingDataGroup();
  lineS = new lineseeker();

  while ((iters < val.maxiter) && (steplength > val.epsilon)) {
    newx = xbefore;
    newf = best_nearby(delta, fbefore);
    /* if we made some improvements, pursue that direction */
    keep = 1;
    while ((newf < fbefore) && (keep == 1)) {
      for (i = 0; i < numvar; i++) {
        /* firstly, arrange the sign of delta[] */
        if (newx[i] <= xbefore[i])
          delta[i] = 0.0 - fabs(delta[i]);
        else
          delta[i] = fabs(delta[i]);
      }

      lineS->DoLineseek(xbefore, newx, newf, net);
      xbefore = newx;
      fbefore = newf;
      newf = lineS->GetBestF();
      newx = lineS->GetBestX();

      /* if the further (optimistic) move was bad */
      if (newf >= fbefore)
        break;

      /* else, look around from that point */
      xbefore = newx;
      fbefore = newf;
      newf = best_nearby(delta, fbefore);

      /* if the further (optimistic) move was bad */
      if (newf >= fbefore)
        break;

      /* make sure that the differences between the new and the old */
      /* points are due to actual displacements - beware of roundoff */
      /* errors that might cause newf < fbefore */
      keep = 0;
      for (i = 0; i < numvar; i++) {
        keep = 1;
        if (fabs(newx[i] - xbefore[i]) > (0.5 * fabs(delta[i])))
          break;
        else
          keep = 0;
      }
    }

    if ((steplength >= val.epsilon) && (newf >= fbefore)) {
      steplength = steplength * val.rho;
      for (i = 0; i < numvar; i++)
        delta[i] *= val.rho;
    }
  }

  if (iters >= val.maxiter)
    cout << "\nHooke and Jeeves optimisation completed after " << iters
      << " iterations (max " << val.maxiter << ")\nThe model terminated "
      << "because the maximum number of iterations was reached\n";

  if (newf > fbefore) {
    newx = xbefore;
    newf = fbefore;
  }
  delete lineS;
  f = newf;
  x = newx;
  net->setBestX(x);
  return (iters);
}

int hooke::SetPoint(int n, double flast) {
  // return 0 and do nothing if the changes goes out of bounds.
  z = newx;
  double next = newx[n] + delta[n];

  if (next < lbd[n])
    return 0;
  else if (next > ubd[n])
    return 0;
  else {
    z[n] = next;
    net->setX(z);
    par[nsent] = n;
    fbefore[nsent] = flast;
    nsent++;
    return 1;
  }
}

// ********************************************************
// functions for class bfgs
// ********************************************************
struct bfgscon {
  int SHANNONSCALING;
  int DIFFICULTGRAD;
  int BFGSPAR;
};

struct mainbfgscon{
  int MAXBFGSITER;
  double ERRORTOL;
  double XTOL;
  int MAXROUNDS;
  int PRINTING;
};

bfgscon readbfgsConstants() {
  char letter;
  int i = 0;
  char text[MaxStrLength];
  bfgscon tmp;

  ifstream bffile("bfgsconstants", ios::in|ios::binary|ios::nocreate);
  if (!bffile) {
    cerr << "Unable to open inputfile bfgsconstants\n";
    exit(EXIT_FAILURE);
  }

  while (!bffile.eof()) {
    bffile >> text;
    if (strcasecmp(text, ";") == 0) {
      bffile.get(letter);
      while ((letter != '\n') && !bffile.eof())
        bffile.get(letter);

    } else if (strcasecmp(text, "//") == 0) {
      bffile.get(letter);
      while ((letter != '\n') && !bffile.eof())
        bffile.get(letter);

    } else {
      if (strcasecmp(text, "SHANNONSCALING") == 0) {
        bffile >> tmp.SHANNONSCALING;
        i++;
      }
      if (strcasecmp(text, "DIFFICULTGRAD") == 0) {
        bffile >> tmp.DIFFICULTGRAD;
        i++;
      }
      if (strcasecmp(text, "BFGSPAR") == 0) {
        bffile >> tmp.BFGSPAR;
        i++;
      }
    }
  }

  bffile.close();
  if (i != 3) {
    cerr << "Error in reading from input file bfgsconstants\n";
    exit(EXIT_FAILURE);
  }
  return(tmp);
}

mainbfgscon getMainbfgsConstants() {
  char letter;
  int i = 0;
  char text[MaxStrLength];
  mainbfgscon tmp;

  ifstream bfgsfile("bfgsconstants", ios::in|ios::binary|ios::nocreate);
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
      while ((letter !=  '\n') && !bfgsfile.eof())
        bfgsfile.get(letter);
    } else {
      if (strcasecmp(text, "MAXBFGSITER") == 0) {
        bfgsfile >> tmp.MAXBFGSITER;
        i++;
      }
      if (strcasecmp(text, "ERRORTOL") == 0) {
        bfgsfile >> tmp.ERRORTOL;
        i++;
      }
      if (strcasecmp(text, "XTOL") == 0) {
        bfgsfile >> tmp.XTOL;
        i++;
      }
      if (strcasecmp(text, "MAXROUNDS") == 0) {
        bfgsfile >> tmp.MAXROUNDS;
        i++;
      }
      if (strcasecmp(text, "PRINTING") == 0) {
        bfgsfile >> tmp.PRINTING;
        i++;
      }
    }
  }

  bfgsfile.close();
  if (i != 5) {
    cerr << "Error in reading from input file bfgsconstants\n";
    exit(EXIT_FAILURE);
  }
  return(tmp);
}

bfgs::bfgs(netInterface* netC, gradient* g, armijo* lineseek) {
  net = netC;
  numberOfVariables = net->getNumOfVarsInDataGroup();
  vector temp(numberOfVariables);
  x = net->getInitialX();
  lineS = lineseek;                       // use lineS to do linesearch
  grad = g;                               // use grad to compute gradient
  deltax = temp;                          // xi-xim1
  h = temp;                               // the line search vector
  gim1 = temp;                            // store previous gradient
  invhess = new double* [numberOfVariables];
  int i;
  for (i = 0; i < numberOfVariables; i++)
    invhess[i] = new double[numberOfVariables];

  xopt = temp;
}

bfgs::~bfgs() {
  int i;
  for (i = 0; i < numberOfVariables; i++)
    delete [] invhess[i];
  delete [] invhess;
}

int bfgs::iteration(int maxit, double errortol, double xtol, int iterno) {
  iter = iterno;
  int ifail = 0;
  int i;
  alpha = 0.0;                 // distance in line search and derivative
  double prevy = 1e69;         // previous function value
  double error = 2*errortol;   // scaled gradient norm, to be compared with tolerance
  double relchng = 1.0;        // relative change in function valuedouble
  normdeltax = 1.0;

  if (iter == 0) {
    // For the first iteration, define the parameters
    // and the Hessian matrix, the gradient and the direction vector

    bfgscon par;
    par = readbfgsConstants();
    ShannonScaling = par.SHANNONSCALING;
    difficultgrad = par.DIFFICULTGRAD;
    bfgs_constant = par.BFGSPAR;
    armijoproblem = 0;
    grad->initializeDiaghess();
    ComputeGradient();
    y = grad->getBaseF_x();
    normgrad = grad->getNormGrad();

    if (difficultgrad >= 1)
      diaghess = grad->getDiagonalHess();

    gi = grad->getGradient();
    normx = norm();

    DefineHessian();
    ComputeDirectionVector();
    dery = 0.0;
    normh = 0.0;
    for (i = 0; i < numberOfVariables; i++) {
      normh += h[i] * h[i];
      dery += gi[i] * h[i];
    }

    if (dery > 0) {
      cout << "Error in BFGS - the derivative is positive\n";
      ifail = 4;
    }
    error = normgrad / (1.0 + absolute(y));

  } else {
    // Loop over BFGS iterations
    // continue until tolerance criterion is satisfied
    // or give up because x's do not move -- only valid if function does not
    // change either and must do one iteration

    ComputeDirectionVector();
    dery = 0.0;
    normh = 0.0;
    for (i = 0; i < numberOfVariables; i++) {
      normh += h[i] * h[i];
      dery += gi[i] * h[i];
    }

    // if scaling of direction vector is to be done, then do it here.
    if (iter <= numberOfVariables && ShannonScaling == 1)
      ScaleDirectionVector();

    if (dery > 0) {
      cout << "Error in BFGS - the derivative is positive\n";
      ifail = 4;
    }
    error = normgrad / (1.0 + absolute(y));
  }

  doLineseek();
  if (lineS->GetPower() != -1)
    alpha = min(s, 1.0) * pow(lineS->GetBeta(), lineS->GetPower());

  //JMB - changed to check if a double is very close to zero
  if (absolute(alpha) < 1e-100) {
    armijoproblem += 1;
    difficultgrad += 1;
    ifail = -2;
    if (armijoproblem > 1)
     ifail = 6;
  }

  if (alpha < 0.0) {
    // negative alpha indicates wrong derivative - Hessian wrong
    ifail = 1;
  } else {
    UpdateXandGrad();
    prevy = y;
    ComputeGradient();
    y = grad->getBaseF_x();
    normgrad = grad->getNormGrad();
    gi = grad->getGradient();
    if (difficultgrad >= 1)
      diaghess = grad->getDiagonalHess();

    error = normgrad / (1.0 + absolute(y));
    if (ifail != 6 && ifail != -2) {
      if (bfgs_constant == 1) {
        int update = bfgsUpdate();
        if (update == 5)
          ifail = update;
      }
    }
    normx = sqrt(normx);
    relchng = (prevy - y) / (1.0 + absolute(prevy));
  }
  net->setBestX(x);

  f = y;
  if (ifail > 0)
    return(ifail);

  if (error <= errortol)
    ifail= 0;
  else if (normdeltax < xtol && ifail != -2)
    ifail= 2;
  else
    ifail= -1;

  return(ifail);
}

void bfgs::ComputeGradient() {
  int i;
  i = grad->computeGradient(net, x, difficultgrad);
  int tmp = grad->getDifficultGrad();
  difficultgrad = tmp;
  if (i == net->NET_ERROR()) {
    cerr << "Error in BFGS - did not receive gradient data\n";
    cout << "Error in BFGS - stop netcommunication - last x was\n";
    cout << net->unscaleX(x);
    net->stopNetComm();
    exit(EXIT_FAILURE);
  }
}

void bfgs::doLineseek() {
  s = -1e10;
  double low = GetS(0);
  double upp = GetS(1);
  if (upp <= 0.0) {
    cout << "Warning in BFGS - upperbound for alpha is negative\n";
    upp = 1.0;
  }
  s = upp;
  if (ShannonScaling == 1) {
    if (iter == 0 || upp < 1.0)
      s = upp;
    else
      s = 1.0;
  }
  lineS->DoArmijo(x, y, dery, h, net, min(s, 1.0));
}

void bfgs::ScaleDirectionVector() {
  int i;
  dery *= normdeltax / normh;
  for (i = 0; i < numberOfVariables; i++)
    h[i] *= normdeltax / normh;
}

void bfgs::ComputeDirectionVector() {
  int i, j;
  for (i = 0; i < numberOfVariables; i++) {
    h[i] = 0;
    for (j = 0; j < numberOfVariables; j++)
      h[i] -= invhess[i][j] * gi[j];
  }
}

void bfgs::DefineHessian() {
  int i, j;

  for (i = 0; i < numberOfVariables; i++) {
    // start with the identity matrix
    for (j = 0; j < numberOfVariables; j++)
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
      cout << "Error in BFGS - negative inverse hessian\n";
      break;
    }
  }
}

void bfgs::UpdateXandGrad() {
  int i;
  double xi;
  normdeltax = 0.0;
  for (i = 0; i < numberOfVariables; i++) {
    //store the old gradient
    gim1[i] = gi[i];
    xi = x[i];
    x[i] += alpha * h[i];
    deltax[i]=x[i] - xi;
    normdeltax += deltax[i] * deltax[i];
  }
}

int bfgs::bfgsUpdate() {
  int i, j;

   /* prepare the BFGS update */
  double deltaxg = 0.0;
  double deltaghg = 0.0;
  vector hg(numberOfVariables);
  vector deltag(numberOfVariables);
  normx = 0.0;

  for (i = 0; i < numberOfVariables; i++) {
    hg[i] = 0.0;
    deltag[i] = gi[i] - gim1[i];
    deltaxg += deltax[i] * deltag[i];
    normx += x[i] * x[i];
  }

  if (deltaxg <= 0.0)
    cout << "Warning in BFGS - instability error\n";

  for (i = 0; i < numberOfVariables; i++) {
    for (j = 0; j < numberOfVariables; j++)
      hg[i] += invhess[i][j] * deltag[j];

    deltaghg += deltag[i] * hg[i];
  }

  /* do the BFGS update */
  for (i = 0; i < numberOfVariables; i++) {
    for (j = 0; j < numberOfVariables; j++) {
      invhess[i][j] += deltax[i] * deltax[j] / (deltaxg) - hg[i] * hg[j] /
        deltaghg + deltaghg * (deltax[i] / deltaxg - hg[i] / deltaghg) *
        (deltax[j] / deltaxg - hg[j] / deltaghg);
    }
  }

  if (deltaxg <= 0)
    return(5);
  else
    return(-999);
}

double bfgs::norm() {
  int i;
  double normx;
  normx = 0.0;
  for (i = 0; i < numberOfVariables; i++)
    normx += x[i] * x[i];
  normx = sqrt(normx);
  return(normx);
}

void bfgs::printResult() {
  ofstream outputfile;
  outputfile.open("finalvals");
  if (!outputfile) {
    cerr << "Error in BFGS - could not print finalvals\n";
    cout << "Stop netcommunication. Last x was:\n" << (net->unscaleX(x));
    net->stopNetComm();
    exit(EXIT_FAILURE);
  }
  outputfile << (net->unscaleX(x));
  outputfile.close();
}

void bfgs::printGradient() {
  ofstream outputfile;
  outputfile.open("gradient");
  if (!outputfile) {
    cerr << "Error in BFGS - could not print gradient\n";
    cout << "Stop netcommunication.  The gradient was: " << gi;
    net->stopNetComm();
    exit(EXIT_FAILURE);
  }
  outputfile << gi;
  outputfile.close();
}


void bfgs::printInverseHessian() {
  int i, j;
  ofstream outputfile;
  outputfile.open("hessian");
  if (!outputfile) {
    cerr << "Error in BFGS - could not print hessian\n";
    exit(EXIT_FAILURE);
  }
  for (i = 0; i < numberOfVariables; i++) {
    for (j = 0; j < numberOfVariables; j++)
      outputfile << invhess[i][j] << " ";
    outputfile << endl;
  }
  outputfile.close();
}

double bfgs::GetS(int get) {
  double b = 1.e69;
  double a = -1.e69;

  vector alpha_l(numberOfVariables);
  vector alpha_u(numberOfVariables);
  vector upper;
  vector lower;
  upper = net->getUpperScaleConstant();
  lower = net->getLowerScaleConstant();

  int i;
  for (i = 0; i < numberOfVariables; i++) {
    if (h[i] > 0) {
      alpha_l[i] = (lower[i] - x[i]) / h[i];
      alpha_u[i] = (upper[i] - x[i]) / h[i];
    } else if (h[i] < 0) {
      alpha_u[i] = (lower[i] - x[i]) / h[i];
      alpha_l[i] = (upper[i] - x[i]) / h[i];
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
    cout << "Error in BFGS - unrecognised return value\n";
    exit(EXIT_FAILURE);
  }
}

double bfgs::max(double a, double b) {
  if (a < b)
    return b;
  else
    return a;
}

double bfgs::min(double a, double b) {
  if (a < b)
    return a;
  else
    return b;
}

vector bfgs::GetBestX() {
  return x;
}

double bfgs::GetBestF() {
  return f;
}

void bfgs::printX() {
  cout << net->unscaleX(x);
}


minimizer::minimizer(netInterface* netInt) {
  net = netInt;
}

minimizer::~minimizer() {
}

int minimizer::DoSearch() {
  mainbfgscon par;
  par = getMainbfgsConstants();
  int maxiter = par.MAXBFGSITER;
  double errortol = par.ERRORTOL;
  double xtol = par.XTOL;
  int maxrounds = par.MAXROUNDS;

  vector lausn;
  int i, counter, bfgsFail;
  int rounds = 0;

  gradient* grad = new netGradient(net->getNumOfVarsInDataGroup());
  armijo* lines = new armijo();
  min = new bfgs(net, grad, lines);
  for (rounds = 0; rounds < maxrounds; rounds++) {
    i = 0;
    bfgsFail = -1;
    counter = 0;

    vector temp = net->getInitialX();
    while ((i < maxiter) && (bfgsFail < 0)) {
      bfgsFail = min->iteration(1, errortol, xtol, i);
      i++;
    }
    if (bfgsFail == 0)
      rounds = maxrounds;

    cout << min->GetBestX();
    cout << "f is: " << min->GetBestF() << endl;
    net->setBestX(min->GetBestX());
  }

  if (par.PRINTING) {
    min->printGradient();
    //min->printResult();
    min->printInverseHessian();
  }

  f = min->GetBestF();
  delete min;
  delete grad;
  delete lines;
  return 1;
}
