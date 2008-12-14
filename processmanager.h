#ifndef processmanager_h
#define processmanager_h

#include "datastructure.h"
#include "netcommunication.h"
#include "commandlineinfo.h"
#include "mathfunc.h"
#include "paramin.h"

/* The class runtime provides functions to set time for beginning of
 * execution and compute estimated run time. New run time is measured
 * from execution of startRun() to stopRun(a) in seconds.
 * Estimated run time is calculated as: (execution time * a) + (new runtime * (1 - a))
 * if estimated run time is not equal to -1.0.
 * if execution time is equal to -1.0 then execution time is set to new run time.
 * If run time is less than 1 sec, run time = 0.
 * WARNING: Might need a more accurate time function - now using
 * difftime(time_t t1, time_t t2) which returns 0 when difference in seconds < 1.
 * but we do not know the accuracy of difftime */

class RunTime {
private:
  time_t startExec;
  double execTime;
public:
  RunTime();
  ~RunTime();
  void startRun();
  void stopRun(double a);
  double getRunTime();
  time_t getStartTime();
  int isRunning();
  void setRunTime(double t);
};

/* The class ProcessManager is a manager for a group of processes were
 * each process has a unique task id (tid).
 * It provides functions to get information about the general state of
 * the processes and how many processes are available.
 * The ProcessManager dispatches processes by FIFO order (The first free
 * process to arrive is dispatched first) */

class ProcessManager {
protected:
  int errorNoProcesses;
  int errorWaitForProcesses;
  Queue* freeProcesses;
  int totalNumProc;
  int* procStat;
  int maxNumHosts;
  int pmCondor;
public:
  int* getStatus();
  ProcessManager();
  virtual ~ProcessManager();
  virtual void initializePM(int numProc, int condor);
  virtual void addProc(int id);
  virtual void addMoreProc(int id);
  int getNumFreeProc();
  int isFreeProc();
  virtual void setFreeProc(int id);
  int getTidToSend(NetCommunication* n);
  int checkForNewProcess(NetCommunication* n);
  virtual int getNextTidToSend(int numLeftToSend, NetCommunication* n);
  virtual void sent(int procID);
  int allReceived();
  int getStatus(int id);
  void setStatus(int id, int stat);
  void processDown(int id);
  int getNumGoodProc();
  void noProcessesRunning();
  void removeBadProc();
  int noAvailableProcesses();
  int waitForBetterProcesses();
};

/* The class WorkLoadScheduler is a derived class of ProcessManager. It
 * keeps track of running time of processes and dispatches processes
 * based on workload and running time of processes.
 * Runtime is measured from: when master sends data until master receives
 * it not actual runtime of process.*/

class WorkLoadScheduler : public ProcessManager {
private:
  double bestTime;
  double alpha;
  double hostMultiple;
  double besttimeMultiple;
  RunTime** runInfo;
public:
  WorkLoadScheduler(CommandLineInfo* info);
  ~WorkLoadScheduler();
  virtual void initializePM(int totalNumProc, int condor);
  virtual void addProc(int id);
  virtual void addMoreProc(int id);
  virtual void setFreeProc(int tid);
  virtual int getNextTidToSend(int numLeftToSend, NetCommunication* n);
  virtual void sent(int procID);
  int quickHostsAvailable();
  int quickBusyProcesses();
};

#endif
