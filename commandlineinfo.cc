#include "commandlineinfo.h"
#include "commentstream.h"
#include "paramin.h"

void CommandLineInfo::showCorrectUsage(char* error) {
  cerr << "Error in command line value - unrecognised option " << error << endl
    << "Required options are -i <filename> and -function <function>\n"
    << "For more information try running Paramin with the -h switch\n";
  exit(EXIT_FAILURE);
}

void CommandLineInfo::showCorrectUsage() {
  cerr << "Error on command line when starting Paramin\n"
    << "Required options are -i <filename> and -function <function>\n"
    << "For more information try running Paramin with the -h switch\n";
  exit(EXIT_FAILURE);
}

void CommandLineInfo::showUsage() {
  cout << "Required information for running Paramin:\n"
    << " -i <filename>                read model parameters from <filename>\n"
    << " -function <function>         read function and options to be minimised\n"
    << "                              (this must be the last option specified)\n"
    << "\nOptional information for controlling Paramin:\n"
    << " -opt <filename>              read optimising parameters from <filename>\n"
    << " -network <filename>          read network information from <filename>\n"
    << " -o <filename>                print model parameters to <filename>\n"
    << "                              (default filename is 'params.out')\n"
    << " -scale                       scale the variables to be optimised\n"
    << " -condor                      use CONDOR to control the PVM network\n"
    << "\nOther valid command line options:\n"
    << " -v --version                 display version information and exit\n"
    << " -h --help                    display this help screen and exit\n"
    << "\nFor more information see the web page at http://www.hafro.is/gadget/paramin.html\n\n";
  exit(EXIT_SUCCESS);
}

CommandLineInfo::CommandLineInfo() {
  numProc = 0;
  toScale = 0;
  condor = 0;
  waitForMaster = 300;
  hostMultiple = 2.0;
  runtimeMultiple = 0.5;
  besttimeMultiple = 3.0;
}

CommandLineInfo::~CommandLineInfo() {
}

void CommandLineInfo::Read(int aNumber, char* const aVector[]) {
  int k, len;

  if (aNumber > 1) {
    k = 1;
    while (k < aNumber) {
      if (strcasecmp(aVector[k], "-scale") == 0) {
        toScale = 1;

      } else if (strcasecmp(aVector[k], "-condor") == 0) {
        #ifndef CONDOR
          cout << "\nError - Paramin cannot run CONDOR without CONDOR supported being compiled\n";
        #else
          condor = 1;
        #endif

      } else if (strcasecmp(aVector[k], "-i") == 0) {
        if (k == aNumber - 1)
          showCorrectUsage(aVector[k]);
        k++;
        inputfile.resize(1, aVector[k]);

      } else if (strcasecmp(aVector[k], "-o") == 0) {
        if (k == aNumber - 1)
          showCorrectUsage(aVector[k]);
        k++;
        outputfile.resize(1, aVector[k]);

      } else if (strcasecmp(aVector[k], "-network") == 0) {
        if (k == aNumber - 1)
          showCorrectUsage(aVector[k]);
        k++;
        networkfile.resize(1, aVector[k]);
        this->ReadNetworkInfo();

      } else if (strcasecmp(aVector[k], "-opt") == 0) {
        if (k == aNumber - 1)
          showCorrectUsage(aVector[k]);
        k++;
        optfile.resize(1, aVector[k]);

      } else if ((strcasecmp(aVector[k], "-v") == 0) || (strcasecmp(aVector[k], "--version") == 0)) {
        cout << "Paramin version " << PARAMINVERSION << endl << endl;
        exit(EXIT_SUCCESS);

      } else if ((strcasecmp(aVector[k], "-h") == 0) || (strcasecmp(aVector[k], "--help") == 0)) {
        showUsage();

      } else if ((strcasecmp(aVector[k], "-func") == 0) || (strcasecmp(aVector[k], "-function") == 0)) {
        if (k == aNumber - 1)
          showCorrectUsage(aVector[k]);
        k++;
        function.resize(1, aVector[k]);
        while (k < aNumber - 1) {
          k++;
          function.resize(1, aVector[k]);
        }

      } else
        showCorrectUsage(aVector[k]);

      k++;
    }

    // Must have set both name of function and name of the inputfile
    if (function.Size() < 1)
      showCorrectUsage();
    if (inputfile.Size() != 1)
      showCorrectUsage();

  } else
    showCorrectUsage();

  // Set the default value for the output
  if (outputfile.Size() < 1) {
    char* defaultname = new char[strlen("params.out") + 1];
    strcpy(defaultname, "params.out");
    outputfile.resize(1, defaultname);
    delete[] defaultname;
  }

  // Set the network paramaters for CONDOR is required
  if (condor)
    waitForMaster = -1;
}

int CommandLineInfo::NumOfProc() {
  return numProc;
}

const VectorOfCharPtr& CommandLineInfo::FunctionNameArgs() {
  return function;
}

int CommandLineInfo::WaitForMaster() {
  return waitForMaster;
}

double CommandLineInfo::RunTimeMultiple() {
  return runtimeMultiple;
}

double CommandLineInfo::BestTimeMultiple() {
  return besttimeMultiple;
}

double CommandLineInfo::HostMultiple() {
  return hostMultiple;
}

int CommandLineInfo::ToScale() {
  return toScale;
}

int CommandLineInfo::runCondor() {
  return condor;
}

char* CommandLineInfo::InputFilename() {
  if (inputfile.Size() != 1) {
    cerr << "Error, have not been able to get inputfile from commandline\n";
    exit(EXIT_FAILURE);
  }
  return inputfile[0];
}

char* CommandLineInfo::OutputFilename() {
  return outputfile[0];
}

char* CommandLineInfo::OptFilename() {
  return optfile[0];
}

int CommandLineInfo::OptinfoGiven() {
  return (optfile.Size() > 0);
}

void CommandLineInfo::ReadNetworkInfo() {

  //Need to add check for values which can not be accepted...
  int i = 0;
  char text[MaxStrLength];
  strncpy(text, "", MaxStrLength);
  ifstream infile(networkfile[0]);
  CommentStream commin(infile);
  commin >> ws;
  commin >> text >> ws;
  while (!commin.eof()) {
    if (strcasecmp(text, "numproc") == 0) {
      commin >> numProc >> ws;
      if (numProc < 0) {
        cerr << "number of processors specified must be greater than 0\n";
        infile.close();
        infile.clear();
        exit(EXIT_FAILURE);
      }

    } else if (strcasecmp(text, "waitformaster") == 0) {
      commin >> waitForMaster >> ws;
      if (waitForMaster < -1) {
        cerr << "Value of waitformaster must be greater that -1, got: " << waitForMaster << "\n";
        infile.close();
        infile.clear();
        exit(EXIT_FAILURE);
      }

    } else if (strcasecmp(text, "runtimemultiple") == 0) {
      commin >> runtimeMultiple >> ws;
      if (runtimeMultiple < 0) {
        cerr << "runtimeMultiple must be greater that 0, got: " << runtimeMultiple << "\n";
        infile.close();
        infile.clear();
        exit(EXIT_FAILURE);
      }

    } else if (strcasecmp(text, "hostmultiple") == 0) {
      commin >> hostMultiple >> ws;
    } else if (strcasecmp(text, "besttimemultiple") == 0) {
      commin >> besttimeMultiple >> ws;
    } else {
      cerr << "Error in networkfile, unknown option: " << text << "\n";
      infile.close();
      infile.clear();
      exit(EXIT_FAILURE);
    }
    commin >> text;
    i++;
  }

  if (i == 0) {
    cout << "Warning - no network info given in networkfile - using default values." << endl;
  }
  infile.close();
  infile.clear();
}
