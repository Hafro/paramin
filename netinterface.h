#ifndef netinterface_h
#define netinterface_h

#include "dataconverter.h"
#include "datascaler.h"
#include "processmanager.h"
#include "netdatacontrol.h"
#include "condition.h"
#include "paramin.h"
#include "initialinputfile.h"

class Condition;

/** 
 * \class NetInterface
 * \brief NetInterface is a class which manages netcommunication. NetInterface uses the class NetCommunication to send/receive data. The class ProcessManager isused to dispatch processes to be used by NetInterface. It uses the class NetDataControl to store data and keep track of information about status of data concerning netcommunication. It can use the class DataConverter to prepare data beforesending and the class DataScaler to scale/unscale data after/before sending. 
*/

class NetInterface {
private:
  int maxNumX;
  int numTries;
  /**
   * \brief dctrl stores data for current datagroup
   */
  NetDataControl* dctrl;      
  /**
   * \brief pManager keeps track of processes
   */
  ProcessManager* pManager;   
  /**
   * \brief net manages all netcommunication.
   */
  NetCommunication* net;      
  /**
   * \brief dataConvert prepares data for sending
   */
  DataConverter* dataConvert; 
  /**
   * \brief scales/unscaled data after/before sending/receiving
   */
  DataScaler* scaler;         
  /**
   * \brief dataSet keeps track of unsent data which has been set
   */
  Queue* dataSet;             
  /**
   * \brief number of tags that have been used.
   */
  int numberOfTags;           
  /**
   * \brief initial starting point for sending data
   */
  vector initialX;            
  int receiveID;
  int numVarInDataGroup;
  int numVarToSend;
  vector h;
  vector alphaX;
  vector upperScale;
  vector lowerScale;
  vector opt;
  vector tmp;  
  vector xUnscale;  
  vector xConvert;  
  vector xToSend;  
  int isAlpha;
  int toscale;
  VectorOfCharPtr switches;
  vector upperBound;
  vector lowerBound;
public:
  NetInterface(NetCommunication* netComm, ProcessManager* pm, CommandLineInfo* commandline);
  ~NetInterface();
  // ******************************************************
  // Functions for reading input values from file
  // ******************************************************
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

  // ******************************************************
  // Function for initiating values before start using class NetInterface
  // ******************************************************
  void initiateNetComm(ProcessManager* pm, int condor);
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
  int getReceiveID();
  void sentDataItem(int id);
  int getNumNotAns();
  int getNumDataItemsSet();
  int getNumDataItemsAnswered();
  void setFirstAnsweredData();
  int getNumVarsInDataGroup();
  int getNumVarsInSendData();
  int dataGroupFull();
  int allSent();
  // ******************************************************
  // Functions for data converting and data scaling vectors
  // ******************************************************
  const vector& makeVector(const vector& vec);
  const vector& unscaleX(const vector& vec);
  const vector& convertX(const vector& vec);
  const vector& prepareVectorToSend(const vector& vec);
  const vector& getLowerbound();
  const vector& getUpperbound();
  const vector& getOptInfo();
  VectorOfCharPtr getSwitches();
  // ******************************************************
  // Input and output functions for data
  // ******************************************************
  void printX(int id);
  void printXUnscaled(int id);
  void printResult(char* outputFileName);
  // ******************************************************
  // Functions for sending/receiving
  // ******************************************************
  int sendOne(int processID, int x_id);
  void sendOne();
  int resend();
  int receiveOne();
  int sendAll();
  int sendAllOnCondition(Condition* con);
  int sendToIdleHosts();
  int sendToAllIdleHosts();
  int receiveAndSend();
  int receiveAll();
  int receiveOnCondition(Condition* con);
  int sendAndReceiveAllData();
  int sendAndReceiveTillCondition(Condition* con);
  int sendAndReceiveSetData(Condition* con);

  //Added for condor
  int probeForReceiveOne();
  int receiveOneNonBlocking();
  int sendToIdleHostIfCan();
  int sendOneAndDataid(int processID, int x_id);
  int checkHostForSuspend();
  int checkHostForDelete();
  int checkHostForResume();
  void checkHealthOfProcesses();
  int sendAllCondor();
  int receiveAndSendCondor();
  int receiveAllCondor();
  int sendDataCondor();
  int sendAndReceiveAllDataCondor();
  int sendAndReceiveSetDataCondor(Condition* con);
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
  void sendStringValue();
  void sendStringValue(int processID);
  void sendBoundValues();
  void sendBoundValues(int processID);

  //Added for condor
  int netNoneToReceive();
  int netNeedMoreData();
  int netNeedMoreHosts();
  int netDataNotSent();
  int netWaitForBetterProcesses();
};

#endif
