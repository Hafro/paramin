#ifndef paraminsimann_h
#define paraminsimann_h

#include "paraminsearch.h"

class ParaminSimann : public ParaminSearch {
private:
  /**
   * \brief starting values for the variables of the function
   */
  vector xstart;      
  /**
   * \brief the function value at xstart..
   */
  double fstart;        
  /**
   * \brief the variable vector returned each time.
   */
  vector xp;          
  /**
   * \brief the function value returned each time.
   */
  double fp;          
  vector fstar;
  double cs;
  double uratio;
  double lratio;
  /**
   * \brief the step length vector.
   */
  vector vm;          
  vector initialVM;
  double T;
  /**
   * \brief number of iterations before temperature reduction.
   */
  int nt;             
  int check;
  int ns;
  double eps;
  double rt;
  /**
   * \brief 1 = maximization, 0 = minimization.
   */
  int maxim;         
  /**
   * \brief acpPointId[0..numvar], acpPointId[i] = -1 if point with trial parameter x[i] was not accepted else acpPointId[i] = returnId of the value which gives the accepted point.
   */
  int* acpPointId;     
  /**
   * \brief number of accepted points for each parameter
   */
  int* nacp;      
  /**
   * \brief total number of function evaluations.
   */
  int nfcnev;     
  /**
   * \brief denotes in what order the points were sent
   */
  int *Id;            
  int returnId;
  /**
   * \brief number of hosts available.
   */
  int NumberOfHosts;
  /**
   * \brief the number of accepted function evaluations accepted in (ns*numvar) number of function evaluations.
   */
  int naccepted_nsloop;  
public:
  ParaminSimann(NetInterface* netInt);
  virtual ~ParaminSimann();
  void Read(CommentStream& infile, char* text);
  void doSearch(const vector& startx, double startf);
  void SetXP(int k);
  void AcceptPoint();
  void UpdateVM();
  void ReceiveValue();
};

#endif
