#include "netinterface.h"

// ********************************************************
// Functions for sending/receiving
// ********************************************************
void NetInterface::sendOne() {
  int cansend, tid, dataid;
  //tid = pManager->getNextTidToSend(net);
  tid = pManager->getNextTidToSend(dctrl->getNumLeftToSend(), net);
  while (tid == waitForBetterProcesses())
    tid = pManager->getNextTidToSend(dctrl->getNumLeftToSend(), net);
    
  if (tid < 0) {
    cerr << "Error in netinterface - no processes available\n";
    stopNetComm();
    exit(EXIT_FAILURE);
  }

  assert(!dataSet->isEmpty());
  dataid = dataSet->get();
  cansend = sendOne(tid, dataid);
  if (cansend != 1) {
    // could not send data with id = dataid sendDataso update queue
    dataSet->putFirst(dataid);
    cerr << "Error in netinterface - failed to send data\n";
    stopNetComm();
    exit(EXIT_FAILURE);
  }
}

int NetInterface::sendOne(int processID, int x_id) {
  // Function returns ERROR if error occures while trying to send
  // Function returns SUCCESS if successfully sent data
  // Function return 0 if process with process identity = processID can not be used.
  int i, cansend;
  // Vector vec;
  DoubleVector vecSend;

  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  if (processID < 0 || (processID >= net->getNumProcesses())) {
    cerr << "Error in netinterface - invalid process id\n";
    stopNetComm();
    return net->netError();
  }
  if (x_id < 0 || (x_id >= dctrl->getTotalSet())) {
    cerr << "Error in netinterface - invalid vector id\n";
    stopNetComm();
    return net->netError();
  }

  // prepare data to be sent
  assert(numVarToSend > 0);
  NetDataVariables* sp = new NetDataVariables(numVarToSend);
  // vec = dctrl->getX(x_id);
  vecSend = prepareVectorToSend(dctrl->getX(x_id));
  for (i = 0; i < numVarToSend; i++) {
    sp->x[i] = vecSend[i];
  }
  
  sp->x_id = x_id;
  sp->tag = dctrl->getTag();
  cansend = net->sendData(sp, processID);
  delete sp;
  if (cansend == 1) {
    //successfully sent sp using processID
    dctrl->sentOne(x_id);
    pManager->sent(processID);
    return net->netSuccess();

  } else if (cansend == -1) {
    cerr << "Error in netinterface - failed to send data\n";
    return net->netError();
  } else if (cansend == 0) {
    // process with id = processID is down
    pManager->processDown(processID);
    return cansend;
  } else {
    cerr << "Error in netinterface - unrecognised return value\n";
    stopNetComm();
    return net->netError();
  }
}

int NetInterface::resend() {
  int i, canresend;
  int tid = -1;
  int sendID;
  // Vector vec;
  DoubleVector vecSend;

  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  if (!pManager->isFreeProc()) {
    cerr << "Error in netinterface - no free processes\n";
    stopNetComm();
    return net->netError();
  }
  if (dctrl->allReceived()) {
    cerr << "Error in netinterface - no data to resend\n";
    stopNetComm();
    return net->netError();
  }

  tid = pManager->getNextTidToSend(dctrl->getNumLeftToSend(), net);
  if (tid < 0)
    tid = pManager->getTidToSend(net);

  if (tid < 0) {
    // there are no free processes
    cerr << "Error in netinterface - no free processes\n";
    stopNetComm();
    return net->netError();
  }

  sendID = dctrl->getNextXToResend();
  // vec = dctrl->getX(sendID);
  vecSend = prepareVectorToSend(dctrl->getX(sendID));
  assert(numVarToSend > 0);
  NetDataVariables* sp = new NetDataVariables(numVarToSend);
  for (i = 0; i < numVarToSend; i++)
    sp->x[i] = vecSend[i];

  sp->x_id = sendID;
  sp->tag = dctrl->getTag();
  canresend = net->sendData(sp, tid);
  while (canresend == 0 && pManager->isFreeProc()) {
   tid = pManager->getNextTidToSend(dctrl->getNumLeftToSend(), net);
   if (tid < 0)
     tid = pManager->getTidToSend(net);
   canresend = net->sendData(sp, tid);
  }

  delete sp;
  if (canresend == 1) {
    dctrl->resentOne(sendID);
    pManager->sent(tid);
    return net->netSuccess();

  } else if (canresend == -1) {
    cerr << "Error in netinterface - failed to resend data\n";
    return net->netError();
  } else if (canresend == 0 && !pManager->isFreeProc()) {
    cerr << "Error in netinterface - failed to resend data\n";
    stopNetComm();
    return net->netError();
  } else {
    cerr << "Error in netinterface - unrecognised return value\n";
    stopNetComm();
    return net->netError();
  }
}

int NetInterface::receiveOne() {
  // function returns SUCCESS if successfully could receive data
  // function return ERROR if netcommunication is down or error occurs
  // functions returns 0 if cannot receive data but netcommunication is OK

  int received;
  double res;
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }

  NetDataResult* dp = new NetDataResult;
  received = net->receiveData(dp);
  if (received == 1 && (dp->who >= 0)) {
    // received data successfully
    pManager->setFreeProc(dp->who);
    if (dctrl->getTag() == dp->tag) {
      // received data belonging to datagroup
      if (scaler != NULL) {
	  // Vector tempV;
	  // tempV = makeVector(dctrl->getX(dp->x_id));
	  // res = scaler->scaleResult(dp->result, dp->x_id, tempV);
	  res = scaler->scaleResult(dp->result, dp->x_id, makeVector(dctrl->getX(dp->x_id)));
      } else {
        res = dp->result;
      }

      if (dctrl->hasAnswer(dp->x_id)) {
        receiveID = -1;
      } else {
        receiveID = dp->x_id;
        dctrl->setY(dp->x_id, res);
      }

    } else {
      // received data which does not belong to current datagroup
      receiveID = -1;
    }

    delete dp;
    return received;

  } else if (received == 1 && dp->who == -1) {
    // Was not able to receive but can continue netcommunication,
    cerr << "Error in netinterface - could not receive data\n";
    receiveID = -1;
    delete dp;
    // probably timeout occured so should check for health of processes.
    net->getHealthOfProcesses(pManager->getStatus());
    return !received;

  } else if (received == -1) {
    cerr << "Error in netinterface - failed to receive data\n";
    receiveID = -1;
    delete dp;
    return net->netError();

  } else {
    cerr << "Error in netinterface - unrecognised return value\n";
    receiveID = -1;
    stopNetComm();
    delete dp;
    return net->netError();
  }
}

int NetInterface::sendToAllIdleHosts() {
  // functions returns ERROR if error occures in netcommunication
  // function return SUCCESS if successfully sent all available data

  int tid = 0;
  int dataID;
  int cansend = 1;
  
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }

  if (!dctrl->allSent()) {
    tid = pManager->getNextTidToSend(dctrl->getNumLeftToSend(), net);
  };
  while ((tid != this->noAvailableProcesses()) && !dctrl->allSent()
      && (cansend == 1 || cansend == 0)) {

    // have not sent all data belonging to dctrl
    if (tid == this->waitForBetterProcesses())
      tid = pManager->getTidToSend(net);

    if (tid != this->noAvailableProcesses()) {
      assert(tid >= 0);
      assert(!dataSet->isEmpty());
      dataID = dataSet->get();
      cansend = sendOne(tid, dataID);

      if (cansend != 1)
        dataSet->putFirst(dataID);

      if (!dctrl->allSent() && (cansend == 1 || cansend == 0))
        tid = pManager->getNextTidToSend(dctrl->getNumLeftToSend(), net);
    }
  }

  if (cansend > 1 || cansend < -1) {
    cerr << "Error in netinterface - unrecognised return value\n";
    stopNetComm();
    return net->netError();
  } else if (cansend == -1) {
    return net->netError();
  } else {
    return net->netSuccess();
  }
}

int NetInterface::sendToIdleHosts() {
  // function returns ERROR if error occures while trying to send data,
  // function returns SUCCESS if successfully sent available data
  int tid = 0;
  int dataID;
  int cansend = 1;
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  if (!dctrl->allSent()) {
	// dctrl->getNumLeftToSend() er í lagi, vesen í getNextTidToSend
      tid = pManager->getNextTidToSend(dctrl->getNumLeftToSend(), net);
      //tid = 0;
  }
  while ((tid != this->noAvailableProcesses()) && (tid != this->waitForBetterProcesses())
      && (!dctrl->allSent()) && (cansend == 1 || cansend == 0)) {
    // Have not sent all data and there is a suitable process available
    assert(tid >= 0);
    assert(!dataSet->isEmpty());
    dataID = dataSet->get();
    cansend = sendOne(tid, dataID);
    if (cansend != 1)
      dataSet->putFirst(dataID);
    if ((cansend == 1 || cansend == 0) && !dctrl->allSent()) {
      tid = pManager->getNextTidToSend(dctrl->getNumLeftToSend(), net);
    };
  }

  if (cansend > 1 || cansend < -1) {
    cerr << "Error in netinterface - unrecognised return value\n";
    stopNetComm();
    return net->netError();
  } else if (cansend == -1) {
    return net->netError();
  } else {
    return net->netSuccess();
  }
}

int NetInterface::sendAll() {
  assert(dctrl != NULL);
  int OK = sendToIdleHosts();
  while (!dctrl->allSent() && OK == 1) {
    // try to receive data from a host as no more suitable processes
    // available and still have data to send
    OK = receiveAndSend();
  }
  return OK;
}

int NetInterface::receiveAndSend() {
  // This function tries to keep all processes busy by first trying to
  // receive and then sending available data to all suitable processes
  // It is assumed that there is an outstanding message to be received.
  int counter = 0;
  int cansend = 1;
  int canreceive = 0;

  while (counter < numTries && canreceive == 0) {
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
    return net->netError();

  } else if (canreceive == 0) {
    // Did not receive any data
    if (!pManager->isFreeProc()) {
      // no free process after trying to receive
      cerr << "Error in netinterface - not receiving from busy process\n";
      return net->netError();
    } else {
      // There is a free processs that can be used, will try to send
      cansend = sendToAllIdleHosts();
      return cansend;
    }

  } else {
    cerr << "Error in netinterface - unrecognised return value\n";
    stopNetComm();
    return net->netError();
  }
}

int NetInterface::receiveOnCondition(Condition* con) {
  int counter = 0;
  int cond = 0;
  int receiveID;
  int canreceive = 1;

  while (!dctrl->allReceived() && !cond&& canreceive == 1) {
    canreceive = receiveOne();
    while (counter < (numTries - 1) && canreceive == 0) {
      canreceive = receiveOne();
      counter++;
    }

    if (canreceive == 1) {
      receiveID = getReceiveID();
      if (receiveID >= 0)
        cond = con->computeCondition();
    }
    counter = 0;
  }

  if (canreceive == -1) {
    return net->netError();

  } else if (canreceive == 0) {
    cerr << "Error in netinterface - no received data\n";
    stopNetComm();
    return net->netError();

  } else if (cond == 1) {
    // condition has been met
    return cond;

  } else if (dctrl->allReceived() && cond == 0) {
    // condition has not been met and nothing to receive
    return cond;

  } else {
    cerr << "Error in netinterface - unrecognised return value\n";
    stopNetComm();
    return net->netError();
  }
}

int NetInterface::receiveAll() {
  int counter = 0;
  int canreceive = 1;
  int cansend = 1;

  while (!(dctrl->allReceived()) && (canreceive == 0 || canreceive == 1) && cansend == 1) {
    canreceive = receiveOne();
    while (counter < (numTries - 1) && canreceive == 0) {
      canreceive = receiveOne();
      counter++;
    }

    if (canreceive == 0) {
      // Did not receive anything - try to resend data
      if (!pManager->isFreeProc()) {
        cerr << "Error in netinterface - no received data\n";
        canreceive = -1;
      } else if (!dctrl->allReceived() && pManager->isFreeProc()) {
        // have not received all so will resend data
        cansend = resend();
      }
    }
    counter = 0;
  }

  if (canreceive == -1 || cansend == -1) {
    return net->netError();
  } else if (canreceive > 1 || canreceive < -1) {
    stopNetComm();
    return net->netError();
  } else if (cansend > 1 || cansend < -1) {
    stopNetComm();
    return net->netError();
  } else if (dctrl->allReceived()) {
    return net->netSuccess();
  } else {
    cerr << "Error in netinterface - unrecognised return value\n";
    stopNetComm();
    return net->netError();
  }
}

int NetInterface::sendAndReceiveAllData() {
  int cansend_receive;
  cansend_receive = sendAll();
  if (cansend_receive == 1)
   cansend_receive = receiveAll();

  if (cansend_receive == 1)
    return net->netSuccess();
  else
    return net->netError();
}

int NetInterface::sendAllOnCondition(Condition* con) {
  int cond = 0;
  int receiveID;
  int OK = 1;

  // start by sending data to all free suitable hosts.
  OK = sendToIdleHosts();
  // receive and send until received all or condition is true or error
  while (!dctrl->allSent() && cond == 0&& OK == 1) {
    // try to receive data from a host
    OK = receiveAndSend();
    // if received from dctrl then check if condition is true
    if (OK == 1) {
      receiveID = getReceiveID();
      if (receiveID >= 0)
        cond = con->computeCondition();
    }
  }

  if (OK != 1)
    return net->netError();
  else
    return cond;
}

int NetInterface::sendAndReceiveSetData(Condition* con) {
  int cond = 0;
  int sendreceive = 1;
  int numTries = 0;
  int counter, newreturns, numLeftToReceive, totalNumHosts;

  while ((sendreceive != -1) && cond == 0) {
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
        while (numTries < (numTries - 1) && sendreceive == 0) {
          sendreceive = receiveOne();
          numTries++;
        }

        if (sendreceive == 0) {
          if (!pManager->isFreeProc()) {
            cerr << "Error in netinterface - received no data\n";
            sendreceive = -1;
            stopNetComm();
          } else if (!dctrl->allReceived() && pManager->isFreeProc()) {
            sendreceive = resend();
          }
        }
        numTries = 0;

      } else {
        sendreceive = receiveOne();
        while (numTries < (numTries - 1) && sendreceive == 0) {
          sendreceive = receiveOne();
          numTries++;
        }

        if (sendreceive == 0) {
          if (!pManager->isFreeProc()) {
            cerr << "Error in netinterface - received no data\n";
            sendreceive = -1;
            stopNetComm();
          }
        }
      }

      if (sendreceive == 1)
        cond = con->computeCondition();
    }
  }

  if (sendreceive == -1)
    return net->netError();
  else if (cond == 1)
    return cond;
  else
    return net->netError();
}

int NetInterface::sendAndReceiveTillCondition(Condition* con) {
  int cond;
  cond = sendAllOnCondition(con);
  if (cond == 0)
    cond = receiveOnCondition(con);
  return cond;
}

// ********************************************************
// Functions concerning netcommunication
// ********************************************************
int NetInterface::startNetComm() {
  return net->startNetCommunication();
}

void NetInterface::sendStringValue() {
  assert(numVarToSend > 0);
  assert(switches.Size() == numVarToSend);
  int SEND = net->sendData(switches);
  if (!(SEND == netSuccess())) {
    cerr << "Error in netinterface - failed to send switches\n";
    exit(EXIT_FAILURE);
  }
}

void NetInterface::sendStringValue(int processID) {
  assert(numVarToSend > 0);
  assert(switches.Size() == numVarToSend);
  int SEND = net->sendData(switches,processID);
  if (!(SEND == netSuccess())) {
    cerr << "Error in netinterface - failed to send switches\n";
    exit(EXIT_FAILURE);
  }
}

void NetInterface::sendBoundValues() {
  assert(numVarToSend > 0);
  assert(lowerBound.Size() == numVarToSend);
  int SEND = net->sendBoundData(lowerBound);
  if (!(SEND == netSuccess())) {
    cerr << "Error in netinterface - failed to send lowerbounds\n";
    exit(EXIT_FAILURE);
  }

  assert(upperBound.Size() == numVarToSend);
  SEND = net->sendBoundData(upperBound);
  if (!(SEND == netSuccess())) {
    cerr << "Error in netinterface - failed to send upperbounds\n";
    exit(EXIT_FAILURE);
  }
}

void NetInterface::sendBoundValues(int processID) {
  assert(numVarToSend > 0);
  assert(lowerBound.Size() == numVarToSend);
  int SEND = net->sendBoundData(lowerBound, processID);
  if (!(SEND == netSuccess())) {
    cerr << "Error in netinterface - failed to send lowerbounds\n";
    exit(EXIT_FAILURE);
  }
  assert(upperBound.Size() == numVarToSend);
  SEND = net->sendBoundData(upperBound, processID);
  if (!(SEND == netSuccess())) {
    cerr << "Error in netinterface - failed to send upperbounds\n";
    exit(EXIT_FAILURE);
  }
}

void NetInterface::stopNetComm() {
  net->stopNetCommunication();
  pManager->noProcessesRunning();
}

int NetInterface::getNumFreeProcesses() {
  return pManager->getNumFreeProc();
}

int NetInterface::getTotalNumProc() {
  return net->getNumProcesses();
}

int NetInterface::getNumTags() {
  return numberOfTags;
}

int NetInterface::getNextMsgTag() {
  int tempTag = numberOfTags;
  numberOfTags++;
  return tempTag;
}

int NetInterface::isUpAndRunning() {
  return net->netCommStarted();
}

int NetInterface::netError() {
  return net->netError();
}

int NetInterface::netSuccess() {
  return net->netSuccess();
}

int NetInterface::noAvailableProcesses() {
  return pManager->noAvailableProcesses();
}

int NetInterface::waitForBetterProcesses() {
  return pManager->waitForBetterProcesses();
}
