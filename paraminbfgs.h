#ifndef paraminbfgs_h
#define paraminbfgs_h

#include "paraminsearch.h"
/**
 * \class ParaminBFGS
 * \brief BFGS
 *
 * ABSTRACT:
 * The BFGS-method is a gradient iterative method that work as follows: We start at some point and successively generate new points, so that the function value decreases in each iteration. This is done by choosing a direction that makes an angle greater than 90 degrees with the gradient of the function.
 *
 * Authors
 * Gunnar Stefansson
 * Kristjana Yr Jonsdottir
 * Thordis Linda Thorarinsdottir
 *
 * The BFGS-method is one of the Quasi-Newton methods. Quasi-Newton methods are iterative methods which approximate Newton's method without calculating second derivatives. They are of the form
 *      x^k+1 = x^k + a^k * d^k,
 *      d^k = -D^k \nabla f( x^k ),
 * where D^k is a positive definite matrix, updated using the BFGS updating formula. To find the value of a, an inexact linesearch called the Armijo rule is used. That method finds an a such that Armijo condition is satiefied, which is based on getting a function value that is some amount better than the best know function value. This is done by trying at first an initial stepsize an than reducing the stepsize until a value that satisfies the Armijo condition is found. For details, see D.P. Bertsekas, "Nonlinear Programming", Athena Scientific, Belmont, 1999. As the objective function is not necessarily differentiable, the gradient is calculted numerically.
 */
class ParaminBFGS : public ParaminSearch {
private:
  /**
   * \brief This object handles the linesearch
   */
  Armijo* lineS;
  /**
   * \brief This object computes the gradient at x
   */
  NetGradient* grad;
  /**
   * \brief iteration number
   */
  int iter;
  /**
   * \brief stores the diagonal entry of the hessian matrix.
   */
  Vector diaghess;
  /**
   * \brief stores the inverse hessian matrix.
   */
  double** invhess;
  /**
   * \brief stores the gradient at xi
   */
  Vector gi;
  /**
   * \brief stores the gradient at xi-1
   */
  Vector gim1;
  /**
   * \brief direction vector used in line minimization.
   */
  Vector h;
  /**
   * \brief derivative used in line minimization.
   */
  double dery;
  /**
   * \brief difference between current bestx and the one before
   */
  Vector deltax;
  /**
   * \brief norm of x
   */
  double normx;
  /**
   * \brief norm of h (search direction)
   */
  double normh;
  /**
   * \brief norm of the gradient
   */
  double normgrad;
  /**
   * \brief norm of deltax
   */
  double normdeltax;
  int get;
  /**
   * \brief an indicator of how many points are needed for the gradient calculations
   */
  int difficultgrad;
  int armijoproblem;
  int initial_difficultgrad;
  int to_print;
  double s;
  /**
   * \brief if Shannon scaling is to be used
   */
  int shannonScaling;
  Vector xopt;
  int bfgs_constant;
  double errortol;
  double xtol;
  int maxrounds;  // There is a diff between bfgs.maxiter and maxronds
  int bfgsFail;
  double error;
  // AJ adding for testing
  int computedGrad;

public:
  /**
   * \brief the default constructor
   * \param netInt is a NetInterface to initialise ParaminSearch
   */
  ParaminBFGS(NetInterface* netInt);
  /**
   * \brief The default destructor
   */
  virtual ~ParaminBFGS();
  /**
   * \brief The BFGS file reader
   * \param infile is the CommentStream to read the optimisation parameters from
   * \param text is the latest entry from infile
   */
  void Read(CommentStream& infile, char* text);
  /**
   * \brief Does a BFGS search
   * \param startx is the starting point of the BFGS
   * \param startf is the f-value at startx
   */
  void doSearch(const Vector& startx, double startf);
  /**
   * \brief Computes the gradient at x
   */
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
