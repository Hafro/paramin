#ifndef netdatacontrol_h
#define netdatacontrol_h

#include "paramin.h"
#include "vector.h"

/* The class NetInfo keeps together information concerning status of
 * data that is being sent/received during netcommunication. It
 * contains functions to get information about status of data. */

class NetInfo {
public:
  int set;
  int sent;
  int received;
  int numPendingAnswers;

  NetInfo();
  ~NetInfo();
  int hasSet();
  int hasReceived();
  int hasSent();
  int numCopiesSent();
};

/* The class coordinates stores the datapair (x, y) where
 * x is a vector of p paramaters. It contains functions to
 * set and access (x, y) and parameter p in x. */

class coordinates {
private:
  vector x;
  double y;
public:
  coordinates();
  ~coordinates();
  vector getX();
  double getParameter(int num);
  double getY();
  void setX(const vector& v1);
  void setParameter(double p, int num);
  void setY(double y1);
  int getNumParameters();
};

/* The class NetDataControl stores a set of datapairs (x[0..p-1], y)
 * which can be identified with a specific tag and each pair has a
 * unique identity. x[0..p-1] is thought of being sent to a process
 * and y is received from a process using net communication.  NetDataControl
 * provides functions to initialize and access each datapair and get
 * information about the status of net communication for the set of
 * datapairs as well as each pair.  NetDataControl uses the class
 * coordinates to store each datapair and the class NetInfo to set
 * information about the status of the data pair concerning netcommunication */

class NetDataControl {
private:
  int tag;               // identification for the set of datapairs.
  int numberOfx;         // number of vectors set, numberOfx <= totalNumx
  int totalNumx;         // total number of vectors that can be set,
  int numberSent;        // number of x-vectors that have been sent to processes
  int numberAnswers;     // number of y-s that have been received from processes
  int nextAns;           // nextAns points to identity of the next data pair
  int numPar;            // number of parameters in vector x
  coordinates** xyCoord; // stores a set data pairs equals totalNumx
  NetInfo** nInfo;       // for each datapair there is a corresponding class
                         // which keeps track of the status of netcommunication.
  int resendId;          // resendId points to the last x which can be resent
public:
  NetDataControl(int numberOfx, int numberOfParameters, int t);
  ~NetDataControl();
  void setX(const vector& x1);
  void setY(int id, double fx);
  void setDataPair(const vector& x1, double fx);
  vector getX(int id);
  vector getNextXToSend();
  double getY(int id);
  vector getNextAnsweredX();
  double getNextAnsweredY();
  int getNextSendId();
  int getNextXToResend();
  void sentOne(int id);
  void resentOne(int id);
  int getIdToSetNext();
  int getTag();
  int getNumLeftToSend();
  int allSent();
  int getNumNotAnswered();
  int getTotalReceived();
  int getTotalSet();
  void setFirstAnswered();
  int allReceived();
  int getNumAnswered();
  int getLastSetId();
  int getMaxNumData();
  int hasAnswer(int id);
  int isFull();
};

#endif
