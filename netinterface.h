#ifndef netinterface_h
#define netinterface_h

#ifdef GADGET_NETWORK
#include "initialinputfile.h"
#endif

#include "dataconverter.h"
#include "datascaler.h"
#include "processmanager.h"
#include "netdatacontrol.h"
#include "condition.h"
#include "paramin.h"

class condition;

/* NetInterface is a class which manages netcommunication.
 * NetInterface uses the class NetCommunication to send/receive data.
 * The class ProcessManager is used to dispatch processes to be used
 * by NetInterface. It uses the class NetDataControl to store data
 * and keep track of information about status of data concerning
 * netcommunication. It can use the class DataConverter to prepare
 * data before sending and the class DataScaler to scale/unscale
 * data after/before sending. */

class NetInterface {
private:
  int MAXNUMX;
  int NUM_TRIES_TO_RECEIVE;
  NetDataControl* dctrl;      // dctrl stores data for current datagroup
  ProcessManager* pManager;   // pManager keeps track of processes
  NetCommunication* net;      // net manages all netcommunication.
  DataConverter* dataConvert; // dataConvert prepares data for sending
  DataScaler* scaler;         // scales/unscaled data after/before sending/receiving
  queue* dataSet;             // dataSet keeps track of unsent data which has been set
  int numberOfTags;           // number of tags that have been used.
  vector initialX;            // initial starting point for sending data
  int receiveId;
  int numVarInDataGroup;
  int numVarToSend;
  vector h;
  vector alphaX;
  vector upperScale;
  vector lowerScale;
  int ISALPHA;
  int TOSCALE;
  #ifdef GADGET_NETWORK
    VectorOfCharPtr switches;
    vector upperBound;
    vector lowerBound;
  #endif
public:
  NetInterface(char* initValsFilename, NetCommunication* netComm, ProcessManager* pm, int to_scale);
  NetInterface(char* initValsFileName, NetCommunication* netComm, ProcessManager* pm);
  ~NetInterface();
  // ******************************************************
  // Functions for reading input values from file
  // ******************************************************
  #ifdef GADGET_NETWORK
   /* initvals can be opened and initvals has one of those three formats:
    * 1.   Only vector value/s
    *      x11 x12 x13 .. x1n
    *      ..
    *      xm1 xm2 xm3 .. xmn
    *      where n is number of variables in vector and m is >= 1.
    * 2.   switches
    *      xsw1 sw2 ... swk (where sw[1..k-1] are valid switch values.
    *      and not two switches have the same value.
    *      Vector values as above in 1 and k is equal to n.
    * 3.   switches (optional) value optimize (optional) lowerbound upperbound.
    *      sw1 (optional) x1 1/0 (optional but must have value 1 or 0) low1 upp1
    *      ...
    *      swn x1n optn lown uppn
    *      where n is number of variables in vector. If switches are
    *      given then no two switches can be equal.
    *
    * if initvals is of type 1 or 2 then can add all vector values
    * to one datagroup. If initvals is of type 3 and optimize given then not
    * all variables can be set to 0. */
    void readInputFile(char* initvalsFileName);
    void setSwitches(InitialInputFile* data);
    void setVector(InitialInputFile* data);
    void setNumVars(InitialInputFile* data);
    void setOptInfo(InitialInputFile* data);
  #endif

  void readAllInitVals(char* fileName);
  void readInitVals(char* fileName);
  // ******************************************************
  // Function for initiating values before start using class NetInterface
  // ******************************************************
  void initiateNetComm(ProcessManager* pm);
  // ******************************************************
  // Functions for starting/stopping a new data group
  // ******************************************************
  void startNewDataGroup();
  void startNewDataGroup(int numInGroup);
  void startNewDataGroup(const vector& x1, const vector& h1);
  void startNewDataGroup(int numInGroup, const vector& x1, const vector& h1);
  void stopUsingDataGroup();
  // ******************************************************
  // Functions for setting/getting netdata
  // ******************************************************
  void setX(const vector& x1);
  void setXFirstToSend(const vector& x1);
  void setDataPair(const vector& x1, double fx);
  vector getX(int id);
  double getY(int id);
  vector getNextAnswerX();
  double getNextAnswerY();
  vector getInitialX();
  void setBestX(const vector& x);
  vector getUpperScaleConstant();
  vector getLowerScaleConstant();
  // ******************************************************
  // Functions for getting/setting information about datagroup
  // ******************************************************
  int getReceiveId();
  void sentDataItem(int id);
  int getNumNotAns();
  int getNumDataItemsSet();
  int getNumDataItemsAnswered();
  void setFirstAnsweredData();
  int getNumVarsInDataGroup();
  int getNumVarsInSendData();
  int dataGroupFull();
  // ******************************************************
  // Functions for data converting and data scaling vectors
  // ******************************************************
  vector makeVector(const vector& vec);
  vector unscaleX(const vector& vec);
  vector convertX(const vector& vec);
  vector prepareVectorToSend(const vector& vec);
  vector getLowerbound();
  vector getUpperbound();
  vector getOptInfo();
  #ifdef GADGET_NETWORK
    VectorOfCharPtr getSwitches();
  #endif
  // ******************************************************
  // Input and output functions for data
  // ******************************************************
  void printX(int id);
  void printXUnscaled(int id);
  void printResult(char* outputFileName);
  // ******************************************************
  // Functions for sending/receiving
  // ******************************************************
  int sendOne(int processId, int x_id);
  void sendOne();
  int resend();
  int receiveOne();
  int sendAll();
  int sendAllOnCondition(condition* con);
  int sendToIdleHosts();
  int sendToAllIdleHosts();
  int receiveAndSend();
  int receiveAll();
  int receiveOnCondition(condition* con);
  int sendAndReceiveAllData();
  int sendAndReceiveTillCondition(condition* con);
  int sendAndReceiveSetData(condition* con);

  //Added for condor
  int probeForReceiveOne();
  int receiveOneNonBlocking();
  int sendToIdleHostIfCan();
  int sendOneAndDataid(int processId, int x_id);
  int checkHostForSuspend();
  int checkHostForDelete();
  int checkHostForResume();
  void checkHealthOfProcesses();
  int sendAllCondor();
  int receiveAndSendCondor();
  int receiveAllCondor();
  int sendDataCondor();
  int sendAndReceiveAllDataCondor();
  int sendAndReceiveSetDataCondor(condition* con);
  int sendToAllIdleHostsCondor();

  // ******************************************************
  // Functions concerning netcommunication
  // ******************************************************
  int startNetComm();
  void stopNetComm();
  int getNumFreeProcesses();
  int getTotalNumProc();
  int getNextMsgTag();
  int getNumTags();
  int isUpAndRunning();
  int netError();
  int netSuccess();
  int noAvailableProcesses();
  int waitForBetterProcesses();
  #ifdef GADGET_NETWORK
    void sendStringValue();
    void sendStringValue(int processId);
    void sendBoundValues();
    void sendBoundValues(int processId);
  #endif

  //Added for condor
  int netNoneToReceive();
  int netNeedMoreData();
  int netNeedMoreHosts();
  int netDataNotSent();
  int netWaitForBetterProcesses();
  bool isFreeProcess();
};

#endif
