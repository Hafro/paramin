#include "paraminsimann.h"

// ********************************************************
// functions for class ParaminSimann
// ********************************************************
ParaminSimann::ParaminSimann(NetInterface* netInt) : ParaminSearch(netInt) {
  lnobds = 0;
  maxim = 0;
  T = 100;
  cs = 2.0;

  vector tempVec(numvar);
  vm = tempVec;
  vm.setValue(1.0);
  xp = tempVec;
  nfcnev = 0;
  nt = 2;
  ns = 5;
  check = 4;
  uratio = 0.7;
  lratio = 0.3;
  rt = 0.85;
  eps = 1e-4;
  maxiterations = 2000;
  
  Id = new int[numvar];
  nacp = new int[numvar];
  acpPointId = new int[numvar];
  // total number of processes initiated at beginning
  NumberOfHosts = net->getTotalNumProc();
}

ParaminSimann::~ParaminSimann() {
  delete[] Id;
  delete[] nacp;
  delete[] acpPointId;
}

void ParaminSimann::Read(CommentStream& infile, char* text) {
  int i = 0;
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
      vm.setValue(temp);

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

void ParaminSimann::doSearch(const vector& startx, double startf) {
  int i, numtoset, numacc;
  int numloops_ns, numloops_nt;
  int numset_nsloop;     // 0 < numset_nsloop <= numvar
  int rock = 0;

  bestx = startx;
  xstart = startx;
  if (xstart > upperbound || xstart < lowerbound) {
    cerr << "Error in Simmulated Annealing - x is not within bounds\n";
    exit(EXIT_FAILURE);
  }
  initialVM = vm;

  bestf = startf;
  if (!maxim)
    bestf = -bestf;

  vector tempVec(check);
  tempVec.setValue(bestf);
  fstar = tempVec;
  fstart = bestf;

  for (i = 0; i < numvar; i++) {
    Id[i] = i;
    nacp[i] = 0;
    acpPointId[i] = -1;
  }
  // Find out how many values to set at beginning of each ns loop.
  if (numvar > NumberOfHosts)
    numtoset = NumberOfHosts;
  else
    numtoset = numvar;

  // start the main loop.  Note that it terminates if (i) the algorithm
  // succesfully otimizes the function or (ii) there are too many func. eval.
  while ((rock == 0) && (nfcnev < maxiterations)) {
    numloops_nt = 0;
    net->startNewDataGroup((ns * nt * (numvar + 1)));
    while ((numloops_nt < nt) && (nfcnev < maxiterations)) {
      numloops_ns = 0;
      naccepted_nsloop = 0;
      randomOrder(Id);

      while ((numloops_ns < ns) && (nfcnev < maxiterations)) {
        if (numloops_ns == 0) {
          for (i = 0; i < numtoset; i++)
            SetXP(Id[i]);
          numset_nsloop = numtoset;
        } else
          numset_nsloop = 0;

        // sends all available data to all free hosts.
        net->sendToAllIdleHosts();
        while ((numset_nsloop < numvar) && (nfcnev < maxiterations)) {
          // get a function value and do any update that's necessary.
          ReceiveValue();
          if (nfcnev < maxiterations) {
            SetXP(Id[numset_nsloop]);
            numset_nsloop++;
          }
        }

        numacc = naccepted_nsloop;
        numloops_ns++;
      }

      if (nfcnev < maxiterations) {
        while ((net->getNumNotAns() > 0) && (nfcnev < maxiterations)) {
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
      acpPointId[i] = -1;

    // check termination criteria.
    for (i = check - 1; i > 0; i--)
      fstar[i] = fstar[i - 1];
    fstar[0] = fstart;

    rock = 0;
    if (absolute(bestf - fstart) < eps) {
      rock = 1;
      for (i = 0; i < check - 1; i++)
        if (absolute(fstar[i + 1] - fstar[i]) > eps)
          rock = 0;
    }

    cout << "\nChecking convergence criteria after " << nfcnev << " function evaluations ...\n";

    // if termination criteria is not met, prepare for another loop.
    if (rock == 0) {
      T *= rt;
      cout << "Reducing the temperature to " << T << endl;
      fstart = bestf;
      xstart = bestx;
    }
  }

  // Either reached termination criteria or maxiterations or both
  if (!maxim)
    bestf = -bestf;

  net->setBestX(bestx);

  if ((nfcnev >= maxiterations) && (rock == 1)) {
    cout << "\nSimulated Annealing optimisation completed after " << nfcnev
      << " iterations (max " << maxiterations << ")\nThe model terminated "
      << "because the maximum number of iterations was reached\n";
  } else {
    cout << "\nStopping Simulated Annealing\n\nThe optimisation stopped after " << nfcnev
      << " function evaluations (max " << maxiterations << ")\nThe optimisation stopped "
      << "because an optimum was found for this run\n";
  }
  cout << "number of times out of bounds: " << lnobds << endl;
}

// generate xp, the trial value of x - note use of vm to choose xp.
void ParaminSimann::SetXP(int k) {
  int i;
  vector temp;
  int id;

  for (i = 0; i < numvar; i++) {
    if (i == k) {
      xp[k] = xstart[k] + (randomNumber() * 2.0 - 1.0) * vm[k];
      if (xp[k] < lowerbound[k] || xp[k] > upperbound[k]) {
        lnobds++;
	while((xp[k] < lowerbound[k] || xp[k] > upperbound[k])) {
	  xp[k] = xstart[k] + (randomNumber() * 2.0 - 1.0) * vm[k];
	}
	
      }
    } else {
      if (acpPointId[i] >= 0) {
        // Use parameter from x with id = nacp_ns[i] which was accepted earlier
        temp = net->getX(acpPointId[i]);
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
  xstart = xp;
  fstart = fp;
  nacp[Id[returnId % numvar]]++;
  acpPointId[Id[returnId % numvar]] = returnId;
  naccepted_nsloop++;

  // if better than any other point record as new optimum.
  if (fp > bestf) {
    bestx = xp;
    cout << "\nNew optimum after " << nfcnev << " function evaluations, f(x) = " << -fp << " at\n";
    cout << net->unscaleX(bestx);
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
    returnId = net->getReceiveId();
    if (returnId >= 0) {
      // received data belonging to correct datagroup
      nfcnev++;
      fp = net->getY(returnId);
      xp = net->getX(returnId);
      if (!maxim)
        fp = -fp;
      // accept the new point if the function value increases.
      if (fp >= fstart)
        AcceptPoint();
      else {
        p = expRep((fp - fstart) / T);
        pp = randomNumber();

        // accept the new point if Metropolis condition is satisfied.
        if (pp < p)
          AcceptPoint();
        else
          nrej++;// AJ 24.03.04 nrej not used anywhere
      }
    }

  } else {
    cerr << "Trying to receive value during Simulated Annealing but failed" << endl;
    exit(EXIT_FAILURE);
  }
}
