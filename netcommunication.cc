#include "netcommunication.h"

NetCommunication::NetCommunication(const VectorOfCharPtr& funcNameArgs, int nh) {
  // pvmConst contains information about which tags and dataencoding to use
  pvmConst = new PVMConstants();
  nHostInn = nh;

  int i;
  if (funcNameArgs.Size() <= 0) {
    cerr << "Must have name of the function to start on slaves\n";
    exit(EXIT_FAILURE);
  }
  slaveProgram = new char[strlen(funcNameArgs[0]) + 1];
  strcpy(slaveProgram, funcNameArgs[0]);
  numarg = funcNameArgs.Size() - 1;
  slaveArguments = new char*[numarg + 1];
  for (i = 0; i < numarg; i++) {
    slaveArguments[i] = new char[strlen(funcNameArgs[i + 1]) + 1];
    strcpy(slaveArguments[i], funcNameArgs[i + 1]);
  }
  slaveArguments[numarg] = NULL;

  numVar = -1;
  mytid = -1;
  nhost = 0;
  narch = 0;
  numProcesses = 0;
  numGoodProcesses = 0;
  NETSTARTED = 0;
  tids = NULL;
  status = NULL;
  ERROR = -1;
  SUCCESS = 1;

  likelihoodHJ = 0.0;
  likelihoodSA = 0.0;
  likelihoodBfgs = 0.0;
  convergedSA = 0;
  convergedHJ = 0;
  convergedBfgs = 0;

  //For condor
  NONTORECEIVE = 0;
  tidsCounter = 0;
  maxNumHosts = 500;
  NEEDMOREHOSTS = 2;
  NEEDMOREDATA = 3;
  WAITFORPROCESS = 4;
  DATANOTSENT = 5;
}

NetCommunication::~NetCommunication() {
  int i;
  if (tids != NULL)
    delete[] tids;
  if (status != NULL)
    delete[] status;
  if (hostTids != NULL)
    delete[] hostTids;
  if (dataIds != NULL)
    delete[] dataIds;
  delete[] slaveProgram;
  for (i = 0; i < numarg; i++)
    delete[] slaveArguments[i];
  delete[] slaveArguments;
  if (NETSTARTED == 1)
    stopNetCommunication();
  delete pvmConst;
}

// ********************************************************
// Functions for starting and stopping netcommunication
// ********************************************************
int NetCommunication::startPVM() {
  int info;

  if (mytid < 0) {
    // have not yet enrolled in PVM
    mytid = pvm_mytid();
    if (mytid < 0) {
      printErrorMsg("Error in netcommunication - PVM not started");
      return ERROR;
    }

    info = pvm_config(&nhost, &narch, &hostp);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - PVM not responding");
      return ERROR;
    }

    assert(nhost > 0);
    tids = new int[maxNumHosts];
    status = new int[maxNumHosts];
    hostTids = new int[maxNumHosts];  //Added jongud
    dataIds = new int[maxNumHosts];   //Added jongud

    if (nHostInn > 0)
      nhost = nHostInn;

    // export PWD to all processes spawned
    char* tempString;
    tempString = new char[strlen("PWD") + 1];
    strcpy(tempString, "PWD");
    // Take out for multiprocessor maybe also Norway??
    info = pvm_export(tempString);
    delete[] tempString;
    return SUCCESS;

  } else {
    cerr << "Warning in netcommunication - already enrolled in PVM\n";
    return SUCCESS;
  }
}

int NetCommunication::startNetCommunication() {
  int i, OK, info;

  if (NETSTARTED == 1 && mytid >= 0) {
    // have alredy enrolled in pvm and spawned program on slaves
    cerr << "Warning in netcommunication - already enrolled in PVM and running " << slaveProgram << " on slaves\n";
    return SUCCESS;

  } else {
    if (numVar <= 0) {
      cerr << "Error in netcommunication - number of variables must be positive\n";
      return ERROR;
    }

    OK = startPVM();
    if (OK == 1) {
      for (i = 0; i < nhost; i++) {
        tidsCounter = i;
        OK = spawnOneProgram(tidsCounter);
        if (OK == ERROR) {
          cerr << "Error in netcommunication - unable to spawn process\n";
          return ERROR;
        }
      }
    }

    info = pvm_config(&nhost, &narch, &hostp);
    for (i = 0; i < nhost; i++) {
      /* hosts to be monitored for deletion, suspension and resumption*/
      hostTids[i] = hostp[i].hi_tid;
    }

#ifdef CONDOR
    //jongud added pvm_notify for if host get suspended
    info = pvm_notify(PvmHostSuspend, pvmConst->getHostSuspendTag(), nhost, hostTids);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - pvmnotify failed");
      stopNetCommunication();
      return ERROR;
    }
#endif

    //jongud added pvm_notify for if host is deleted
    info = pvm_notify(PvmHostDelete, pvmConst->getHostDeleteTag(), nhost, hostTids);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - pvmnotify failed");
      stopNetCommunication();
      return ERROR;
    }

#ifdef CONDOR
    //jongud added pvm_notify for if host resumes
    info = pvm_notify(PvmHostResume, pvmConst->getHostResumeTag(), nhost, hostTids);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - pvmnotify failed");
      stopNetCommunication();
      return ERROR;
    }
#endif

    //jongud added pvm_notify for if host is added
    info = pvm_notify(PvmHostAdd, pvmConst->getAddHostTag(), -1, 0);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - pvmnotify failed");
      stopNetCommunication();
      return ERROR;
    }

    if (OK == 1) {
      // Have started slaveProgram slaveArguments on all nhost hosts.
      // send initial info to all slave processes
      OK = startProcesses();
      if (OK == 1) {
        NETSTARTED = 1;
        return SUCCESS;
      } else if (OK == -1) {
        return ERROR;
      } else {
        printErrorMsg("Error in netcommunication - unrecognised return value");
        stopNetCommunication();
        return ERROR;
      }

    } else if (OK == 0) {
      // could not spawn all nhost processes
      stopNetCommunication();
      return ERROR;
    } else {
      printErrorMsg("Error in netcommunication - unrecognised return value");
      stopNetCommunication();
      return ERROR;
    }
  }
}

void NetCommunication::stopNetCommunication() {
  int i, tid, info, numTasks;
  int stopparam = -1;

  tid = pvm_mytid();
  if (tid > 0) {
    // pvmd is up and running and have joined pvm
    if (NETSTARTED == 1) {
      // have successfully spawned slaves and sent initial message to them
      info = pvm_initsend(pvmConst->getDataEncode());
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to stop communication");

      } else {
        info = pvm_pkint(&stopparam, 1, 1);
        if (info < 0) {
          printErrorMsg("Error in netcommunication - unable to stop communication");

        } else {
          //Checking the status of processes and shutting them down.
          //jongud. PVM is checked to see id of processes
          int* tidsArray = new int[maxNumHosts];
          info = pvm_tasks(0, &numTasks, &taskp);
          for (i = 0; i < numTasks; i++)
            tidsArray[i] = taskp[i].ti_tid;

          info = pvm_mcast(tidsArray, numTasks, pvmConst->getStopTag());
          if (info < 0) {
            printErrorMsg("Error in netcommunication - unable to stop communication");
          }
	  delete[] tidsArray;
        }
      }
    }
    pvm_exit();
  }
  mytid = -1;
  NETSTARTED = 0;
}

int NetCommunication::spawnProgram() {
  int i, nspawn;

  // Round-robin assignment is used to distribute the nhost processes on
  // hosts participating. As nhost = total number of hosts participating,
  // one process is started on each host. If startPVM is changed so that
  // continue even though not all hosts can participate then must change
  // this or set nhost to actual hosts .....

  i = pvm_catchout(stdout);
  nspawn = pvm_spawn(slaveProgram, slaveArguments, PvmTaskDefault, NULL, nhost, tids);
  //nspawn = pvm_spawn(slaveProgram, slaveArguments, 1024 + 1, "ask.ii.uib.no", nhost, tids);

  if (nspawn < 0) {
    printErrorMsg("Error in netcommunication - unable to spawn process");
    stopNetCommunication();
    exit(EXIT_FAILURE);

  } else if (nspawn < nhost) {
    printErrorMsg("Error in netcommunication - unable to spawn process");
    return ERROR;

  } else {
    numProcesses = nhost;
    numGoodProcesses = nhost;
    for (i = 0; i < numProcesses; i++)
      status[i] = 1;
    return SUCCESS;
  }
}

int NetCommunication::spawnOneProgram(int vectorNumber) {
  int i, nspawn;

  i = pvm_catchout(stdout);
  nspawn = pvm_spawn(slaveProgram, slaveArguments, PvmTaskDefault, NULL, 1, &tids[vectorNumber]);
  if (nspawn < 0) {
    printErrorMsg("Error in netcommunication - unable to spawn one process");
    stopNetCommunication();
    exit(EXIT_FAILURE);

  } else if (tids[vectorNumber] < 0) {
    printErrorMsg("Error in netcommunication - unable to spawn one process");
    return ERROR;

  } else {
    numProcesses++;
    numGoodProcesses++;
    status[vectorNumber] = 1;
    return SUCCESS;
  }
}

int NetCommunication::spawnOneMoreProgram(int& newTid, int vectorNumber) {
  int i, nspawn;
  i = pvm_catchout(stdout);

  int* newTidVector = new int[1];
  nspawn = pvm_spawn(slaveProgram, slaveArguments, PvmTaskDefault, NULL, 1, newTidVector);
  newTid = newTidVector[0];
  delete[] newTidVector;

  if (nspawn < 0) {
    printErrorMsg("Error in netcommunication - unable to spawn one more process");
    stopNetCommunication();
    exit(EXIT_FAILURE);

  } else if (newTid < 0) {
    printErrorMsg("Error in netcommunication - unable to spawn one more process");
    return ERROR;

  } else {
    numProcesses++;
    numGoodProcesses++;
    status[vectorNumber] = 1;
    return SUCCESS;
  }
}

int NetCommunication::startProcesses() {
  //Send number of variables, group name and number of processes to spawned processes
  int cansend = 1;
  int i, info;

  info = pvm_notify(PvmTaskExit, pvmConst->getDiedTag(), nhost, tids);
  if (info < 0) {
    printErrorMsg("Error in netcommunication - pvmnotify failed");
    stopNetCommunication();
    return ERROR;
  }

  for (i = 0; i < nhost; i++) {
    // send initial message to all spawned processes
    cansend = sendInitialMessage(i);
    if (cansend == -1) {
      // Error occured in sending inital message to process with id = i
      printErrorMsg("Error in netcommunication - unable to send message");
      return ERROR;

    } else if (cansend == 0) {
      printErrorMsg("Error in netcommunication - unable to send message");
      status[i] = -1;
      return ERROR;

    } else if (cansend == 1) {
      status[i] = 1;

    } else {
      printErrorMsg("Error in netcommunication - unrecognised return value");
      stopNetCommunication();
      return ERROR;
    }
  }
  return SUCCESS;
}

int NetCommunication::startOneProcess(int processNum, int processTid) {
  int info, cansend;
  int tidToNotify[] = {processTid};
  int hostTidToNotify[] = {hostTids[processNum]};

  //jongud. pvm_notify for 1 process.
  info = pvm_notify(PvmTaskExit, pvmConst->getDiedTag(), 1, tidToNotify);
  if (info < 0) {
    printErrorMsg("Error in netcommunication - pvmnotify failed");
    stopNetCommunication();
    return ERROR;
  }

#ifdef CONDOR
  //jongud added pvm_notify for if host get suspended
  info = pvm_notify(PvmHostSuspend, pvmConst->getHostSuspendTag(), 1, hostTidToNotify);
  if (info < 0) {
    printErrorMsg("Error in netcommunication - pvmnotify failed");
    stopNetCommunication();
    return ERROR;
  }
#endif

  //jongud added pvm_notify for if host is deleted
  info = pvm_notify(PvmHostDelete, pvmConst->getHostDeleteTag(), 1, hostTidToNotify);
  if (info < 0) {
    printErrorMsg("Error in netcommunication - pvmnotify failed");
    stopNetCommunication();
    return ERROR;
  }

#ifdef CONDOR
  //jongud added pvm_notify for if host resumes
  info = pvm_notify(PvmHostResume, pvmConst->getHostResumeTag(), 1, hostTidToNotify);
  if (info < 0) {
    printErrorMsg("Error in netcommunication - pvmnotify failed");
    stopNetCommunication();
    return ERROR;
  }
#endif

  //Send initial message to the last spawned process, nhost == number of processes
  cansend = sendInitialMessageToTid(processTid, processNum);
  if (cansend == -1) {
    printErrorMsg("Error in netcommunication - unable to send message");
    return ERROR;
  } else if (cansend == 0) {
    status[processNum] = -1;
  } else if (cansend == 1) {
    status[processNum] = 1;
  } else {
    printErrorMsg("Error in netcommunication - unrecognised return value");
    stopNetCommunication();
    return ERROR;
  }

  // Have successfully sent starting message to slave
  tids[processNum] = processTid;
  return SUCCESS;
}

int NetCommunication::sendInitialMessage(int id) {
  int OK, info;

  if (id < 0 || id >= nhost) {
    printErrorMsg("Error in netcommunication - invalid slave ID");
    return 0;
  }

  // check if process with identity = id is up and running
  OK = checkProcess(id);
  if (OK == 1) {
    info = pvm_initsend(pvmConst->getDataEncode());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send initial message");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_pkint(&numVar, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send initial message");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_pkint(&id, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send initial message");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_send(tids[id], pvmConst->getStartTag());

    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send initial message");
      stopNetCommunication();
      return ERROR;
    }
    return SUCCESS;

  } else if (OK == -1) {
    printErrorMsg("Error in netcommunication - unable to check status");
    stopNetCommunication();
    return ERROR;

  } else if (OK == 0) {
    printErrorMsg("Error in netcommunication - unable to send initial message");
    return OK;

  } else {
    printErrorMsg("Error in netcommunication - unrecognised return value");
    stopNetCommunication();
    return ERROR;
  }
}

int NetCommunication::sendInitialMessageToTid(int tid, int processNum) {
  int OK;
  int info;

  if ((processNum < 0)|| (processNum >= numProcesses)) {
    printErrorMsg("Error in netcommunication - invalid slave ID");
    return 0;
  }

  // check if process with identity = id is up and running
  OK = checkProcessByTid(tid, processNum);
  if (OK == 1) {
    info = pvm_initsend(pvmConst->getDataEncode());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to start process");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_pkint(&numVar, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to start process");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_pkint(&processNum, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to start process");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_send(tid, pvmConst->getStartTag());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to start process");
      stopNetCommunication();
      return ERROR;
    }
    // sent successfully to slave with tids[id] numVar and id
    return SUCCESS;

  } else if (OK == -1) {
    printErrorMsg("Error in netcommunication - unable to check process");
    stopNetCommunication();
    return ERROR;
  } else if (OK == 0) {
    return OK;
  } else {
    printErrorMsg("Error in netcommunication - unrecognised return value");
    stopNetCommunication();
    return ERROR;
  }
}

int NetCommunication::sendNumberOfVariables() {
  int info;

  if (numVar <= 0) {
    printErrorMsg("Error in netcommunication - invalid number of variables");
    stopNetCommunication();
    return ERROR;
  }

  // tell slave program how many variables to expect in vector sent
  info = pvm_initsend(pvmConst->getDataEncode());
  if (info < 0) {
    printErrorMsg("Error in netcommunication - unable to send variables");
    stopNetCommunication();
    return ERROR;
  }

  info = pvm_pkint(&numVar, 1, 1);
  if (info < 0) {
    printErrorMsg("Error in netcommunication - unable to send variables");
    stopNetCommunication();
    return ERROR;
  }

  info = pvm_mcast(tids, nhost, pvmConst->getStartTag());
  if (info < 0) {
    printErrorMsg("Error in netcommunication - unable to send variables");
    stopNetCommunication();
    return ERROR;
  }
  return SUCCESS;
}

int NetCommunication::checkProcess(int id) {
  int info, bufId, recvTid;
  assert(id >= 0);
  assert(id < numProcesses);

  bufId = pvm_probe(tids[id], pvmConst->getDiedTag());
  if (bufId > 0) {
    // message has arrived from tids[id] that has halted
    info = pvm_recv(tids[id], pvmConst->getDiedTag());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to check process");
      return ERROR;
    }

    info = pvm_upkint(&recvTid, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to check process");
      return ERROR;
    }

    if (recvTid != tids[id])
      return ERROR;

    status[id] = -1;
    numGoodProcesses--;
    return 0;

  } else if (bufId < 0) {
    printErrorMsg("Error in netcommunication - unable to check process");
    return ERROR;
  } else {
    return SUCCESS;
  }
}

int NetCommunication::checkProcessByTid(int tidToCheck,int processNum) {
  int info, recvTid, bufId;

  bufId = pvm_probe(tidToCheck, pvmConst->getDiedTag());
  if (bufId > 0) {
    // message has arrived from tids[id] that has halted
    info = pvm_recv(tidToCheck, pvmConst->getDiedTag());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to check process");
      return ERROR;
    }

    info = pvm_upkint(&recvTid, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to check process");
      return ERROR;
    }

    if (recvTid != tidToCheck)
      return ERROR;

    status[processNum] = -1;
    numGoodProcesses--;
    return 0;

  } else if (bufId < 0) {
    printErrorMsg("Error in netcommunication - unable to check process");
    return ERROR;
  } else {
    return SUCCESS;
  }
}

void NetCommunication::getHealthOfProcesses(int* procTids) {
  checkProcesses();
  int i;
  for (i = 0; i < numProcesses; i++)
    procTids[i] = status[i];
}

int NetCommunication::getHealthOfProcessesAndHostAdded(int* procTids) {
  int i, newProcess = -1;
  checkProcesses();
  checkHostsForDelete();
  checkHostsForResume();
  newProcess = checkHostsForAdded();
  for (i = 0; i < numProcesses; i++)
    procTids[i] = status[i];
  return newProcess;
}

void NetCommunication::checkProcesses() {
  int i, info, tidDown;

  int received = pvm_nrecv(-1, pvmConst->getDiedTag());
  while (received > 0) {
    // got message that task is down, receive it
    info = pvm_upkint(&tidDown, 1, 1);
    i = 0;
    while ((tids[i] != tidDown) && (i < numProcesses))
      i++;

    assert((i >= 0) && (i < numProcesses));
    status[i] = -1;
    numGoodProcesses--;
    received = pvm_nrecv(-1, pvmConst->getDiedTag());
  }

  if (received < 0) {
    printErrorMsg("Error in netcommunication - unable to check process");
    stopNetCommunication();
    exit(EXIT_FAILURE);
  }
}

void  NetCommunication::checkHostsForSuspend() {
  int i, j, info;
  int taskSuspended = 0;
  int hostSuspended;

  int received = pvm_nrecv(-1, pvmConst->getHostSuspendTag());
  while (received > 0) {
    // got message that task is suspended, receive it
    info = pvm_upkint(&hostSuspended, 1, 1);
    // find id of hostSuspended
    i = 0;
    while ((hostTids[i] != hostSuspended) && (i < numProcesses))
      i++;

    assert((i >= 0) && (i < numProcesses));
    j = 0;
    while ((taskp[j].ti_host != hostSuspended) && (i < numProcesses))
      j++;

    assert((j >= 0) && (j < numProcesses));
    taskSuspended = taskp[j].ti_tid;
    status[j] = -1;
    numGoodProcesses--;
    received = pvm_nrecv(-1, pvmConst->getHostSuspendTag());
  }

  if (received < 0) {
    printErrorMsg("Error in netcommunication - unable to check process");
    stopNetCommunication();
    exit(EXIT_FAILURE);
  }
}

void NetCommunication::checkHostsForDelete() {
  int i, info, tidDelete;

  int received = pvm_nrecv(-1, pvmConst->getHostDeleteTag());
  while (received > 0) {
    // got message that task is down, receive it
    info = pvm_upkint(&tidDelete, 1, 1);
    // find id of tidDown
    i = 0;
    while ((hostTids[i] != tidDelete) && (i < numProcesses))
      i++;

    assert((i >= 0) && (i < numProcesses));
    status[i] = -1;
    numGoodProcesses--;
    received = pvm_nrecv(-1, pvmConst->getHostDeleteTag());
  }

  if (received < 0) {
    printErrorMsg("Error in netcommunication - unable to check process");
    stopNetCommunication();
    exit(EXIT_FAILURE);
  }
}

int NetCommunication::spawnAndStartOneProcess(int processNumber) {
  int check;
  int newTid = 0;

  check = spawnOneMoreProgram(newTid, processNumber);
  if (!(check == SUCCESS)) {
    printErrorMsg("Error in netcommunication - unable to spawn process");
    return ERROR;
  }

  check = startOneProcess(processNumber, newTid);
  if (!(check == SUCCESS)) {
    printErrorMsg("Error in netcommunication - unable to spawn process");
    return ERROR;
  }
  return SUCCESS;
}

int NetCommunication::checkHostsForAdded() {
  int i, check, info, infos, tidAdded;
  int numOfProcessAdded = -1;
  int checkIfNew = 0;
  int received = pvm_nrecv(-1, pvmConst->getAddHostTag());

  if (received > 0) {
    // got message that host is added, receive it
    info = pvm_upkint(&tidAdded, 1, 1);
    // find id of tidDown
    for (i = 0; i < (tidsCounter + 1); i++) {
      if (tids[i] == tidAdded) {
        checkIfNew++;
        numOfProcessAdded = i;
      }
    }

    if (checkIfNew == 0) {
      nhost++;
      numOfProcessAdded = tidsCounter + 1;
      hostTids[numOfProcessAdded] = tidAdded;
      check = spawnAndStartOneProcess(numOfProcessAdded);

    } else {
      check = spawnAndStartOneProcess(numOfProcessAdded);
    }

    if (check == SUCCESS) {
      pvm_addhosts(NULL, 1, &infos);
      tidsCounter++;
      return numOfProcessAdded;
    } else if (check == ERROR) {
      printErrorMsg("Error in netcommunication - unable to spawn process");
      return ERROR;
    } else {
      printErrorMsg("Error in netcommunication - unable to spawn process");
      return ERROR;
    }

  } else if (received == 0) {
    return ERROR;
  } else {
    printErrorMsg("Error in netcommunication - unable to check process");
    stopNetCommunication();
    return ERROR;
  }
}

void NetCommunication::checkHostsForResume() {
  int i, info, tidResume;

  int received = pvm_nrecv(-1, pvmConst->getHostResumeTag());
  while (received > 0) {
    // got message that task is down, receive it
    info = pvm_upkint(&tidResume, 1, 1);
    // find id of tidDown
    i = 0;
    while ((hostTids[i] != tidResume) && (i < numProcesses))
      i++;

    assert((i >= 0) && (i < numProcesses));
    status[i] = 1;
    numGoodProcesses++;
    received = pvm_nrecv(-1, pvmConst->getHostResumeTag());
  }

  if (received < 0) {
    printErrorMsg("Error in netcommunication - unable to check process");
    stopNetCommunication();
    exit(EXIT_FAILURE);
  }
}

int NetCommunication::checkHostForSuspendReturnsDataid(int* procTids) {
  int processId = -1;
  int dataId = -1;
  int i, info;
  int taskSuspended = 0;
  int hostSuspended;

  int received = pvm_nrecv(-1, pvmConst->getHostSuspendTag());
  if (received > 0) {
    // got message that task is suspended, receive it
    info = pvm_upkint(&hostSuspended, 1, 1);
    // find id of hostSuspended
    i = 0;
    while ((hostTids[i] != hostSuspended) && (i < numProcesses))
      i++;

    assert((i >= 0) && (i < numProcesses));
    taskSuspended = tids[i];
    status[i] = -1;
    numGoodProcesses--;
    for (i = 0; i < numProcesses; i++)
      procTids[i] = status[i];

    for (i = 0; (i < numProcesses) && (processId == -1); i++)
      if (tids[i] == taskSuspended)
        processId = i;

    if (processId == -1) {
      printErrorMsg("Error in netcommunication - invalid process ID");
      exit(EXIT_FAILURE);
    }

    dataId = dataIds[processId];
    return dataId;

  }
  if (received < 0) {
    printErrorMsg("Error in netcommunication - unable to check process");
    stopNetCommunication();
    exit(EXIT_FAILURE);
  }
  return -2;
}

int NetCommunication::checkHostForDeleteReturnsDataid(int* procTids) {
  int processId = -1;
  int dataId = -1;
  int i, j, info;
  int taskDeleted = 0;
  int hostDeleted;

  int received = pvm_nrecv(-1, pvmConst->getHostDeleteTag());
  if (received > 0) {
    // got message that task is deleted, receive it
    info = pvm_upkint(&hostDeleted, 1, 1);
    // find id of hostDeleted
    i = 0;
    while ((hostTids[i] != hostDeleted) && (i < numProcesses))
      i++;

    assert((i >= 0) && (i < numProcesses));
    j = 0;
    while ((taskp[j].ti_host != hostDeleted) && (i < numProcesses))
      j++;

    assert((j >= 0) && (j < numProcesses));
    taskDeleted = taskp[j].ti_tid;
    status[j] = -1;
    numGoodProcesses--;
    for (i = 0; i < numProcesses; i++)
      procTids[i] = status[i];

    for (i = 0; (i < numProcesses) && (processId == -1); i++)
      if (tids[i] == taskDeleted)
        processId = i;

    if (processId == -1) {
      printErrorMsg("Error in netcommunication - invalid process ID");
      exit(EXIT_FAILURE);
    }

    dataId = dataIds[processId];
    return dataId;
  }

  if (received < 0) {
    printErrorMsg("Error in netcommunication - unable to check process");
    stopNetCommunication();
    exit(EXIT_FAILURE);
  }
  return -2;
}

int NetCommunication::checkHostForResumeReturnsDataid(int* procTids) {
  int processId = -1;
  int dataId = -1;
  int i, j, info;
  int taskResumed = 0;
  int hostResumed;

  int received = pvm_nrecv(-1, pvmConst->getHostResumeTag());
  if (received > 0) {
    // got message that task is resumed, receive it
    info = pvm_upkint(&hostResumed, 1, 1);
    // find id of hostResumed
    i = 0;
    while ((hostTids[i] != hostResumed) && (i < numProcesses))
      i++;

    assert((i >= 0) && (i < numProcesses));
    j = 0;
    while ((taskp[j].ti_host != hostResumed) && (i < numProcesses))
      j++;

    assert((j >= 0) && (j < numProcesses));
    taskResumed = taskp[j].ti_tid;
    status[j] = 1;
    numGoodProcesses++;
    for (i = 0; i < numProcesses; i++)
      procTids[i] = status[i];

    int hostTidToNotify[] = {hostResumed};

#ifdef CONDOR
    info = pvm_notify(PvmHostSuspend, pvmConst->getHostSuspendTag(), 1, hostTidToNotify);
#endif
    for (i = 0; (i < numProcesses) && (processId == -1); i++)
      if (tids[i] == taskResumed)
        processId = i;

    if (processId == -1) {
      printErrorMsg("Error in netcommunication - invalid process ID");
      exit(EXIT_FAILURE);
    }

    dataId = dataIds[processId];
    return dataId;
  }

  if (received < 0) {
    printErrorMsg("Error in netcommunication - unable to check process");
    stopNetCommunication();
    exit(EXIT_FAILURE);
  }
  return -2;
}

// ********************************************************
// Functions for sending and receiving messages
// ********************************************************
#ifdef GADGET_NETWORK
int NetCommunication::sendData(VectorOfCharPtr sendP) {
  int i, info;

  if (NETSTARTED == 1) {
    info = pvm_initsend(pvmConst->getDataEncode());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send data");
      stopNetCommunication();
      return ERROR;
    }

    assert(sendP.Size() >= numVar);
    for (i = 0; i < numVar; i++) {
      info = pvm_pkstr(sendP[i]);
      if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send data");
        stopNetCommunication();
        return ERROR;
      }
    }

    info = pvm_mcast(tids, nhost, pvmConst->getMasterSendStringTag());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send data");
      stopNetCommunication();
      return ERROR;
    } else
      return SUCCESS;

  } else {
    printErrorMsg("Error in netcommunication - unable to send data");
    return ERROR;
  }
}

int NetCommunication::sendData(VectorOfCharPtr sendP, int processId) {
  int i, info;

  if (NETSTARTED == 1) {
    info = pvm_initsend(pvmConst->getDataEncode());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send data");
      stopNetCommunication();
      return ERROR;
    }

    assert(sendP.Size() >= numVar);
    for (i = 0; i < numVar; i++) {
      info = pvm_pkstr(sendP[i]);
      if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send data");
        stopNetCommunication();
        return ERROR;
      }
    }

    info = pvm_send(tids[processId], pvmConst->getMasterSendStringTag());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send data");
      stopNetCommunication();
      return ERROR;
    } else {
      return SUCCESS;
    }

  } else {
    printErrorMsg("Error in netcommunication - unable to send data");
    return ERROR;
  }
}

int NetCommunication::sendBoundData(vector sendP) {
  int i, info;
  double* temp;

  if (NETSTARTED == 1) {
    info = pvm_initsend(pvmConst->getDataEncode());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send data");
      stopNetCommunication();
      return ERROR;
    }

    temp = new double[numVar];
    for (i = 0; i < numVar; i++)
      temp[i] = sendP[i];

    info = pvm_pkdouble(temp, numVar, 1);

    delete[] temp;
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send data");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_mcast(tids, nhost, pvmConst->getMasterSendBoundTag());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send data");
      stopNetCommunication();
      return ERROR;
    } else
      return SUCCESS;

  } else {
    printErrorMsg("Error in netcommunication - unable to send data");
    return ERROR;
  }
}

int NetCommunication::sendBoundData(vector sendP, int processId) {
  int i, info;
  double* temp;

  if (NETSTARTED == 1) {
    info = pvm_initsend(pvmConst->getDataEncode());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send data");
      stopNetCommunication();
      return ERROR;
    }

    temp = new double[numVar];
    for (i = 0; i < numVar; i++)
      temp[i] = sendP[i];

    info = pvm_pkdouble(temp, numVar, 1);
    delete[] temp;
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send data");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_send(tids[processId], pvmConst->getMasterSendBoundTag());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send data");
      stopNetCommunication();
      return ERROR;
    } else
      return SUCCESS;

  } else {
    printErrorMsg("Error in netcommunication - unable to send data");
    return ERROR;
  }
}
#endif

int NetCommunication::sendData(NetDataVariables* sendP, int processId) {
  int info;
  int cansend = 1;
  assert(processId >= 0);
  assert(processId < numProcesses);

  if (NETSTARTED == 1) {
    // check is process with id = processId is up and running
    cansend = checkProcess(processId);
    if (cansend == -1) {
      printErrorMsg("Error in netcommunication - invalid process ID");
      stopNetCommunication();
      return ERROR;

    } else if (cansend == 0) {
      //process with id = processId is not up and running
      return cansend;

    } else if (cansend == 1) {
      info = pvm_initsend(pvmConst->getDataEncode());
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to send data");
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkint(&sendP->tag, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to send data");
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkint(&sendP->x_id, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to send data");
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkdouble(sendP->x, numVar, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to send data");
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_send(tids[processId], pvmConst->getMasterSendVarTag());
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to send data");
        stopNetCommunication();
        return ERROR;
      }
      return SUCCESS;

    } else {
      printErrorMsg("Error in netcommunication - unable to send data");
      stopNetCommunication();
      return ERROR;
    }

  } else {
    printErrorMsg("Error in netcommunication - unable to send data");
    return ERROR;
  }
}

int NetCommunication::sendData(NetDataVariables* sendP, int processId, int dataId) {
  int info;
  int cansend = 1;
  assert(processId >= 0);
  assert(processId < numProcesses);

  if (NETSTARTED == 1) {
    // check is process with id = processId is up and running
    cansend = checkProcess(processId);
    if (cansend == -1) {
      printErrorMsg("Error in netcommunication - invalid process ID");
      stopNetCommunication();
      return ERROR;

    } else if (cansend == 0) {
      //process with id = processId is not up and running
      return cansend;

    } else if (cansend == 1) {
      info = pvm_initsend(pvmConst->getDataEncode());
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to send data");
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkint(&sendP->tag, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to send data");
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkint(&sendP->x_id, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to send data");
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkdouble(sendP->x, numVar, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to send data");
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_send(tids[processId], pvmConst->getMasterSendVarTag());
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to send data");
        stopNetCommunication();
        return ERROR;
      }

      dataIds[processId] = dataId;
      return SUCCESS;

    } else {
      printErrorMsg("Error in netcommunication - unable to send data");
      stopNetCommunication();
      return ERROR;
    }

  } else {
    printErrorMsg("Error in netcommunication - unable to send data");
    return ERROR;
  }
}

int NetCommunication::receiveData(NetDataResult* rp) {
  int info;

  if (NETSTARTED == 1) {
    // receive data from any process which sends data with msgtag = receiveTag
    info = pvm_recv(-1, pvmConst->getMasterReceiveDataTag());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to receive data");
      stopNetCommunication();
      return ERROR;

    } else {
      info = pvm_upkint(&rp->tag, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to receive data");
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_upkdouble(&rp->result, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to receive data");
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_upkint(&rp->who, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to receive data");
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_upkint(&rp->x_id, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to receive data");
        stopNetCommunication();
        return ERROR;
      }
      return SUCCESS;
    }

  } else {
    printErrorMsg("Error in netcommunication - unable to receive data");
    return ERROR;
  }
}

//Added jongud
int NetCommunication::probeForReceiveData() {
  int bufid;
  bufid = pvm_probe(-1, pvmConst->getMasterReceiveDataTag());
  if (bufid > 0)
    return SUCCESS;
  else if (bufid == 0)
    return NONTORECEIVE;
  else {
    printErrorMsg("Error in netcommunication - unable to receive data");
    stopNetCommunication();
    return ERROR;
  }
}

int NetCommunication::receiveDataNonBlocking(NetDataResult* rp) {
  int info;

  info = pvm_nrecv(-1, pvmConst->getMasterReceiveDataTag());
  if (info == 0) {
    return NONTORECEIVE;

  } else if (info > 0) {
    // received information from process
    info = pvm_upkint(&rp->tag, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to receive data");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkdouble(&rp->result, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to receive data");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkint(&rp->who, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to receive data");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkint(&rp->x_id, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to receive data");
      stopNetCommunication();
      return ERROR;
    }
    dataIds[rp->who] = -1;
    return SUCCESS;

  } else {
    printErrorMsg("Error in netcommunication - unable to receive data");
    stopNetCommunication();
    return ERROR;
  }
}

// ********************************************************
// Functions which set/return information about netcommunication
// ********************************************************
int NetCommunication::getNumHosts() {
  return nhost;
}

int NetCommunication::getNumProcesses() {
  return numProcesses;
}

int NetCommunication::getNumVar() {
  return numVar;
}

int NetCommunication::getNumRunningProcesses() {
  return numGoodProcesses;
}

int NetCommunication::netCommStarted() {
  return NETSTARTED;
}

void NetCommunication::setNumInSendVar(int nVar) {
  if (nVar <= 0) {
    cerr << "Error in netcommunication - number of variables must be positive\n";
    exit(EXIT_FAILURE);
  }
  numVar = nVar;
}

void NetCommunication::printErrorMsg(const char* errorMsg) {
  char* msg;
  msg = new char[strlen(errorMsg) + 1];
  strcpy(msg, errorMsg);
  pvm_perror(msg);
  delete[] msg;
  cerr << errorMsg << endl;
}

int NetCommunication::netError() {
  return ERROR;
}

int NetCommunication::netSuccess() {
  return SUCCESS;
}

int NetCommunication::netDataNotSent() {
  return DATANOTSENT;
}

int NetCommunication::netNoneToReceive() {
  return NONTORECEIVE;
}

int NetCommunication::netNeedMoreHosts() {
  return NEEDMOREHOSTS;
}

int NetCommunication::netNeedMoreData() {
  return NEEDMOREDATA;
}

int NetCommunication::netWaitForBetterProcesses() {
  return WAITFORPROCESS;
}

MasterCommunication::MasterCommunication(CommandLineInfo* info)
  : NetCommunication(info->FunctionNameArgs(), info->NumOfProc()) {

  int wait = info->WaitForMaster();
  tmout = new timeval;
  if (wait == -1)
    tmout = NULL;
  else if (wait >= 0) {
    tmout->tv_sec = wait;
    tmout->tv_usec = 0;
  } else {
    cerr << "Error in netcommunication - invalid value for wait " << wait << "\n";
    exit(EXIT_FAILURE);
  }
}

MasterCommunication::~MasterCommunication() {
  delete tmout;
}

int MasterCommunication::receiveData(NetDataResult* rp) {
  int info, timeout;
  timeout = pvm_trecv(-1, pvmConst->getMasterReceiveDataTag(), tmout);
  if (timeout < 0) {
    printErrorMsg("Error in netcommunication - unable to receive data");
    stopNetCommunication();
    return ERROR;

  } else if (timeout > 0) {
    // received information from process within tmout.
    info = pvm_upkint(&rp->tag, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to receive data");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkdouble(&rp->result, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to receive data");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkint(&rp->who, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to receive data");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkint(&rp->x_id, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to receive data");
      stopNetCommunication();
      return ERROR;
    }
    return SUCCESS;

  } else {
    rp->who = -1;
    return SUCCESS;
  }
}

//Added method jongud. 17.07.02 . Not finished.
int MasterCommunication::receiveDataNonBlocking(NetDataResult* rp) {
  int info;
  info = pvm_nrecv(-1, pvmConst->getMasterReceiveDataTag());

  if (info == 0)
    return NONTORECEIVE;

  else if (info > 0) {
   // received information from process
    info = pvm_upkint(&rp->tag, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to receive data");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkdouble(&rp->result, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to receive data");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkint(&rp->who, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to receive data");
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkint(&rp->x_id, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to receive data");
      stopNetCommunication();
      return ERROR;
    }
    dataIds[rp->who] = -1;
    return SUCCESS;

  } else {
    printErrorMsg("Error in netcommunication - unable to receive data");
    stopNetCommunication();
    return ERROR;
  }
}
