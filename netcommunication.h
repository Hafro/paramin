#ifndef netcommunication_h
#define netcommunication_h

#include "paramin.h"
#include "pvm3.h"
#include "netdata.h"
#include "pvmconstants.h"
#include "vector.h"
#ifdef GADGET_NETWORK
#include "vectorofcharptr.h"
#endif

/* AJ, 26.02.99
 * The class netCommunication provides functions for a master process to
 * handle netcommunication with its slave processes using Parallell
 * Virtual Machine (PVM) for actual communication. To successfully start
 * netcommunication with a group of processes, pvmd must have been
 * started on all hosts participating in PVM.
 * netCommunication includes functions to enroll in PVM providing pvmd is
 * running on all machines participating in PVM, spawning new processes
 * and halting running processes. Maximum number of hosts which can participate
 * in pvm is equal to MAXHOSTS.
 * It also includes functions to send data of type netDataVariables and
 * vectorofcharptr to a spawned process and receiving data of type
 * netDataResult from a spawned process.
 * functions which return information about the status of netcommunication */

class netCommunication {
protected:
  pvmconstants* pvmConst;
  int nHostInn;
  // pvm variables, used to start PVM and spawn processes on hosts
  int mytid;         // tid of master process (me)
  int narch;         // number of different architectures taking part in VM.
  int nhost;
  int* tids;
  int* status;
  struct pvmhostinfo *hostp;    // array that contains infformation about each host
  int numVar;                   // number of variables in x in netDatavariables
  int numProcesses;             // number of processes started
  int numGoodProcesses;         // number of running processes
  char* slaveProgram;           // name of program which will be started on slaves
  char** slaveArguments;        // arguments for slaveProgram

  int NETSTARTED;
  int ERROR;
  int SUCCESS;
public:
  netCommunication(char* progN, char** progA, int nh);
  virtual ~netCommunication();
  int startPVM();
  int spawnProgram();
  int sendNumberOfVariables();
  int startProcesses();
  int sendInitialMessage(int id);
  int startNetCommunication();
  void stopNetCommunication();
  int checkProcess(int id);
  void checkProcesses();
  void getHealthOfProcesses(int* procTids);
  int sendData(netDataVariables* sendP, int processId);
  virtual int receiveData(netDataResult* rp);
  void setNumInSendVar(int nVar);
  int getNumberOfHosts();
  int getNumVar();
  int getNumProcesses();
  int getNumRunningProcesses();
  void printErrorMsg(const char* errorMsg);
  int netCommStarted();
  int NET_ERROR();
  int NET_SUCCESS();
#ifdef GADGET_NETWORK
  int sendData(vectorofcharptr sendP);
  int sendBoundData(vector sendP);
#endif
};

/* The class masterCommunication is a derived class of
 * netCommunication. The only difference between netCommunication and
 * masterCommunication is that when masterCommunication receives
 * messages it will wait for a message for a specified time but
 * netCommunication will wait forever */

class masterCommunication : public netCommunication {
private:
  struct timeval tmout;
public:
  masterCommunication(char* progN, char** progA, int nh, int waitSec);
  virtual ~masterCommunication();
  virtual int receiveData(netDataResult* rp);
};

#endif
