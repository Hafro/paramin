#ifndef lineseeker_h
#define lineseeker_h

#include "linesearch.h"

class LineSeeker : public LineSearch {
private:
  NetInterface* net;
public:
  LineSeeker();
  virtual ~LineSeeker();
  int doLineseek(const Vector& xold, const Vector& xnew, double fnew, NetInterface* netI);
  int outstandingRequests();
  double getAlpha() { return 0.0;};
  int computeConditionFunction() { return 0; };
};

#endif
