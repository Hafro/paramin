#include "optimizer.h"

// ********************************************************
// functions for class Optimizer
// ********************************************************
Optimizer::Optimizer(CommandLineInfo* info, NetInterface* net) {

  useSimann = 0;
  useHooke = 0;
  useBfgs = 0;
  parSA = NULL;
  parHJ = NULL;
  parBFGS = NULL;

  // Initialise random number generator
  srand(time(NULL));

  outputfile = info->OutputFilename();
  if (info->OptinfoGiven())
    this->ReadOptInfo(info->OptFilename(), net);
  else {
    cout << "No optimizing information given - using default information\n";
    parHJ = new ParaminHooke(net);
    useHooke = 1;
  }

  // Set the starting value of x and f
  startx = net->getInitialX();
  // startx is scaled if scaling is used
  net->startNewDataGroup(1);
  net->setX(startx);
  net->sendAndReceiveAllData();
  startf = net->getY(0);
  net->stopUsingDataGroup();
}

Optimizer::~Optimizer() {
  if (parSA != NULL)
    delete parSA;
  if (parHJ != NULL)
    delete parHJ;
  if (parBFGS != NULL)
    delete parBFGS;
}

void Optimizer::ReadOptInfo(char* optfilename, NetInterface* net) {

  char text[MaxStrLength];
  strncpy(text, "", MaxStrLength);
  ifstream infile(optfilename);
  CommentStream commin(infile);

  // Now I need to look for seed, [simann], [hooke] or [bfgs]
  commin >> text;
  while (!commin.eof()) {
    commin >> ws;  //trim whitespace from infile
    if ((strcasecmp(text, "seed")) == 0 && (!commin.eof())) {
      commin >> seed >> ws >> text;
      srand(seed);

    } else if (strcasecmp(text, "[simann]") == 0) {
      parSA = new ParaminSimann(net);
      useSimann = 1;

      if (!commin.eof()) {
        commin >> text;
	if ((text[0] == '[') || (strcasecmp(text, "seed") == 0))
          cerr << "Warning - no optimisation parameters specified for Simulated Annealing algorithm\n";
	else
          parSA->Read(commin, text);
	  
      } else
        cerr << "Warning - no optimisation parameters specified for Simulated Annealing algorithm\n";

    } else if (strcasecmp(text, "[hooke]") == 0) {
      parHJ = new ParaminHooke(net);
      useHooke = 1;

      if (!commin.eof()) {
        commin >> text;
	if ((text[0] == '[') || (strcasecmp(text, "seed") == 0))
          cerr << "Warning - no optimisation parameters specified for Hooke & Jeeves algorithm\n";
	else
          parHJ->Read(commin, text);
	  
      } else
        cerr << "Warning - no optimisation parameters specified for Hooke & Jeeves algorithm\n";

    } else if (strcasecmp(text, "[bfgs]") == 0) {
      parBFGS = new ParaminBFGS(net);
      useBfgs = 1;

      if (!commin.eof()) {
        commin >> text;
	if ((text[0] == '[') || (strcasecmp(text, "seed") == 0))
          cerr << "Warning - no optimisation parameters specified for BFGS algorithm\n";
	else
          parBFGS->Read(commin, text);
	  
      } else
        cerr << "Warning - no optimisation parameters specified for BFGS algorithm\n";

    } else {
      cerr << "Error in optfile, expecting to read seed, [simann], [hooke] or [bfgs] but got: " << text << "\n";
      infile.close();
      infile.clear();
      exit(EXIT_FAILURE);
    }
  }

  infile.close();
  infile.clear();

  if (useSimann == 0 && useHooke == 0 && useBfgs == 0) {
    cerr << "Error in optinfofile - no valid optimisation methods found\n";
    exit(EXIT_FAILURE);
  }
}

void Optimizer::OptimizeFunc() {
  if (useSimann) {
    cout << "\nStarting Simulated Annealing\n";
    parSA->doSearch(startx, startf);
    // startx is scaled if scaling is used..
    startx = parSA->getBestX();
    startf = parSA->getBestF();
    // printing x as used from optmethod, not in full....
    cout << "\nBest point from Simulated Annealing is f(x) = " << startf << " at\n" << startx;
  }

  if (useHooke) {
    cout << "\nStarting Hooke & Jeeves\n";
    parHJ->doSearch(startx, startf);
    startx = parHJ->getBestX();
    startf = parHJ->getBestF();
    cout << "\nBest point from Hooke & Jeeves is f(x) = " << startf << " at\n" << startx;
  }

  if (useBfgs) {
    cout << "\nStarting BFGS\n";
    parBFGS->doSearch(startx, startf);
    startx = parBFGS->getBestX();
    startf = parBFGS->getBestF();
    cout << "\nBest point from BFGS is f(x) = " << startf << " at\n" << startx;
  }
}

const vector& Optimizer::getBestX(NetInterface* net) {
  return net->prepareVectorToSend(startx);
}

double Optimizer::getBestF() {
  return startf;
}

void Optimizer::PrintResult(NetInterface* net) {
  // write the best point out to a file
  int i;
  time_t timenow;
  timenow = time(NULL);
  ofstream outfile;
  outfile.open(outputfile);
  if (!outfile) {
    cout << "\nWarning - can't open outputfile\nThe best point calculated is f(x) = "
      << startf << " at\n" << this->getBestX(net);

  } else {
    // write the data in the gadget format so this file can be used as a starting point
    outfile << "; Output from Paramin version " << PARAMINVERSION << " on " << ctime(&timenow);
    if (useSimann) {
      if (parSA->GetConverged())
        outfile << "; Simulated Annealing stopped because the convergence criteria were met\n";
      else
        outfile << "; Simulated Annealing stopped because the maximum number of function evaluations was reached\n";
    } 
    if (useHooke) {
      if (parHJ->GetConverged())
        outfile << "; Hooke & Jeeves stopped because the convergence criteria were met\n";
      else
        outfile << "; Hooke & Jeeves stopped because the maximum number of function evaluations was reached\n";
    } 
    if (useBfgs) {
      if (parBFGS->GetConverged())
        outfile << "; BFGS stopped because the convergence criteria were met\n";
      else
        outfile << "; BFGS stopped because the maximum number of function evaluations was reached\n";
    }
    outfile  << "; The final likelihood value was " << startf << "\nswitch\tvalue\t\tlower\tupper\toptimise\n";
    vector output;
    output = this->getBestX(net);
    for (i = 0; i < output.dimension(); i++) {
      outfile << net->getSwitches()[i] << TAB << setw(12) << setprecision(8)
        << output[i] << TAB << setw(8) << setprecision(4) << net->getLowerbound()[i]
        << setw(8) << setprecision(4) << net->getUpperbound()[i]
        << setw(8) << net->getOptInfo()[i] << endl;

    }
  }
  outfile.close();
}
