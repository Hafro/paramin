#include "pvm3.h"
#include "paramin.h"
#include "optimizer.h"

int main(int argc, char* argv[]) {
  time_t startExec;
  startExec = time(NULL);
  cout << "Starting Paramin version " << PARAMINVERSION << " at " << ctime(&startExec) << endl;

  NetCommunication* net;
  Optimizer* optimize;
  NetInterface* netInt;
  ProcessManager* processM;
  CommandLineInfo* commandline;

  commandline = new CommandLineInfo;
  // Find out options given on command line
  commandline->read(argc, argv);

  net = new MasterCommunication(commandline);
  processM = new WorkLoadScheduler(commandline);
  netInt = new NetInterface(net, processM, commandline);
  optimize = new Optimizer(commandline, netInt);

  cout << "Starting function value from inputfiles: " << optimize->getBestF() << " at: \n" << optimize->getBestX(netInt);
  // This prints X full, unscaled and including all parameteters. Not x as
  // possibly used by opt. methods.
  optimize->OptimizeFunc();
  optimize->printResult(netInt);

  // clean up
  delete netInt;
  delete processM;
  delete net;
  delete commandline;
  delete optimize;

  time_t stopExec;
  stopExec = time(NULL);
  cout << "\nParamin finished at " << ctime(&stopExec) << "The time taken for this Paramin run was "
    << difftime(stopExec, startExec) << " seconds\n\n";

  return EXIT_SUCCESS;
}
