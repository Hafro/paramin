#include "netdatacontrol.h"

netInfo::netInfo() {
  set = 0;
  sent = 0;
  received = 0;
  numberOfPendingAnswers = 0;
}

netInfo::~netInfo() {
}

int netInfo::hasSet() {
  return set;
}

int netInfo::hasReceived() {
  return (sent && received);
}

int netInfo::hasSent() {
  return sent;
}

int netInfo::numberOfCopiesSent() {
  return numberOfPendingAnswers;
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

netDataControl::netDataControl(int numx, int numberOfParameters, int t) {
  numberOfx = 0;
  totalNumx = numx;
  numberSent = 0;
  numberAnswers = 0;
  resendId = -1;
  tag = t;
  numPar = numberOfParameters;
  xyCoord = new coordinates*[numx];
  nInfo = new netInfo*[numx];
  int i;
  for (i = 0; i < numx; i++) {
    xyCoord[i] = new coordinates();
    nInfo[i] = new netInfo;
  }
}

netDataControl::~netDataControl() {
  int i;
  for (i = 0; i < totalNumx; i++) {
    delete xyCoord[i];
    xyCoord[i] = NULL;
    delete nInfo[i];
    nInfo[i] = NULL;
  }
  delete xyCoord;
  delete nInfo;
}

// ----------------------------------------------------------------------------
// Functions for setting data
void netDataControl::setX(const vector& x1) {
  if (numberOfx < 0 || numberOfx >= totalNumx) {
    cerr << "Error in dataControl - cannot store vector\n";
    exit(EXIT_FAILURE);
  }
  if ((x1.dimension()) != numPar) {
    cerr << "Error in dataControl - wrong number of parameters\n";
    exit(EXIT_FAILURE);
  }
  xyCoord[numberOfx]->setX(x1);
  nInfo[numberOfx]->set = 1;
  numberOfx++;
}

int netDataControl::getIdToSetNext() {
  // might change
  if (numberOfx == totalNumx)
    return -1;
  else
    return numberOfx;
}

void netDataControl::setY(int id, double fx) {
  if (id < 0 || id >= totalNumx) {
    cerr << "Error in dataControl - illegal id\n";
    exit(EXIT_FAILURE);
  }
  if (!nInfo[id]->hasReceived()) {
    // have not received an answer yet
    xyCoord[id]->setY(fx);
    nInfo[id]->received = 1;
    numberAnswers++;
  }
  nInfo[id]->numberOfPendingAnswers--;

  if (nInfo[id]->sent == 0)
    cout << "Warning - setting f(x) but have not sent x with identity: " << id << endl;
}

void netDataControl::setDataPair(const vector& x1, double fx) {
  setX(x1);
  sentOne(numberOfx-1);
  setY(numberOfx-1, fx);
}

// ----------------------------------------------------------------------------
// Functions for getting data
vector netDataControl::getX(int id) {
  if (id < 0 || id >= numberOfx) {
    cerr << "Error in dataControl - have not set vector with id " << id << endl;
    exit(EXIT_FAILURE);
  }
  vector vec = xyCoord[id]->getX();
  return vec;
}

vector netDataControl::getNextXToSend() {
  int i = 0;
  int FOUND = 0;
  vector vec;

  if (allSent()) {
    cerr << "Error in dataControl - no vector to send\n";
    exit(EXIT_FAILURE);
  }
  if (numberSent < 0 || numberSent >= numberOfx) {
    cerr << "Error in dataControl - number ot send out of bounds\n";
    exit(EXIT_FAILURE);
  }
  while (i < totalNumx && FOUND == 0) {
    if (nInfo[i]->hasSet() == 1 && nInfo[i]->hasSent() == 0)
      FOUND = 1;
    i++;
  }
  vec = xyCoord[i - 1]->getX();
  return vec;
}

double netDataControl::getY(int id) {
  if (id < 0 || id >= totalNumx) {
    cerr << "Error in dataControl - illegal id\n";
    exit(EXIT_FAILURE);
  }
  if (!nInfo[id]->hasSet())
    cout << "Warning in dataControl - x has not been set yet\n";
  if (!nInfo[id]->hasReceived())
    cout << "Warning in dataControl - y has not been set yet\n";
  return xyCoord[id]->getY();
}

vector netDataControl::getNextAnsweredX() {
  if (numberAnswers == 0) {
    cerr << "Error in dataControl - no answers received\n";
    exit(EXIT_FAILURE);
  }
  if (nextAns >= numberOfx) {
    cerr << "Error in dataControl - illegal answer received\n";
    exit(EXIT_FAILURE);
  }

  while (!nInfo[nextAns]->received)
    nextAns++;

  int temp = nextAns;
  nextAns++;
  vector vec;
  vec = xyCoord[temp]->getX();
  return vec;

}

double netDataControl::getNextAnsweredY() {
  int temp = nextAns - 1;
  if (temp < 0 || temp >= numberOfx) {
    cerr << "Error in dataControl - illegal answer received\n";
    exit(EXIT_FAILURE);
  }
  return xyCoord[temp]->getY();
}

// ----------------------------------------------------------------------------
// Functions concerning sending data
int netDataControl::getNextSendId() {
  int i = 0;
  int FOUND = 0;
  if (allSent()) {
    cerr << "Error in dataControl - no more vectors to send\n";
    exit(EXIT_FAILURE);
  }
  while (i < numberOfx && FOUND == 0) {
    if (nInfo[i]->hasSet() == 1 && nInfo[i]->hasSent() == 0)
      FOUND = 1;
    i++;
  }
  return (i - 1);
}

int netDataControl::getNextXToResend() {
  int numPending = 1;
  int counter = 0;
  if (allReceived()) {
    cerr << "Error in dataControl - no data to resend\n";
    exit(EXIT_FAILURE);
  }
  resendId++;
  if (resendId == totalNumx)
    resendId = 0;
  while ((nInfo[resendId]->hasReceived() || !nInfo[resendId]->hasSent()
      || nInfo[resendId]->numberOfPendingAnswers > numPending)) {
    counter++;
    resendId++;
    if (resendId == totalNumx)
      resendId = 0;
    if (counter == totalNumx)
      numPending++;
  }
  return resendId;
}

void netDataControl::sentOne(int id) {
  if ((id < 0) || (id >= numberOfx)) {
    cerr << "Error in dataControl - illegal id\n";
    exit(EXIT_FAILURE);
  }
  if (nInfo[id]->sent == 1)
    cout << "Warning in dataControl - already sent data with identity: " << id << endl;
  else {
    nInfo[id]->sent = 1;
    nInfo[id]->numberOfPendingAnswers++;
    numberSent++;
  }
}

void netDataControl::resentOne(int id) {
  if ((id < 0) || (id >= numberOfx)) {
    cerr << "Error in dataControl - illegal id\n";
    exit(EXIT_FAILURE);
  }
  nInfo[id]->numberOfPendingAnswers++;
}

// ----------------------------------------------------------------------------
// Functions for getting general information about state of datagroup
int netDataControl::allSent() {
  assert(numberOfx >= numberSent);
  return (numberOfx == numberSent);
}

int netDataControl:: getNumNotAnswered() {
  return (numberSent - numberAnswers);
}

int netDataControl::getTotalReceived() {
  return numberAnswers;
}

int netDataControl::getTotalSet() {
  return numberOfx;
}

void netDataControl::setFirstAnswered() {
  nextAns = 0;
}

int netDataControl::allReceived() {
  return (numberSent == numberAnswers);
}

int netDataControl::getNumAnswered() {
  return numberAnswers;
}

int netDataControl::getNumberLeftToSend() {
  return (numberOfx - numberSent);
}

int netDataControl::getTag() {
  return tag;
}

int netDataControl::getLastSetId() {
  int temp = numberOfx - 1;
  return temp;
}

int netDataControl::getMaxNumberOfData() {
  return totalNumx;
}

int netDataControl::hasAnswer(int id) {
  if (id < 0 || id >= totalNumx) {
    cerr << "Error in dataControl - id out of bounds\n";
    exit(EXIT_FAILURE);
  }
  return nInfo[id]->hasReceived();
}

int netDataControl::isFull() {
  int FULL = (getMaxNumberOfData() == getTotalSet());
  return FULL;
}
