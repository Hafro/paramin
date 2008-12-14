#include "optimizer.h"

// ********************************************************
// functions for class Optimizer
// ********************************************************
Optimizer::Optimizer(CommandLineInfo* info, NetInterface* net) {
    int i;
    netInt = net;
    // useSA = 0;
    // useHJ = 0;
    // useBFGS = 0;
    // parSA = NULL;
    // parHJ = NULL;
    // parBFGS = NULL;

  // Initialise random number generator
  srand(time(NULL));

  outputfile = info->getOutputFilename();
  if (info->getOptInfoFileGiven())
    this->readOptInfo(info->getOptFilename());
  else {
    cout << "No optimizing information given - using default information\n";
    // parHJ = new ParaminHooke(net);
    // useHJ = 1;
    optvec.resize(new ParaminHooke(net));
  }
  // DoubleVector startx;
  // Set the starting value of x and f
  // startx = net->getInitialX();
  // cout << "best x in optimizer is: " << endl;
  // for (i = 0; i < startx.Size(); i++)
  // cout << startx[i] << " ";
  // cout << endl;
  // startx is scaled if scaling is used
 
  netInt->startNewDataGroup(1);
  netInt->setX(netInt->getInitialX());
  netInt->sendAndReceiveAllData();
  //cout << "in optmizier got fx " << netInt->getY(0) << endl;
  netInt->setScore(netInt->getY(0));
  // startf = net->getY(0);
  netInt->stopUsingDataGroup();
}

Optimizer::~Optimizer() {
/* 
 if (parSA != NULL)
    delete parSA;
  if (parHJ != NULL)
    delete parHJ;
  if (parBFGS != NULL)
    delete parBFGS;
*/
    int i;
    for (i = 0; i < optvec.Size(); i++)
	delete optvec[i];

}

void Optimizer::readOptInfo(char* optfilename) {
  char* text =  new char[MaxStrLength];
  strncpy(text, "", MaxStrLength);
  ifstream infile(optfilename);
  CommentStream commin(infile);
  int i = 0;
  // Now need to look for seed, [simann], [hooke] or [bfgs]
  commin >> text;
  while (!commin.eof()) {
    commin >> ws;  //trim whitespace from infile
    if ((strcasecmp(text, "seed")) == 0 && (!commin.eof())) {
      commin >> seed >> ws >> text;
      srand(seed);

    } else if (strcasecmp(text, "[simann]") == 0) {
	// parSA = new ParaminSimann(net);
	//useSA = 1;
	optvec.resize(new ParaminSimann(netInt));

    } else if (strcasecmp(text, "[hooke]") == 0) {
	// parHJ = new ParaminHooke(net);
	// useHJ = 1;
	optvec.resize(new ParaminHooke(netInt));
    } else if (strcasecmp(text, "[bfgs]") == 0) {
	//parBFGS = new ParaminBFGS(net);
	// useBFGS = 1;
	optvec.resize(new ParaminBFGS(netInt));
    }
    else {
	// set log failure here...
	cerr << " did not find bfgs, hooke, simann or seed" << endl;
	// exit ..
    }
    if (!commin.eof()) {
      if (optvec.Size() > 0)   {
	commin >> text;
	   if ((text[0] == '[') || (strcasecmp(text, "seed") == 0))
	     cerr << "Warning - no optimisation parameters specified for optimization algorithm\n";
	   else {
	     optvec[i]->read(commin, text);
	     i++;
	   }
      }
      } else
        cerr << "Warning - no optimisation parameters specified for optimization algorithm\n";
  

  }
  if (optvec.Size() == 0) {
      // set here log message...
      optvec.resize(new ParaminHooke(netInt));
  }
  delete [] text;
  infile.close();
  infile.clear();

}

void Optimizer::OptimizeFunc() {
    int i;
    for (i = 0; i < optvec.Size(); i++)
	optvec[i]->OptimiseLikelihood();
  
}

void Optimizer::getScore(DoubleVector& x, double fx) {
  //netInt->getInitialScore(x, fx);
  fx = netInt->getInitialScore(x);
}

double Optimizer::getBestF() {
  return netInt->getScore();
}

void Optimizer::printResult() {
  // write the best point out to a file
  int i;
  time_t timenow;
  timenow = time(NULL);
  ofstream outfile;
  outfile.open(outputfile);
  DoubleVector x;
  double fx;
  //netInt->getInitialScore(x, fx);
  fx = netInt->getInitialScore(x);
  if (!outfile) {
         
    cout << "\nWarning - can't open outputfile\nThe best point calculated is f(x) = "
      << fx << " at\n";
    this->printX(x);

  } else {
    // write the data in the gadget format so this file can be used as a starting point
    outfile << "; Output from Paramin version " << PARAMINVERSION << " on " << ctime(&timenow);
    // AJ must decide on prec... set it somewher eelse. ...
    for (i = 0; i <  optvec.Size(); i++) 
	optvec[i]->Print(outfile, 8);
    
    outfile  << "; The final likelihood value was " << fx << "\nswitch\tvalue\t\tlower\tupper\toptimise\n";
    for (i = 0; i < x.Size(); i++) {
      outfile << (netInt->getSwitches())[i].getName() << TAB << setw(12) << setprecision(8)
        << x[i] << TAB << setw(8) << setprecision(4) << netInt->getLowerbound()[i]
        << setw(8) << setprecision(4) << netInt->getUpperbound()[i]
        << setw(8) << netInt->getOptInfo()[i] << endl;

    }
  }
  outfile.close();
}
void Optimizer::printX(const DoubleVector& vec) {
    int i;
    for (i = 0; i < vec.Size(); i++)
	cout << vec[i] << sep;
    cout << endl;
}
