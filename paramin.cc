#include "pvm3.h"
#include "search.h"
#include "paramin.h"

struct con {
  int VARS;           // max. number of variables.
  double vm_comp;     // step length for sim. ann.
  double c_comp;      // step length adjustment.
  int T;              // starting temperature.
  int MAXIM;          // 0 for minimization, 1 for maximization in SA
  int SIM_ITER;       // max. number of iter. in one sim.
  int HOOKE_ITER;     // max. number of iter. in one H&J.
  double EPSILON;     // halt criteria.
  int scaling;
  int SA;
  int HJ;
  int BFGS;
};

con getConstants();

int main(int argc, char* argv[]) {

  time_t startExec;
  startExec = time(NULL);
  cout << "Starting Paramin version " << paramin_version << " at " << ctime(&startExec);
  con par;
  par = getConstants();
  srand(time(NULL));  // Initialize the random number generator
  int i = 0;
  netCommunication* net;
  search* minMethod;
  double f = 0.0;
  netInterface* netInt;
  processManager* processM;

  // if no program specified on commandline, quit
  if (argc < 7) {
    cout << "\nMust specify number of processors, -i inputfilename -o outputfilename\n"
      << "and the name of program to be run on slaves\n"
      << "For example: master 4 -i inputfile -o outputfile slaveProg\n"
      << "Will start 4 processes of slaveProg, reading input values from inputfile\n"
      << "and writing the final minimum vector value to outputfile\n"
      << "If number of processes specified is equal to 0\n"
      << "the program will start as many processes as number of available hosts\n";
    exit(EXIT_FAILURE);
  }

  char letter;
  int NOTDIGIT = 0;
  // Check if number of processor specified on command line is a number
  while (i < strlen(argv[1])) {
    letter = argv[1][i];
    if (!isdigit(letter))
      NOTDIGIT = 1;
    i++;
  }
  if (NOTDIGIT == 1) {
    cout << "On commandline number of processors specified must be a number, got: " << argv[1] << endl;
    exit(EXIT_FAILURE);
  }

  // set number of processes to be started
  int numHosts;
  numHosts = atoi(argv[1]);
  if (numHosts < 0) {
    cout << "Number of processes to be started must be greater than 0\n";
    exit(EXIT_FAILURE);
  }

  // check for inputfile
  char* inputfilename = NULL;
  char* outputfilename = NULL;
  int fileLength = 0;
  if (strcmp(argv[2], "-i") != 0) {
    cout << "Must have -i inputfilename given on commandline, got: " << argv[2] << endl;
    exit(EXIT_FAILURE);
  } else {
    fileLength = strlen(argv[3]);
    inputfilename = new char[fileLength + 1];
    strcpy(inputfilename, argv[3]);
  }

  // check for outputfile
  if (strcmp(argv[4], "-o") != 0) {
    cout << "Must have -o outputfilename given on commandline, got: " << argv[4] << endl;
    exit(EXIT_FAILURE);
  } else {
    fileLength = strlen(argv[5]);
    outputfilename = new char[fileLength + 1];
    strcpy(outputfilename, argv[5]);
  }

  // set argument vector for program to start on slaves
  int numArguments = argc - 6;
  char** args;
  args = new char*[numArguments];
  for (i = 0; i < argc - 7; i++) {
    args[i] = new char[strlen(argv[i + 7]) + 1];
    strcpy(args[i], argv[i + 7]);
  }
  args[i] = NULL;
  net = new masterCommunication(argv[6], args, numHosts, 300);
  processM = new workLoadScheduler(0.5);
  netInt = new netInterface(inputfilename, net, processM, par.scaling);

  // vm is the step length vector used in simulated annealing, and
  // c controls the step length ajustment.
  // maybe enough to have input as par.c_comp and par.vm_comp but not the whole vector
  int n = netInt->getNumOfVarsInDataGroup();
  vector c(n);
  vector vm(n);
  c.setValue(par.c_comp);
  vm.setValue(par.vm_comp);
  cout << "Starting value from inputfiles\n" << netInt->getInitialX();

  int numit = 0;
  if (par.SA == 1) {
    cout << "\nStarting Simulated Annealing\n";
    minMethod = new simann(netInt, par.MAXIM, par.SIM_ITER, c, par.T, vm);
    while (numit < par.SIM_ITER) {
      numit += minMethod->DoSearch();
      f = minMethod->GetBestF();
    }
    cout << "Best point from Simulated Annealing is f(x) = " << f << " at\n" << netInt->getInitialX();
    delete minMethod;
  }

  numit = 0;
  if (par.HJ == 1) {
    cout << "\nStarting Hooke and Jeeves\n";
    minMethod = new hooke(netInt);
    while (numit < par.HOOKE_ITER) {
      numit += minMethod->DoSearch();
      f = minMethod->GetBestF();
    }
    cout << "Best point from Hooke and Jeeves is f(x) = " << f << " at\n" << netInt->getInitialX();
    delete minMethod;
  }

  if (par.BFGS == 1) {
    cout << "\nStarting BFGS\n";
    minMethod = new minimizer(netInt);
    minMethod->DoSearch();
    f = minMethod->GetBestF();
    cout << "Best point from BFGS is f(x) = " << f << " at\n" << netInt->getInitialX();
    delete minMethod;
  }

  // write the best point out to a file called outputfile
  ofstream outfile;
  outfile.open(outputfilename);
  vector temp = netInt->getInitialX();
  if (!outfile) {
    cout << "Warning - can't open outputfile\nThe best point calculated is:\n";
    cout << netInt->prepareVectorToSend(temp) << endl;
  } else {

#ifdef GADGET_NETWORK
    // write the data in the gadget format so this file can be used as a starting point
    outfile << "; Output from Paramin version " << paramin_version << " on " << ctime(&startExec)
      << "; The final likelihood value was " << f << "\nswitch\tvalue\t\tlower\tupper\toptimize\n";
    vectorofcharptr switches = netInt->getSwitches();
    vector lower = netInt->getLowerbound();
    vector upper = netInt->getUpperbound();
    vector result = netInt->prepareVectorToSend(temp);
    vector optinfo = netInt->getOptInfo();
    for (i = 0; i < lower.dimension(); i++) {
      outfile << switches[i] << "\t" << setw(10) << setprecision(8) << result[i];
      outfile << "\t" << lower[i] << "\t" << upper[i] << "\t" << optinfo[i] << endl;
    }
#else
    // write the output data in the old format
    outfile << netInt->prepareVectorToSend(temp) << endl;
#endif

  }
  outfile.close();

  // clean up
  delete netInt;
  delete processM;
  delete net;
  if (inputfilename != NULL) {
    delete inputfilename;
    inputfilename = NULL;
  }
  if (outputfilename != NULL) {
    delete outputfilename;
    outputfilename = NULL;
  }
  for (i = 0; i < argc - 7; i++)
    delete args[i];
  delete [] args;

  time_t stopExec;
  stopExec = time(NULL);
  cout << "Paramin finished at " << ctime(&stopExec) << "Paramin runtime was "
    << difftime(stopExec, startExec) << " seconds\n";

  exit(EXIT_SUCCESS);
}

con getConstants() {
  char letter;
  int i = 0;
  char text[MaxStrLength];
  con tmp;

  ifstream mainfile("mainconstants", ios::in|ios::binary|ios::nocreate);
  if (!mainfile) {
    cout << "Cannot open input file mainconstants\n";
    exit(EXIT_FAILURE);
  }

  while (!mainfile.eof()) {
    mainfile >> text;
    if (strcasecmp(text, ";") == 0) {
      mainfile.get(letter);
      while ((letter != '\n') && !mainfile.eof())
        mainfile.get(letter);

    } else if(strcasecmp(text, "//") == 0) {
      mainfile.get(letter);
      while ((letter != '\n') && !mainfile.eof())
        mainfile.get(letter);

    } else {
      if (strcasecmp(text, "NUMVARS") == 0) {
        mainfile >> tmp.VARS;
        i++;
      } else if (strcasecmp(text, "VM_COMPONENT") == 0) {
        mainfile >> tmp.vm_comp;
        i++;
      } else if (strcasecmp(text, "C_COMPONENT") == 0) {
        mainfile >> tmp.c_comp;
        i++;
      } else if (strcasecmp(text, "TEMPERATURE") == 0) {
        mainfile >> tmp.T;
        i++;
      } else if (strcasecmp(text, "MAXIM") == 0) {
        mainfile >> tmp.MAXIM;
        i++;
      } else if (strcasecmp(text, "SIM_ITER") == 0) {
        mainfile >> tmp.SIM_ITER;
        i++;
      } else if (strcasecmp(text, "HOOKE_ITER") == 0) {
        mainfile >> tmp.HOOKE_ITER;
        i++;
      } else if (strcasecmp(text, "EPSILON") == 0) {
        mainfile >> tmp.EPSILON;
        i++;
      } else if (strcasecmp(text, "SCALING") == 0) {
        mainfile >> tmp.scaling;
        i++;
      } else if (strcasecmp(text, "SimulatedAnnealing") == 0) {
        mainfile >> tmp.SA;
        i++;
      } else if (strcasecmp(text, "HookeAndJeeves") == 0) {
        mainfile >> tmp.HJ;
        i++;
      } else if (strcasecmp(text, "BFGS") == 0) {
        mainfile >> tmp.BFGS;
        i++;
      }
    }
  }

  mainfile.close();
  if (i != 12) {
    cout << "Error in reading from input file mainconstants\n"
      << "Expected variables are: NUMVARS, VM_COMPONENT, C_COMPONENT,\n"
      << "TEMPERATURE, MAXIM, SIM_ITER, HOOKE_ITER, EPSILON, SCALING,\n"
      << "SimulatedAnnealing, HookeAndJeeves and BFGS\n";
    exit(EXIT_FAILURE);
  }
  return (tmp);
}
