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
    int i;
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
  /*
    if (switches.Size() > 0) {
      for (i = 0; i < switches.Size(); i++)
	  delete [] switches[i];
 }
  */
}

// ********************************************************
// Functions for reading input values from file
// ********************************************************
void NetInterface::readInputFile(char* initvalsFileName) {
    // must be careful about setX. that added to datagroup!!!
    // if needed....
  InitialInputFile* readInput = new InitialInputFile(initvalsFileName);
  readInput->readFromFile();
  
  if (readInput->isRepeatedValues()) {
      this->setRepeatedValues(readInput);
    
  } else {
      this->setOptInfo(readInput);
  };
  // need to check if better do diff or just keep this here and for sure
  // use numVarInDataGroup
  alphaX_h.resize(numVarInDataGroup, 0.0);
   delete readInput;
}

void NetInterface::setRepeatedValues(InitialInputFile* data) {
  // initvalsFile contains only vector values, no optimization info
  DoubleVector xValue;
  data->getSwitches(switches);
  data->getValues(xValue);
  numVarToSend = xValue.Size();
  if (numVarToSend <= 0)  {
    cerr << "Error in netinterface - could not read vectors from file\n";
    exit(EXIT_FAILURE);
  }
  numVarInDataGroup = numVarToSend;
  startNewDataGroup();
  setX(xValue);
  while (data->isDataLeft()) {
    data->readNextLine();
    data->getValues(xValue);
    setX(xValue);
  }
}
void NetInterface::setOptInfo(InitialInputFile* data) {
    int i;
    DoubleVector xValue;
    IntVector optValue;
    data->getVectors(switches, xValue, lowerBound, upperBound, optValue);
    numVarToSend = xValue.Size();
    if (numVarToSend <= 0)  {
	cerr << "Error in netinterface - could not read vectors from file\n";
	exit(EXIT_FAILURE);
    }
    if (optValue.Size() == 0 || numVarToSend == optValue.Size())  {
	// no opt info or all parameters used for optimizing
	initialX = xValue;
	lowerScale = lowerBound;
	upperScale = upperBound;
	numVarInDataGroup = numVarToSend;
    }
    else {
	numVarInDataGroup = 0;
	for (i = 0; i < numVarToSend; i++) {
	if (optValue[i] == 1) {
	    initialX.resize(1,xValue[numVarInDataGroup]);
	    lowerScale.resize(1, lowerBound[numVarInDataGroup]);
	    upperScale.resize(1, upperBound[numVarInDataGroup]);
	    numVarInDataGroup++;
	};
	dataConvert = new DataConverter();
	dataConvert->setInitialData(optValue, xValue);
    }
    // have set opt info if any
    }
    if (toscale == 1) {
	 scaler = new DataScaler();
	 scaler->setInitialData(lowerScale, upperScale);
	 lowerScale.Reset();
	 upperScale.Reset();
	 lowerScale.resize(numVarInDataGroup, -1.0);
	 upperScale.resize(numVarInDataGroup, 1.0);
	 // maybe need to do diff!!!
	 initialX = scaler->scaleX(initialX);    
  };
    
    
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
  if (lowerBound.Size() > 0)
    sendBoundValues();
}
