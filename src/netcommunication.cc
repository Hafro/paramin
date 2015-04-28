#include "netcommunication.h"
#include <iostream>

NetCommunication::NetCommunication(const CharPtrVector& funcNameArgs, int nh) {
  	// pvmConst contains information about which tags and dataencoding to use
  	pvmConst = new PVMConstants();
  	nHostInn = nh;
  	int i;
  	if (funcNameArgs.Size() <= 0) 
	{
    	cerr << "Must have name of the function to start on slaves\n";
    	exit(EXIT_FAILURE);
  	}
  	slaveProgram = new char[strlen(funcNameArgs[0]) + 1];
  	strcpy(slaveProgram, funcNameArgs[0]);
  	numarg = funcNameArgs.Size() - 1;
  	slaveArguments = new char*[numarg + 1];
  	for (i = 0; i < numarg; i++) 
	{
    	slaveArguments[i] = new char[strlen(funcNameArgs[i + 1]) + 1];
    	strcpy(slaveArguments[i], funcNameArgs[i + 1]);
  	}
  	slaveArguments[numarg] = NULL;
	
	numVar = -1;
	mytid = -1;
	nhost = 0;
	narch = 0;
	numProcesses = 0;
	numGoodProcesses = 0;
	NETSTARTED = 0;
	tids = NULL;
	status = NULL;
	ERROR = -1;
	SUCCESS = 1;
	
	likelihoodHJ = 0.0;
	likelihoodSA = 0.0;
	likelihoodBFGS = 0.0;
	convergedSA = 0;
	convergedHJ = 0;
	convergedBFGS = 0;
	maxNumHosts = 500;
}

NetCommunication::~NetCommunication() 
{
  	int i;
  	if (tids != NULL)
    	delete[] tids;
  	if (status != NULL)
    	delete[] status;
  	if (hostTids != NULL)
    	delete[] hostTids;
  	if (dataIDs != NULL)
    	delete[] dataIDs;
  	delete[] slaveProgram;
  	for (i = 0; i < numarg; i++)
    	delete[] slaveArguments[i];
  	delete[] slaveArguments;
  	if (NETSTARTED == 1)
    	stopNetCommunication();
  	delete pvmConst;
}

// ********************************************************
// Functions for starting and stopping netcommunication
// ********************************************************
int NetCommunication::startPVM() 
{
	/*
		Þetta fall er afgreitt í bili!
		ATH: Þarf e.t.v. að skoða parametrana í MPI_Init.
	*/
	int info;
  	// Held að það sé í lagi að hafa bara NULL hér...
  	MPI_Init(NULL, NULL);
	if (mytid < 0) 
	{
    	MPI_Comm_rank(MPI_COMM_WORLD, &mytid);
    	if (mytid < 0) 
		{
      		printErrorMsg("Error in netcommunication - MPI not started");
      		return ERROR;
    	}
	
		int flag;
		flag = 0;
		// Checks whether MPI_Init has been called successfully.
		MPI_Initialized(&flag);
		if (!flag)
		{
			printErrorMsg("Error in netcommunication - MPI_Init has not been called!");
		}
		// nhost á að vera 1 hérna
		MPI_Comm_size(MPI_COMM_WORLD, &nhost);

    	tids = new int[maxNumHosts];
    	status = new int[maxNumHosts];
    	hostTids = new int[maxNumHosts];  //Added jongud
    	dataIDs = new int[maxNumHosts];   //Added jongud

    	if (nHostInn > 0)
      		nhost = nHostInn;
	}
	return 1;
}

int NetCommunication::startNetCommunication() 
{
	/*
		Þetta fall er afgreitt í bili!
	*/
	int i, OK, info;
  	if (NETSTARTED == 1 && mytid >= 0) 
	{
    	// have alredy enrolled in MPI and spawned program on slaves
    	cerr << "Warning in netcommunication - already enrolled in MPI and running " << slaveProgram << " on slaves\n";
    	return SUCCESS;
	} 
	else 
	{
    	if (numVar <= 0) 
		{
      		cerr << "Error in netcommunication - number of variables must be positive\n";
      		return ERROR;
    	}
    	OK = startPVM();
		int* errcodes = new int[nhost];
		if (OK == 1) 
		{
			/* 
				Í PVM var notað pvm_catchout(stdin) til að fá output úr
				child processunum, í MPI setur maður flag á eftir mpirun
				til að fá sambærilega hegðun, þá búast til skrár fyrir hvert
				process sem skrifa út allt sem kemur úr stdout og stderr í þeim. 
			*/
			MPI_Comm_spawn(slaveProgram, slaveArguments, nhost, MPI_INFO_NULL, 
	          	0, MPI_COMM_WORLD, &intercomm, errcodes);
			int tidsCounter;
			for (i = 0; i < nhost; i++)
			{
				tidsCounter = i;
				if(errcodes[i] == 0)
				{
					numProcesses++;
					numGoodProcesses++;
					status[i] = 1;
					tids[i]=i;
				}
				else
				{
					cerr << "Error in netcommunication - unable to spawn process\n";
					return ERROR;
				}
			}
			delete [] errcodes;
		}

		/*
		Þýðandinn var eitthvað að kvarta hérna...
    	for (i = 0; i < nhost; i++) 
		{
      		
			//hosts to be monitored for deletion, suspension and resumption
      		hostTids[i] = hostp[i].hi_tid;
    	}

		*/
    	if (OK == 1) 
		{
      		// Have started slaveProgram slaveArguments on all nhost hosts.
      		// send initial info to all slave processes
      		OK = startProcesses();
      		if (OK == 1) 
			{
        		NETSTARTED = 1;
        		return SUCCESS;
      		} 
			else if (OK == -1) 
			{
        		return ERROR;
      		} 
			else 
			{
        		printErrorMsg("Error in netcommunication - unrecognised return value");
        		stopNetCommunication();
        		return ERROR;
      		}

    	} 
		else if (OK == 0) 
		{
      		stopNetCommunication();
      		return ERROR;
    	} 
		else 
		{
      		printErrorMsg("Error in netcommunication - unrecognised return value");
      		stopNetCommunication();
      		return ERROR;
    	}
  	}
}

void NetCommunication::stopNetCommunication() 
{
	/*
		Fínt í bili.
	*/
	int i, tid, info, numTasks;
  	int stopparam = -1;
	
  	MPI_Comm_rank(MPI_COMM_WORLD, &tid);
  	if (tid >= 0) 
	{
		for(int i=0; i<nhost; i++)
		{
			MPI_Send(&stopparam, 1, MPI_INT, i, pvmConst->getStopTag(), intercomm);
		}
		MPI_Finalize();
  	}
  	mytid = -1;
  	NETSTARTED = 0;
}

int NetCommunication::startProcesses() 
{
	/*
		Þetta fall er afgreitt í bili!
	*/
  //Send number of variables, group name and number of processes to spawned processes
  	int cansend = 1;
  	int i, info;

  	for (i = 0; i < nhost; i++) 
	{
    	// send initial message to all spawned processes
    	cansend = sendInitialMessage(i);
    	if (cansend == -1) 
		{
      		// Error occured in sending inital message to process with id = i
      		printErrorMsg("Error in netcommunication - unable to send message");
      		return ERROR;
		} 
		else if (cansend == 0) 
		{
      		printErrorMsg("Error in netcommunication - unable to send message");
      		status[i] = -1;
      		return ERROR;
    	} 
		else if (cansend == 1) 
		{
      		status[i] = 1;
    	} 
		else 
		{
      		printErrorMsg("Error in netcommunication - unrecognised return value");
      		stopNetCommunication();
      		return ERROR;
    	}
  	}
  	return SUCCESS;
}

int NetCommunication::sendInitialMessage(int id) 
{
  	int OK, info;
	
  	if (id < 0 || id >= nhost) 
	{
    	printErrorMsg("Error in netcommunication - invalid slave ID");
    	return 0;
  	}

  	// check if process with identity = id is up and running
  	// spurning að sleppa þessu í bili, leyfum þessu að vera á meðan hitt er klárað.
  	OK = checkProcess(id);
  	if (OK == 1) 
	{
		MPI_Comm parentcomm;
		MPI_Comm_get_parent( &parentcomm );
		if(parentcomm == MPI_COMM_NULL)
		{
			MPI_Send(&numVar, 1, MPI_INT, id, pvmConst->getStartTag(), intercomm);
			MPI_Send(&id, 1, MPI_INT, id, pvmConst->getStartTag(), intercomm);
		}
		else
		{
			printErrorMsg("Error in netcommunication - slave calling master send");
	    	stopNetCommunication();
		}
    	return SUCCESS;

  	} 
	else if (OK == -1) 
	{
    	printErrorMsg("Error in netcommunication - unable to check status");
    	stopNetCommunication();
    	return ERROR;
  	} 
	else if (OK == 0) 
	{
    	printErrorMsg("Error in netcommunication - unable to send initial message");
    	return OK;
  	} 
	else 
	{
    	printErrorMsg("Error in netcommunication - unrecognised return value");
    	stopNetCommunication();
    	return ERROR;
  	}
}

int NetCommunication::checkProcess(int id) {
	/*
		Þetta fall er ekki að gera neitt, því það sendir adrei nein
		út með tagginu getTaskDiedTag()
	*/
  	int info, bufID, recvTid, flag;
  	MPI_Status stats, recvstats;
  	assert(id >= 0);
  	assert(id < numProcesses);

  	//bufID = pvm_probe(tids[id], pvmConst->getTaskDiedTag());

  	// Non-blocking probe which checks for a message with this tag, if there is no message then
  	// flag is false, otherwise it is true, then something is maybe wrong with the process!
	
	// ATH: Þetta flag mun líklega alltaf vera false... það tékkar bara strax og heldur svo 
	// áfram, þetta virkar eins og pvm_probe, svo þetta ætti að vera í lagi hér.
	// Held samt að það sé irrelevant að vera með þetta checkprocess núna, það á allt að
	// vera í lagi... 
	bufID = MPI_Iprobe(id, pvmConst->getTaskDiedTag(), intercomm, &flag, &stats);
  	if (flag == true) {
    // message has arrived from tids[id] that has halted
    //info = pvm_recv(tids[id], pvmConst->getTaskDiedTag());

	// Blocking receive-message for bookkeeping of status of the process.
		MPI_Recv(&recvTid, 1, MPI_INT, stats.MPI_SOURCE, pvmConst->getTaskDiedTag(), intercomm, &recvstats);
  	//if (info < 0) {
    //  printErrorMsg("Error in netcommunication - unable to check process");
    //  return ERROR;
    //}

    //info = pvm_upkint(&recvTid, 1, 1);
    //if (info < 0) {
    //  printErrorMsg("Error in netcommunication - unable to check process");
    //  return ERROR;
    //}
	
	
    if (recvTid != tids[id])
      	return ERROR;

    status[id] = -1;
    numGoodProcesses--;
    return 0;

  	} 
  	else 
	{
    	return SUCCESS;
  	}
}

void NetCommunication::checkProcesses() {
	/*
		Þetta fall er komið í bili.
		Þetta fall erl íka í raun óþarfi...
	*/
  int i, info, tidDown, flag;
  MPI_Request req;
  MPI_Status nonb;
  MPI_Irecv(&tidDown,1,MPI_INT,MPI_ANY_SOURCE,pvmConst->getTaskDiedTag(),intercomm,&req);
  MPI_Test(&req, &flag, &nonb);
  while (flag == true) {
    // got message that task is down, receive it
    i = 0;
    while ((tids[i] != tidDown) && (i < numProcesses))
      i++;

    assert((i >= 0) && (i < numProcesses));
    status[i] = -1;
    numGoodProcesses--;
	MPI_Irecv (&tidDown,1,MPI_INT,MPI_ANY_SOURCE,pvmConst->getTaskDiedTag(),intercomm,&req);
	MPI_Test(&req, &flag, &nonb);
  }
}

void NetCommunication::getHealthOfProcesses(int* procTids) {
	/*
		Þetta fall er afgreitt.
		
	*/
  checkProcesses();
  int i;
  for (i = 0; i < numProcesses; i++)
    procTids[i] = status[i];
}

// ********************************************************
// Functions for sending and receiving messages
// ********************************************************
int NetCommunication::sendData(const ParameterVector& sendP) {
	/*
		Komið í bili, þarf samt að skoða MPI_PACK eða eitthvað álíka til að 
		raða inn í buffer og senda strengina, gæti verið að maður þurfi þá að
		pakka int með sem er lengd char fylkisins. Það er samt bara kallað á þetta
		fall einu sinni í byrjun til að senda switches, svo að það ætti að vera í lagi.
		!!! ATH !!!
		Passa að allir sem kalla á þetta sendi communicator !!!
		!!!
	*/
    // must absolutely check if this is possible or can not delete
    // stringValue now.}}}}}}}}}}}}}x
    int i, info;
  char** stringValue;
  if (NETSTARTED == 1) {
      stringValue = new char*[numVar];
      for (i = 0; i < numVar; i++) {
	  	stringValue[i] = new char(strlen(sendP[i].getName())+1);
        strcpy(stringValue[i], sendP[i].getName());
		// This was done with pvm_mcast in the old version, it works similar to this, but it might be
		// broadcasting the data via a tree structure, this should not create too much overhead.
		for(int j = 0; j<nhost; j++)
		{
			MPI_Send(stringValue[i],strlen(stringValue[i]), MPI_BYTE, j, pvmConst->getMasterSendStringTag(), intercomm);
		}
      };
    assert(sendP.Size() >= numVar);
    
	for (i = 0; i < numVar; i++)
	delete [] stringValue[i];
    delete [] stringValue;
    if (info < 0) {
      printErrorMsg("Error in netcommunication - unable to send data");
      stopNetCommunication();
      return ERROR;
    } else
      return SUCCESS;

  } else {
    printErrorMsg("Error in netcommunication - unable to send data");
    return ERROR;
  }
}

int NetCommunication::sendData(const ParameterVector& sendP, int processID) {
	/*
		Búið í bili...
	*/
	int i, info;
  	char** stringValue;
  	if (NETSTARTED == 1) 
	{
      	stringValue = new char*[numVar];
      	for (i = 0; i < numVar; i++) 
		{
	  		stringValue[i] = new char(strlen(sendP[i].getName())+1);
        	strcpy(stringValue[i], sendP[i].getName());
			// This could be causing some overhead, could consider packing it in a buffer
			// before I send it, like the old pvm version, let's see if this works ok.
			// I think this function is only called once.
			MPI_Send(stringValue[i],strlen(stringValue[i]), MPI_BYTE,tids[processID],pvmConst->getMasterSendStringTag(),intercomm);  
      	};
    	assert(sendP.Size() >= numVar);

    	for (i = 0; i < numVar; i++)
			delete [] stringValue[i];
    	delete [] stringValue;
		return SUCCESS;
  	} 
	else 
	{
    	printErrorMsg("Error in netcommunication - unable to send data");
    	return ERROR;
  	}
}

int NetCommunication::sendBoundData(const DoubleVector& sendP) 
{
	/*
		Komið í bili...
	*/
  	int i, info;
  	double* temp;

  	if (NETSTARTED == 1) 
	{
    	temp = new double[numVar];
    	for (i = 0; i < numVar; i++)
      		temp[i] = sendP[i];
		for(int j = 0; j< nhost; j++)
		{
			// This was originally done with pvm_mcast, question if this is causing overhead.
			MPI_Send(temp, numVar , MPI_DOUBLE,j,pvmConst->getMasterSendBoundTag(),intercomm);
		}
    	delete[] temp;
		return SUCCESS;
  	} 
	else 
	{
    	printErrorMsg("Error in netcommunication - unable to send data");
    	return ERROR;
  	}
}

int NetCommunication::sendBoundData(const DoubleVector& sendP, int processID) 
{
	/*
		Komið í bili!
	*/
  	int i, info;
  	double* temp;

  	if (NETSTARTED == 1) 
	{
    	temp = new double[numVar];
    	for (i = 0; i < numVar; i++)
      		temp[i] = sendP[i];

		MPI_Send(temp,numVar, MPI_DOUBLE,tids[processID],pvmConst->getMasterSendBoundTag(),intercomm); 
    	delete[] temp;
    	return SUCCESS;
  	} 
	else 
	{
    	printErrorMsg("Error in netcommunication - unable to send data");
    	return ERROR;
  	}
}

int NetCommunication::sendData(NetDataVariables* sendP, int processID) 
{
	/*
		Komið í bili!
	*/
  	int info;
  	int cansend = 1;
  	assert(processID >= 0);
  	assert(processID < numProcesses);

  	if (NETSTARTED == 1) 
	{
    	// check is process with id = processID is up and running
    	cansend = checkProcess(processID);
    	if (cansend == -1) 
		{
      		printErrorMsg("Error in netcommunication - invalid process ID");
      		stopNetCommunication();
      		return ERROR;

    	} 
		else if (cansend == 0) 
		{
      		//process with id = processID is not up and running
      		return cansend;

    	} 
		else if (cansend == 1) 
		{
			MPI_Send(&sendP->tag,1,MPI_INT, tids[processID],pvmConst->getMasterSendVarTag(),intercomm);
			MPI_Send(&sendP->x_id,1,MPI_INT, tids[processID],pvmConst->getMasterSendVarTag(),intercomm);
			MPI_Send(sendP->x,numVar,MPI_DOUBLE, tids[processID],pvmConst->getMasterSendVarTag(),intercomm);
      		return SUCCESS;
		} 
		else 
		{
      		printErrorMsg("Error in netcommunication - unable to send data");
      		stopNetCommunication();
      		return ERROR;
    	}

 	} 
	else 
	{
    	printErrorMsg("Error in netcommunication - unable to send data");
    	return ERROR;
  	}
}

int NetCommunication::receiveData(NetDataResult* rp) 
{
	/*
		Komið í bili...
		Þarf að passa að kasta villu ef einhver af þessum nær ekki að receive-a, nota kannski
		MPI_Probe...
	*/
  	int info;
	MPI_Status status, status2;

  	if (NETSTARTED == 1) 
	{
		MPI_Recv(&rp->tag, 1, MPI_INT, MPI_ANY_SOURCE, pvmConst->getMasterReceiveDataTag(), intercomm, &status);
		MPI_Recv(&rp->result, 1, MPI_DOUBLE, status.MPI_SOURCE, pvmConst->getMasterReceiveDataTag(), intercomm, &status2);
		MPI_Recv(&rp->who, 1, MPI_INT, status.MPI_SOURCE, pvmConst->getMasterReceiveDataTag(), intercomm, &status2);
		MPI_Recv(&rp->x_id, 1, MPI_INT, status.MPI_SOURCE, pvmConst->getMasterReceiveDataTag(), intercomm, &status2);
		cout << "Skrifa result úr netcomm: " << rp->result << "\n";
    	return SUCCESS;
  	} 
	else 
	{
    	printErrorMsg("Error in netcommunication - unable to receive data");
    	return ERROR;
  	}
}

// ********************************************************
// Functions which set/return information about netcommunication
// ********************************************************
int NetCommunication::getNumHosts() 
{
  	return nhost;
}

int NetCommunication::getNumProcesses() 
{
  	return numProcesses;
}

int NetCommunication::getNumVar() 
{
  	return numVar;
}

int NetCommunication::getNumRunningProcesses() 
{
  	return numGoodProcesses;
}

int NetCommunication::netCommStarted() 
{
  	return NETSTARTED;
}

void NetCommunication::setNumInSendVar(int nVar) 
{
  	if (nVar <= 0) 
	{
    	cerr << "Error in netcommunication - number of variables must be positive\n";
    	exit(EXIT_FAILURE);
  	}
  	numVar = nVar;
}

void NetCommunication::printErrorMsg(const char* errorMsg) 
{
	/*
		Eina fallið sem ég virðist þurfa að eiga eitthvað við hér...
	*/
  	char* msg;
  	msg = new char[strlen(errorMsg) + 1];
  	strcpy(msg, errorMsg);
	// Ákvað að gera þetta svona, vona að þetta flood-i ekki command line...
	cout << msg << "\n";
  	delete[] msg;
  	cerr << errorMsg << endl;
}

int NetCommunication::netError() {
  return ERROR;
}

int NetCommunication::netSuccess() {
  return SUCCESS;
}

MasterCommunication::MasterCommunication(CommandLineInfo* info)
  : NetCommunication(info->getFunction(), info->getNumProc()) 
{
  	int wait = info->getWaitMaster();
  	tmout = new timeval;
  	if (wait == -1)
    	tmout = NULL;
  	else if (wait >= 0) 
	{
    	tmout->tv_sec = wait;
    	tmout->tv_usec = 0;
  	} 
	else 
	{
    	cerr << "Error in netcommunication - invalid value for wait " << wait << "\n";
    	exit(EXIT_FAILURE);
  	}
}

MasterCommunication::~MasterCommunication() 
{
  	delete tmout;
}

int MasterCommunication::receiveData(NetDataResult* rp) 
{
	/*
		Komið í bili...
		Þarf að passa að kasta villu ef einhver af þessum nær ekki að receive-a, nota kannski
		MPI_Probe, þetta var gert með Timeout receive í gömlu útgáfunni...
	*/
  	int info;
	MPI_Status status, status2;

  	if (NETSTARTED == 1) 
	{
		MPI_Recv(&rp->tag, 1, MPI_INT, MPI_ANY_SOURCE, pvmConst->getMasterReceiveDataTag(), intercomm, &status);
		MPI_Recv(&rp->result, 1, MPI_DOUBLE, status.MPI_SOURCE, pvmConst->getMasterReceiveDataTag(), intercomm, &status2);
		MPI_Recv(&rp->who, 1, MPI_INT, status.MPI_SOURCE, pvmConst->getMasterReceiveDataTag(), intercomm, &status2);
		MPI_Recv(&rp->x_id, 1, MPI_INT, status.MPI_SOURCE, pvmConst->getMasterReceiveDataTag(), intercomm, &status2);
    	return SUCCESS;
  	} 
	else 
	{
    	printErrorMsg("Error in netcommunication - unable to receive data");
    	return ERROR;
  	}
}