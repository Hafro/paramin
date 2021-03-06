#ifndef netcommunication_h
#define netcommunication_h

#include "paramin.h"
#include "mpi.h"

#include "netdata.h"
#include "pvmconstants.h"
#include "doublevector.h"
#include "commandlineinfo.h"
#include "parametervector.h"

// AJ, 26.02.99
/**
 * \class NetCommunication
 * \brief The class NetCommunication provides functions for a master process to handle netcommunication with its slave processes using Parallell Virtual Machine(PVM) for actual communication. To successfully start netcommunication with a group of processes, pvmd must have been started on all hosts participating in PVM.NetCommunication includes functions to enroll in PVM providing pvmd is running on all machines participating in PVM, spawning new processes and halting running processes. Maximum number of hosts which can participate in pvm is equal to MAXHOSTS. It also includes functions to send data of type NetDataVariables and CharPtrVector to a spawned process and receiving data of type NetDataResult from a spawned process. functions which return information about the status of netcommunication 
 */

class NetCommunication {
private:
  int convergedSA;
  int convergedHJ;
  int convergedBFGS;
  double likelihoodSA;
  double likelihoodHJ;
  double likelihoodBFGS;
protected:
  PVMConstants* pvmConst;
  int nHostInn;
  // pvm variables, used to start PVM and spawn processes on hosts
  /**
   * \brief tid of master process (me)
   */
  int mytid;         
  /**
   * \brief number of different architectures taking part in VM.
   */
  int narch;         
  int nhost;
  int numarg;
  int* tids;
  int* status;
  /**
   * \brief array that contains information about each host
   */
  struct pvmhostinfo *hostp;    
  /**
   * \brief array that contains information about each task
   */
  struct pvmtaskinfo *taskp;    
  /**
   * \brief number of variables in x in netDatavariables
   */
  int numVar;                   
  /**
   * \brief number of processes started
   */
  int numProcesses;             
  /**
   * \brief number of running processes
   */
  int numGoodProcesses;       
  /**
   * \brief name of program which will be started on slaves
   */
  char* slaveProgram;           
  /**
   * \brief arguments for slaveProgram
   */
  char** slaveArguments;        

  int NETSTARTED;
  int ERROR;
  int SUCCESS;

  int* hostTids;
  int* dataIDs;

public:
  // this should be changed to use commandline...
	MPI_Comm intercomm;
  NetCommunication(const CharPtrVector& funcNameArgs, int nh);
  virtual ~NetCommunication();
  int maxNumHosts;
  int startPVM();
  int startProcesses();
  int sendInitialMessage(int id);
  int startNetCommunication();
  void stopNetCommunication();
  int checkProcess(int id);
  void checkProcesses();
  void getHealthOfProcesses(int* procTids);
  int sendData(NetDataVariables* sendP, int processID);
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
  void setConvergedSA(int set) { convergedSA = set; };
  int getConvergedSA() { return convergedSA; };
  double getLikelihoodSA() const { return likelihoodSA; };
  void setLikelihoodSA(double set) { likelihoodSA = set; };
  void setConvergedBFGS(int set) { convergedBFGS = set; };
  int getConvergedBFGS() { return convergedBFGS; };
  double getLikelihoodBFGS() const { return likelihoodBFGS; };
  void setLikelihoodBFGS(double set) { likelihoodBFGS = set; };
  void setConvergedHJ(int set) { convergedHJ = set; };
  int getConvergedHJ() { return convergedHJ; };
  double getLikelihoodHJ() const { return likelihoodHJ; };
  void setLikelihoodHJ(double set) { likelihoodHJ = set; };

  int sendData(const ParameterVector& sendP);
  int sendData(const ParameterVector& sendP, int processID);
  int sendBoundData(const DoubleVector& sendP);
  int sendBoundData(const DoubleVector& sendP, int processID);

};

/**
 * \class MasterCommunication
 * \brief The class MasterCommunication is a derived class of NetCommunication. The only difference between NetCommunication and MasterCommunication is that when MasterCommunication receives messages it will wait for a message for a specified time but NetCommunication will wait forever 
*/

class MasterCommunication : public NetCommunication {
private:
  struct timeval* tmout;
public:
  MasterCommunication(CommandLineInfo* info);
  virtual ~MasterCommunication();
  virtual int receiveData(NetDataResult* rp);
};

#endif
