#include "netinterface.h"

// ********************************************************
// Constructors and destructor for class NetInterface
// ********************************************************
NetInterface::NetInterface(NetCommunication* netComm,
  ProcessManager* pm, CommandLineInfo* commandline) {

  maxNumX = 500;
  numTries = 4;
  scaler = NULL;
  dataConvert = NULL;
  dctrl = NULL;
  dataSet = NULL;
  numVarInDataGroup = 0;
  numVarToSend = 0;
  toscale = commandline->getScale();
  net = netComm;
  readInputFile(commandline->getInputFilename());
  initiateNetComm(pm, commandline->runCondor());
}

NetInterface::~NetInterface() {
  stopNetComm();
  stopUsingDataGroup();
  if (toscale == 1) {
    delete scaler;
    scaler = NULL;
  }
  if (dataConvert != NULL) {
    delete dataConvert;
    dataConvert = NULL;
  }
}

// ********************************************************
// Functions for reading input values from file
// ********************************************************
void NetInterface::readInputFile(char* initvalsFileName) {
  InitialInputFile* readInput = new InitialInputFile(initvalsFileName);
  readInput->readFromFile();

  if (readInput->readSwitches())
    setSwitches(readInput);

  if (readInput->repeatedValuesFileFormat() == 1) {
    // initvalsFile contains only vector values, no optimization info
    numVarInDataGroup = readInput->numVariables();
    numVarToSend = numVarInDataGroup;
    startNewDataGroup();
    setVector(readInput);
    while (!readInput->reachedEndOfFile()) {
      readInput->readVectorFromLine();
      setVector(readInput);
    }
  } else {
    // initvalsFile contains vector values and optimization info
    setOptInfo(readInput);
  }
  delete readInput;
}

void NetInterface::setSwitches(InitialInputFile* data) {
  int i;
  for (i = 0; i < data->numVariables(); i++)
     switches.resize(1, data->Switches(i).getValue());
}

void NetInterface::setVector(InitialInputFile* data) {
  int i, j;
  assert(numVarInDataGroup > 0);
  assert(data->numVariables() == numVarToSend);
  vector tempVector(numVarInDataGroup);
  if ((data->repeatedValuesFileFormat()) || (numVarInDataGroup == numVarToSend)) {
    assert(numVarInDataGroup == numVarToSend);
    for (i = 0; i < numVarInDataGroup; i++)
      tempVector[i] = data->Values(i);

  } else {
    assert(numVarInDataGroup < numVarToSend);
    j = 0;
    for (i = 0; i < numVarToSend; i++) {
      if (data->Optimise(i) == 1) {
        tempVector[j] = data->Values(i);
        j++;
      }
    }
    assert(j == numVarInDataGroup);
  }

  if (dataGroupFull()) {
    cerr << "Error in netinterface - datagroup is full\n";
    exit(EXIT_FAILURE);
  }
  setX(tempVector);
}

void NetInterface::setNumVars(InitialInputFile* data) {
  int i;
  numVarToSend = data->numVariables();
  if (numVarToSend <= 0) {
    cerr << "Error in netinterface - could not read vectors from file\n";
    exit(EXIT_FAILURE);
  }
  assert(numVarInDataGroup == 0);
  for (i = 0; i < numVarToSend; i++) {
    if (data->Optimise(i) == 1)
      numVarInDataGroup++;
  }

  if (numVarInDataGroup == 0) {
    cerr << "Error in netinterface - no variables to optimise\n";
    exit(EXIT_FAILURE);
  }
}

void NetInterface::setOptInfo(InitialInputFile* data) {
  int i, j;
  setNumVars(data);
  vector xvec(numVarInDataGroup);
  vector low(numVarInDataGroup);
  vector upp(numVarInDataGroup);
  vector xfull(numVarToSend);
  vector lowfull(numVarToSend);
  vector uppfull(numVarToSend);
  int* xind;
  xind = new int[numVarToSend];

  if (numVarInDataGroup == numVarToSend) {
    // only need lower, upper and xvec
    for (i = 0; i < numVarInDataGroup; i++) {
      xvec[i] = data->Values(i);
      low[i] = data->Lower(i);
      upp[i] = data->Upper(i);
    }
    uppfull = upp;
    lowfull = low;

  } else {
    j = 0;
    for (i = 0; i < numVarToSend; i++) {
      xfull[i] = data->Values(i);
      xind[i] = data->Optimise(i);
      lowfull[i] = data->Lower(i);
      uppfull[i] = data->Upper(i);
      if (xind[i] == 1) {
        xvec[j] = data->Values(i);
        low[j] = data->Lower(i);
        upp[j] = data->Upper(i);
        j++;
      }
    }
    assert(j == numVarInDataGroup);
    dataConvert = new DataConverter();
    dataConvert->setInitialData(xind, xfull);
  }

  // store the bounds so they can be sent to gadget
  upperBound = uppfull;
  lowerBound = lowfull;

  delete[] xind;
  if (toscale == 0) {
    lowerScale = low;
    upperScale = upp;
    initialX = xvec;
  } else {
    scaler = new DataScaler();
    vector temp(numVarInDataGroup);
    temp.setValue(1.0);
    upperScale = temp;
    temp.setValue(-1.0);
    lowerScale = temp;
    scaler->setInitialData(low, upp);
    initialX = (scaler->scaleX(xvec));
  }
}

// ********************************************************
// Function for initiating values before start using class NetInterface
// ********************************************************
void NetInterface::initiateNetComm(ProcessManager* pm, int condor) {
  int netStarted, numProc;
  isAlpha = -1;
  pManager = pm;
  net->setNumInSendVar(numVarToSend);
  netStarted = startNetComm();
  if (netStarted != 1) {
    cerr << "Error in netinterface - could not start netcommunication\n";
    exit(EXIT_FAILURE);
  } else {
    numProc = net->getNumProcesses();
    if (numProc <=0) {
      cerr << "Error in netinterface - no processes\n";
      exit(EXIT_FAILURE);
    }
    pManager->initializePM(numProc, condor);
    numberOfTags = 0;
    receiveID = -1;
  }

  if (switches.Size() > 0)
    sendStringValue();
  if (lowerBound.dimension() > 0)
    sendBoundValues();
}
