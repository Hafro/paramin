#ifndef commandlineinfo_h
#define commandlineinfo_h

#include "vectorofcharptr.h"

class CommandLineInfo {
public:
  CommandLineInfo();
  ~CommandLineInfo();
  void Read(int aNumber, char *const aVector[]);
  void ReadNetworkInfo();
  void OpenOptinfofile(char* filename);
  void CloseOptinfofile();
  void showCorrectUsage(char* error);
  void showCorrectUsage();
  void showUsage();
  int NumOfProc();
  int WaitForMaster();
  double RunTimeMultiple();
  double BestTimeMultiple();
  double HostMultiple();
  const VectorOfCharPtr& FunctionNameArgs();
  int ToScale();
  int runCondor();
  char* InputFilename();
  char* OutputFilename();
  char* OptFilename();
  int OptinfoGiven();
private:
  VectorOfCharPtr inputfile;
  VectorOfCharPtr outputfile;
  VectorOfCharPtr optfile;
  VectorOfCharPtr networkfile;
  VectorOfCharPtr function;
  int numProc;
  int toScale;
  int condor;
  // Time in sec. for how long slave will wait for
  // a message from master before bailing out.
  // value of 0 means that slave will check for a message from master and
  // immediately return even if there is no message from master
  // value of -1 will have slave wait forever for a message. Default value
  // set to 300.
  int waitForMaster;
  double runtimeMultiple;
  double hostMultiple;
  double besttimeMultiple;
};

#endif
