#ifndef lineseeker_h
#define lineseeker_h

#include "linesearch.h"
#include "wolfe.h"

class LineSeeker : public Wolfe {
private:
  NetInterface* net;
public:
  LineSeeker();
  virtual ~LineSeeker();
  int doLineseek(const vector& xold, const vector& xnew, double fnew, NetInterface* netI);
  int outstandingRequests();
};

#endif
