#include "netinterface.h"

// ********************************************************
// Constructors and destructor for class NetInterface
// ********************************************************
NetInterface::NetInterface(NetCommunication* netComm,
  ProcessManager* pm, CommandLineInfo* commandline) {

  MAXNUMX = 500;
  NUM_TRIES_TO_RECEIVE = 4;
  scaler = NULL;
  dataConvert = NULL;
  dctrl = NULL;
  dataSet = NULL;
  numVarInDataGroup = 0;
  numVarToSend = 0;
  TOSCALE = commandline->ToScale();
  net = netComm;
  #ifdef GADGET_NETWORK
    readInputFile(commandline->InputFilename());
  #else
    readInitVals(commandline->InputFilename());
  #endif
  initiateNetComm(pm, commandline->runCondor());
}

NetInterface::NetInterface(char* initvalsFileName,
  NetCommunication* netComm, ProcessManager* pm) {

  MAXNUMX = 500;
  NUM_TRIES_TO_RECEIVE = 4;
  scaler = NULL;
  dataConvert = NULL;
  dctrl = NULL;
  dataSet = NULL;
  numVarInDataGroup = 0;
  numVarToSend = 0;
  net = netComm;
  readAllInitVals(initvalsFileName);
  initiateNetComm(pm, 0);
}

NetInterface::~NetInterface() {
  stopNetComm();
  stopUsingDataGroup();
  if (TOSCALE == 1) {
    delete scaler;
    scaler = NULL;
  }
  if (dataConvert != NULL) {
    delete dataConvert;
    dataConvert = NULL;
  }
}

#ifdef GADGET_NETWORK
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
      if (data->Optimize(i) == 1) {
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
    if (data->Optimize(i) == 1)
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
      xind[i] = data->Optimize(i);
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
  if (!TOSCALE) {
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
#endif

// ********************************************************
// Functions for reading input values from file
// ********************************************************
void NetInterface::readAllInitVals(char* fileName) {
  int i, j;
  double inputVector[NUMVARS];
  ifstream inputFile;
  int firstLine = 0;

  // try to open file
  inputFile.open(fileName);
  if (!inputFile) {
    cerr << "Error in netinterface - could not open file " << fileName << endl;
    exit(EXIT_FAILURE);
  }

  i = 0;
  inputFile >> inputVector[i];
  while (inputFile) {
    i++;
    inputVector[i] = inputFile.peek();
    if (inputVector[i] == '\n') {
      if (firstLine == 0) {
        numVarInDataGroup = i;
        numVarToSend = i;
        startNewDataGroup();
        firstLine = 1;
      }

      vector tempVector(numVarInDataGroup);
      for (j = 0; j < numVarInDataGroup; j++)
        tempVector[j] = inputVector[j];

      if (dataGroupFull()) {
        cerr << "Error in netinterface - datagroup is full\n";
        exit(EXIT_FAILURE);
      }

      setX(tempVector);
      i = 0;
    }
    inputFile >> inputVector[i];
  }

  if (i >= numVarInDataGroup) {
    cerr << "Error in netinterface - last vector invalid\n";
    exit(EXIT_FAILURE);
  }
  if (numVarInDataGroup <= 0) {
    cerr << "Error in netinterface - no valid data read from " << fileName << endl;
    exit(EXIT_FAILURE);
  }
}

void NetInterface::readInitVals(char* fileName) {
  double x[NUMVARS];         // the vector of parameter values
  double xfullraw[NUMVARS];  // initial values of x, on original scale
  int xind[NUMVARS];
  double lbd[NUMVARS];
  double ubd[NUMVARS];

  numVarInDataGroup = 0;
  numVarToSend = 0;
  double w, l, u;
  FILE *fp;
  int c;

  fp = fopen(fileName, "r");
  if (fp == NULL) {
    cerr << "Error in netinterface - could not open file " << fileName << endl;
    exit(EXIT_FAILURE);
  }

  while (fscanf(fp, "%lf %lf %lf %d", &w, &l, &u, &c) == 4) {
    assert(numVarToSend < NUMVARS);
    xind[numVarToSend] = c;
    xfullraw[numVarToSend] = w;
    numVarToSend++;

    if (c == 1) {
      assert(numVarInDataGroup < NUMVARS);
      x[numVarInDataGroup] = w;
      lbd[numVarInDataGroup] = l;
      ubd[numVarInDataGroup] = u;
      numVarInDataGroup++;
    }
  }

  fclose(fp);
  if (numVarInDataGroup == 0) {
    cerr << "Error in netinterface - no variables to optimise\n";
    exit(EXIT_FAILURE);
  }

  if (numVarInDataGroup != numVarToSend) {
    // need to convert data
    vector tempfull(xfullraw, numVarToSend);
    dataConvert = new DataConverter();
    dataConvert->setInitialData(xind, tempfull);
  }

  vector low(lbd, numVarInDataGroup);
  vector upp(ubd, numVarInDataGroup);
  vector vec(x, numVarInDataGroup);
  if (TOSCALE == 0) {
    upperScale = upp;
    lowerScale = low;
    initialX = vec;
  } else {
    scaler = new DataScaler();
    vector temp(numVarInDataGroup);
    temp.setValue(1.0);
    upperScale = temp;
    temp.setValue(-1.0);
    lowerScale = temp;
    scaler->setInitialData(low, upp);
    initialX = (scaler->scaleX(vec));
  }
}

// ********************************************************
// Function for initiating values before start using class NetInterface
// ********************************************************
void NetInterface::initiateNetComm(ProcessManager* pm, int condor) {
  int netStarted, numProc;
  ISALPHA = -1;
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
    receiveId = -1;
  }

#ifdef GADGET_NETWORK
  if (switches.Size() > 0)
    sendStringValue();
  if (lowerBound.dimension() > 0)
    sendBoundValues();
#endif
}
