#ifndef paraminsearch_h
#define paraminsearch_h

#include "netcommunication.h"
#include "linesearch.h"
#include "wolfe.h"
#include "armijo.h"
#include "paramin.h"
#include "netgradient.h"
#include "commandlineinfo.h"

class ParaminSearch {
protected:
  NetInterface* net;   // Used for parallell computations
  vector lowerbound;
  vector upperbound;
  int maxiterations; // maximum number of iterations for one minimization
  int numvar;         // number of variables.
  // Must be carefule as x is also used in bfgs, should change to new name
  vector bestx;       // x which gives best point???
  double bestf;       // best point found so far..
  // The optimal value of the function, found so far
  // but where in the algo. Always or just at end..
public:
  ParaminSearch();
  ParaminSearch(NetInterface* netInt);
  virtual ~ParaminSearch();
  virtual void doSearch(const vector& startx, double startf) = 0;
  virtual void Read(CommentStream& infile, char* text) = 0;
  void randomOrder(int* vec);
  const vector& getBestX();
  double getBestF();
  double expRep(double n);
  double randomNumber();
};

#endif
