#ifndef optimizer_h
#define optimizer_h

#include "paraminsearch.h"
#include "paraminhooke.h"
#include "paraminsimann.h"
#include "paraminbfgs.h"

class Optimizer {
public:
  Optimizer(CommandLineInfo* info, NetInterface* net);
  ~Optimizer();
  void OptimizeFunc();
  void PrintResult(NetInterface* net);
  const vector& getBestX(NetInterface* net);
  double getBestF();
  void ReadOptInfo(char* optfilename, NetInterface* net);
 private:
  ParaminSearch* parSA;
  ParaminSearch* parHJ;
  ParaminSearch* parBFGS;
  int seed;
  vector startx;
  double startf;
  int useSimann;
  int useHooke;
  int useBfgs;
  char* outputfile;
};

#endif
