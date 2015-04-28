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
  DoubleVector initialX;
  // The function value of initialX
  double initialScore;
  
  int receiveID;
  int numVarInDataGroup;
  int numVarToSend;
  DoubleVector h;
  DoubleVector alphaX;
  DoubleVector upperScale;
  DoubleVector lowerScale;
  // DoubleVector opt;
  DoubleVector alphaX_h;
  IntVector optVec;
  // DoubleVector xUnscale;  
  // DoubleVector xConvert;  
  // DoubleVector xToSend;  
  int isAlpha;
  int toscale;
  ParameterVector switches;
  DoubleVector upperBound;
  DoubleVector lowerBound;
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
  void setRepeatedValues(InitialInputFile* data);
  void setOptInfo(InitialInputFile* data);

  // ******************************************************
  // Function for initiating values before start using class NetInterface
  // ******************************************************
  void initiateNetComm(ProcessManager* pm);
  // ******************************************************
  // Functions for starting/stopping a new data group
  // ******************************************************
  void startNewDataGroup();
  void startNewDataGroup(int numInGroup);
  void startNewDataGroup(const DoubleVector& x1, const DoubleVector& h1);
  void startNewDataGroup(int numInGroup, const DoubleVector& x1, const DoubleVector& h1);
  void stopUsingDataGroup();
  // ******************************************************
  // Functions for setting/getting netdata
  // ******************************************************
  void setX(const DoubleVector& x1);
  void setXFirstToSend(const DoubleVector& x1);
  void setDataPair(const DoubleVector& x1, double fx);
  const DoubleVector& getX(int id);
  double getY(int id);
  const DoubleVector& getNextAnswerX();
  double getNextAnswerY();
  /* returns x as used by the optimization algorithm. That is only the optimizations parameters are included and x is scaled if scaling is usred
   */
  const DoubleVector& getInitialX();
  /* x contains the full value of the initialX. All parameters, including those that are not optimized if any. And X is unscaled if scaling is used. fx contains the function value of x.
   */
  void setScore(double fx);
  double getScore();
  double getInitialScore(DoubleVector& x);
  //void getInitialScore(DoubleVector& x, double fx);
  /* x must be of the same size as used by the optimizations algorithm.
   */
  void setInitialScore(const DoubleVector& x, double fx);
  const DoubleVector& getUpperScaleConstant();
  const DoubleVector& getLowerScaleConstant();
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
  // Functions for data converting and data scaling Vectors
  // ******************************************************
  const DoubleVector& makeVector(const DoubleVector& vec);
  const DoubleVector& unscaleX(const DoubleVector& vec);
  const DoubleVector& convertX(const DoubleVector& vec);
  const DoubleVector& prepareVectorToSend(const DoubleVector& vec);
  const DoubleVector& getLowerbound();
  const DoubleVector& getUpperbound();
  const IntVector& getOptInfo();
  const ParameterVector& getSwitches();
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
};

#endif
