#include "netcommunication.h"

NetCommunication::NetCommunication(char* progN, char** progA, int nh) {
  // pvmConst contains information about which tags and dataencoding to use
  pvmConst = new PVMConstants();
  nHostInn = nh;
  slaveProgram = progN;
  slaveArguments = progA;
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
  if (tids != NULL)
    delete[] tids;
  if (status != NULL)
    delete[] status;
  if (hostTids != NULL)
    delete[] hostTids;
  if (dataIds != NULL)
    delete[] dataIds;
  if (NETSTARTED == 1)
    stopNetCommunication();
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

    info  = pvm_config(&nhost, &narch, &hostp);
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
    tempString = new char[strlen("PWD" + 1)];
    strcpy(tempString, "PWD");
    // Take out for multiprocessor maybe also Norway??
    info = pvm_export(tempString);
    delete[] tempString;
    return SUCCESS;

  } else {
    cout << "Warning in netcommunication - already enrolled in PVM\n";
    return SUCCESS;
  }
}

#ifndef CONDOR
int NetCommunication::startNetCommunication() {
  int OK;

  if (NETSTARTED == 1 && mytid >= 0) {
    // have alredy enrolled in pvm and spawned program on slaves
    cout << "Warning in netcommunication - already enrolled in PVM and running " << slaveProgram << " on slaves\n";
    return SUCCESS;

  } else {
    if (numVar <= 0) {
      cerr << "Error in netcommunication - number of variables must be positive\n";
      return ERROR;
    }

    OK = startPVM();
    if (OK == 1) {
      // have enrolled in PVM and configured nhost hosts
      OK = spawnProgram();
      if (OK == 1) {
        // have started slaveProgram on all nhost hosts - send initial info
        OK = startProcesses();
        if (OK == 1) {
          // successfully started processes
          NETSTARTED = 1;
          return SUCCESS;

        } else if (OK == -1) {
          return ERROR;
        } else {
          cerr << "Error in netcommunication - unrecognised return value\n";
          stopNetCommunication();
          return ERROR;
        }

      } else if (OK == 0) {
        // could not spawn all nhost processes
        stopNetCommunication();
        return ERROR;
      } else {
        cerr << "Error in netcommunication - unrecognised return value\n";
        stopNetCommunication();
        return ERROR;
      }

    } else if (OK == 0) {
      // could not enroll in pvm or error occured using pvm
      stopNetCommunication();
      return ERROR;
    } else {
      cerr << "Error in netcommunication - unrecognised return value\n";
      stopNetCommunication();
      return ERROR;
    }
  }
}
#endif

#ifdef CONDOR
int NetCommunication::startNetCommunication() {
  int i, OK, info;

  if (NETSTARTED == 1 && mytid >= 0) {
    // have alredy enrolled in pvm and spawned program on slaves
    cout << "Warning in netcommunication - already enrolled in PVM and running " << slaveProgram << " on slaves\n";
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
          cerr << "Error in netcommunication - spawnoneprogram failed\n";
          return ERROR;
        }
      }
    }

    pvm_config(&nhost, &narch, &hostp);
    for (i = 0; i < nhost; i++) {
      /* hosts to be monitored for deletion, suspension and resumption*/
      hostTids[i] = hostp[i].hi_tid;
    }

    //jongud added pvm_notify for if host get suspended
    info = pvm_notify(PvmHostSuspend, pvmConst->getHostSuspendTag(), nhost, hostTids);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - pvmnotify failed");
      stopNetCommunication();
      return ERROR;
    }

    //jongud added pvm_notify for if host is deleted
    info = pvm_notify(PvmHostDelete, pvmConst->getHostDeleteTag(), nhost, hostTids);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - pvmnotify failed");
      stopNetCommunication();
      return ERROR;
    }

    //jongud added pvm_notify for if host resumes
    info = pvm_notify(PvmHostResume, pvmConst->getHostResumeTag(), nhost, hostTids);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - pvmnotify failed");
      stopNetCommunication();
      return ERROR;
    }

    //jongud added pvm_notify for if host is added
    info = pvm_notify(PvmHostAdd, pvmConst->getAddHostTag(), -1, 0);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - pvmnotify failed");
      stopNetCommunication();
      return ERROR;
    }

    char* nullString = "0";
    int infos;
    if (OK == 1) {
      // Have started slaveProgram slaveArguments  on all nhost hosts.
      // send initial info to all slave processes
      OK = startProcesses();
      if (OK == 1) {
        NETSTARTED = 1;
        //Ask condor for another host jongud
        pvm_addhosts(&nullString, 1, &infos);
        return SUCCESS;
      } else if (OK == -1) {
        return ERROR;
      } else {
        cerr << "Error in netcommunication - unrecognised return value\n";
        stopNetCommunication();
        return ERROR;
      }

    } else if (OK == 0) {
      // could not spawn all nhost processes
      stopNetCommunication();
      return ERROR;
    } else {
      cerr << "Error in netcommunication - unrecognised return value\n";
      stopNetCommunication();
      return ERROR;
    }
  }
}
#endif

#ifndef CONDOR
void NetCommunication::stopNetCommunication() {
  int tid, info;
  int stopparam = -1;

  tid = pvm_mytid();
  if (tid > 0) {
    // pvmd is up and running and have joined pvm
    if (NETSTARTED == 1) {
      // have successfully spawned slaves and sent initial message to them
      info = pvm_initsend(pvmConst->getDataEncode());
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to stop communication");
        cerr << "Error in netcommunication - unable to stop communication\n";

      } else {
        info = pvm_pkint(&stopparam, 1, 1);
        if (info < 0) {
          printErrorMsg("Error in netcommunication - unable to stop communication");
          cerr << "Error in netcommunication - unable to stop communication\n";

        } else {
          // maybe update tid according to status before broadcasting??
          info = pvm_mcast(tids, nhost, pvmConst->getStopTag());
          if (info < 0) {
            printErrorMsg("Error in netcommunication - unable to stop communication");
            cerr << "Error in netcommunication - unable to stop communication\n";
          }
        }
      }
    }
    pvm_exit();
  }
  mytid = -1;
  NETSTARTED = 0;
}
#endif

#ifdef CONDOR
void NetCommunication::stopNetCommunication() {
  int i, tid, info;
  int stopparam = -1;

  tid = pvm_mytid();
  if (tid > 0) {
    // pvmd is up and running and have joined pvm
    if (NETSTARTED == 1) {
      // have successfully spawned slaves and sent initial message to them
      info = pvm_initsend(pvmConst->getDataEncode());
      if (info < 0) {
        printErrorMsg("Error in netcommunication - unable to stop communication");
        cerr << "Error in netcommunication - unable to stop communication\n";

      } else {
        info = pvm_pkint(&stopparam, 1, 1);
        if (info < 0) {
          printErrorMsg("Error in netcommunication - unable to stop communication");
          cerr << "Error in netcommunication - unable to stop communication\n";

        } else {
          //Checking the status of processes and shutting them down.
          //jongud. PVM is checked to see id of processes
          int numberOfTasks;
          int* tidsArray = new int[maxNumHosts];
          info = pvm_tasks(0, &numberOfTasks, &taskp);
          for (i = 0; i < numberOfTasks; i++)
            tidsArray[i] = taskp[i].ti_tid;

          info = pvm_mcast(tidsArray, numberOfTasks, pvmConst->getStopTag());
          if (info < 0) {
            printErrorMsg("Error in netcommunication - unable to stop communication");
            cerr << "Error in netcommunication - unable to stop communication\n";
          }
        }
      }
    }
    pvm_exit();
  }
  mytid = -1;
  NETSTARTED = 0;
}
#endif

int NetCommunication::spawnProgram() {
  int i, nspawn;
  char* emptyString;
  emptyString = new char[strlen("" + 1)];
  strcpy(emptyString, "");

  // Round-robin assignment is used to distribute the nhost processes on
  // hosts participating. As nhost = total number of hosts participating,
  // one process is started on each host. If startPVM is changed so that
  // continue even though not all hosts can participate then must change
  // this or set nhost to actual hosts .....

  i = pvm_catchout(stdout);
  nspawn = pvm_spawn(slaveProgram, slaveArguments, 0, emptyString, nhost, tids);
  //nspawn = pvm_spawn(slaveProgram, slaveArguments, 1024 + 1, "ask.ii.uib.no", nhost, tids);
  delete[] emptyString;

  if (nspawn < 0) {
    printErrorMsg("Error in netcommunication - unable to spawn process");
    stopNetCommunication();
    exit(EXIT_FAILURE);

  } else if (nspawn < nhost) {
    printErrorMsg("Error in netcommunication - unable to spawn process");
    cerr << "Error in netcommunication - unable to start " << slaveProgram << " on all hosts\n"
      << "The following hosts were not able to start:\n";
    for (i = nspawn; i < nhost; i++)
      cerr << tids[i] << endl;
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
  nspawn = pvm_spawn(slaveProgram, slaveArguments, PvmTaskArch, "0", 1, &tids[vectorNumber]);
  if (nspawn < 0) {
    printErrorMsg("Error in netcommunication - unable to spawn process");
    stopNetCommunication();
    exit(EXIT_FAILURE);

  } else if (tids[vectorNumber] < 0) {
    printErrorMsg("Error in netcommunication - unable to spawn process");
    cerr << "Error in netcommunication - unable to start " << slaveProgram << " on all hosts\n"
      << "The following host was not able to start: " << tids[vectorNumber] << endl;
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
  int* newTidVector = new int[1];

  i = pvm_catchout(stdout);
  //pvm_spawn to start machines in condor
  nspawn = pvm_spawn(slaveProgram, slaveArguments, PvmTaskArch, "0", 1, newTidVector);
  newTid = newTidVector[0];
  if (nspawn < 0) {
    printErrorMsg("Error in netcommunication - unable to spawn process");
    stopNetCommunication();
    exit(EXIT_FAILURE);

  } else if (newTid < 0) {
    printErrorMsg("Error in netcommunication - unable to spawn process");
    cerr << "Error in netcommunication - unable to start " << slaveProgram << " on all hosts\n"
      << "The following host was not able to start: " << newTid << endl;
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
    printErrorMsg("Error in netcommunication - unable to notify");
    stopNetCommunication();
    return ERROR;
  }

  for (i = 0; i < nhost; i++) {
    // send initial message to all spawned processes
    cansend = sendInitialMessage(i);
    if (cansend == -1) {
      // Error occured in sending inital message to process with id = i
      cerr << "Error in netcommunication - unable to send message\n";
      return ERROR;

    } else if (cansend == 0) {
      cerr << "Error in netcommunication - unable to send message\n";
      status[i] = -1;
      return ERROR;

    } else if (cansend == 1) {
      status[i] = 1;

    } else {
      cerr << "Error in netcommunication - unrecognised return value\n";
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
    printErrorMsg("Error in netcommunication - unable to notify");
    stopNetCommunication();
    return ERROR;
  }

  //jongud added pvm_notify for if host get suspended
  info = pvm_notify(PvmHostSuspend, pvmConst->getHostSuspendTag(), 1, hostTidToNotify);
  if (info < 0) {
    printErrorMsg("Error in netcommunication - unable to notify");
    stopNetCommunication();
    return ERROR;
  }

  //jongud added pvm_notify for if host is deleted
  info = pvm_notify(PvmHostDelete, pvmConst->getHostDeleteTag(), 1, hostTidToNotify);
  if (info < 0) {
    printErrorMsg("Error in netcommunication - unable to notify");
    stopNetCommunication();
    return ERROR;
  }

  //jongud added pvm_notify for if host resumes
  info = pvm_notify(PvmHostResume, pvmConst->getHostResumeTag(), 1, hostTidToNotify);
  if (info < 0) {
    printErrorMsg("Error in netcommunication - unable to notify");
    stopNetCommunication();
    return ERROR;
  }

  //Send initial message to the last spawned process, nhost == number of processes
  cansend = sendInitialMessageToTid(processTid, processNum);
  if (cansend == -1) {
    cerr << "Error in netcommunication - unable to send message\n";
    return ERROR;
  } else if (cansend == 0) {
    status[processNum] = -1;
  } else if (cansend == 1) {
    status[processNum] = 1;
  } else {
    cerr << "Error in netcommunication - unrecognised return value\n";
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
    cerr << "Error in netcommunication - illegal id for slave\n";
    return 0;
  }

  // check if process with identity = id is up and running
  OK = checkProcess(id);
  if (OK == 1) {
    info = pvm_initsend(pvmConst->getDataEncode());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not sending message");
      cerr << "Error in netcommunication - unable to send initial message\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_pkint(&numVar, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not sending message");
      cerr << "Error in netcommunication - unable to send initial message\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_pkint(&id, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not sending message");
      cerr << "Error in netcommunication - unable to send initial message\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_send(tids[id], pvmConst->getStartTag());

    if (info < 0) {
      printErrorMsg("Error in netcommunication - not sending message");
      cerr << "Error in netcommunication - unable to send initial message\n";
      stopNetCommunication();
      return ERROR;
    }
    return SUCCESS;

  } else if (OK == -1) {
    cerr << "Error in netcommunication - unable to check status of process\n";
    stopNetCommunication();
    return ERROR;

  } else if (OK == 0) {
    cerr << "Error in netcommunication - unable to send initial message\n";
    return OK;

  } else {
    cerr << "Error in netcommunication - unrecognised return value\n";
    stopNetCommunication();
    return ERROR;
  }
}

int NetCommunication::sendInitialMessageToTid(int tid, int processNum) {
  int OK;
  int info;

  if ((processNum < 0)|| (processNum >= numProcesses)) {
    cerr << "Error in netcommunication - unknown process number\n";
    return 0;
  }

  // check if process with identity = id is up and running
  OK = checkProcessByTid(tid, processNum);
  if (OK == 1) {
    info = pvm_initsend(pvmConst->getDataEncode());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not starting process");
      cerr << "Error in netcommunication - unable to start process\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_pkint(&numVar, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not starting process");
      cerr << "Error in netcommunication - unable to start process\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_pkint(&processNum, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not starting process");
      cerr << "Error in netcommunication - unable to start process\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_send(tid, pvmConst->getStartTag());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not starting process");
      cerr << "Error in netcommunication - unable to start process\n";
      stopNetCommunication();
      return ERROR;
    }
    // sent successfully to slave with tids[id] numVar and id
    return SUCCESS;

  } else if (OK == -1) {
    cerr << "Error in netcommunication - unable to check process\n";
    stopNetCommunication();
    return ERROR;
  } else if (OK == 0) {
    return OK;
  } else {
    cerr << "Error in netcommunication - unrecognised return value\n";
    stopNetCommunication();
    return ERROR;
  }
}

int NetCommunication::sendNumberOfVariables() {
  int info;

  if (numVar <= 0) {
    cerr << "Error in netcommunication - number of variables must be positive\n";
    stopNetCommunication();
    return ERROR;
  }

  // tell slave program how many variables to expect in vector sent
  info = pvm_initsend(pvmConst->getDataEncode());
  if (info < 0) {
    printErrorMsg("Error in netcommunication - not sending variables");
    cerr << "Error in netcommunication - unable to send number of variables\n";
    stopNetCommunication();
    return ERROR;
  }

  info = pvm_pkint(&numVar, 1, 1);
  if (info < 0) {
    printErrorMsg("Error in netcommunication - not sending variables");
    cerr << "Error in netcommunication - unable to send number of variables\n";
    stopNetCommunication();
    return ERROR;
  }

  info = pvm_mcast(tids, nhost, pvmConst->getStartTag());
  if (info < 0) {
    printErrorMsg("Error in netcommunication - not sending variables");
    cerr << "Error in netcommunication - unable to send number of variables\n";
    stopNetCommunication();
    return ERROR;
  }
  return SUCCESS;
}

int NetCommunication::checkProcess(int id) {
  int info, bufId, recvTid;
  assert (id >= 0);
  assert (id < numProcesses);

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
    cerr << "Error in netcommunication - unable to check process\n";
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
    cerr << "Error in netcommunication - unable to check process\n";
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
    cerr << "Error in netcommunication - unable to check process\n";
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
    cerr << "Error in netcommunication - unable to check process\n";
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
    cerr << "Error in netcommunication - unable to check process\n";
    stopNetCommunication();
    exit(EXIT_FAILURE);
  }
}

int NetCommunication::spawnAndStartOneProcess(int processNumber) {
  int check;
  int newTid = 0;

  check = spawnOneMoreProgram(newTid, processNumber);
  if (!(check == SUCCESS)) {
    cerr << "Error in netcommunication - unable to spawn one more program\n";
    return ERROR;
  }

  check = startOneProcess(processNumber, newTid);
  if (!(check == SUCCESS)) {
    cerr << "Error in netcommunication - unable to spawn one more program\n";
    return ERROR;
  }
  return SUCCESS;
}

int NetCommunication::checkHostsForAdded() {
  int i, check, info, infos, tidAdded;
  int numOfProcessAdded = -1;
  char* nullString = "0";
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
      pvm_addhosts(&nullString, 1, &infos);
      tidsCounter++;
      return numOfProcessAdded;
    } else if (check == ERROR) {
      cerr << "Error in netcommunication - unable to spawn process\n";
      return ERROR;
    } else {
      cerr << "Error in netcommunication - unable to spawn process\n";
      return ERROR;
    }

  } else if (received == 0) {
    return ERROR;
  } else {
    cerr << "Error in netcommunication - unable to check process\n";
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
    cerr << "Error in netcommunication - unable to check process\n";
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
      cerr << "Error in netcommunication - unable to find process id\n";
      exit(EXIT_FAILURE);
    }

    dataId = dataIds[processId];
    return dataId;

  }
  if (received < 0) {
    cerr << "Error in netcommunication - unable to check process\n";
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
      cerr << "Error in netcommunication - unable to find process id\n";
      exit(EXIT_FAILURE);
    }

    dataId = dataIds[processId];
    return dataId;
  }

  if (received < 0) {
    cerr << "Error in netcommunication - unable to check process\n";
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

    info = pvm_notify(PvmHostSuspend, pvmConst->getHostSuspendTag(), 1, hostTidToNotify);
    for (i = 0; (i < numProcesses) && (processId == -1); i++)
      if (tids[i] == taskResumed)
        processId = i;

    if (processId == -1) {
      cerr << "Error in netcommunication - unable to find process id\n";
      exit(EXIT_FAILURE);
    }

    dataId = dataIds[processId];
    return dataId;
  }

  if (received < 0) {
    cerr << "Error in netcommunication - unable to check process\n";
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
      printErrorMsg("Error in netcommunication - not sending data");
      cerr << "Error in netcommunication - unable to send data\n";
      stopNetCommunication();
      return ERROR;
    }

    assert(sendP.Size() >= numVar);
    for (i = 0; i < numVar; i++) {
      info = pvm_pkstr(sendP[i]);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not sending data");
        cerr << "Error in netcommunication - unable to pack data\n";
        stopNetCommunication();
        return ERROR;
      }
    }

    info = pvm_mcast(tids, nhost, pvmConst->getMasterSendStringTag());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not sending data");
      cerr << "Error in netcommunication - unable to send data\n";
      stopNetCommunication();
      return ERROR;
    } else
      return SUCCESS;

  } else {
    cerr << "Error in netcommunication - unable to send data\n";
    return ERROR;
  }
}

int NetCommunication::sendData(VectorOfCharPtr sendP, int processId) {
  int i, info;

  if (NETSTARTED == 1) {
    info = pvm_initsend(pvmConst->getDataEncode());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not sending data");
      cerr << "Error in netcommunication - unable to send data\n";
      stopNetCommunication();
      return ERROR;
    }

    assert(sendP.Size() >= numVar);
    for (i = 0; i < numVar; i++) {
      info = pvm_pkstr(sendP[i]);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not sending data");
        cerr << "Error in netcommunication - unable to pack data\n";
        stopNetCommunication();
        return ERROR;
      }
    }

    info = pvm_send(tids[processId], pvmConst->getMasterSendStringTag());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not sending data");
      cerr << "Error in netcommunication - unable to send data\n";
      stopNetCommunication();
      return ERROR;
    } else {
      return SUCCESS;
    }

  } else {
    cerr << "Error in netcommunication - unable to send data\n";
    return ERROR;
  }
}

int NetCommunication::sendBoundData(vector sendP) {
  int i, info;
  double* temp;

  if (NETSTARTED == 1) {
    info = pvm_initsend(pvmConst->getDataEncode());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not sending data");
      cerr << "Error in netcommunication - unable to send data\n";
      stopNetCommunication();
      return ERROR;
    }

    temp = new double[numVar];
    for (i = 0; i < numVar; i++)
      temp[i] = sendP[i];

    info = pvm_pkdouble(temp, numVar, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not sending data");
      cerr << "Error in netcommunication - unable to pack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_mcast(tids, nhost, pvmConst->getMasterSendBoundTag());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not sending data");
      cerr << "Error in netcommunication - unable to send data\n";
      stopNetCommunication();
      return ERROR;
    } else
      return SUCCESS;

  } else {
    cerr << "Error in netcommunication - unable to send data\n";
    return ERROR;
  }
}

int NetCommunication::sendBoundData(vector sendP, int processId) {
  int i, info;
  double* temp;

  if (NETSTARTED == 1) {
    info = pvm_initsend(pvmConst->getDataEncode());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not sending data");
      cerr << "Error in netcommunication - unable to send data\n";
      stopNetCommunication();
      return ERROR;
    }

    temp = new double[numVar];
    for (i = 0; i < numVar; i++)
      temp[i] = sendP[i];

    info = pvm_pkdouble(temp, numVar, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not sending data");
      cerr << "Error in netcommunication - unable to pack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_send(tids[processId], pvmConst->getMasterSendBoundTag());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not sending data");
      cerr << "Error in netcommunication - unable to send data\n";
      stopNetCommunication();
      return ERROR;
    } else
      return SUCCESS;

  } else {
    cerr << "Error in netcommunication - unable to send data\n";
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
      cerr << "Error in netcommunication - invalid process id " << processId << endl;
      stopNetCommunication();
      return ERROR;

    } else if (cansend == 0) {
      //process with id = processId is not up and running
      return cansend;

    } else if (cansend == 1) {
      info = pvm_initsend(pvmConst->getDataEncode());
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not sending data");
        cerr << "Error in netcommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkint(&sendP->tag, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not sending data");
        cerr << "Error in netcommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkint(&sendP->x_id, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not sending data");
        cerr << "Error in netcommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkdouble(sendP->x, numVar, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not sending data");
        cerr << "Error in netcommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_send(tids[processId], pvmConst->getMasterSendVarTag());
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not sending data");
        cerr << "Error in netcommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }
      return SUCCESS;

    } else {
      cerr << "Error in netcommunication - invalid response\n";
      stopNetCommunication();
      return ERROR;
    }

  } else {
    cerr << "Error in netcommunication - cannot send data\n";
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
      cerr << "Error in netcommunication - invalid process id " << processId << endl;
      stopNetCommunication();
      return ERROR;

    } else if (cansend == 0) {
      //process with id = processId is not up and running
      return cansend;

    } else if (cansend == 1) {
      info = pvm_initsend(pvmConst->getDataEncode());
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not sending data");
        cerr << "Error in netcommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkint(&sendP->tag, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not sending data");
        cerr << "Error in netcommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkint(&sendP->x_id, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not sending data");
        cerr << "Error in netcommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkdouble(sendP->x, numVar, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not sending data");
        cerr << "Error in netcommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_send(tids[processId], pvmConst->getMasterSendVarTag());
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not sending data");
        cerr << "Error in netcommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }

      dataIds[processId] = dataId;
      return SUCCESS;

    } else {
      cerr << "Error in netcommunication - invalid response\n";
      stopNetCommunication();
      return ERROR;
    }

  } else {
    cerr << "Error in netcommunication - cannot send data\n";
    return ERROR;
  }
}

int NetCommunication::receiveData(NetDataResult* rp) {
  int info;

  if (NETSTARTED == 1) {
    // receive data from any process which sends data with msgtag = receiveTag
    info = pvm_recv(-1, pvmConst->getMasterReceiveDataTag());
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not receiving data");
      cerr << "Error in netcommunication - PVM not responding\n";
      stopNetCommunication();
      return ERROR;

    } else {
      info = pvm_upkint(&rp->tag, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not receiving data");
        cerr << "Error in netcommunication - unable to unpack data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_upkdouble(&rp->result, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not receiving data");
        cerr << "Error in netcommunication - unable to unpack data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_upkint(&rp->who, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not receiving data");
        cerr << "Error in netcommunication - unable to unpack data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_upkint(&rp->x_id, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netcommunication - not receiving data");
        cerr << "Error in netcommunication - unable to unpack data\n";
        stopNetCommunication();
        return ERROR;
      }
      return SUCCESS;
    }

  } else {
    cerr << "Error in netcommunication - cannot receive data\n";
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
    printErrorMsg("Error in netcommunication - not receiving data");
    cerr << "Error in netcommunication - unable to receive data\n";
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
      printErrorMsg("Error in netcommunication - not receiving data");
      cerr << "Error in netcommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkdouble(&rp->result, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not receiving data");
      cerr << "Error in netcommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkint(&rp->who, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not receiving data");
      cerr << "Error in netcommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkint(&rp->x_id, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not receiving data");
      cerr << "Error in netcommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }
    dataIds[rp->who] = -1;
    return SUCCESS;

  } else {
    printErrorMsg("Error in netcommunication - not receiving data");
    cerr << "Error in netcommunication - unable to unpack data\n";
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

MasterCommunication::MasterCommunication(char* progN, char** progA, int nh, int waitSec)
  : NetCommunication(progN, progA, nh) {

  tmout.tv_sec = waitSec;
  tmout.tv_usec = 0;
}

MasterCommunication::~MasterCommunication() {
}

int MasterCommunication::receiveData(NetDataResult* rp) {
  int info, timeout;
  timeout = pvm_trecv(-1, pvmConst->getMasterReceiveDataTag(), &tmout);
  if (timeout < 0) {
    printErrorMsg("Error in netcommunication - not receiving data");
    cerr << "Error in netcommunication - PVM not responding\n";
    stopNetCommunication();
    return ERROR;

  } else if (timeout > 0) {
    // received information from process within tmout.
    info = pvm_upkint(&rp->tag, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not receiving data");
      cerr << "Error in netcommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkdouble(&rp->result, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not receiving data");
      cerr << "Error in netcommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkint(&rp->who, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not receiving data");
      cerr << "Error in netcommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkint(&rp->x_id, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not receiving data");
      cerr << "Error in netcommunication - unable to unpack data\n";
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
      printErrorMsg("Error in netcommunication - not receiving data");
      cerr << "Error in netcommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkdouble(&rp->result, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not receiving data");
      cerr << "Error in netcommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkint(&rp->who, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not receiving data");
      cerr << "Error in netcommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkint(&rp->x_id, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netcommunication - not receiving data");
      cerr << "Error in netcommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }
    dataIds[rp->who] = -1;
    return SUCCESS;

  } else {
    printErrorMsg("Error in netcommunication - not receiving data");
    cerr << "Error in netcommunication - PVM not responding\n";
    stopNetCommunication();
    return ERROR;
  }
}
