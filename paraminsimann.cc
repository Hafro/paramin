#include "paraminsimann.h"

// ********************************************************
// functions for class ParaminSimann
// ********************************************************
ParaminSimann::ParaminSimann(NetInterface* netInt) : ParaminSearch(netInt) {
  type = OPTSIMANN;
  maxim = 0;
  T = 100.0;
  cs = 2.0;

  // Vector tempVec(numvar);
  // vm = tempVec;
  vm.resize(numvar, 1.0);
      // vm.setValue(1.0);
      // xp = tempVec;
  xp.resize(numvar, 0.0);
  // changed to iters...
  // nfcnev = 0;
  nt = 2;
  ns = 5;
  check = 4;
  uratio = 0.7;
  lratio = 0.3;
  rt = 0.85;
  eps = 1e-4;
  maxiterations = 2000;
  /// NEED TO check if 0 is OK or maybe -1????
  // ID = new int[numvar];
  // nacp = new int[numvar];
  // acpPointID = new int[numvar];
  ID.resize(numvar, 0);
  nacp.resize(numvar, 0);
  acpPointID.resize(numvar, 0);
  // total number of processes initiated at beginning
  NumberOfHosts = net->getTotalNumProc();
  // converged = 0;
}

ParaminSimann::~ParaminSimann() {
    // delete[] ID;
    // delete[] nacp;
    // delete[] acpPointID;
}

void ParaminSimann::read(CommentStream& infile, char* text) {
  int i = 0;
  int j = 0;
  double temp;
 
  while (!infile.eof() && strcasecmp(text, "seed") && strcasecmp(text, "[hooke]") && strcasecmp(text, "[bfgs]")) {
    infile >> ws;
    if (strcasecmp(text, "ns") == 0) {
      infile >> ns;

    } else if (strcasecmp(text, "nt") == 0) {
      infile >> nt;

    } else if (strcasecmp(text, "check") == 0) {
      infile >> check;

    } else if (strcasecmp(text, "rt") == 0) {
      infile >> rt;

    } else if ((strcasecmp(text, "eps") == 0) || (strcasecmp(text, "simanneps") == 0)) {
      infile >> eps;

    } else if (strcasecmp(text, "t") == 0) {
      infile >> T;

    } else if (strcasecmp(text, "cstep") == 0) {
      infile >> cs;

    } else if (strcasecmp(text, "uratio") == 0) {
      infile >> uratio;

    } else if (strcasecmp(text, "lratio") == 0) {
      infile >> lratio;

    } else if (strcasecmp(text, "vm") == 0) {
      infile >> temp;
      for (j = 0; j < vm.Size(); j++)
	  vm[j] = temp;

    } else if ((strcasecmp(text, "maxiterations") == 0) || (strcasecmp(text, "simanniter") == 0)) {
      infile >> maxiterations;

    } else {
      cerr << "Error while reading optinfo for Simulated Annealing - unknown option " << text << endl;
      exit(EXIT_FAILURE);
    }
    infile >> text;
    i++;
  }

  if (i == 0)
    cerr << "Warning - no optinfo give for Simulated Annealing in file - using default parameters" << endl;

  //check the values specified in the optinfo file ...
  if ((uratio < 0.5) || (uratio > 1)) {
    cerr << "\nError in value of uratio - setting to default value of 0.7\n";
    uratio = 0.7;
  }
  if ((lratio < 0) || (lratio > 0.5)) {
    cerr << "\nError in value of lratio - setting to default value of 0.3\n";
    lratio = 0.3;
  }
  if ((rt < 0) || (rt > 1)) {
    cerr << "\nError in value of rt - setting to default value of 0.85\n";
    rt = 0.85;
  }
  cs = cs / lratio;
}

void ParaminSimann::OptimiseLikelihood() {
  int i, numtoset;
  int numloops_ns, numloops_nt;
  int numset_nsloop;     // 0 < numset_nsloop <= numvar
  int rock = 0;

  bestx = net->getInitialX();
  xstart = bestx;
  for (i = 0; i < numvar; i++) {
      if (xstart[i] > upperbound[i] || xstart[i] < lowerbound[i]) {
	  cerr << "Error in Simmulated Annealing - x is not within bounds\n";
	  exit(EXIT_FAILURE);
      }
  }
  initialVM = vm;

  bestf = net->getScore();
  
  if (!maxim)
    bestf = -bestf;

  // Vector tempVec(check);
  // tempVec.setValue(bestf);
  fstar.Reset();
  fstar.resize(check, bestf);
  // fstar = tempVec;
  fstart = bestf;

  for (i = 0; i < numvar; i++) {
    ID[i] = i;
    nacp[i] = 0;
    acpPointID[i] = -1;
  }
  // Find out how many values to set at beginning of each ns loop.
  if (numvar > NumberOfHosts)
    numtoset = NumberOfHosts;
  else
    numtoset = numvar;

  // start the main loop.  Note that it terminates if (i) the algorithm
  // succesfully otimizes the function or (ii) there are too many func. eval.
  while ((rock == 0) && (iters < maxiterations)) {
    numloops_nt = 0;
    net->startNewDataGroup((ns * nt * (numvar + 1)));
    while ((numloops_nt < nt) && (iters < maxiterations)) {
      numloops_ns = 0;
      naccepted_nsloop = 0;
      randomOrder(ID);

      while ((numloops_ns < ns) && (iters < maxiterations)) {
        if (numloops_ns == 0) {
          for (i = 0; i < numtoset; i++)
            SetXP(ID[i]);
          numset_nsloop = numtoset;
        } else
          numset_nsloop = 0;

        // sends all available data to all free hosts.
        net->sendToAllIdleHosts();
        while ((numset_nsloop < numvar) && (iters < maxiterations)) {
          // get a function value and do any update that's necessary.
          ReceiveValue();
          if (iters < maxiterations) {
            SetXP(ID[numset_nsloop]);
            numset_nsloop++;
          }
        }

        numloops_ns++;
      }

      if (iters < maxiterations) {
        while ((net->getNumNotAns() > 0) && (iters < maxiterations)) {
          // get a function value and do any update that's necessary
          ReceiveValue();
        }
        // adjust vm so approximately half of all evaluations are accepted.
        UpdateVM();

      } else
        i = net->receiveAll();  //Why? this takes time

      numloops_nt++;
    }

    net->stopUsingDataGroup();
    for (i = 0; i < numvar; i++)
      acpPointID[i] = -1;

    // check termination criteria.
    for (i = check - 1; i > 0; i--)
      fstar[i] = fstar[i - 1];
    fstar[0] = fstart;

    rock = 0;
    if (fabs(bestf - fstart) < eps) {
      rock = 1;
      for (i = 0; i < check - 1; i++)
        if (fabs(fstar[i + 1] - fstar[i]) > eps)
          rock = 0;
    }

    cout << "\nChecking convergence criteria after " << iters << " function evaluations ...\n";

    // if termination criteria is not met, prepare for another loop.
    if (rock == 0) {
      T *= rt;
      cout << "Reducing the temperature to " << T << endl;
	  bestf = -bestf;
      cout << "simann best result so far " << bestf << endl;
	  bestf = -bestf;
      fstart = bestf;
      xstart = bestx;
    }
  }

  // Either reached termination criteria or maxiterations or both
  if (!maxim)
    bestf = -bestf;

  net->setInitialScore(bestx, bestf);
  score = bestf;
	// Breytti rock == 0 í stað rock == 1
  if ((iters >= maxiterations) && (rock == 0)) {
    cout << "\nSimulated Annealing optimisation completed after " << iters
      << " iterations (max " << maxiterations << ")\nThe model terminated "
      << "because the maximum number of iterations was reached\n";
  } else {
    cout << "\nStopping Simulated Annealing\n\nThe optimisation stopped after " << iters
      << " function evaluations (max " << maxiterations << ")\nThe optimisation stopped "
      << "because an optimum was found for this run\n";
    converge = 1;
  }
}

// generate xp, the trial value of x - note use of vm to choose xp.
void ParaminSimann::SetXP(int k) {
  int i;
  DoubleVector temp;
  int id;
  for (i = 0; i < numvar; i++) {
    if (i == k) {
      xp[k] = xstart[k] + ((randomNumber() * 2.0) - 1.0) * vm[k];
      if (xp[k] < lowerbound[k] || xp[k] > upperbound[k]) {
	while((xp[k] < lowerbound[k] || xp[k] > upperbound[k])) {
	  xp[k] = xstart[k] + ((randomNumber() * 2.0) - 1.0) * vm[k];
	}
	
      }
    } else {
      if (acpPointID[i] >= 0) {
        // Use parameter from x with id = nacp_ns[i] which was accepted earlier
        temp = net->getX(acpPointID[i]);
        xp[i] = temp[i];
      } else
        xp[i] = xstart[i];
    }
  }
  
  if (!net->dataGroupFull()) {
    // net->setX(xp);          // this uses a FIFO stack
    net->setXFirstToSend(xp);  // this uses a LIFO stack
  } else {
    cerr << "During Simulated Annealing have set too many values" << endl;
    exit(EXIT_FAILURE);
  }
  net->sendToAllIdleHosts();
}

void ParaminSimann::AcceptPoint() {
  int i;
  DoubleVector temp;
  xstart = xp;
  fstart = fp;
  nacp[ID[returnID % numvar]]++;
  acpPointID[ID[returnID % numvar]] = returnID;
  naccepted_nsloop++;

  // if better than any other point record as new optimum.
  if (fp > bestf) {
    bestx = xp;
    cout << "\nNew optimum after " << iters << " function evaluations, f(x) = " << -fp << " at\n";
    temp = net->unscaleX(bestx);
    for (i = 0; i < temp.Size(); i++)
	cout << temp[i] << " ";
    bestf = fp;
  }
}

void ParaminSimann::UpdateVM() {
  int i;
  double ratio;

  // adjust vm so that approximately half of all evaluations are accepted.
  for (i = 0; i < numvar; i++) {
    ratio = (double) nacp[i] / ns;
    nacp[i] = 0;
    if (ratio > uratio) {
      vm[i] = vm[i] * (1.0 + cs * (ratio - uratio));
    } else if (ratio < lratio) {
      vm[i] = vm[i] / (1.0 + cs * (lratio - ratio));
    }

    if (vm[i] < rathersmall)
      vm[i] = rathersmall;
    if (vm[i] > (upperbound[i] - lowerbound[i]))
      vm[i] = upperbound[i] - lowerbound[i];

  }
}

// get a function value and do any updates that are necessary.
void ParaminSimann::ReceiveValue() {
  double p, pp;
  int receive;

  receive = net->receiveOne();
  if (receive == net->netSuccess()) {
    returnID = net->getReceiveID();
    if (returnID >= 0) {
      // received data belonging to correct datagroup
      iters++;
      fp = net->getY(returnID);
      xp = net->getX(returnID);
      if (!maxim)
        fp = -fp;
      // accept the new point if the function value increases.
      if (fp >= fstart)  {
        AcceptPoint();
	//cout << "accepted point" << endl;
	//cout << "fp is " << fp << endl;
      }
      else {
        p = expRep((fp - fstart) / T);
        pp = randomNumber();

        // accept the new point if Metropolis condition is satisfied.
        if (pp < p)
          AcceptPoint();
      }
    }

  } else {
    cerr << "Trying to receive value during Simulated Annealing but failed" << endl;
    exit(EXIT_FAILURE);
  }
}
void ParaminSimann::Print(ofstream& outfile, int prec) {
	outfile << "; Simmulated annealing algorithm ran for " << iters
	    << " function evaluations\n; and stopped when the likelihood value was "
	    << setprecision(prec) << score;
	  if (converge == -1)
	    outfile << "\n; because an error occured during the optimisation\n";
	  else if (converge == 1)
	    outfile << "\n; because the convergence criteria were met\n";
	  else
	    outfile << "\n; because the maximum number of function evaluations was reached\n";
}
