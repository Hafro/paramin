#include "processmanager.h"

runtime::runtime() {
  execTime = -1.0;
  startExec = -1;
}

runtime::~runtime() {
}

void runtime::startRun() {
  startExec = time(NULL);
}

void runtime::stopRun(double a) {
  time_t stopExec;
  double newTime;
  stopExec = time(NULL);
  newTime = difftime(stopExec, startExec);
  if (execTime == -1.0)
    execTime = newTime;
  else {
    // Use last recorded run time and new time to estimate new run time
    execTime = (execTime * a) + (newTime * (1 - a));
  }
  startExec = -1;
}

double runtime::getRunTime() {
  return execTime;
}

void runtime::setRunTime(double t) {
  execTime = t;
}

time_t runtime::getStartTime() {
  return startExec;
}

int runtime::isRunning() {
  return !(startExec == -1);
}

processManager::processManager() {
  // Return value if no processes available.
  NO_PROCESSES = -1;
  // Return value if should wait for processes.
  WAIT_FOR_PROCESSES = -2;
  freeProcesses = new queue();
  totalNumProc = 0;
  procStat = NULL;
}

processManager::~processManager() {
  delete freeProcesses;
  freeProcesses = NULL;
  if (procStat != NULL) {
    delete [] procStat;
    procStat = NULL;
  }
}

void processManager::initializePM(int numProc) {
  int i;
  if (numProc <= 0) {
    cerr << "Error in processManager - number of processes must be positive\n";
    exit(EXIT_FAILURE);
  }
  totalNumProc = numProc;
  procStat = new int[totalNumProc];
  for (i = 0; i < totalNumProc; i++) {
    procStat[i] = -1;
    addProc(i);
    setStatus(i, 1);
  }
}

void processManager::addProc(int id) {
  if (id < 0 || id >= totalNumProc) {
    cerr << "Error in processManager - illegal process id " << id << endl;
    exit(EXIT_FAILURE);
  }
  if (freeProcesses->contains(id))
    cout << "Warning in processManager - process with id " << id << " already exists\n";
  else
    freeProcesses->put(id);
}

int processManager::allReceived() {
  int numGoodProc = getNumGoodProc();
  return (freeProcesses->getNumberOfItems() >= numGoodProc);
}

int processManager::getNumFreeProc() {
  int numFree;
  removeBadProc();
  numFree = freeProcesses->getNumberOfItems();
  return numFree;
}

void processManager::removeBadProc() {
  int id;
  int counter = 0;
  int numFreeProc = freeProcesses->getNumberOfItems();
  while (counter < numFreeProc) {
    id = freeProcesses->get();
    assert ((id >= 0) && (id < totalNumProc));
    if (procStat[id] == 1)
      freeProcesses->put(id);
    counter++;
  }
}

int processManager::isFreeProc() {
  int freeProc;
  removeBadProc();
  freeProc = !(freeProcesses->isEmpty());
  return freeProc;
}

int processManager::getNumGoodProc() {
  int i;
  int numGoodProc = 0;
  for (i = 0; i < totalNumProc; i++) {
    if(procStat[i] == 1)
      numGoodProc++;
  }
  return numGoodProc;
}

void processManager::setFreeProc(int id) {
  if (id < 0 || id >= totalNumProc) {
    cerr << "Error in processManager - illegal process id " << id << endl;
    exit(EXIT_FAILURE);
  }
  if (freeProcesses->contains(id))
    cout << "Warning in processManager - when setting process id " << id << endl;
  else
    freeProcesses->put(id);
}

int processManager::getNextTidToSend(netCommunication* n) {
  int tid;
  n->getHealthOfProcesses(procStat);
  removeBadProc();
  if (freeProcesses->isEmpty())
    return NO_PROCESSES;
  else {
    tid = freeProcesses->get();
    return tid;
  }
}

int processManager::getNextTidToSend(int numLeftToSend, netCommunication* n) {
  int tid;
  n->getHealthOfProcesses(procStat);
  removeBadProc();
  if (freeProcesses->isEmpty())
    return NO_PROCESSES;
  else {
    tid = freeProcesses->get();
    return tid;
  }
}

void processManager::sent(int processId) {
}

int* processManager::getStatus() {
  return procStat;
}

int processManager::getStatus(int id) {
  if ((id < 0) || (id >= totalNumProc)) {
    cerr << "Error in processManager - illegal process id " << id << endl;
    exit(EXIT_FAILURE);
  }
  return procStat[id];
}

void processManager::setStatus(int id, int stat) {
  if ((id < 0) || (id >= totalNumProc)) {
    cerr << "Error in processManager - illegal process id " << id << endl;
    exit(EXIT_FAILURE);
  }
  if ((stat != 1) && (stat != -1)) {
    cerr << "Error in processManager - illegal status " << stat << endl;
    exit(EXIT_FAILURE);
  }
  procStat[id] = stat;
}

void processManager::processDown(int id) {
  setStatus(id, -1);
}

void processManager::noProcessesRunning() {
  int i;
  for (i = 0; i < totalNumProc; i++)
    procStat[i] = -1;
  while (!freeProcesses->isEmpty())
    freeProcesses->get();
}

int processManager::NO_AVAILABLE_PROCESSES() {
  return NO_PROCESSES;
}

int processManager::WAIT_FOR_BETTER_PROCESSES() {
  return WAIT_FOR_PROCESSES;
}

workLoadScheduler::workLoadScheduler(double a) {
  alpha = a;
  runInfo = NULL;
  bestTime = 360000.0;
}

workLoadScheduler::~workLoadScheduler() {
  int i;
  if (runInfo != NULL) {
    for (i = 0; i < totalNumProc; i++)
      delete runInfo[i];
    delete runInfo;
  }
}

void workLoadScheduler::initializePM(int totalNumProc) {
  runInfo = new runtime*[totalNumProc];
  processManager::initializePM(totalNumProc);
}

void workLoadScheduler::addProc(int id) {
  processManager::addProc(id);
  runInfo[id] = new runtime();
}

void workLoadScheduler::setFreeProc(int tid) {
  runInfo[tid]->stopRun(alpha);
  if (bestTime > runInfo[tid]->getRunTime())
    bestTime = runInfo[tid]->getRunTime();
  processManager::setFreeProc(tid);
}

int workLoadScheduler::getNextTidToSend(int numLeftToSend, netCommunication* n) {
  int id, q;
  int nextTid = -1;
  n->getHealthOfProcesses(procStat);
  processManager::removeBadProc();
  if (freeProcesses->isEmpty()) {
    return NO_PROCESSES;
  } else if (numLeftToSend >= 2*totalNumProc) {
    nextTid = freeProcesses->get();
    return nextTid;
  } else {
    id = quickHostsAvailable();
    if (id >= 0)
      return id;

    q = quickBusyProcesses();
    if (q == 1)
      return WAIT_FOR_PROCESSES;

    id = freeProcesses->get();
    return id;
  }
}

void workLoadScheduler::sent(int procId) {
  if (procId < 0 || procId >= totalNumProc) {
    cerr << "Error in workLoadScheduler - illegal process id " << procId << endl;
    exit(EXIT_FAILURE);
  }
  runInfo[procId]->startRun();
}

int workLoadScheduler::quickHostsAvailable() {
  int available = 0, counter = 0;
  while (!(available) && counter < totalNumProc) {
    if ((!runInfo[counter]->isRunning()) && (procStat[counter] == 1))
      available = (runInfo[counter]->getRunTime() <= 2 * bestTime);
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

int workLoadScheduler::quickBusyProcesses() {
  int available = 0, counter = 0;
  while ((available == 0) && (counter < totalNumProc)) {
    if ((runInfo[counter]->isRunning() == 1) && (procStat[counter] == 1))
      available = ((runInfo[counter]->getRunTime()) <= 2 * bestTime);
    counter++;
  }

  if (available == 1)
    return available;
  else
    return -1;
}
