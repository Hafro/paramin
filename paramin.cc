#include "pvm3.h"
#include "paramin.h"
#include "optimizer.h"
#include "errorhandler.h"

ErrorHandler handle;

int main(int argc, char* argv[]) {
  time_t startExec;
  startExec = time(NULL);
  cout << "Starting Paramin version " << PARAMINVERSION << " at " << ctime(&startExec) << endl;
  int i;
  NetCommunication* net;
  Optimizer* optimize;
  NetInterface* netInt;
  ProcessManager* processM;
  CommandLineInfo* commandline;

  commandline = new CommandLineInfo();
  // Find out options given on command line
  commandline->read(argc, argv);

  net = new MasterCommunication(commandline);
  processM = new WorkLoadScheduler(commandline);
  netInt = new NetInterface(net, processM, commandline);
  optimize = new Optimizer(commandline, netInt);
  /* taking out, will get info from the optimization...
  DoubleVector temp(optimize->getBestX(netInt));
  cout << "Starting function value from inputfiles: " << optimize->getBestF() << " at: \n";
  for (i = 0; i < temp.Size(); i++) {
      cout << temp[i] << " ";
  }
  cout << endl;
  */
  // This prints X full, unscaled and including all parameteters. Not x as
  // possibly used by opt. methods.
  // AJ. no, this prints x only using the optimizing parameters....But unscaled.
  optimize->OptimizeFunc();
  optimize->printResult();
  
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
