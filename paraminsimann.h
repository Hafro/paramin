#ifndef paraminsimann_h
#define paraminsimann_h

#include "paraminsearch.h"

class ParaminSimann : public ParaminSearch {
private:
  vector xstart;      // starting values for the variables of the function
  double fstart;        // the function value at xstart..
  vector xp;          // the variable vector returned each time.
  double fp;          // the function value returned each time.
  vector fstar;

  double cs;
  double uratio;
  double lratio;
  vector vm;          // the step length vector.
  vector initialVM;
  double  T;
  int nt;             // number of iterations before temperature reduction.
  int check;
  int ns;
  double eps;
  double rt;
  int maxim;         // 1 = maximization, 0 = minimization.

  int* acpPointId;     // acpPointId[0..numvar], acpPointId[i] = -1 if
                       // point with trial parameter x[i] was not accepted else
                       // acpPointId[i] = returnId of the value which
                       // gives the accepted point.
  int* nacp;      // number of accepted points for each parameter
  int nfcnev;         // total number of function evaluations.

  int *Id;            // denotes in what order the points were sent
  int returnId;
  int NumberOfHosts;  // number of hosts available.

  // AJ 24.03 parameters which are not used in the implementation
  // int iprint;         // controls printing inside simann.
  // int nnew;           // number of optimums.
  // int nobds;          // total number out of bound
    // int nup;
  int nrej;
  // int ndown;
  int lnobds;             // total number of times x is out of bounds
  int naccepted_nsloop;  // the number of accepted function evaluations
                         // accepted in (ns*numvar) number of function evaluations.

public:
  ParaminSimann(NetInterface* netInt);
  virtual ~ParaminSimann();
  void Read(CommentStream& infile, char* text);
  void doSearch(const vector& startx, double startf);
  void SetXP(int k);
  void SetTotalX();
  void AcceptPoint();
  void UpdateVM();
  void ReceiveValue();
};

#endif
