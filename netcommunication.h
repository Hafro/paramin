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
 * The class NetCommunication provides functions for a master process to
 * handle netcommunication with its slave processes using Parallell
 * Virtual Machine (PVM) for actual communication. To successfully start
 * netcommunication with a group of processes, pvmd must have been
 * started on all hosts participating in PVM.
 * NetCommunication includes functions to enroll in PVM providing pvmd is
 * running on all machines participating in PVM, spawning new processes
 * and halting running processes. Maximum number of hosts which can participate
 * in pvm is equal to MAXHOSTS.
 * It also includes functions to send data of type NetDataVariables and
 * VectorOfCharPtr to a spawned process and receiving data of type
 * NetDataResult from a spawned process.
 * functions which return information about the status of netcommunication */

class NetCommunication {
protected:
  PVMConstants* pvmConst;
  int nHostInn;
  // pvm variables, used to start PVM and spawn processes on hosts
  int mytid;         // tid of master process (me)
  int narch;         // number of different architectures taking part in VM.
  int nhost;
  int* tids;
  int tidsCounter;
  int* status;
  struct pvmhostinfo *hostp;    // array that contains infformation about each host
  struct pvmtaskinfo *taskp;    //Array that contains information about each task
  int numVar;                   // number of variables in x in netDatavariables
  int numProcesses;             // number of processes started
  int numGoodProcesses;         // number of running processes
  char* slaveProgram;           // name of program which will be started on slaves
  char** slaveArguments;        // arguments for slaveProgram

  int NETSTARTED;
  int ERROR;
  int SUCCESS;

  int* hostTids;
  int* dataIds;
  int maxNumHosts;

  int NONTORECEIVE;
  int NEEDMOREHOSTS;
  int NEEDMOREDATA;
  int WAITFORPROCESS;
  int DATANOTSENT;




public:
  NetCommunication(char* progN, char** progA, int nh);
  virtual ~NetCommunication();
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
  int sendData(NetDataVariables* sendP, int processId);
  virtual int receiveData(NetDataResult* rp);
  void setNumInSendVar(int nVar);
  int getNumHosts();
  int getNumVar();
  int getNumProcesses();
  int getNumRunningProcesses();
  void printErrorMsg(const char* errorMsg);
  int netCommStarted();
  int netError();
  int netSuccess();

  //Added for condor
  void checkHostsForSuspend();
  void checkHostsForDelete();
  void checkHostsForResume();
  int checkHostsForAdded();
  int checkHostForSuspendReturnsDataid(int* procTids);
  int checkHostForDeleteReturnsDataid(int* procTids);
  int checkHostForResumeReturnsDataid(int* procTids);
  virtual int receiveDataNonBlocking(NetDataResult* rp);
  int startOneProcess(int processNum, int processTid);
  int spawnAndStartOneProcess(int processNumber);
  int sendInitialMessageToTid(int tid, int processNum);
  int spawnOneProgram(int vectorNumber);
  int spawnOneMoreProgram(int& newTid, int vectorNumber);
  int netDataNotSent();
  int netNoneToReceive();
  int netNeedMoreHosts();
  int netNeedMoreData();
  int netWaitForBetterProcesses();
  int getHealthOfProcessesAndHostAdded(int* procTids);
  int probeForReceiveData();
  int checkProcessByTid(int tidToCheck, int processNum);
  int sendData(NetDataVariables* sendP, int processId, int dataId);



#ifdef GADGET_NETWORK
  int sendData(VectorOfCharPtr sendP);
  int sendData(VectorOfCharPtr sendP, int processId);
  int sendBoundData(vector sendP);
  int sendBoundData(vector sendP, int processId);
#endif
};

/* The class MasterCommunication is a derived class of
 * NetCommunication. The only difference between NetCommunication and
 * MasterCommunication is that when MasterCommunication receives
 * messages it will wait for a message for a specified time but
 * NetCommunication will wait forever */

class MasterCommunication : public NetCommunication {
private:
  struct timeval tmout;
public:
  MasterCommunication(char* progN, char** progA, int nh, int waitSec);
  virtual ~MasterCommunication();
  virtual int receiveData(NetDataResult* rp);
  virtual int receiveDataNonBlocking(NetDataResult* rp);

};

#endif
