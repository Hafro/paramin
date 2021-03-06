#ifndef paraminsimann_h
#define paraminsimann_h

#include "paraminsearch.h"
class ParaminSimann : public ParaminSearch {
private:
  /**
   * \brief starting values for the variables of the function
   */
  DoubleVector xstart;      
  /**
   * \brief the function value at xstart..
   */
  double fstart;        
  /**
   * \brief the variable vector returned each time.
   */
  DoubleVector xp;          
  /**
   * \brief the function value returned each time.
   */
  double fp;          
  DoubleVector fstar;
  double cs;
  double uratio;
  double lratio;
  /**
   * \brief the step length vector.
   */
  DoubleVector vm;          
  DoubleVector initialVM;
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
   * \brief acpPointID[0..numvar], acpPointID[i] = -1 if point with trial parameter x[i] was not accepted else acpPointID[i] = returnID of the value which gives the accepted point.
   */
  IntVector acpPointID;
  //int* acpPointID;     
  /**
   * \brief number of accepted points for each parameter
   */
  // int* nacp;
  IntVector nacp;
  /**
   * \brief total number of function evaluations.
   */
  // int nfcnev;     
  /**
   * \brief denotes in what order the points were sent
   */
  // int *ID;
  IntVector ID;
  int returnID;
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
  void read(CommentStream& infile, char* text);
  // void doSearch(const DoubleVector& startx, double startf);
  void OptimiseLikelihood();
  void SetXP(int k);
  void AcceptPoint();
  void UpdateVM();
  void ReceiveValue();
  void Print(ofstream& outfile, int prec);

};

#endif
