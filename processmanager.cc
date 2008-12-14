#include "processmanager.h"

RunTime::RunTime() {
  execTime = -1.0;
  startExec = -1;
}

RunTime::~RunTime() {
}

void RunTime::startRun() {
  startExec = time(NULL);

}

void RunTime::stopRun(double a) {
  time_t stopExec;
  double newTime;
  stopExec = time(NULL);
  newTime = difftime(stopExec, startExec);

  //minimum run time is set to 1
  if ((fabs(newTime) < verysmall)) {
    newTime = execTime;
    if (newTime == -1.0)
      execTime = 1.0;
  }    

  if (execTime == -1.0)
    execTime = newTime;
  else
    execTime = (execTime * a) + (newTime * (1 - a));
  startExec = -1;
}

double RunTime::getRunTime() {
  return execTime;
}

void RunTime::setRunTime(double t) {
  execTime = t;
}

time_t RunTime::getStartTime() {
  return startExec;
}

int RunTime::isRunning() {
  return !(startExec == -1);
}

ProcessManager::ProcessManager() {
  maxNumHosts = 500;
  // Return value if no processes available.
  errorNoProcesses = -1;
  // Return value if should wait for processes.
  errorWaitForProcesses = -2;
  freeProcesses = new Queue();
  totalNumProc = 0;
  pmCondor = 0;
  procStat = NULL;
}

ProcessManager::~ProcessManager() {
  delete freeProcesses;
  freeProcesses = NULL;
  if (procStat != NULL) {
    delete[] procStat;
    procStat = NULL;
  }
}

void ProcessManager::initializePM(int numProc, int condor) {
  int i;
  if (numProc <= 0) {
    cerr << "Error in processmanager - number of processes must be positive\n";
    exit(EXIT_FAILURE);
  }
  totalNumProc = numProc;
  pmCondor = condor;

  if (pmCondor)
    procStat = new int[maxNumHosts];
  else
    procStat = new int[totalNumProc];
  for (i = 0; i < totalNumProc; i++) {
    procStat[i] = -1;
    addProc(i);
    setStatus(i, 1);
  }
}

void ProcessManager::addProc(int id) {
  if (id < 0 || id >= totalNumProc) {
    cerr << "Error in processmanager - invalid process id " << id << endl;
    exit(EXIT_FAILURE);
  }
  if (!(freeProcesses->contains(id))) {
    freeProcesses->put(id);
  }
  else
    cerr << "Warning in processmanager - process with id " << id << " already exists\n";
}

//jongud Added method to add more processes than to begin with
void ProcessManager::addMoreProc(int id) {
  if (id < 0 || id >= (totalNumProc + 1)) {
    cerr << "Error in processmanager - invalid process id " << id << endl;
    exit(EXIT_FAILURE);
  }
  if (!(freeProcesses->contains(id))) {
    freeProcesses->put(id);
    totalNumProc++;
  } else
    cerr << "Warning in processmanager - process with id " << id << " already freed\n";
}


int ProcessManager::allReceived() {
  int numGoodProc = getNumGoodProc();
  return (freeProcesses->getNumItems() >= numGoodProc);
}

int ProcessManager::getNumFreeProc() {
  removeBadProc();
  return (freeProcesses->getNumItems());
}

void ProcessManager::removeBadProc() {
  int id;
  int counter = 0;
  int numFreeProc = freeProcesses->getNumItems();
  while (counter < numFreeProc) {
    id = freeProcesses->get();
    assert((id >= 0) && (id < totalNumProc));
    if (procStat[id] == 1)
      freeProcesses->put(id);
    counter++;
  }
}

int ProcessManager::isFreeProc() {
  removeBadProc();
  return (!(freeProcesses->isEmpty()));
}

int ProcessManager::getNumGoodProc() {
  int i;
  int numGoodProc = 0;
  for (i = 0; i < totalNumProc; i++) {
    if (procStat[i] == 1)
      numGoodProc++;
  }
  return numGoodProc;
}

void ProcessManager::setFreeProc(int id) {
  if (id < 0 || id >= totalNumProc) {
    cerr << "Error in processmanager - invalid process id " << id << endl;
    exit(EXIT_FAILURE);
  }
  if (!(freeProcesses->contains(id)))
    freeProcesses->put(id);
  else
    cerr << "Warning in processmanager - process with id " << id << " already freed\n";
}

int ProcessManager::getTidToSend(NetCommunication* n) {
  int tid;
  n->getHealthOfProcesses(procStat);
  removeBadProc();
  if (freeProcesses->isEmpty())
    return errorNoProcesses;
  else {
    tid = freeProcesses->get();
    return tid;
  }
}

int ProcessManager::checkForNewProcess(NetCommunication* n) {
  int tid;
  tid = n->getHealthOfProcessesAndHostAdded(procStat);
  removeBadProc();
  if (tid > -1)
    addMoreProc(tid);
  return tid;
}

int ProcessManager::getNextTidToSend(int numLeftToSend, NetCommunication* n) {
  int tid;
  tid = n->getHealthOfProcessesAndHostAdded(procStat);
  removeBadProc();
  if (tid > -1)
    addMoreProc(tid);
  if (freeProcesses->isEmpty())
    return errorNoProcesses;
  else {
    tid = freeProcesses->get();
    return tid;
  }
}

void ProcessManager::sent(int processID) {
}

int* ProcessManager::getStatus() {
  return procStat;
}

int ProcessManager::getStatus(int id) {
  if ((id < 0) || (id >= totalNumProc)) {
    cerr << "Error in processmanager - invalid process id " << id << endl;
    exit(EXIT_FAILURE);
  }
  return procStat[id];
}

void ProcessManager::setStatus(int id, int stat) {
  if ((id < 0) || (id >= totalNumProc)) {
    cerr << "Error in processmanager - invalid process id " << id << endl;
    exit(EXIT_FAILURE);
  }
  if ((stat != 1) && (stat != -1)) {
    cerr << "Error in processmanager - invalid status " << stat << endl;
    exit(EXIT_FAILURE);
  }
  procStat[id] = stat;
}

void ProcessManager::processDown(int id) {
  setStatus(id, -1);
}

void ProcessManager::noProcessesRunning() {
  int i;
  for (i = 0; i < totalNumProc; i++)
    procStat[i] = -1;
  while (!freeProcesses->isEmpty())
    freeProcesses->get();
}

int ProcessManager::noAvailableProcesses() {
  return errorNoProcesses;
}

int ProcessManager::waitForBetterProcesses() {
  return errorWaitForProcesses;
}


WorkLoadScheduler::WorkLoadScheduler(CommandLineInfo* info) {
  alpha = info->getRunMultiple();
  hostMultiple = info->getHostMultiple();
  besttimeMultiple = info->getBestMultiple();
  runInfo = NULL;
  bestTime = 360000.0;
}

WorkLoadScheduler::~WorkLoadScheduler() {
  int i;
  if (runInfo != NULL) {
    for (i = 0; i < totalNumProc; i++)
      delete runInfo[i];
    delete[] runInfo;
  }
}

void WorkLoadScheduler::initializePM(int totalNumProc, int condor) {
  runInfo = new RunTime*[totalNumProc];
  ProcessManager::initializePM(totalNumProc, condor);
}

void WorkLoadScheduler::addProc(int id) {
  ProcessManager::addProc(id);
  runInfo[id] = new RunTime();
}

void WorkLoadScheduler::addMoreProc(int id) {
  ProcessManager::addMoreProc(id);
  runInfo[id] = new RunTime();
}

void WorkLoadScheduler::setFreeProc(int tid) {
  runInfo[tid]->stopRun(alpha);
  if (bestTime > runInfo[tid]->getRunTime())
    bestTime = runInfo[tid]->getRunTime();
  ProcessManager::setFreeProc(tid);
}

int WorkLoadScheduler::getNextTidToSend(int numLeftToSend, NetCommunication* n) { 
  int id, q;
  int nextTid = -1;
  n->getHealthOfProcesses(procStat);
  ProcessManager::removeBadProc();
  if (freeProcesses->isEmpty()) {
    return errorNoProcesses;
  } else if (numLeftToSend >= hostMultiple * totalNumProc) {
    nextTid = freeProcesses->get();
    return nextTid;
  } else {
    id = quickHostsAvailable();
    if (id >= 0)
      return id;

    q = quickBusyProcesses();
    if (q == 1)
      return errorWaitForProcesses;

    id = freeProcesses->get();
    return id;
  }
										  
}

void WorkLoadScheduler::sent(int procID) {
  if (procID < 0 || procID >= totalNumProc) {
    cerr << "Error in workloadscheduler - invalid process id " << procID << endl;
    exit(EXIT_FAILURE);
  }
  runInfo[procID]->startRun();
}

int WorkLoadScheduler::quickHostsAvailable() {
  int available = 0, counter = 0;
  while (!(available) && counter < totalNumProc) {
    if ((!runInfo[counter]->isRunning()) && (procStat[counter] == 1))
      available = (runInfo[counter]->getRunTime() <= besttimeMultiple * bestTime);
    counter++;
  }

  if (available == 1) {
    int tid = freeProcesses->get();
    while (tid != (counter - 1)) {
      freeProcesses->put(tid);
      tid = freeProcesses->get();
    }
    return tid;
  } else
    return -1;
}

int WorkLoadScheduler::quickBusyProcesses() {
  int available = 0, counter = 0;
  while ((available == 0) && (counter < totalNumProc)) {
    if ((runInfo[counter]->isRunning() == 1) && (procStat[counter] == 1))
      available = ((runInfo[counter]->getRunTime()) <= besttimeMultiple * bestTime);
    counter++;
  }

  if (available == 1)
    return 1;
  else
    return -1;
}
