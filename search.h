#ifndef search_h
#define search_h

#include "netcommunication.h"
#include "linesearch.h"
#include "paramin.h"
#include "netgradient.h"

class bfgs {
private:
  armijo* lineS;
  gradient* grad;     // grad computes the gradient of x.
  NetInterface* net;  // used for netcommunication by grad and lines.
  int numberOfVariables;
  int iter;
  vector diaghess;    // stores the diagonal entry of the hessian matrix.
  double** invhess;   // stores the inverse hessian matrix.
  vector x;           // best x found so far, x[0..numberOfVariables-1].
  double y;           // current function value at x,
  double f;           // best known function value.
  vector gi, gim1;    // store the gradients at xi and xi-1.
  vector h;           // direction vector used in line minimization.
  double dery;        // derivative used in line minimization.
  double alpha;       // distance in line minimization.
  vector deltax;
  double normx, normh, normgrad, normdeltax;
  vector upper;       // upper bound of x.
  vector lower;       // lower bound of x.
  int get;
  int difficultgrad;
  int armijoproblem;
  double s;
  int ShannonScaling; // if Shannon scaling is to be used let
  vector xopt;
  int bfgs_constant;
public:
  bfgs(NetInterface* netInt, gradient* g, armijo* lineseek);
  ~bfgs();
  void computeGradient();
#ifdef CONDOR
  void computeGradientCondor();
#endif
  void doLineseek();
#ifdef CONDOR
  void doLineseekCondor();
#endif
  int iteration(int maxit, double errortol, double xtol, int iteration);
  int iterationCondor(int maxit,double errortol, double xtol, int iteration);
  void scaleDirectionVector();
  void computeDirectionVector();
  void defineHessian();
  void updateXandGrad();
  int bfgsUpdate();
  double norm();
  void printResult();
  void printGradient();
  void printInverseHessian();
  double getS(int get);
  double max(double a, double b);
  double min(double a, double b);
  vector getBestX();
  void printX();
  double getBestF();
  // About termination criteria:
  // bfgsfail = -1 iff minimization will be continued.
  // bfgsfail = 0 iff error <= errortol
  // bfgsfail = 1 iff alpha < 0.
  // bfgsfail = 2 iff normdeltax < xtol.
  // bfgsfail = 4 iff dery > 0.
  // bfgsfail = 5 iff there's a instability error.
  // bfgsfail = 6 iff you couldn't get alpha from linesearch (alpha = 0).
  // bfgsfail = -2 iff you couldn get alpha from linesearch and you want to increase difficultgrad
};

class search {
public:
  vector x;
  double f;
  virtual ~search();
  virtual int doSearch() = 0;
#ifdef CONDOR
  virtual int doSearchCondor() = 0;
#endif
  vector getBestX(NetInterface *net);
  double getBestF();
};

class minimizer : public search {
private:
  bfgs* min;
  NetInterface* net;
public:
#ifdef CONDOR
  int doSearchCondor();
#endif
  minimizer(NetInterface* netInt);
  virtual ~minimizer();
  int doSearch();
};

class simann : public search {
private:
  vector xstart;      // starting values for the variables of the function
  vector c;           // the vector that controls the step length adjustment.
  vector initialC;
  vector initialVm;
  vector vm;          // the step length vector.
  vector fstar;
  vector xopt;        // the variables that optimize the function.
  vector xp;
  double  t;
  double initialT;
  double fopt;        // the optimal value of the function.
  double fp;          // the function value returned each time.
  int nnew;           // number of optimums.
  int nacp[200];      // number of accepted points for each parameter
  int n;              // number of variables in the function to be optimized.
  int nt;             // number of iterations before temperature reduction.
  int nacc;           // the number of accepted function evaluations.
  int maxevl;         // maximum number of function evaluations.
  int iprint;         // controls printing inside simann.
  int nobds;          // total number out of bound
  int lnobds;
  int nfcnev;         // total number of function evaluations.
  int maxmin;         // 1 = maximization, 0 = minimization.
  int *Id;            // denotes in what order the points were sent
  int returnId;
  int nup, nrej, ndown;
  NetInterface* net;  // used for parallel computations.
  int NumberOfHosts;  // number of hosts available.
public:
  simann(NetInterface* netInt, int MAXIM, int SIM_ITER,
    const vector& c, int T, const vector& vm);
  virtual ~simann();
  int doSearch();
#ifdef CONDOR
  int doSearchCondor();
#endif
  int sendData();
  double expRep(double d);
  double simannRandom();
  vector getXP(int k);
  void acceptPoint();
  void updateVM();
  int IdContains(int q);
  void receiveValue();
  void receiveValueNonBlocking();
};

class hooke : public search {
private:
  vector z;           // dummy variable.
  vector xbefore;     // last best point.
  vector newx;        // next point to search from in bestNearby.
  int *par;           // which parameter was changed at point the i-th point sent.
  int nevl;
  int nsent;          // number of points sent to pvm
  int numvar;         // number of variables.
  vector delta;       // the changes tried in bestNearby.
  vector fbefore;     // the value at previous point.
  NetInterface* net;  // used for parallel computations.
  int NumberOfHosts;  // number of hosts available.
  int iters;          // number of iterations
  int param[NUMVARS];
  lineseeker* lineS;
  vector lbd;
  vector ubd;
public:
  hooke(NetInterface* netInt);
  virtual ~hooke();
  int doSearch();
#ifdef CONDOR
  int doSearchCondor();
#endif
  double bestNearby(const vector& delta, double prevbest);
#ifdef CONDOR
  double bestNearbyCondor(const vector& delta, double prevbest);
#endif
  int setPoint(int n, double flast);
  int sendData();
};

#endif
