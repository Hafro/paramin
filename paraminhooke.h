#ifndef paraminhooke_h
#define paraminhooke_h

#include "paraminsearch.h"
#include "lineseeker.h"

class ParaminHooke : public ParaminSearch {
private:
  double epsilon;   // Halt criteria
  double rho;       // resizing multiplier for the stepsize
  double lambda;     // initial value for the step length (default 0)

  vector xset;           // xset = bestx with one parameter, p, changed:
                         // xset[p] = bestx[p] + delta[p]
                         // 0 <= p < numvar.
  vector xbefore;     // last best point.
  double fbefore;
  double freceive;
  int *par;           // which parameter was changed at point the i-th point sent.
  int numiters;
  int numparamset;
  vector delta;       // the changes tried in best_nearby.
  vector previousf;     // the opt value when xset added to datagroup.
  int NumberOfHosts;  // number of hosts available.
  int* param;
  LineSeeker* lineS;
  int* change;
  int returnId;      // id of the last returned function value.
  // test parameter AJ
  int totalsent;

public:
  ParaminHooke(NetInterface* netInt);
  virtual ~ParaminHooke();
  void Read(CommentStream& infile, char* text);
  void doSearch(const vector& startx, double startf);
  void bestNearby();
  int SetPoint( int n);
  int MyDataGroup();
  void ReceiveValue();
  int UpdateOpt();
  void SetPointNearby();
  void SetPointNextCoord();
};

#endif
