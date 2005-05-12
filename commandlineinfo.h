#ifndef commandlineinfo_h
#define commandlineinfo_h

#include "charptrvector.h"

class CommandLineInfo {
public:
  CommandLineInfo();
  ~CommandLineInfo();
  void read(int aNumber, char *const aVector[]);
  void readNetworkInfo();
  void showCorrectUsage(char* error);
  void showCorrectUsage();
  void showUsage();
  int getNumProc() { return numProc; };
  int getWaitMaster() { return waitMaster; };
  double getRunMultiple() { return runMultiple; };
  double getBestMultiple() { return bestMultiple; };
  double getHostMultiple() { return hostMultiple; };
  int getScale() { return scale; };
  int runCondor() { return condor; };
  char* getInputFilename();
  char* getOutputFilename();
  char* getOptFilename();
  int getOptInfoFileGiven();
  const CharPtrVector& getFunction();
private:
  CharPtrVector inputfile;
  CharPtrVector outputfile;
  CharPtrVector optfile;
  CharPtrVector networkfile;
  CharPtrVector function;
  int numProc;
  int scale;
  int condor;
  // Time in sec. for how long slave will wait for
  // a message from master before bailing out.
  // value of 0 means that slave will check for a message from master and
  // immediately return even if there is no message from master
  // value of -1 will have slave wait forever for a message. Default value
  // set to 300.
  int waitMaster;
  double runMultiple;
  double hostMultiple;
  double bestMultiple;
};

#endif
