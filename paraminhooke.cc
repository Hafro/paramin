#include "paraminhooke.h"

// ********************************************************
// functions for class ParaminHooke
// ********************************************************
ParaminHooke::ParaminHooke(NetInterface* netInt) : ParaminSearch(netInt) {
  numiters = 0;
  returnId = -1;
  lambda = 0;
  rho = 0.5;
  epsilon = 1e-4;
  maxiterations = 1000;
}

ParaminHooke::~ParaminHooke() {
}

void ParaminHooke::Read(CommentStream& infile, char* text)  {
  int i = 0;
  infile >> text >> ws;
  while (!infile.eof() && !(strcasecmp(text, "[simann]") == 0) && !(strcasecmp(text, "[bfgs]") == 0) && !(strcasecmp(text, "seed") == 0)) {

    if ((strcasecmp(text, "epsilon") == 0) || (strcasecmp(text, "hookeeps") == 0)) {
      infile >> epsilon >> ws;

    } else if ((strcasecmp(text, "maxiterations") == 0) || (strcasecmp(text, "hookeiter") == 0)) {
      infile >> maxiterations >> ws;

    } else if (strcasecmp(text, "rho") == 0) {
      infile >> rho >> ws;

    } else if (strcasecmp(text, "lambda") == 0) {
      infile >> lambda >> ws;

    } else if (strcasecmp(text, "bndcheck") == 0) {
      //JMB - read and ignore bndcheck
      infile >> text >> ws;

    } else {
      cerr << "Error while reading optinfo for Hooke - unknown option " << text << endl;
      exit(EXIT_FAILURE);
    }
    i++;
    infile >> text >> ws;
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

void ParaminHooke::doSearch(const vector& startx, double startf) {
  double steplength;
  int i, keep;

  NumberOfHosts = net->getTotalNumProc();
  int maxnumindatagroup = numvar*10;
  vector tempV(maxnumindatagroup);
  previousf = tempV;
  par = new int[maxnumindatagroup];
  xbefore = startx;
  fbefore = startf;
  bestx = startx;
  bestf = startf;
  vector tempVec(numvar);
  delta = tempVec;
  int numFromLineSeek;
  change = new int[numvar];
  param = new int[numvar];
  // The original definition of the delta array has not been changed, even
  // though we're sometimes working with scaled x-values.
  for (i = 0; i < numvar; i++) {
    delta[i] = absolute(bestx[i] * rho);
    if (delta[i] == 0.0)
      delta[i] = rho;
    param[i] = i;
  }

  steplength = ((lambda < verysmall) ? rho : lambda);
  lineS = new LineSeeker();

  while ((numiters < maxiterations) && (steplength > epsilon)) {
    /* randomize the order of the parameters once in a while, to avoid */
    /* the order having an influence on which changes are accepted.    */
    randomOrder(param);

    /* find best new point, one coord at a time */
    bestNearby();

    //JMB check for too many iterations here
    
    /* if we made some improvements, pursue that direction */
    keep = 1;
    while ((bestf < fbefore) && (keep == 1) && (numiters < maxiterations)) {
      for (i = 0; i < numvar; i++) {
        /* firstly, arrange the sign of delta[] */
        if (bestx[i] <= xbefore[i])
          delta[i] = 0.0 - absolute(delta[i]);
        else
          delta[i] = absolute(delta[i]);
      }

      numFromLineSeek = lineS->doLineseek(xbefore, bestx, bestf, net);
      numiters += numFromLineSeek;
      xbefore = bestx;
      fbefore = bestf;
      bestf = lineS->getBestF();
      bestx = lineS->getBestX();

      // if the further (optimistic) move was bad
      if (bestf >= fbefore) {
        bestf = fbefore;
        bestx = xbefore;

      } else {
        // else, look around from that point
        if (numiters < maxiterations) {
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
              if (absolute(bestx[i] - xbefore[i]) > rathersmall)
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

  cout << "\nStopping Hooke and Jeeves\n\nThe optimisation stopped after " << numiters
    << " iterations (max " << maxiterations << ")\nThe steplength was reduced to "
    << steplength << " (min " << epsilon << ")\n";

  if (numiters >= maxiterations)
    cout << "The optimisation stopped because the maximum number of iterations" << "\nwas reached and NOT because an optimum was found for this run\n";
  else
    cout << "The optimisation stopped because an optimum was found for this run\n";

  delete lineS;
  delete[] change;
  delete[] param;
  delete[] par;
  net->setBestX(bestx);
}

// JMB - CONDOR version is different ...
void ParaminHooke::bestNearby() {
  double ftmp;
  int i;
  int withinbounds;
  int newopt;
  net->startNewDataGroup(10 * numvar);

  for (i = 0; i < numvar; i++)
    change[i] = 0;
  numparamset = 0;
  i = 0;
  // sending as many points as there are processors in total.
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
    if (numiters % 1000 == 0)
      cout << "\nAfter " << numiters << " function evaluations, f(x) = " << bestf << " at\n" << bestx;

    if (numiters < maxiterations) {
      if (MyDataGroup()) {
        // Update the optimum if received a better value
        newopt = UpdateOpt();
        if (!newopt)
          SetPointNearby();
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
  vector z(bestx);
  double next = bestx[n] + delta[n];
  int numset;

  if (next < lowerbound[n])
    return 0;
  else if (next > upperbound[n])
    return 0;
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
  return (returnId >= 0);
}

void ParaminHooke::ReceiveValue() {
  int receive = net->receiveOne();
  if (receive == net->netError()) {
    cerr << "Error in Hooke - failed to receive data in wolfe\n";
    net->stopUsingDataGroup();
    exit(EXIT_FAILURE);
  }
  returnId = net->getReceiveId();
  if (MyDataGroup()) {
    numiters++;
    freceive = net->getY(returnId);
  }
}

int ParaminHooke::UpdateOpt() {
  int newopt = 0;
  if (freceive < bestf) {
    bestf = freceive;
    bestx = net->getX(returnId);
    newopt = 1;
  }
  return newopt;
}

void ParaminHooke::SetPointNearby()  {
  int withinbounds = 0;
  int returnparam;
  returnparam = par[returnId];
  if (change[returnparam] == 1) {
    if (freceive < previousf[returnId])
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
