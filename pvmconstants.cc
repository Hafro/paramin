#include "pvmconstants.h"

#ifdef GADGET_NETWORK
#include "gadget.h"
#else
#include "paramin.h"
#endif

PVMConstants::PVMConstants() {
  endType = 1;
  startType = 2;
  taskDied = 3;
  masterSendNetDataVar = 4;
  masterSendString = 5;
  masterReceiveNetDataResult = 6;
  masterSendBound = 7;
  addHost = 8;
  hostSuspend = 9;
  hostResume = 10;
  hostDelete = 11;
  taskExit = 12;
  dataencode = 1;
}

PVMConstants::~PVMConstants() {
}

const void PVMConstants::setDataEncode(int dc) {
  if (dc == 0 || dc == 1 || dc == 2)
    dataencode = dc;
  else {
    cerr << "Error in pvmconstants - illegal value for dataencode\n";
    exit(EXIT_FAILURE);
  }
}

const int PVMConstants::getDiedTag() const {
  return taskDied;
}

const int PVMConstants::getStopTag() const {
  return endType;
}

const int PVMConstants::getStartTag() const {
  return startType;
}

const int PVMConstants::getMasterSendVarTag() const {
  return masterSendNetDataVar;
}

const int PVMConstants::getMasterSendStringTag() const {
  return masterSendString;
}

const int PVMConstants::getMasterSendBoundTag() const {
  return masterSendBound;
}

const int PVMConstants::getMasterReceiveDataTag() const {
  return masterReceiveNetDataResult;
}

const int PVMConstants::getDataEncode() const {
  return dataencode;
}

const int PVMConstants::getAddHostTag() const {
  return addHost;
}

const int PVMConstants::getHostSuspendTag() const {
  return hostSuspend;
}

const int PVMConstants::getHostResumeTag() const {
  return hostResume;
}

const int PVMConstants::getHostDeleteTag() const {
  return hostDelete;
}

const int PVMConstants::getTaskExitTag() const {
  return taskExit;
}
