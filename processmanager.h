#ifndef processmanager_h
#define processmanager_h

#include "datastructure.h"
#include "netcommunication.h"
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

class runtime {
private:
  time_t startExec;
  double execTime;
public:
  runtime();
  ~runtime();
  void startRun();
  void stopRun(double a);
  double getRunTime();
  time_t getStartTime();
  int isRunning();
  void setRunTime(double t);
};

/* The class processManager is a manager for a group of processes were
 * each process has a unique task id (tid).
 * It provides functions to get information about the general state of
 * the processes and how many processes are available.
 * The processManager dispatches processes by FIFO order (The first free
 * process to arrive is dispatched first) */

class processManager {
protected:
  int NO_PROCESSES;
  int WAIT_FOR_PROCESSES;
  queue* freeProcesses;
  int totalNumProc;
  int* procStat;
public:
  int* getStatus();
  processManager();
  virtual ~processManager();
  virtual void initializePM(int numProc);
  virtual void addProc(int id);
  int getNumFreeProc();
  int isFreeProc();
  virtual void setFreeProc(int id);
  int getNextTidToSend(netCommunication* n);
  virtual int getNextTidToSend(int numLeftToSend, netCommunication* n);
  virtual void sent(int procId);
  int allReceived();
  int getStatus(int id);
  void setStatus(int id, int stat);
  void processDown(int id);
  int getNumGoodProc();
  void noProcessesRunning();
  void removeBadProc();
  int NO_AVAILABLE_PROCESSES();
  int WAIT_FOR_BETTER_PROCESSES();
};

/* The class workLoadScheduler is a derived class of processManager. It
 * keeps track of running time of processes and dispatches processes
 * based on workload and running time of processes.
 * Runtime is measured from: when master sends data until master receives
 * it not actual runtime of process.*/

class workLoadScheduler : public processManager {
private:
  double bestTime;
  double alpha;
  runtime** runInfo;
public:
  workLoadScheduler(double a);
  ~workLoadScheduler();
  virtual void initializePM(int totalNumProc);
  virtual void addProc(int id);
  virtual void setFreeProc(int tid);
  virtual int getNextTidToSend(int numLeftToSend, netCommunication* n);
  virtual void sent(int procId);
  int quickHostsAvailable();
  int quickBusyProcesses();
};

#endif
