#ifndef paraminbfgs_h
#define paraminbfgs_h

#include "paraminsearch.h"

class ParaminBFGS : public ParaminSearch {
private:
  Armijo* lineS;
  gradient* grad;     // grad computes the gradient of x.
  int iter;
  vector diaghess;    // stores the diagonal entry of the hessian matrix.
  double** invhess;   // stores the inverse hessian matrix.
  // vector x;           // best x found so far, x[0..numberOfVariables-1].
  // double y;           // current function value at x,
  // Probably do not need this f...
  // double f;           // best known function value.
  vector gi, gim1;    // store the gradients at xi and xi-1.
  vector h;           // direction vector used in line minimization.
  double dery;        // derivative used in line minimization.
  // double alpha;       // distance in line minimization.
  vector deltax;
  double normx, normh, normgrad, normdeltax;
  int get;
  int difficultgrad;
  int armijoproblem;
  int initial_difficultgrad;
  int to_print;
  double s;
  int shannonScaling; // if Shannon scaling is to be used let
  vector xopt;
  int bfgs_constant;
  double errortol;
  double xtol;
  int maxrounds;  // There is a diff between bfgs.maxiter and maxronds
  int bfgsFail;
  double error;
  // AJ adding for testing
  int computedGrad;
public:
  ParaminBFGS(NetInterface* netInt);
  virtual ~ParaminBFGS();
  void Read(CommentStream& infile, char* text);
  void doSearch(const vector& startx, double startf);
  void ComputeGradient();
  void doLineseek();
  void iteration();
  void SetInitialValues();
  void UpdateValues();
  void ScaleDirectionVector();
  void ComputeDirectionVector();
  void DefineHessian();
  void UpdateXandGrad();
  int bfgsUpdate();
  double norm();
  void printResult();
  void printGradient();
  void printInverseHessian();
  double GetS(int get);
  void printX();
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

#endif
