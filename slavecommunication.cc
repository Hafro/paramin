#include "slavecommunication.h"
#include "gadget.h"

//! Constructor for SlaveCommunication
    /*!
      \Some clean-up may be needed here, since there is no
	  \timeout receive for MPI.
      \param c1 the first argument.
      \param c2 the second argument.
    */
SlaveCommunication::SlaveCommunication() 
{
	MAXWAIT = 30;
  	pvmConst = new PVMConstants();
  	typeReceived = -1;
  	parenttid = -1;
  	mytid = -1;
  	myID = -1;
  	numVar = 0;
  	netDataVar = NULL;
  	tmout.tv_sec = MAXWAIT;
  	tmout.tv_usec = 0;
}

//! Destructor for SlaveCommunication
SlaveCommunication::~SlaveCommunication() 
{
  	delete pvmConst;
  	if (netDataVar != NULL) 
	{
    	delete netDataVar;
    	netDataVar = NULL;
  	}
}

//! Outputs the error message.
void SlaveCommunication::printErrorMsg(const char* errorMsg) 
{
  	char* msg;
  	msg = new char[strlen(errorMsg) + 1];
  	strcpy(msg, errorMsg);
  	cerr << msg << "\n";
  	delete[] msg;
}

//! Initialize netcommunication
    /*!
      \Enrolls in MPI, probes and receives beginning message.
    */
int SlaveCommunication::startNetCommunication() 
{
	/*
		Þetta fall er komið í bili!
	*/
  	int info, bytes, type, source;
  	int OK = 1;
  	int bufID = 0;

  	//enroll in pvm and get identity of process for myself
	MPI_Init(NULL,NULL);
	MPI_Comm parentcomm;
	MPI_Status status;
	
	MPI_Comm_get_parent(&parentcomm);
  	if (parentcomm == MPI_COMM_NULL) 
	{
    	printErrorMsg("Error in slavecommunication - process has not been spawned");
    	return 0;
  	}
	int flag;
	MPI_Iprobe(0, MPI_ANY_TAG, parentcomm, &flag,&status);
	if(flag == true)
	{
		if (status.MPI_TAG == pvmConst->getStopTag()) 
		{
	    	int stopMessage;
			MPI_Recv(&stopMessage, 1, MPI_INT, 0, MPI_ANY_TAG, parentcomm, &status);
	    	typeReceived = pvmConst->getStopTag();
	    	return !OK;
	    } 
		else if (status.MPI_TAG == pvmConst->getStartTag()) 
		{
			MPI_Recv(&numVar, 1, MPI_INT, 0, MPI_ANY_TAG, parentcomm, &status);
	    	if (numVar <= 0) 
			{
	    		cerr << "Error in slavecommunication - number of variables received from master is less than zero\n";
	    		return !OK;
	    	}
			MPI_Recv(&myID, 1, MPI_INT, 0, MPI_ANY_TAG, parentcomm, &status);
	    	if (myID < 0) 
			{
	    		cerr << "Error in slavecommunication - received invalid id of " << myID << endl;
	    		return !OK;
	    	}
	    	netDataVar = new NetDataVariables(numVar);
	    	typeReceived = pvmConst->getStartTag();
	    	return numVar;
	    }
		else
		{
			cerr << "Error in slavecommunication - received unrecognised tag of type " << status.MPI_TAG << endl;
	    	return !OK;
		}
	}
	else
	{
		cerr << "Error in slavecommunication - non-blocking-probe fail" << endl;
    	return !OK;
	}
}

//! Stops MPI.
void SlaveCommunication::stopNetCommunication() 
{
	MPI_Finalize();
}

//! Sends results to Master, paramin.
int SlaveCommunication::sendToMaster(double res) 
{
  	int info;
  	assert(netDataVar != NULL);
  	if (netDataVar->x_id < 0 || netDataVar->tag < 0) 
	{
    	printErrorMsg("Error in slavecommunication - invalid id received\n");
    	return 0;
  	} 
	else if (myID < 0) 
	{
    	printErrorMsg("Error in slavecommunication - invalid id received\n");
    	return 0;
  	} 
	else 
	{
    	NetDataResult* sendData = new NetDataResult;
    	sendData->who = myID;
    	sendData->result = res;
    	sendData->x_id = netDataVar->x_id;
    	sendData->tag = netDataVar->tag;
    	info = send(sendData);
    	delete sendData;
    	if (info > 0) 
		{
      		netDataVar->tag = -1;
      		netDataVar->x_id = -1;
      		return 1;
    	} 
		else
      		return 0;
  	}
  	return 0;
}
//! This could be simplified by making a new MPI struct and use only one send
//! and a probe on the Master's behalf.
int SlaveCommunication::send(NetDataResult* sendData) 
{
  	int info;
	MPI_Comm parentcomm;
	MPI_Comm_get_parent(&parentcomm);
	MPI_Send(&sendData->tag,1,MPI_INT, 0, pvmConst->getMasterReceiveDataTag(),parentcomm);
	MPI_Send(&sendData->result,1,MPI_DOUBLE, 0, pvmConst->getMasterReceiveDataTag(),parentcomm);
	MPI_Send(&sendData->who,1,MPI_INT, 0, pvmConst->getMasterReceiveDataTag(),parentcomm);
	MPI_Send(&sendData->x_id,1,MPI_INT, 0, pvmConst->getMasterReceiveDataTag(),parentcomm);
  	return 1;
}

//! Reiceive based on messagetag which was probed for.
int SlaveCommunication::receiveFromMaster() 
{
  	int bufID = 0;
  	int OK = 1;
  	int info, bytes, source, type;
  	typeReceived = -1;
	MPI_Status status;
	MPI_Comm parentcomm;
	MPI_Comm_get_parent(&parentcomm);
    
	// This was implemented with a timeout receive in PVM, since there is no 
	// Timeout receive in MPI we use a blocking Probe.
    MPI_Probe(0, MPI_ANY_TAG, parentcomm, &status);
    if (status.MPI_TAG == pvmConst->getStopTag()) 
	{
    	//receive information from master to quit
		int stopMessage;
		MPI_Recv(&stopMessage, 1, MPI_INT, 0, MPI_ANY_TAG, parentcomm, &status);
    	typeReceived = pvmConst->getStopTag();
    	return !OK;
    } 
	else if (status.MPI_TAG == pvmConst->getMasterSendStringTag()) 
	{
    	// There is an incoming message of data type stringDataVariables
    	info = receiveString();
    	typeReceived = pvmConst->getMasterSendStringTag();
    	if (info > 0)
    		return 1;
    	return 0;
    } 
	else if (status.MPI_TAG == pvmConst->getMasterSendBoundTag()) 
	{
    	// There is an incoming message of data type double for the bounds
    	info = receiveBound();
    	typeReceived = pvmConst->getMasterSendBoundTag();
    	if (info > 0)
    		return 1;
    	return 0;
    } 
	else if (status.MPI_TAG == pvmConst->getMasterSendVarTag()) 
	{
    	//There is an incoming message of data type NetDataVariables
    	info = receive();
    	if (info > 0) 
		{
    		typeReceived = pvmConst->getMasterSendVarTag();
    		return 1;
    	}
    	return 0;
    } 
	else 
	{
    	cerr << "Error in slavecommunication - received unrecognised tag of type " << type << endl;
    	return !OK;
    }
  	
}

int SlaveCommunication::receiveBound() 
{
 	int i;
    double* temp = new double[numVar];
	MPI_Status status;
	MPI_Comm parentcomm;
	MPI_Comm_get_parent(&parentcomm);
	MPI_Recv(temp,numVar,MPI_DOUBLE,0,MPI_ANY_TAG,parentcomm,&status);
  	netDataDouble.Reset();
  	netDataDouble.resize(numVar, 0.0);
  	for (i = 0; i < numVar; i++) 
	{
      	netDataDouble[i] = temp[i];
  	}
  	delete [] temp;
  	return 1;
}

int SlaveCommunication::receive() 
{
  	int info, i;
	MPI_Status status;
	MPI_Comm parentcomm;
	MPI_Comm_get_parent(&parentcomm);
	MPI_Recv(&netDataVar->tag,1,MPI_INT, 0,MPI_ANY_TAG, parentcomm,&status);
	MPI_Recv(&netDataVar->x_id, 1, MPI_INT, 0,MPI_ANY_TAG, parentcomm, &status);
	MPI_Recv(netDataVar->x, numVar,MPI_DOUBLE,0,MPI_ANY_TAG, parentcomm, &status);
  	return 1;
}

int SlaveCommunication::receivedVector() 
{
  	if (pvmConst->getMasterSendVarTag() == typeReceived)
    	return 1;
  	return 0;
}

void SlaveCommunication::getVector(DoubleVector& vec) 
{
	/*
		Senda út villu ef þetta if condition er ekki uppfyllt... G.E.
	*/
  	int i;
  	if (vec.Size() != numVar) 
	{
      // error....
  	}
 
  	for (i = 0; i < numVar; i++) 
	  	vec[i] = netDataVar->x[i];
}

int SlaveCommunication::getReceiveType() 
{
  	return typeReceived;
}

int SlaveCommunication::receiveString() 
{
  	int OK = 1;
  	int i, info;
  	char* tempString = new char[MaxStrLength + 1];
  	strncpy(tempString, "", MaxStrLength);
  	tempString[MaxStrLength] = '\0';
	MPI_Status status;
	MPI_Comm parentcomm;
	MPI_Comm_get_parent(&parentcomm);
	
  	for (i = 0; i < numVar; i++) 
	{
		MPI_Recv(tempString, MaxStrLength, MPI_BYTE, 0,MPI_ANY_TAG, parentcomm, &status);
    	Parameter pm(tempString);
    	netDataStr.resize(pm);
  	}
  	delete [] tempString;
  	return OK;
}

int SlaveCommunication::receivedString() 
{
  	if (pvmConst->getMasterSendStringTag() == typeReceived)
    	return 1;
  	return 0;
}

int SlaveCommunication::receivedBounds() 
{
  	if (pvmConst->getMasterSendBoundTag() == typeReceived)
    	return 1;
  	return 0;
}

const ParameterVector& SlaveCommunication::getStringVector() 
{
  	return netDataStr;
}

const Parameter& SlaveCommunication::getString(int num) 
{
  	assert(num >= 0);
  	assert(netDataStr.Size() == numVar);
  	return netDataStr[num];
}

void SlaveCommunication::getBound(DoubleVector& vec) 
{
  	int i;
  	if (vec.Size() != numVar)
      // errror...
  /*
  for (i = 0; i < numVar; i++)
    vec[i] = netDataDouble[i];
  */
  vec = netDataDouble;
}