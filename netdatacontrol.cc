#include "netdatacontrol.h"

NetInfo::NetInfo() {
  set = 0;
  sent = 0;
  received = 0;
  numPendingAnswers = 0;
}

NetInfo::~NetInfo() {
}

int NetInfo::hasSet() {
  return set;
}

int NetInfo::hasReceived() {
  return (sent && received);
}

int NetInfo::hasSent() {
  return sent;
}

int NetInfo::numCopiesSent() {
  return numPendingAnswers;
}

coordinates::coordinates() {
  y = 0.0;
}

coordinates::~coordinates() {
}

double coordinates::getY() {
  return y;
}

vector coordinates::getX() {
  return x;
}

double coordinates::getParameter(int num) {
  assert(num >= 0);
  if (x.dimension() <= 0) {
    cerr << "Error in coordinates - vector has not been initialised\n";
    exit(EXIT_FAILURE);
  }
  return x[num];
}

void coordinates::setX(const vector& x1) {
  x = x1;
}

void coordinates::setParameter(double p, int num) {
  if ((num < 0) || (num >= x.dimension())) {
    cerr << "Error in coordinates - number is out of bounds\n";
    exit(EXIT_FAILURE);
  }
  x[num] = p;
}

void coordinates::setY(double y1) {
  y = y1;
}

int coordinates::getNumParameters() {
  return x.dimension();
}

NetDataControl::NetDataControl(int numx, int numberOfParameters, int t) {
  numberOfx = 0;
  totalNumx = numx;
  numberSent = 0;
  numberAnswers = 0;
  resendId = -1;
  tag = t;
  numPar = numberOfParameters;
  xyCoord = new coordinates*[numx];
  nInfo = new NetInfo*[numx];
  int i;
  for (i = 0; i < numx; i++) {
    xyCoord[i] = new coordinates();
    nInfo[i] = new NetInfo;
  }
}

NetDataControl::~NetDataControl() {
  int i;
  for (i = 0; i < totalNumx; i++) {
    delete xyCoord[i];
    xyCoord[i] = NULL;
    delete nInfo[i];
    nInfo[i] = NULL;
  }
  delete[] xyCoord;
  delete[] nInfo;
}

// ----------------------------------------------------------------------------
// Functions for setting data
void NetDataControl::setX(const vector& x1) {
  if (numberOfx < 0 || numberOfx >= totalNumx) {
    cerr << "Error in netdatacontrol - cannot store vector\n";
    exit(EXIT_FAILURE);
  }
  if ((x1.dimension()) != numPar) {
    cerr << "Error in netdatacontrol - wrong number of parameters\n";
    exit(EXIT_FAILURE);
  }
  xyCoord[numberOfx]->setX(x1);
  nInfo[numberOfx]->set = 1;
  numberOfx++;
}

int NetDataControl::getIdToSetNext() {
  // might change
  if (numberOfx == totalNumx)
    return -1;
  else
    return numberOfx;
}

void NetDataControl::setY(int id, double fx) {
  if (id < 0 || id >= totalNumx) {
    cerr << "Error in netdatacontrol - illegal id\n";
    exit(EXIT_FAILURE);
  }
  if (!nInfo[id]->hasReceived()) {
    // have not received an answer yet
    xyCoord[id]->setY(fx);
    nInfo[id]->received = 1;
    numberAnswers++;
  }
  nInfo[id]->numPendingAnswers--;

  if (nInfo[id]->sent == 0)
    cerr << "Warning in netdatacontrol - setting f(x) but have not sent x with identity: " << id << endl;
}

void NetDataControl::setDataPair(const vector& x1, double fx) {
  setX(x1);
  sentOne(numberOfx-1);
  setY(numberOfx-1, fx);
}

// ----------------------------------------------------------------------------
// Functions for getting data
vector NetDataControl::getX(int id) {
  if (id < 0 || id >= numberOfx) {
    cerr << "Error in netdatacontrol - have not set vector with id " << id << endl;
    exit(EXIT_FAILURE);
  }
  return xyCoord[id]->getX();
}

vector NetDataControl::getNextXToSend() {
  int i = 0;
  int FOUND = 0;
  vector vec;

  if (allSent()) {
    cerr << "Error in netdatacontrol - no vector to send\n";
    exit(EXIT_FAILURE);
  }
  if (numberSent < 0 || numberSent >= numberOfx) {
    cerr << "Error in netdatacontrol - number ot send out of bounds\n";
    exit(EXIT_FAILURE);
  }
  while (i < totalNumx && FOUND == 0) {
    if (nInfo[i]->hasSet() == 1 && nInfo[i]->hasSent() == 0)
      FOUND = 1;
    i++;
  }
  return xyCoord[i - 1]->getX();
}

double NetDataControl::getY(int id) {
  if (id < 0 || id >= totalNumx) {
    cerr << "Error in netdatacontrol - illegal id\n";
    exit(EXIT_FAILURE);
  }
  if (!nInfo[id]->hasSet())
    cerr << "Warning in netdatacontrol - x has not been set yet\n";
  if (!nInfo[id]->hasReceived())
    cerr << "Warning in netdatacontrol - y has not been set yet\n";
  return xyCoord[id]->getY();
}

vector NetDataControl::getNextAnsweredX() {
  if (numberAnswers == 0) {
    cerr << "Error in netdatacontrol - no answers received\n";
    exit(EXIT_FAILURE);
  }
  if (nextAns >= numberOfx) {
    cerr << "Error in netdatacontrol - illegal answer received\n";
    exit(EXIT_FAILURE);
  }

  while (!nInfo[nextAns]->received)
    nextAns++;

  int temp = nextAns;
  nextAns++;
  return xyCoord[temp]->getX();
}

double NetDataControl::getNextAnsweredY() {
  int temp = nextAns - 1;
  if (temp < 0 || temp >= numberOfx) {
    cerr << "Error in netdatacontrol - illegal answer received\n";
    exit(EXIT_FAILURE);
  }
  return xyCoord[temp]->getY();
}

// ----------------------------------------------------------------------------
// Functions concerning sending data
int NetDataControl::getNextSendId() {
  int i = 0;
  int FOUND = 0;
  if (allSent()) {
    cerr << "Error in netdatacontrol - no more vectors to send\n";
    exit(EXIT_FAILURE);
  }
  while (i < numberOfx && FOUND == 0) {
    if (nInfo[i]->hasSet() == 1 && nInfo[i]->hasSent() == 0)
      FOUND = 1;
    i++;
  }
  return (i - 1);
}

int NetDataControl::getNextXToResend() {
  int numPending = 1;
  int counter = 0;
  if (allReceived()) {
    cerr << "Error in netdatacontrol - no data to resend\n";
    exit(EXIT_FAILURE);
  }
  resendId++;
  if (resendId == totalNumx)
    resendId = 0;
  while ((nInfo[resendId]->hasReceived() || !nInfo[resendId]->hasSent()
      || nInfo[resendId]->numPendingAnswers > numPending)) {
    counter++;
    resendId++;
    if (resendId == totalNumx)
      resendId = 0;
    if (counter == totalNumx)
      numPending++;
  }
  return resendId;
}

void NetDataControl::sentOne(int id) {
  if ((id < 0) || (id >= numberOfx)) {
    cerr << "Error in netdatacontrol - illegal id\n";
    exit(EXIT_FAILURE);
  }
  if (nInfo[id]->sent == 1)
    cerr << "Warning in netdatacontrol - already sent data with identity: " << id << endl;
  else {
    nInfo[id]->sent = 1;
    nInfo[id]->numPendingAnswers++;
    numberSent++;
  }
}

void NetDataControl::resentOne(int id) {
  if ((id < 0) || (id >= numberOfx)) {
    cerr << "Error in netdatacontrol - illegal id\n";
    exit(EXIT_FAILURE);
  }
  nInfo[id]->numPendingAnswers++;
}

// ----------------------------------------------------------------------------
// Functions for getting general information about state of datagroup
int NetDataControl::allSent() {
  assert(numberOfx >= numberSent);
  return (numberOfx == numberSent);
}

int NetDataControl:: getNumNotAnswered() {
  return (numberSent - numberAnswers);
}

int NetDataControl::getTotalReceived() {
  return numberAnswers;
}

int NetDataControl::getTotalSet() {
  return numberOfx;
}

void NetDataControl::setFirstAnswered() {
  nextAns = 0;
}

int NetDataControl::allReceived() {
  return (numberSent == numberAnswers);
}

int NetDataControl::getNumAnswered() {
  return numberAnswers;
}

int NetDataControl::getNumLeftToSend() {
  return (numberOfx - numberSent);
}

int NetDataControl::getTag() {
  return tag;
}

int NetDataControl::getLastSetId() {
  return numberOfx - 1;
}

int NetDataControl::getMaxNumData() {
  return totalNumx;
}

int NetDataControl::hasAnswer(int id) {
  if (id < 0 || id >= totalNumx) {
    cerr << "Error in netdatacontrol - id out of bounds\n";
    exit(EXIT_FAILURE);
  }
  return nInfo[id]->hasReceived();
}

int NetDataControl::isFull() {
  return (getMaxNumData() == getTotalSet());
}
