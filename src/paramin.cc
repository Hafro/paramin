#include "mpi.h"
#include "paramin.h"
#include "optimizer.h"
#include "global.h"
/*#include "errorhandler.h"

ErrorHandler handle;
*/
int main(int argc, char* argv[]) {
	// Tékka hvort viðföng séu í lagi þegar mpirun er notað.
	//cout << "argc = " << argc << endl; 
	//   for(int i = 0; i < argc; i++) 
	//      cout << "argv[" << i << "] = " << argv[i] << endl;
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
  optimize->OptimizeFunc();
  optimize->printResult();
  
  // clean up
  delete netInt;
  delete processM;
  delete net;
  delete optimize;

  time_t stopExec;
  stopExec = time(NULL);
  cout << "\nParamin finished at " << ctime(&stopExec) << "The time taken for this Paramin run was "
    << difftime(stopExec, startExec) << " seconds\n\n";
  // Write time to outputfile specified G.E.
  ofstream outfile;
  outfile.open(commandline->getOutputFilename(),ios::out | ios::app);
  delete commandline;
  outfile << "; Time taken for Paramin was: " << difftime(stopExec, startExec) << " seconds\n\n";
  outfile.close();
  return EXIT_SUCCESS;
}
