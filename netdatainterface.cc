#include "netinterface.h"

// ********************************************************
// Functions for starting/stopping using a new data group
// ********************************************************
void netInterface::startNewDataGroup() {
  if (dctrl != NULL)
    stopUsingDataGroup();
  int newTag = getNextMsgTag();
  dctrl = new netDataControl(MAXNUMX, numVarInDataGroup, newTag);
  if (scaler != NULL)
    scaler->setPenalty(MAXNUMX);
  dataSet = new queue();
  ISALPHA = 0;
}

void netInterface::startNewDataGroup(const vector& x1, const vector& h1) {
  if (dctrl != NULL)
    stopUsingDataGroup();
  int newTag = getNextMsgTag();
  dctrl = new netDataControl(MAXNUMX, 1, newTag);
  if (scaler != NULL)
    scaler->setPenalty(MAXNUMX);
  dataSet = new queue();
  h = h1;
  alphaX = x1;
  ISALPHA = 1;
}

void netInterface::startNewDataGroup(int numInGroup) {
  if (dctrl != NULL)
    stopUsingDataGroup();
  int newTag = getNextMsgTag();
  dctrl = new netDataControl(numInGroup, numVarInDataGroup, newTag);
  if (scaler != NULL)
    scaler->setPenalty(numInGroup);
  dataSet = new queue();
  ISALPHA = 0;
}

void netInterface::startNewDataGroup(int numInGroup, const vector& x1, const vector& h1) {
  if (dctrl != NULL)
    stopUsingDataGroup();
  int newTag = getNextMsgTag();
  dctrl = new netDataControl(numInGroup, 1, newTag);
  if (scaler != NULL)
    scaler->setPenalty(numInGroup);
  dataSet = new queue();
  h = h1;
  alphaX = x1;
  ISALPHA = 1;
}

void netInterface::stopUsingDataGroup() {
  if (dctrl != NULL) {
    delete dctrl;
    dctrl = NULL;
  }
  if (dataSet != NULL) {
    delete dataSet;
    dataSet = NULL;
  }
  receiveId = -1;
  ISALPHA = -1;
}

// ********************************************************
// Functions for setting/getting netdata
// ********************************************************
void netInterface::setX(const vector& x1) {
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  if (dataGroupFull()) {
    cerr << "Error in netInterface - datagroup is full\n";
    exit(EXIT_FAILURE);
  }
  dctrl->setX(x1);
  int id = dctrl->getLastSetId();
  dataSet->put(id);
}

void netInterface::setXFirstToSend(const vector& x1) {
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  if (dataGroupFull()) {
    cerr << "Error in netInterface - datagroup is full\n";
    exit(EXIT_FAILURE);
  }
  dctrl->setX(x1);
  int id = dctrl->getLastSetId();
  dataSet->putFirst(id);
}

void netInterface::setDataPair(const vector& x1, double fx) {
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  if (dataGroupFull()) {
    cerr << "Error in netInterface - datagroup is full\n";
    exit(EXIT_FAILURE);
  }
  dctrl->setDataPair(x1, fx);
}

vector netInterface::getX(int id) {
 if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  return dctrl->getX(id);
}

double netInterface::getY(int id) {
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  return dctrl->getY(id);
}

vector netInterface::getNextAnswerX() {
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  return dctrl->getNextAnsweredX();
}

double netInterface::getNextAnswerY() {
 if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
 }
 return dctrl->getNextAnsweredY();
}

vector netInterface::getUpperScaleConstant() {
  return upperScale;
}

vector netInterface::getLowerScaleConstant() {
  return lowerScale;
}

vector netInterface::getInitialX() {
  return initialX;
}

void netInterface::setBestX(const vector& x) {
  if (x.dimension() != initialX.dimension()) {
    cerr << "Error in netInterface - vectors different size\n";
    exit(EXIT_FAILURE);
  }
  initialX = x;
}

// ********************************************************
// Functions for getting/setting information about datagroup
// ********************************************************
int netInterface::getReceiveId() {
  return receiveId;
}

void netInterface::sentDataItem(int id) {
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  dctrl->sentOne(id);
}

int netInterface::getNumNotAns() {
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  return dctrl->getNumNotAnswered();
}

int netInterface::getNumDataItemsSet() {
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
 return dctrl->getTotalSet();
}

int netInterface::getNumDataItemsAnswered() {
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  return dctrl->getNumAnswered();
}

void netInterface::setFirstAnsweredData() {
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  dctrl->setFirstAnswered();
}

int netInterface::getNumOfVarsInDataGroup() {
  return numVarInDataGroup;
}

int netInterface::getNumOfVarsInSendData() {
  return numVarToSend;
}

int netInterface::dataGroupFull() {
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  return dctrl->isFull();
}

// ********************************************************
// Input and output functions for data
// ********************************************************
void netInterface::printResult(char* outputFileName) {
  ofstream outputfile;
  outputfile.open(outputFileName);
  if (!outputfile) {
    cerr << "Error in netInterface - cannot open " << outputFileName << endl;
    exit(EXIT_FAILURE);
  }
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
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

void netInterface::printX(int id) {
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  cout << dctrl->getX(id);
}

void netInterface::printXUnscaled(int id) {
  if (dctrl == NULL) {
    cerr << "Error in netInterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  vector vec = dctrl->getX(id);
  if (scaler != NULL) {
    vector vecscaled = scaler->unscaleX(vec);
    cout << vecscaled;
  } else
    cout << vec;
}

// ********************************************************
// Functions for data converting and data scaling vectors
// ********************************************************
vector netInterface::makeVector(const vector& vec) {
  vector x1;
  if (ISALPHA == 1) {
    x1 = (alphaX + (vec[0] * h));
    return x1;
  } else
    return vec;
}

vector netInterface::unscaleX(const vector& vec) {
  vector xUnScaled;
  if (scaler != NULL) {
    xUnScaled = scaler->unscaleX(vec);
    return xUnScaled;
  } else
    return vec;
}

vector netInterface::convertX(const vector& vec) {
  vector vCon;
  if (dataConvert != NULL) {
    vCon = dataConvert->convertX(vec);
    return vCon;
  } else
    return vec;
}

vector netInterface::prepareVectorToSend(const vector& vec) {
  vector v;
  vector vUnscaled;
  vector vConverted;

  v = makeVector(vec);
  vUnscaled = unscaleX(v);
  vConverted = convertX(vUnscaled);
  return vConverted;
}

vector netInterface::getLowerbound() {
#ifdef GADGET_NETWORK
  assert(lowerBound.dimension() != 0);
  return lowerBound;
#else
  assert (scaler != NULL);
  return scaler->getLower();
#endif
}

vector netInterface::getUpperbound() {
#ifdef GADGET_NETWORK
  assert(upperBound.dimension() != 0);
  return upperBound;
#else
  assert(scaler != NULL);
  return scaler->getUpper();
#endif
}

#ifdef GADGET_NETWORK
vectorofcharptr netInterface::getSwitches() {
  assert(switches.Size() != 0);
  return switches;
}
#endif

vector netInterface::getOptInfo() {
  int i;
  vector vec(numVarToSend);
  if (dataConvert != NULL)
    vec = dataConvert->getOptInfo();
  else {
    // no dataconverter, so all parameters must be optimised
    for (i = 0; i < numVarToSend; i++)
      vec[i] = 1;
  }

  return vec;
}
