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

void NetInterface::startNewDataGroup(const DoubleVector& x1, const DoubleVector& h1) {
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

void NetInterface::startNewDataGroup(int numInGroup, const DoubleVector& x1, const DoubleVector& h1) {
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
void NetInterface::setX(const DoubleVector& x1) {
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

void NetInterface::setXFirstToSend(const DoubleVector& x1) {
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

void NetInterface::setDataPair(const DoubleVector& x1, double fx) {
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

const DoubleVector& NetInterface::getX(int id) {
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

const DoubleVector& NetInterface::getNextAnswerX() {
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

const DoubleVector& NetInterface::getUpperScaleConstant() {
  return upperScale;
}

const DoubleVector& NetInterface::getLowerScaleConstant() {
  return lowerScale;
}

const DoubleVector& NetInterface::getInitialX() {
  return initialX;
}
double NetInterface::getScore() {
    return initialScore;
}
void NetInterface::setScore(double fx) {
    initialScore = fx;
}
double NetInterface::getInitialScore(DoubleVector& x) {
  //void NetInterface::getInitialScore(DoubleVector& x, double fx) {
    // must make sure this is ok
    x  = this->prepareVectorToSend(initialX);
    //fx = initialScore;
    return initialScore;
}
void NetInterface::setInitialScore(const DoubleVector& x, double fx) {
     if (x.Size() != initialX.Size()) {
	 cerr << "Error in netinterface - vectors different size\n";
	 exit(EXIT_FAILURE);
     }
    initialX = x;
    initialScore = fx;
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
  int i;
  DoubleVector temp;
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  temp = dctrl->getX(id);
  for (i = 0; i < temp.Size(); i++)
      cout << temp[i] << sep;
  cout << endl;
}

void NetInterface::printXUnscaled(int id) {
    int i;
  if (dctrl == NULL) {
    cerr << "Error in netinterface - no valid datagroup\n";
    exit(EXIT_FAILURE);
  }
  DoubleVector vec;
  if (scaler != NULL) {
      // DoubleVector vecscaled = scaler->unscaleX(vec);
      // cout << scaler->unscaleX(dctrl->getX(id));
      //cout << vecscaled;
      vec = scaler->unscaleX(dctrl->getX(id));
  } else
      vec = dctrl->getX(id);
  for (i = 0; i < vec.Size(); i++)
      cout << vec[i] << sep;
  cout << endl;
}

// ********************************************************
// Functions for data converting and data scaling vectors
// ********************************************************
const DoubleVector& NetInterface::makeVector(const DoubleVector& vec) {
  int i;
  // must check if numvarindatagroup is correct and formula to make vector is correct!!!! also check if ok to set here DoubleVector temp and then return const DoubleVector& will it be a copy or not!!!!
  if (isAlpha == 1) {
      // DoubleVector tmp(numVarInDataGroup);
    for (i = 0; i < numVarInDataGroup; i++)
       alphaX_h[i] = alphaX[i] + (vec[0] * h[i]);
    return alphaX_h;
  } else
    return vec;
}

const DoubleVector& NetInterface::unscaleX(const DoubleVector& vec) {
  if (scaler != NULL) {
      // xUnscale = scaler->unscaleX(vec);
      return scaler->unscaleX(vec);
      // return xUnscale;
  } else
    return vec;
}

const DoubleVector& NetInterface::convertX(const DoubleVector& vec) {
  if (dataConvert != NULL) {
      // xConvert = dataConvert->convertX(vec);
    return dataConvert->convertX(vec);
  } else
    return vec;
}

const DoubleVector& NetInterface::prepareVectorToSend(const DoubleVector& vec) {
    // xToSend = convertX(unscaleX(makeVector(vec)));
  return convertX(unscaleX(makeVector(vec)));
}

const DoubleVector& NetInterface::getLowerbound() {
  assert(lowerBound.Size() != 0);
  return lowerBound;
}

const DoubleVector& NetInterface::getUpperbound() {
  assert(upperBound.Size() != 0);
  return upperBound;
}

const ParameterVector& NetInterface::getSwitches() {
  assert(switches.Size() != 0);
  return switches;
}

const IntVector& NetInterface::getOptInfo() {
  int i;
  if (dataConvert != NULL)
    return dataConvert->getOptInfo();
  else {
    // no dataconverter, so all parameters must be optimised
      optVec.Reset();
    optVec.resize(numVarToSend, 1);
    return optVec;
  }
}
