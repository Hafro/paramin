#include "netcommunication.h"

netCommunication::netCommunication(char* progN, char** progA, int nh) {
  // pvmConst contains information about which tags and dataencoding to use
  pvmConst = new pvmconstants();
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
}

netCommunication::~netCommunication() {
  if (tids != NULL)
    delete [] tids;
  if (status != NULL)
    delete [] status;
  if (NETSTARTED == 1)
    stopNetCommunication();
}

// ********************************************************
// Functions for starting and stopping netcommunication
// ********************************************************
int netCommunication::startPVM() {
  int info;

  if (mytid < 0) {
    // have not yet enrolled in PVM
    mytid = pvm_mytid();
    if (mytid < 0) {
      printErrorMsg("Error in netCommunication - PVM not started");
      return ERROR;
    }

    info  = pvm_config(&nhost, &narch, &hostp);
    if (info < 0) {
      printErrorMsg("Error in netCommunication - PVM not responding");
      return ERROR;
    }

    assert(nHostInn >= 0);
    if (nHostInn > 0)
      nhost = nHostInn;

    assert(nhost > 0);
    tids = new int[nhost];
    status = new int[nhost];

    // export PWD to all processes spawned
    char* tempString;
    tempString = new char[strlen("PWD" + 1)];
    strcpy(tempString, "PWD");
    // Take out for multiprocessor maybe also Norway??
    info = pvm_export(tempString);
    delete [] tempString;
    return SUCCESS;

  } else {
    cout << "Already enrolled in PVM\n";
    return SUCCESS;
  }
}

int netCommunication::startNetCommunication() {
  int OK;

  if (NETSTARTED == 1 && mytid >= 0) {
    // have alredy enrolled in pvm and spawned program on slaves
    cout << "Already enrolled in PVM and running " << slaveProgram << " on slaves\n";
    return SUCCESS;

  } else {
    if (numVar <= 0) {
      cerr << "Error in netCommunication - number of variables must be positive\n";
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
          cerr << "Error in netCommunication - unrecognised return value\n";
          stopNetCommunication();
          return ERROR;
        }

      } else if (OK == 0) {
        // could not spawn all nhost processes
        stopNetCommunication();
        return ERROR;
      } else {
        cerr << "Error in netCommunication - unrecognised return value\n";
        stopNetCommunication();
        return ERROR;
      }

    } else if (OK == 0) {
      // could not enroll in pvm or error occured using pvm
      stopNetCommunication();
      return ERROR;
    } else {
      cerr << "Error in netCommunication - unrecognised return value\n";
      stopNetCommunication();
      return ERROR;
    }
  }
}

void netCommunication::stopNetCommunication() {
  int tid, info;
  int stopparam = -1;

  tid = pvm_mytid();
  if (tid > 0) {
    // pvmd is up and running and have joined pvm
    if (NETSTARTED == 1) {
      // have successfully spawned slaves and sent initial message to them
      info = pvm_initsend(pvmConst->getDataEncode());
      if (info < 0) {
        printErrorMsg("Error in netCommunication - unable to stop communication");
        cerr << "Error in netCommunication - unable to stop communication\n";

      } else {
        info = pvm_pkint(&stopparam, 1, 1);
        if (info < 0) {
          printErrorMsg("Error in netCommunication - unable to stop communication");
          cerr << "Error in netCommunication - unable to stop communication\n";

        } else {
          // maybe update tid according to status before broadcasting??
          info = pvm_mcast(tids, nhost, pvmConst->getStopTag());
          if (info < 0) {
            printErrorMsg("Error in netCommunication - unable to stop communication");
            cerr << "Error in netCommunication - unable to stop communication\n";
          }
        }
      }
    }
    pvm_exit();
  }
  mytid = -1;
  NETSTARTED = 0;
}

int netCommunication::spawnProgram() {
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
  delete [] emptyString;

  if (nspawn < 0) {
    printErrorMsg("Error in netCommunication - unable to spawn process");
    stopNetCommunication();
    exit(EXIT_FAILURE);

  } else if (nspawn < nhost) {
    printErrorMsg("Error in netCommunication - unable to spawn process");
    cerr << "Error in netCommunication - unable to start " << slaveProgram << " on all hosts\n";
    cerr << "The following hosts were not able to start:\n";
    for (i = nspawn; i < nhost; i++)
      cout << tids[i] << endl;

    return ERROR;

  } else {
    numProcesses = nhost;
    numGoodProcesses = nhost;
    for (i = 0; i < numProcesses; i++)
      status[i] = 1;

    return SUCCESS;
  }
}

int netCommunication::startProcesses() {
  //Send number of variables, group name and number of processes to spawned processes
  int cansend = 1;
  int info;
  int i;

  info = pvm_notify(PvmTaskExit, pvmConst->getDiedTag(), nhost, tids);
  if (info < 0) {
    printErrorMsg("Error in netCommunication - unable to notify");
    stopNetCommunication();
    return ERROR;
  }

  for (i = 0; i < nhost; i++) {
    // send initial message to all spawned processes
    cansend = sendInitialMessage(i);
    if (cansend == -1) {
      // Error occured in sending inital message to process with id = i
      return ERROR;

    } else if (cansend == 0) {
      cout << "Error in netCommunication - unable to send message\n";
      status[i] = -1;

    } else if (cansend == 1) {
      status[i] = 1;

    } else {
      cerr << "Error in netCommunication - unrecognised return value\n";
      stopNetCommunication();
      return ERROR;
    }
  }
  return SUCCESS;
}

int netCommunication::sendInitialMessage(int id) {
  int OK, info;

  if (id < 0 || id >= nhost) {
    cerr << "Error in netCommunication - illegal id for slave\n";
    return 0;
  }

  // check if process with identity = id is up and running
  OK = checkProcess(id);
  if (OK == 1) {
    info = pvm_initsend(pvmConst->getDataEncode());
    if (info < 0) {
      printErrorMsg("Error in netCommunication - not sending message");
      cerr << "Error in netCommunication - unable to send initial message\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_pkint(&numVar, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netCommunication - not sending message");
      cerr << "Error in netCommunication - unable to send initial message\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_pkint(&id, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netCommunication - not sending message");
      cerr << "Error in netCommunication - unable to send initial message\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_send(tids[id], pvmConst->getStartTag());
    if (info < 0) {
      printErrorMsg("Error in netCommunication - not sending message");
      cerr << "Error in netCommunication - unable to send initial message\n";
      stopNetCommunication();
      return ERROR;
    }
    return SUCCESS;

  } else if (OK == -1) {
    cerr << "Error in netCommunication - unable to check status of process\n";
    stopNetCommunication();
    return ERROR;

  } else if (OK == 0) {
    cerr << "Error in netCommunication - unable to send initial message\n";
    return OK;

  } else {
    cerr << "Error in netCommunication - unrecognised return value\n";
    stopNetCommunication();
    return ERROR;
  }
}

int netCommunication::sendNumberOfVariables() {
  int info;

  if (numVar <= 0) {
    cerr << "Error in netCommunication - number of variables must be positive\n";
    stopNetCommunication();
    return ERROR;
  }

  // tell slave program how many variables to expect in vector sent
  info = pvm_initsend(pvmConst->getDataEncode());
  if (info < 0) {
    printErrorMsg("Error in netCommunication - not sending variables");
    cerr << "Error in netCommunication - unable to send number of variables\n";
    stopNetCommunication();
    return ERROR;
  }

  info = pvm_pkint(&numVar, 1, 1);
  if (info < 0) {
    printErrorMsg("Error in netCommunication - not sending variables");
    cerr << "Error in netCommunication - unable to send number of variables\n";
    stopNetCommunication();
    return ERROR;
  }

  info = pvm_mcast(tids, nhost, pvmConst->getStartTag());
  if (info < 0) {
    printErrorMsg("Error in netCommunication - not sending variables");
    cerr << "Error in netCommunication - unable to send number of variables\n";
    stopNetCommunication();
    return ERROR;
  }
  return SUCCESS;
}

int netCommunication::checkProcess(int id) {
  int info, bufId, recvTid;

  assert (id >= 0);
  assert (id < numProcesses);

  bufId = pvm_probe(tids[id], pvmConst->getDiedTag());
  if (bufId > 0) {
    // message has arrived from tids[id] that has halted
    info = pvm_recv(tids[id], pvmConst->getDiedTag());
    if (info < 0) {
      printErrorMsg("Error in netCommunication - unable to check process");
      return ERROR;
    }

    info = pvm_upkint(&recvTid, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netCommunication - unable to check process");
      return ERROR;
    }

    if (recvTid != tids[id])
      return ERROR;

    status[id] = -1;
    numGoodProcesses--;
    return 0;

  } else if (bufId < 0) {
    cerr << "Error in netCommunication - unable to check process\n";
    return ERROR;

  } else {
    return SUCCESS;
  }
}

void netCommunication::getHealthOfProcesses(int* procTids) {
  checkProcesses();
  int i;
  for (i = 0; i < numProcesses; i++)
    procTids[i] = status[i];
}

void netCommunication::checkProcesses() {
  int i;
  int info;
  int tidDown;
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
    stopNetCommunication();
    exit(EXIT_FAILURE);
  }
}

// ********************************************************
// Functions for sending and receiving messages
// ********************************************************
#ifdef GADGET_NETWORK
int netCommunication::sendData(vectorofcharptr sendP) {
  int i, info;

  if (NETSTARTED == 1) {
    info = pvm_initsend(pvmConst->getDataEncode());
    if (info < 0) {
      printErrorMsg("Error in netCommunication - not sending data");
      cerr << "Error in netCommunication - unable to send data\n";
      stopNetCommunication();
      return ERROR;
    }

    assert(sendP.Size() >= numVar);
    for (i = 0; i < numVar; i++) {
      info = pvm_pkstr(sendP[i]);
      if (info < 0) {
        printErrorMsg("Error in netCommunication - not sending data");
        cerr << "Error in netCommunication - unable to pack data\n";
        stopNetCommunication();
        return ERROR;
      }
    }

    info = pvm_mcast(tids, nhost, pvmConst->getMasterSendStringTag());
    if (info < 0) {
      printErrorMsg("Error in netCommunication - not sending data");
      cerr << "Error in netCommunication - unable to send data\n";
      stopNetCommunication();
      return ERROR;
    } else
      return SUCCESS;

  } else {
    cerr << "Error in netCommunication - unable to send data\n";
    return ERROR;
  }
}

int netCommunication::sendBoundData(vector sendP) {
  int i, info;
  double* temp;

  if (NETSTARTED == 1) {
    info = pvm_initsend(pvmConst->getDataEncode());
    if (info < 0) {
      printErrorMsg("Error in netCommunication - not sending data");
      cerr << "Error in netCommunication - unable to send data\n";
      stopNetCommunication();
      return ERROR;
    }

    temp = new double[numVar];
    for (i = 0; i < numVar; i++)
      temp[i] = sendP[i];

    info = pvm_pkdouble(temp, numVar, 1);
    if (info < 0) {
      printErrorMsg("Error in netCommunication - not sending data");
      cerr << "Error in netCommunication - unable to pack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_mcast(tids, nhost, pvmConst->getMasterSendBoundTag());
    if (info < 0) {
      printErrorMsg("Error in netCommunication - not sending data");
      cerr << "Error in netCommunication - unable to send data\n";
      stopNetCommunication();
      return ERROR;
    } else
      return SUCCESS;

  } else {
    cerr << "Error in netCommunication - unable to send data\n";
    return ERROR;
  }
}
#endif

int netCommunication::sendData(netDataVariables* sendP, int processId) {
  int info;
  int cansend = 1;
  assert(processId >= 0);
  assert(processId < numProcesses);

  if (NETSTARTED == 1) {
    // check is process with id = processId is up and running
    cansend = checkProcess(processId);
    if (cansend == -1) {
      cerr << "Error in netCommunication - invalid process id " << processId << endl;
      stopNetCommunication();
      return ERROR;

    } else if (cansend == 0) {
      //process with id = processId is not up and running
      return cansend;

    } else if (cansend == 1) {
      info = pvm_initsend(pvmConst->getDataEncode());         // assume no data coding needed
      if (info < 0) {
        printErrorMsg("Error in netCommunication - not sending data");
        cerr << "Error in netCommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkint(&sendP->tag, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netCommunication - not sending data");
        cerr << "Error in netCommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkint(&sendP->x_id, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netCommunication - not sending data");
        cerr << "Error in netCommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_pkdouble(sendP->x, numVar, 1);
      if (info < 0) {
        printErrorMsg("Error in netCommunication - not sending data");
        cerr << "Error in netCommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_send(tids[processId], pvmConst->getMasterSendVarTag());
      if (info < 0) {
        printErrorMsg("Error in netCommunication - not sending data");
        cerr << "Error in netCommunication - unable to send data\n";
        stopNetCommunication();
        return ERROR;
      }
      return SUCCESS;

    } else {
      cerr << "Error in netCommunication - invalid response\n";
      stopNetCommunication();
      return ERROR;
    }

  } else {
    cerr << "Error in netCommunication - cannot send data\n";
    return ERROR;
  }
}

int netCommunication::receiveData(netDataResult* rp) {
  int info;

  if (NETSTARTED == 1) {
    // receive data from any process which sends data with msgtag = receiveTag
    info = pvm_recv(-1, pvmConst->getMasterReceiveDataTag());
    if (info < 0) {
      printErrorMsg("Error in netCommunication - not receiving data");
      cerr << "Error in netCommunication - PVM not responding\n";
      stopNetCommunication();
      return ERROR;

    } else {
      info = pvm_upkint(&rp->tag, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netCommunication - not receiving data");
        cerr << "Error in netCommunication - unable to unpack data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_upkdouble(&rp->result, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netCommunication - not receiving data");
        cerr << "Error in netCommunication - unable to unpack data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_upkint(&rp->who, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netCommunication - not receiving data");
        cerr << "Error in netCommunication - unable to unpack data\n";
        stopNetCommunication();
        return ERROR;
      }

      info = pvm_upkint(&rp->x_id, 1, 1);
      if (info < 0) {
        printErrorMsg("Error in netCommunication - not receiving data");
        cerr << "Error in netCommunication - unable to unpack data\n";
        stopNetCommunication();
        return ERROR;
      }
      return SUCCESS;
    }

  } else {
    cerr << "Error in netCommunication - cannot receive data\n";
    return ERROR;
  }
}

// ********************************************************
// Functions which set/return information about netcommunication
// ********************************************************
int netCommunication::getNumberOfHosts() {
  return nhost;
}

int netCommunication::getNumProcesses() {
  return numProcesses;
}

int netCommunication::getNumVar() {
  return numVar;
}

int netCommunication::getNumRunningProcesses() {
  return numGoodProcesses;
}

int netCommunication::netCommStarted() {
  return NETSTARTED;
}

void netCommunication::setNumInSendVar(int nVar) {
  if (nVar <= 0) {
    cerr << "Error in netCommunication - number of variables must be positive\n";
    exit(EXIT_FAILURE);
  }
  numVar = nVar;
}

void netCommunication::printErrorMsg(const char* errorMsg) {
  char* msg;
  msg = new char[strlen(errorMsg) + 1];
  strcpy(msg, errorMsg);
  pvm_perror(msg);
  delete [] msg;
}

int netCommunication::NET_ERROR() {
  return ERROR;
}

int netCommunication::NET_SUCCESS() {
  return SUCCESS;
}

masterCommunication::masterCommunication(char* progN, char** progA, int nh, int waitSec)
  : netCommunication(progN, progA, nh) {

  tmout.tv_sec = waitSec;
  tmout.tv_usec = 0;
}

masterCommunication::~masterCommunication() {
}

int masterCommunication::receiveData(netDataResult* rp) {
  int info, timeout;
  timeout = pvm_trecv(-1, pvmConst->getMasterReceiveDataTag(), &tmout);
  if (timeout < 0) {
    printErrorMsg("Error in netCommunication - not receiving data");
    cerr << "Error in netCommunication - PVM not responding\n";
    stopNetCommunication();
    return ERROR;

  } else if (timeout > 0) {
    // received information from process within tmout.
    info = pvm_upkint(&rp->tag, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netCommunication - not receiving data");
      cerr << "Error in netCommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkdouble(&rp->result, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netCommunication - not receiving data");
      cerr << "Error in netCommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkint(&rp->who, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netCommunication - not receiving data");
      cerr << "Error in netCommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }

    info = pvm_upkint(&rp->x_id, 1, 1);
    if (info < 0) {
      printErrorMsg("Error in netCommunication - not receiving data");
      cerr << "Error in netCommunication - unable to unpack data\n";
      stopNetCommunication();
      return ERROR;
    }
    return SUCCESS;

  } else {
    rp->who = -1;
    return SUCCESS;
  }
}
