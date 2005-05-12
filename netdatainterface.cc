#include "netinterface.h"

// ********************************************************
// Functions for starting/stopping using a new data group
// ********************************************************
void NetInterface::startNewDataGroup() {
  if (dctrl != NULL)
    stopUsingDataGroup();
  int newTag = getNextMsgTag();
  dctrl = new NetDataControl(maxNumX, numVarInDataGroup, newTag);
  if (scaler != NULL)
    scaler->setPenalty(maxNumX);
  dataSet = new Queue();
  isAlpha = 0;
}

void NetInterface::startNewDataGroup(const Vector& x1, const Vector& h1) {
  if (dctrl != NULL)
    stopUsingDataGroup();
  int newTag = getNextMsgTag();
  dctrl = new NetDataControl(maxNumX, 1, newTag);
  if (scaler != NULL)
    scaler->setPenalty(maxNumX);
  dataSet = new Queue();
  h = h1;
  alphaX = x1;
  isAlpha = 1;
}

void NetInterface::startNewDataGroup(int numInGroup) {
  if (dctrl != NULL)
    stopUsingDataGroup();
  int newTag = getNextMsgTag();
  dctrl = new NetDataControl(numInGroup, numVarInDataGroup, newTag);
  if (scaler != NULL)
    scaler->setPenalty(numInGroup);
  dataSet = new Queue();
  isAlpha = 0;
}

void NetInterface::startNewDataGroup(int numInGroup, const Vector& x1, const Vector& h1) {
  if (dctrl != NULL)
    stopUsingDataGroup();
  int newTag = getNextMsgTag();
  dctrl = new NetDataControl(numInGroup, 1, newTag);
  if (scaler != NULL)
    scaler->setPenalty(numInGroup);
  dataSet = new Queue();
  h = h1;
  alphaX = x1;
  isAlpha = 1;
}

void NetInterface::stopUsingDataGroup() {
  if (dctrl != NULL) {
    delete dctrl;
    dctrl = NULL;
  }
  if (dataSet != NULL) {
    delete dataSet;
    dataSet = NULL;
  }
  receiveID = -1;
  isAlpha = -1;
}

// ********************************************************
// Functions for setting/getting netdata
// ********************************************************
void NetInterface::setX(const Vector& x1) {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  if (dataGroupFull()) {
    cerr << "Error in netinterface - datagroup is full\n";
    exit(EXIT_FAILURE);
  }
  dctrl->setX(x1);
  int id = dctrl->getLastSetID();
  dataSet->put(id);
}

void NetInterface::setXFirstToSend(const Vector& x1) {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  if (dataGroupFull()) {
    cerr << "Error in netinterface - datagroup is full\n";
    exit(EXIT_FAILURE);
  }
  dctrl->setX(x1);
  int id = dctrl->getLastSetID();
  dataSet->putFirst(id);
}

void NetInterface::setDataPair(const Vector& x1, double fx) {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  if (dataGroupFull()) {
    cerr << "Error in netinterface - datagroup is full\n";
    exit(EXIT_FAILURE);
  }
  dctrl->setDataPair(x1, fx);
}

Vector NetInterface::getX(int id) {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  return dctrl->getX(id);
}

double NetInterface::getY(int id) {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  return dctrl->getY(id);
}

Vector NetInterface::getNextAnswerX() {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  return dctrl->getNextAnsweredX();
}

double NetInterface::getNextAnswerY() {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  return dctrl->getNextAnsweredY();
}

Vector NetInterface::getUpperScaleConstant() {
  return upperScale;
}

Vector NetInterface::getLowerScaleConstant() {
  return lowerScale;
}

Vector NetInterface::getInitialX() {
  return initialX;
}

void NetInterface::setBestX(const Vector& x) {
  if (x.dimension() != initialX.dimension()) {
    cerr << "Error in netinterface - vectors different size\n";
    exit(EXIT_FAILURE);
  }
  initialX = x;
}

// ********************************************************
// Functions for getting/setting information about datagroup
// ********************************************************
int NetInterface::getReceiveID() {
  return receiveID;
}

void NetInterface::sentDataItem(int id) {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  dctrl->sentOne(id);
}

int NetInterface::getNumNotAns() {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  return dctrl->getNumNotAnswered();
}

int NetInterface::getNumDataItemsSet() {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
 return dctrl->getTotalSet();
}

int NetInterface::getNumDataItemsAnswered() {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  return dctrl->getNumAnswered();
}

void NetInterface::setFirstAnsweredData() {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  dctrl->setFirstAnswered();
}

int NetInterface::getNumVarsInDataGroup() {
  return numVarInDataGroup;
}

int NetInterface::getNumVarsInSendData() {
  return numVarToSend;
}

int NetInterface::dataGroupFull() {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  return dctrl->isFull();
}

int NetInterface::allSent() {
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  return dctrl->allSent();
}

// ********************************************************
// Input and output functions for data
// ********************************************************
void NetInterface::printResult(char* outputFileName) {
  ofstream outputfile;
  outputfile.open(outputFileName);
  if (!outputfile) {
    cerr << "Error in netinterface - cannot open " << outputFileName << endl;
    exit(EXIT_FAILURE);
  }
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  int numAnswers = dctrl->getTotalReceived();
  int counter = 0;
  while (counter < numAnswers) {
    if (dctrl->hasAnswer(counter)) {
      outputfile << (dctrl->getY(counter));
      outputfile << endl;
      counter++;
    }
  }
  outputfile.close();
}

void NetInterface::printX(int id) {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  cout << dctrl->getX(id);
}

void NetInterface::printXUnscaled(int id) {
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  Vector vec = dctrl->getX(id);
  if (scaler != NULL) {
    Vector vecscaled = scaler->unscaleX(vec);
    cout << vecscaled;
  } else
    cout << vec;
}

// ********************************************************
// Functions for data converting and data scaling vectors
// ********************************************************
const Vector& NetInterface::makeVector(const Vector& vec) {
  if (isAlpha == 1) {
    tmp = alphaX + (vec[0] * h);
    return tmp;
  } else
    return vec;
}

const Vector& NetInterface::unscaleX(const Vector& vec) {
  if (scaler != NULL) {
    xUnscale = scaler->unscaleX(vec);
    return xUnscale;
  } else
    return vec;
}

const Vector& NetInterface::convertX(const Vector& vec) {
  if (dataConvert != NULL) {
    xConvert = dataConvert->convertX(vec);
    return xConvert;
  } else
    return vec;
}

const Vector& NetInterface::prepareVectorToSend(const Vector& vec) {
  xToSend = convertX(unscaleX(makeVector(vec)));
  return xToSend;
}

const Vector& NetInterface::getLowerbound() {
  assert(lowerBound.dimension() != 0);
  return lowerBound;
}

const Vector& NetInterface::getUpperbound() {
  assert(upperBound.dimension() != 0);
  return upperBound;
}

CharPtrVector NetInterface::getSwitches() {
  assert(switches.Size() != 0);
  return switches;
}

const Vector& NetInterface::getOptInfo() {
  int i;
  if (dataConvert != NULL)
    opt = dataConvert->getOptInfo();
  else {
    // no dataconverter, so all parameters must be optimised
    Vector vec(numVarToSend);
    opt = vec;
    for (i = 0; i < numVarToSend; i++)
      opt[i] = 1;
  }
  return opt;
}
