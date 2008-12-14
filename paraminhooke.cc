#include "paraminhooke.h"

// ********************************************************
// functions for class ParaminHooke
// ********************************************************
ParaminHooke::ParaminHooke(NetInterface* netInt) : ParaminSearch(netInt) {
  iters = 0;
  returnID = -1;
  lambda = 0;
  rho = 0.5;
  epsilon = 1e-4;
  maxiterations = 1000;
  // already set in optinfo
  // converge = 0;
}

ParaminHooke::~ParaminHooke() {
}

void ParaminHooke::read(CommentStream& infile, char* text)  {
  int i = 0;

  while (!infile.eof() && strcasecmp(text, "seed") && strcasecmp(text, "[hooke]") && strcasecmp(text, "[bfgs]")) {
    infile >> ws;
    if ((strcasecmp(text, "epsilon") == 0) || (strcasecmp(text, "hookeeps") == 0)) {
      infile >> epsilon;

    } else if ((strcasecmp(text, "maxiterations") == 0) || (strcasecmp(text, "hookeiter") == 0)) {
      infile >> maxiterations;

    } else if (strcasecmp(text, "rho") == 0) {
      infile >> rho;

    } else if (strcasecmp(text, "lambda") == 0) {
      infile >> lambda;

    } else if (strcasecmp(text, "bndcheck") == 0) {
      //JMB - read and ignore bndcheck
      infile >> text;

    } else {
      cerr << "Error while reading optinfo for Hooke - unknown option " << text << endl;
      exit(EXIT_FAILURE);
    }
    infile >> text;
    i++;
  }

  if (i == 0)
    cerr << "Warning - no optinfo give for Hooke in file - using default parameters" << endl;

  //check the values specified in the optinfo file ...
  if ((rho < 0) || (rho > 1)) {
    cerr << "\nError in value of rho - setting to default value of 0.5\n";
    rho = 0.5;
  }
  if ((lambda < 0) || (lambda > 1)) {
    cerr << "\nError in value of lambda - setting to default value of " << rho << endl;
    lambda = rho;
  }
}

void ParaminHooke::OptimiseLikelihood() {
  double steplength;
  int i, keep;
  
  NumberOfHosts = net->getTotalNumProc();
  int maxnumindatagroup = numvar*10;
  // maybe not needed can do resize on the go????
  // Vector tempV(maxnumindatagroup);
  //previousf = tempV;
  previousf.Reset();
  previousf.resize(maxnumindatagroup, 0.0);
  //par = new int[maxnumindatagroup];
  par.resize(maxnumindatagroup, 0);
  xbefore = net->getInitialX();
  fbefore = net->getScore();
  bestx = xbefore;
  bestf = fbefore;
 
  cout << "in opt hooke and Jeeves best x is " << endl;
  for (i = 0; i < bestx.Size(); i++)  {
    cout << bestx[i] << " ";
  };
  cout << endl;
  cout << "in opt hooke and jeeves best f is " << bestf << endl;
 
  delta.Reset();
  delta.resize(numvar, 0.0);
  int numFromLineSeek;
  // change = new int[numvar];
  //  param = new int[numvar];
  change.Reset();
  change.resize(numvar, 0);
  param.Reset();
  param.resize(numvar,0);
  // The original definition of the delta array has not been changed, even
  // though we're sometimes working with scaled x-values.
  for (i = 0; i < numvar; i++) {
    delta[i] = fabs(bestx[i] * rho);
    if (delta[i] == 0.0)
      delta[i] = rho;
    param[i] = i;
  }

  steplength = ((lambda < verysmall) ? rho : lambda);
  
  lineS = new LineSeeker();
 
  while ((iters < maxiterations) && (steplength > epsilon)) {
    /* randomize the order of the parameters once in a while, to avoid */
    /* the order having an influence on which changes are accepted.    */
      
    randomOrder(param);

    /* find best new point, one coord at a time */
    bestNearby();

    //JMB check for too many iterations here
    
    /* if we made some improvements, pursue that direction */
    keep = 1;
    while ((bestf < fbefore) && (keep == 1) && (iters < maxiterations)) {
	// cout << "some improvement from bestNearby" << endl;
      for (i = 0; i < numvar; i++) {
        /* firstly, arrange the sign of delta[] */
        if (bestx[i] <= xbefore[i])
          delta[i] = 0.0 - fabs(delta[i]);
        else
          delta[i] = fabs(delta[i]);
      }

      numFromLineSeek = lineS->doLineseek(xbefore, bestx, bestf, net);
      iters += numFromLineSeek;
      xbefore = bestx;
      fbefore = bestf;
      bestf = lineS->getBestF();
      cout << "hooke and jeeves best f " << bestf << endl;
      bestx = lineS->getBestX();

      // if the further (optimistic) move was bad
      if (bestf >= fbefore) {
        bestf = fbefore;
        bestx = xbefore;

      } else {
        // else, look around from that point
        if (iters < maxiterations) {
          xbefore = bestx;
          fbefore = bestf;
          bestNearby();
          /* if the further (optimistic) move was bad */
          if (bestf < fbefore) {
            // bestf can only be equal or less than  fbefore
            /* make sure that the differences between the new and the old */
            /* points are due to actual displacements - beware of roundoff */
            /* errors that might cause newf < fbefore */
            keep = 0;
            for (i = 0; i < numvar; i++) {
              keep = 1;
              // AJ changing to be the same check as in gadget..
              if (fabs(bestx[i] - xbefore[i]) > rathersmall)
                break;
              else
                keep = 0;
            }
          }
        }
      }
    }

    if ((steplength >= epsilon) && (bestf >= fbefore)) {
      steplength = steplength * rho;
      for (i = 0; i < numvar; i++)
        delta[i] *= rho;
    }
  }
  // Must look into this cout..  things.. should be handle...
  cout << "\nStopping Hooke and Jeeves\n\nThe optimisation stopped after " << iters
    << " iterations (max " << maxiterations << ")\nThe steplength was reduced to "
    << steplength << " (min " << epsilon << ")\n";

  if (iters >= maxiterations)
    cout << "The optimisation stopped because the maximum number of iterations" << "\nwas reached and NOT because an optimum was found for this run\n";
  else {
    cout << "The optimisation stopped because an optimum was found for this run\n";
    converge = 1;
  }
  delete lineS;
  // delete[] change;
  // delete[] param;
  // delete[] par;
  net->setInitialScore(bestx, bestf);
  score = bestf;
}

// JMB - CONDOR version is different ...
void ParaminHooke::bestNearby() {
  double ftmp;
  int i, j;
  int withinbounds;
  int newopt;
  net->startNewDataGroup(10 * numvar);
 
  for (i = 0; i < numvar; i++)
    change[i] = 0;
  numparamset = 0;
  i = 0;
  // sending as many points as there are processors in total.
  // ************
  // AJ if never within bound then will set numvar points!!!!
  // 

  while ((i < NumberOfHosts) && (numparamset < numvar)) {
    withinbounds = SetPoint(param[numparamset]);
    if (withinbounds == 1) {
      change[param[numparamset]]++;
      i++;
    }
    numparamset++;
  }
  // send all available data to all free hosts and then receive them
  // back one at a time, sending new points when possible. If point I was
  // trying to set was not within bound then nothing has been set and
  // nothing will be sent. This could lead to poor use of available hosts.
  net->sendToAllIdleHosts();
  while (net->getNumNotAns() > 0) {
    ReceiveValue();
    if (iters % 1000 == 0) {
	cout << "\nAfter " << iters << " function evaluations, f(x) = " << bestf << " at\n"; 
	  for (j = 0; j < bestx.Size(); j++) 
	      cout << bestx[j] << sep;
      cout << endl;
    }
    if (iters < maxiterations) {
      if (MyDataGroup()) {
        // Update the optimum if received a better value
        newopt = UpdateOpt();
        if (!newopt) {
          SetPointNearby();
	};
      }
      if (net->allSent())
        SetPointNextCoord();
      if (!net->allSent())
        net->sendToIdleHosts();

    } else {
      net->receiveAll();
      // This takes time, ?? need to do this
      // since I am receiving all, maybe should check if any
      // of the return values are optimum.
    }
  }
  net->stopUsingDataGroup();
}

int ParaminHooke::SetPoint(int n) {
  // return 0 and do nothing if the changes goes out of bounds.
  DoubleVector z(bestx);
  double next = bestx[n] + delta[n];
  int numset;
 
  if (next < lowerbound[n]) {
      
      return 0;
  }
  else if (next > upperbound[n]) {
      
      return 0;
  }
  else {
      
    numset = net->getNumDataItemsSet();
    z[n] = next;
    net->setX(z);
    par[numset] = n;
    previousf[numset] = bestf;
    return 1;
  }
}

int ParaminHooke::MyDataGroup() {
  return (returnID >= 0);
}

void ParaminHooke::ReceiveValue() {
  int receive = net->receiveOne();
  if (receive == net->netError()) {
    cerr << "Error in Hooke - failed to receive data in linesearch\n";
    net->stopUsingDataGroup();
    exit(EXIT_FAILURE);
  }
  returnID = net->getReceiveID();
  if (MyDataGroup()) {
    iters++;
    freceive = net->getY(returnID);
  }
}

int ParaminHooke::UpdateOpt() {
  int newopt = 0;
  if (freceive < bestf) {
    bestf = freceive;
    bestx = net->getX(returnID);
    newopt = 1;
  }
  return newopt;
}

void ParaminHooke::SetPointNearby()  {
  int withinbounds = 0;
  int returnparam;
  returnparam = par[returnID];
  if (change[returnparam] == 1) {
    if (freceive < previousf[returnID])
      withinbounds = SetPoint(returnparam);
    else {
      delta[returnparam] = 0.0 - delta[returnparam];
      withinbounds = SetPoint(returnparam);
      if (withinbounds)
        change[returnparam]++;
    }
  }
}

void ParaminHooke::SetPointNextCoord()  {
  int withinbounds = 0;
  while ((numparamset < numvar) && withinbounds == 0) {
    withinbounds = SetPoint(param[numparamset]);
    if (withinbounds)
      change[param[numparamset]]++;
    numparamset++;
  }
}
void ParaminHooke::Print(ofstream& outfile, int prec) {
 outfile << "; Hooke & Jeeves algorithm ran for " << iters
    << " function evaluations\n; and stopped when the likelihood value was "
    << setprecision(prec) << score;
  if (converge == -1)
    outfile << "\n; because an error occured during the optimisation\n";
  else if (converge == 1)
    outfile << "\n; because the convergence criteria were met\n";
  else
    outfile << "\n; because the maximum number of function evaluations was reached\n";
}
