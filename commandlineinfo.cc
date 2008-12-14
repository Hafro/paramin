#include "commandlineinfo.h"
#include "commentstream.h"
#include "errorhandler.h"
#include "paramin.h"

extern ErrorHandler handle;

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
  scale = 0;
  condor = 0;
  waitMaster = 300;
  hostMultiple = 2.0;
  runMultiple = 0.5;
  bestMultiple = 3.0;
}

CommandLineInfo::~CommandLineInfo() {
}

void CommandLineInfo::read(int aNumber, char* const aVector[]) {
  int k, len;

  if (aNumber > 1) {
    k = 1;
    while (k < aNumber) {
      if (strcasecmp(aVector[k], "-scale") == 0) {
        scale = 1;

      } else if (strcasecmp(aVector[k], "-condor") == 0) {
        condor = 1;

        #ifndef CONDOR
          handle.logMessage(LOGFAIL, "Error - Paramin cannot currently run in CONDOR mode\nParamin must be recompiled to enable CONDOR support");
        #endif

      } else if (strcasecmp(aVector[k], "-i") == 0) {
        if (k == aNumber - 1)
          this->showCorrectUsage(aVector[k]);
        k++;
        inputfile.resize(aVector[k]);

      } else if (strcasecmp(aVector[k], "-o") == 0) {
        if (k == aNumber - 1)
          this->showCorrectUsage(aVector[k]);
        k++;
        outputfile.resize(aVector[k]);

      } else if (strcasecmp(aVector[k], "-network") == 0) {
        if (k == aNumber - 1)
          this->showCorrectUsage(aVector[k]);
        k++;
        networkfile.resize(aVector[k]);
        this->readNetworkInfo();

      } else if (strcasecmp(aVector[k], "-opt") == 0) {
        if (k == aNumber - 1)
          this->showCorrectUsage(aVector[k]);
        k++;
        optfile.resize(aVector[k]);

      } else if ((strcasecmp(aVector[k], "-v") == 0) || (strcasecmp(aVector[k], "--version") == 0)) {
        cout << "Paramin version " << PARAMINVERSION << endl << endl;
        exit(EXIT_SUCCESS);

      } else if ((strcasecmp(aVector[k], "-h") == 0) || (strcasecmp(aVector[k], "--help") == 0)) {
        this->showUsage();

      } else if ((strcasecmp(aVector[k], "-func") == 0) || (strcasecmp(aVector[k], "-function") == 0)) {
        if (k == aNumber - 1)
          this->showCorrectUsage(aVector[k]);
        k++;
        function.resize(aVector[k]);
        while (k < aNumber - 1) {
          k++;
          function.resize(aVector[k]);
        }

      } else
        this->showCorrectUsage(aVector[k]);

      k++;
    }

    // Must have set both name of function and name of the inputfile
    if (function.Size() < 1)
      this->showCorrectUsage();
    if (inputfile.Size() != 1)
      this->showCorrectUsage();

  } else
    this->showCorrectUsage();

  // Set the default value for the output
  if (outputfile.Size() < 1) {
    char* defaultname = new char[strlen("params.out") + 1];
    strcpy(defaultname, "params.out");
    outputfile.resize(defaultname);
    delete[] defaultname;
  }

  // Set the network paramaters for CONDOR is required
  if (condor)
    waitMaster = -1;
}

const CharPtrVector& CommandLineInfo::getFunction() {
  return function;
}

char* CommandLineInfo::getInputFilename() {
  if (inputfile.Size() != 1) {
    cerr << "Error on commandline - inputfile not specified\n";
    exit(EXIT_FAILURE);
  }
  return inputfile[0];
}

char* CommandLineInfo::getOutputFilename() {
  if (outputfile.Size() != 1) {
    cerr << "Error on commandline - outputfile not specified\n";
    exit(EXIT_FAILURE);
  }
  return outputfile[0];
}

char* CommandLineInfo::getOptFilename() {
  if (optfile.Size() != 1) {
    cerr << "Error on commandline - optfile not specified\n";
    exit(EXIT_FAILURE);
  }
  return optfile[0];
}

int CommandLineInfo::getOptInfoFileGiven() {
  return (optfile.Size() > 0);
}

void CommandLineInfo::readNetworkInfo() {

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
        cerr << "Error in networkfile - numproc must be greater than 0\n";
        infile.close();
        infile.clear();
        exit(EXIT_FAILURE);
      }

    } else if (strcasecmp(text, "waitformaster") == 0) {
      commin >> waitMaster >> ws;
      if (waitMaster < -1) {
        cerr << "Error in networkfile - waitformaster must be greater than -1\n";
        infile.close();
        infile.clear();
        exit(EXIT_FAILURE);
      }

    } else if (strcasecmp(text, "runtimemultiple") == 0) {
      commin >> runMultiple >> ws;
      if (runMultiple < 0) {
        cerr << "Error in networkfile - runtimemultiple must be greater than 0\n";
        infile.close();
        infile.clear();
        exit(EXIT_FAILURE);
      }

    } else if (strcasecmp(text, "hostmultiple") == 0) {
      commin >> hostMultiple >> ws;
    } else if (strcasecmp(text, "besttimemultiple") == 0) {
      commin >> bestMultiple >> ws;
    } else {
      cerr << "Error in networkfile - unknown option " << text << "\n";
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
