#include "netinterface.h"

// ********************************************************
// Functions for sending/receiving
// ********************************************************
void netInterface::sendOne() {
  int cansend, tid, dataid;
  tid = pManager->getNextTidToSend(net);
  if (tid < 0) {
    cerr << "Error in netCommunication - no processes available\n";
    stopNetComm();
    exit(EXIT_FAILURE);
  }

  assert(!dataSet->isEmpty());
  dataid = dataSet->get();
  cansend = sendOne(tid, dataid);
  if (cansend != 1) {
    // could not send data with id = dataid so update queue
    dataSet->putFirst(dataid);
    cout << "Error in netCommunication - failed to send data\n";
    stopNetComm();
    exit(EXIT_FAILURE);
  }
}

int netInterface::sendOne(int processId, int x_id) {
  // Function returns ERROR if error occures while trying to send
  // Function returns SUCCESS if successfully sent data
  // Function return 0 if process with process identity = processId can not be used.
  int i, cansend;
  vector vec;
  vector vecSend;

  if (dctrl == NULL) {
    cerr << "Error in netCommunication - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  if (processId < 0 || (processId >= net->getNumProcesses())) {
    cerr << "Error in netCommunication - illegal process id\n";
    stopNetComm();
    return net->NET_ERROR();
  }
  if (x_id < 0 || (x_id >= dctrl->getTotalSet())) {
    cerr << "Error in netCommunication - illegal vector id\n";
    stopNetComm();
    return net->NET_ERROR();
  }

  // prepare data to be sent
  assert(numVarToSend > 0);
  netDataVariables* sp = new netDataVariables(numVarToSend);
  vec = dctrl->getX(x_id);
  vecSend = prepareVectorToSend(vec);
  for (i = 0; i < numVarToSend; i++)
    sp->x[i] = vecSend[i];

  sp->x_id = x_id;
  sp->tag = dctrl->getTag();
  cansend = net->sendData(sp, processId);
  delete sp;
  if (cansend == 1) {
    //successfully sent sp using processId
    dctrl->sentOne(x_id);
    pManager->sent(processId);
    return net->NET_SUCCESS();

  } else if (cansend == -1) {
    cerr << "Error in netCommunication - failed to send data\n";
    return net->NET_ERROR();
  } else if (cansend == 0) {
    // process with id = processId is down
    pManager->processDown(processId);
    return cansend;
  } else {
    cerr << "Error in netCommunication - unrecognised return value\n";
    stopNetComm();
    return net->NET_ERROR();
  }
}

int netInterface::resend() {
  int i, canresend;
  int tid = -1;
  int sendId;
  vector vec;
  vector vecSend;

  if (dctrl == NULL) {
    cerr << "Error in netCommunication - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  if (!pManager->isFreeProc()) {
    cerr << "Error in netCommunication - no free processes\n";
    stopNetComm();
    return net->NET_ERROR();
  }
  if (dctrl->allReceived()) {
    cerr << "Error in netCommunication - no data to resend\n";
    stopNetComm();
    return net->NET_ERROR();
  }

  tid = pManager->getNextTidToSend(dctrl->getNumberLeftToSend(), net);
  if (tid < 0)
    tid = pManager->getNextTidToSend(net);

  if (tid < 0) {
    // there are no free processes
    cerr << "Error in netCommunication - no free processes\n";
    stopNetComm();
    return net->NET_ERROR();
  }

  sendId = dctrl->getNextXToResend();
  vec = dctrl->getX(sendId);
  vecSend = prepareVectorToSend(vec);
  assert(numVarToSend > 0);
  netDataVariables* sp = new netDataVariables(numVarToSend);
  for (i = 0; i < numVarToSend; i++)
     sp->x[i] = vecSend[i];

  sp->x_id = sendId;
  sp->tag = dctrl->getTag();
  canresend = net->sendData(sp, tid);
  while (canresend == 0 && pManager->isFreeProc()) {
   tid = pManager->getNextTidToSend(dctrl->getNumberLeftToSend(), net);
   if (tid < 0)
     tid = pManager->getNextTidToSend(net);
   canresend = net->sendData(sp, tid);
  }
  delete sp;

  if (canresend == 1) {
    dctrl->resentOne(sendId);
    pManager->sent(tid);
    return net->NET_SUCCESS();

  } else if (canresend == -1) {
    cerr << "Error in netCommunication - failed to resend data\n";
    return net->NET_ERROR();
  } else if (canresend == 0 && !pManager->isFreeProc()) {
    cerr << "Error in netCommunication - failed to resend data\n";
    stopNetComm();
    return net->NET_ERROR();
  } else {
    cerr << "Error in netCommunication - unrecognised return value\n";
    stopNetComm();
    return net->NET_ERROR();
  }
}

int netInterface::receiveOne() {
  // function returns SUCCESS if successfully could receive data
  // function return ERROR if netcommunication is down or error occurs
  // functions returns 0 if cannot receive data but netcommunication is OK

  int received;
  double res;
  if (dctrl == NULL) {
    cerr << "Error in netCommunication - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }

  netDataResult* dp = new netDataResult;
  received = net->receiveData(dp);
  if (received == 1 && (dp->who >= 0)) {
    // received data successfully
    pManager->setFreeProc(dp->who);
    if (dctrl->getTag() == dp->tag) {
      // received data belonging to datagroup
      if (scaler != NULL) {
        vector tempV;
        tempV = makeVector(dctrl->getX(dp->x_id));
        res = scaler->scaleResult(dp->result, dp->x_id, tempV);

      } else {
        res = dp->result;
      }

      if (dctrl->hasAnswer(dp->x_id)) {
        receiveId = -1;
      } else {
        receiveId = dp->x_id;
        dctrl->setY(dp->x_id, res);
      }

    } else {
      // received data which does not belong to current datagroup
      receiveId = -1;
    }

    delete dp;
    return received;

  } else if (received == 1 && dp->who == -1) {
    // Was not able to receive but can continue netcommunication,
    cerr << "Error in netCommunication - could not receive data - timeout error?\n";
    receiveId = -1;
    delete dp;
    // probably timeout occured so should check for health of processes.
    net->getHealthOfProcesses(pManager->getStatus());
    return !received;

  } else if (received == -1) {
    cerr << "Error in netCommunication - failed to receive data\n";
    receiveId = -1;
    delete dp;
    return net->NET_ERROR();

  } else {
    cerr << "Error in netCommunication - unrecognised return value\n";
    receiveId = -1;
    stopNetComm();
    delete dp;
    return net->NET_ERROR();
  }
}

int netInterface::sendToAllIdleHosts() {
  // functions returns ERROR if error occures in netcommunication
  // function return SUCCESS if successfully sent all available data

  int tid = 0;
  int dataId;
  int cansend = 1;

  if (dctrl == NULL) {
    cerr << "Error in netCommunication - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  if (!dctrl->allSent())
    tid = pManager->getNextTidToSend(dctrl->getNumberLeftToSend(), net);

  while ((tid != this->NO_AVAILABLE_PROCESSES()) && !dctrl->allSent() && (cansend == 1 || cansend == 0)) {

    // have not sent all data belonging to dctrl
    if (tid == this->WAIT_FOR_BETTER_PROCESSES()) {
      // want to use all available processes so just take a slow one
      tid = pManager->getNextTidToSend(net);
    }

    if (tid != this->NO_AVAILABLE_PROCESSES()) {
      assert(tid >= 0);
      assert(!dataSet->isEmpty());
      dataId = dataSet->get();
      cansend = sendOne(tid, dataId);

      if (cansend != 1)
        dataSet->putFirst(dataId);

      if (!dctrl->allSent() && (cansend == 1 || cansend == 0))
        tid = pManager->getNextTidToSend(dctrl->getNumberLeftToSend(), net);
    }
  }

  if (cansend > 1 || cansend < -1) {
    cerr << "Error in netCommunication - unrecognised return value\n";
    stopNetComm();
    return net->NET_ERROR();

  } else if (cansend == -1) {
    return net->NET_ERROR();
  } else {
    return net->NET_SUCCESS();
  }
}

int netInterface::sendToIdleHosts() {
  // function returns ERROR if error occures while trying to send data,
  // function returns SUCCESS if successfully sent available data

  int tid = 0;
  int dataId;
  int cansend = 1;

  if (dctrl == NULL) {
    cerr << "Error in netCommunication - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }

  if (!dctrl->allSent())
    tid = pManager->getNextTidToSend(dctrl->getNumberLeftToSend(), net);

  while ((tid != this->NO_AVAILABLE_PROCESSES()) && (tid != this->WAIT_FOR_BETTER_PROCESSES())
      && (!dctrl->allSent()) && (cansend == 1 || cansend == 0)) {

    // Have not sent all data and there is a suitable process available
    assert(tid >= 0);
    assert(!dataSet->isEmpty());
    dataId = dataSet->get();
    cansend = sendOne(tid, dataId);
    if (cansend != 1)
      dataSet->putFirst(dataId);
    if ((cansend == 1 || cansend == 0) && !dctrl->allSent())
      tid = pManager->getNextTidToSend(dctrl->getNumberLeftToSend(), net);
  }

  if (cansend > 1 || cansend < -1) {
    cerr << "Error in netCommunication - unrecognised return value\n";
    stopNetComm();
    return net->NET_ERROR();
  } else if (cansend == -1) {
    return net->NET_ERROR();
  } else {
    return net->NET_SUCCESS();
  }
}

int netInterface::sendAll() {
  int OK = 1;
  assert(dctrl != NULL);
  OK = sendToIdleHosts();
  // have sent all possible data to suitable free hosts
  // receive and send until finished or error
  while (!dctrl->allSent() && OK == 1) {
    // try to receive data from a host as no more suitable processes
    // available and still have data to send
    OK = receiveAndSend();
  }
  return OK;
}

int netInterface::receiveAndSend() {
  // This function tries to keep all processes busy by first trying to
  // receive and then sending available data to all suitable processes
  // It is assumed that there is an outstanding message to be received.

  int counter = 0;
  int cansend = 1;
  int canreceive = 0;

  while (counter < NUM_TRIES_TO_RECEIVE && canreceive == 0) {
    // Try to receive again from a process, netcommunication is OK but
    // could not receive probably because there are no messages coming in
    canreceive = receiveOne();
    counter++;
  }

  if (canreceive == 1) {
    // Received successfully, at least one process is free
    cansend = sendToIdleHosts();
    return cansend;

  } else if (canreceive == -1) {
    // Error occured while trying to receive, netcommunication is down
    return net->NET_ERROR();

  } else if (canreceive == 0) {
    // Did not receive any data
    if (!pManager->isFreeProc()) {
      // no free process after trying to receive
      cerr << "Error in netCommunication - not receiving from busy process\n";
      return net->NET_ERROR();
    } else {
      // There is a free processs that can be used, will try to send
      cansend = sendToAllIdleHosts();
      return cansend;
    }

  } else {
    // Error in return value of receiveOnne
    stopNetComm();
    return net->NET_ERROR();
  }
}

int netInterface::receiveOnCondition(condition* con) {
  int counter = 0;
  int cond = 0;
  int receiveId;
  int canreceive = 1;

  while (!dctrl->allReceived() && !cond&& canreceive == 1) {
    canreceive = receiveOne();
    while (counter < (NUM_TRIES_TO_RECEIVE - 1) && canreceive == 0) {
      canreceive = receiveOne();
      counter++;
    }

    if (canreceive == 1) {
      receiveId = getReceiveId();
      if (receiveId >= 0)
        cond = con->computeCond();
    }
    counter = 0;
  }

  if (canreceive == -1) {
    return net->NET_ERROR();

  } else if (canreceive == 0) {
    cerr << "Error in netCommunication - no received data\n";
    stopNetComm();
    return net->NET_ERROR();

  } else if (cond == 1) {
    // condition has been met
    return cond;

  } else if (dctrl->allReceived() && cond == 0) {
    // condition has not been met and nothing to receive
    return cond;

  } else {
    stopNetComm();
    return net->NET_ERROR();
  }
}

int netInterface::receiveAll() {
  int counter = 0;
  int canreceive = 1;
  int cansend = 1;

  while (!(dctrl->allReceived()) && (canreceive == 0 || canreceive == 1) && cansend == 1) {
    canreceive = receiveOne();
    while (counter < (NUM_TRIES_TO_RECEIVE - 1) && canreceive == 0) {
      canreceive = receiveOne();
      counter++;
    }

    if (canreceive == 0) {
      // Did not receive anything - try to resend data
      if (!pManager->isFreeProc()) {
        cerr << "Error in netCommunication - no received data\n";
        canreceive = -1;
      } else if (!dctrl->allReceived() && pManager->isFreeProc()) {
        // have not received all so will resend data
        cansend = resend();
      }
    }
    counter = 0;
  }

  if (canreceive == -1 || cansend == -1) {
    return net->NET_ERROR();
  } else if (canreceive > 1 || canreceive < -1) {
    stopNetComm();
    return net->NET_ERROR();
  } else if (cansend > 1 || cansend < -1) {
    stopNetComm();
    return net->NET_ERROR();
  } else if (dctrl->allReceived()) {
    return net->NET_SUCCESS();
  } else {
    cerr << "Error in netCommunication - no received data\n";
    stopNetComm();
    return net->NET_ERROR();
  }
}

int netInterface::send_receiveAllData() {
  int cansend_receive;
  cansend_receive = sendAll();
  if (cansend_receive == 1)
   cansend_receive = receiveAll();

  if (cansend_receive == 1)
    return net->NET_SUCCESS();
  else
    return net->NET_ERROR();
}

int netInterface::sendAllOnCondition(condition* con) {
  int cond = 0;
  int receiveId;
  int OK = 1;

  // start by sending data to all free suitable hosts.
  OK = sendToIdleHosts();
  // receive and send until received all or condition is true or error
  while (!dctrl->allSent() && cond == 0&& OK == 1) {
    // try to receive data from a host
    OK = receiveAndSend();
    // if received from dctrl then check if condition is true
    if (OK == 1) {
      receiveId = getReceiveId();
      if (receiveId >= 0)
        cond = con->computeCond();
    }
  }

  if (OK != 1)
    return net->NET_ERROR();
  else
    return cond;
}

int netInterface::send_receive_setData(condition* con) {
  int cond = 0;
  int sendreceive = 1;
  int numTries = 0;
  int counter, newreturns, numLeftToReceive, totalNumHosts;

  while ((sendreceive != -1 ) && cond == 0) {
    counter = getNumDataItemsSet();
    newreturns = getNumDataItemsAnswered();
    numLeftToReceive = counter - newreturns;
    totalNumHosts = getTotalNumProc();

    sendreceive = sendToAllIdleHosts();
    if (sendreceive == 1) {
      numTries = 0;
      if ((numLeftToReceive < totalNumHosts) && dctrl->allSent()) {
        // can start resending, waiting for last data to come in
        sendreceive = receiveOne();
        while (numTries < (NUM_TRIES_TO_RECEIVE - 1) && sendreceive == 0) {
          sendreceive = receiveOne();
          numTries++;
        }

        if (sendreceive == 0) {
          if (!pManager->isFreeProc()) {
            cout << "Error in netCommunication - received no data\n";
            sendreceive = -1;
            stopNetComm();
          } else if (!dctrl->allReceived() && pManager->isFreeProc()) {
            sendreceive = resend();
          }
        }
        numTries = 0;

      } else {
        sendreceive = receiveOne();
        while (numTries < (NUM_TRIES_TO_RECEIVE - 1) && sendreceive == 0) {
          sendreceive = receiveOne();
          numTries++;
        }

        if (sendreceive == 0) {
          if (!pManager->isFreeProc()) {
            cout << "Error in netCommunication - received no data\n";
            sendreceive = -1;
            stopNetComm();
          }
        }
      }

      if (sendreceive == 1)
        cond = con->computeCond();
    }
  }

  if (sendreceive == -1)
    return net->NET_ERROR();
  else if (cond == 1)
    return cond;
  else
    return net->NET_ERROR();
}

int netInterface::send_receiveTillCondition(condition* con) {
  int condition;
  condition = sendAllOnCondition(con);
  if (condition == 0)
    condition = receiveOnCondition(con);
  return condition;
}

// ********************************************************
// Functions concerning netcommunication
// ********************************************************
int netInterface::startNetComm() {
  return net->startNetCommunication();
}

#ifdef GADGET_NETWORK
void netInterface::sendStringValue() {
  assert(numVarToSend > 0);
  assert(switches.Size() == numVarToSend);
  int SEND = net->sendData(switches);
  if (!(SEND == NET_SUCCESS())) {
    cerr << "Error in netCommunication - failed to send switches\n";
    exit(EXIT_FAILURE);
  }
}

void netInterface::sendBoundValues() {
  assert(numVarToSend > 0);
  int SEND;

  assert(lowerBound.dimension() == numVarToSend);
  SEND = net->sendBoundData(lowerBound);
  if (!(SEND == NET_SUCCESS())) {
    cerr << "Error in netCommunication - failed to send lowerbounds\n";
    exit(EXIT_FAILURE);
  }

  assert(upperBound.dimension() == numVarToSend);
  SEND = net->sendBoundData(upperBound);
  if (!(SEND == NET_SUCCESS())) {
    cerr << "Error in netCommunication - failed to send upperbounds\n";
    exit(EXIT_FAILURE);
  }
}
#endif

void netInterface::stopNetComm() {
  net->stopNetCommunication();
  pManager->noProcessesRunning();
}

int netInterface::getNumFreeProcesses() {
  return pManager->getNumFreeProc();
}

int netInterface::getTotalNumProc() {
  return net->getNumProcesses();
}

int netInterface::getNumberOfTags() {
  return numberOfTags;
}

int netInterface::getNextMsgTag() {
  int tempTag = numberOfTags;
  numberOfTags++;
  return tempTag;
}

int netInterface::isUpAndRunning() {
  return net->netCommStarted();
}

int netInterface::NET_ERROR() {
  return net->NET_ERROR();
}

int netInterface::NET_SUCCESS() {
  return net->NET_SUCCESS();
}

int netInterface::NO_AVAILABLE_PROCESSES() {
  return pManager->NO_AVAILABLE_PROCESSES();
}

int netInterface::WAIT_FOR_BETTER_PROCESSES() {
  return pManager->WAIT_FOR_BETTER_PROCESSES();
}
