#ifndef paraminhooke_h
#define paraminhooke_h

#include "paraminsearch.h"
#include "lineseeker.h"
/**
 * \class ParaminHooke
 * \brief Hooke and Jeeves
 *
 *
 *Nonlinear Optimization using the algorithm of Hooke and Jeeves  
 *	12 February 1994	
 *	author: Mark G. Johnson 	   
 *	August 1999
 *	changes for parallel use: Kristjana Yr Jonsdottir and 
 *				  Thordis Linda Thorarinsdottir
 *
 * Find a point X where the nonlinear function f(X) has a local minimum.  X is an n-vector and f(X) is a scalar. In mathematical notation  f: R^n -> R^1. The objective function f() is not required to be continuous. Nor does f() need to be differentiable.  The program does not use or require derivatives of f().		
*
* The software user supplies three things: a subroutine that computes f(X), an initial "starting guess" of the minimum point X, and values for the algorithm convergence parameters. Then the program searches for a local minimum, beginning from the starting guess, using the Direct Search algorithm of Hooke and Jeeves.			
* 
* The original C program is adapted from the Algol pseudocode found in "Algorithm 178: Direct Search" by Arthur F. Kaupe Jr.,Communications of the ACM, Vol 6. p.313 (June 1963). It includes the improvements suggested by Bell and Pike (CACM v.9, p. 684, Sept 1966) and those of Tomlin and Smith, "Remark on Algorithm 178" (CACM v.12). The original paper, which I don't recommend as highly as the one by A. Kaupe, is:  R. Hooke and T. A. Jeeves, "Direct Search Solution of Numerical and Statistical Problems", Journal of the ACM, Vol. 8, April 1961, pp. 212-229. 	   
 */
class ParaminHooke : public ParaminSearch {
private:
  /**
   * \brief This is a user-supplied convergence parameter (more detail below), which should be  set to a value between 0.0 and 1.0. Larger values of rho give greater probability of convergence on highly nonlinear functions, at a cost of more function evaluations. Smaller values of rho reduces the number of evaluations (and the program running time), but increases the risk of nonconvergence.   
   */
  double epsilon;   // Halt criteria
  /**
   * \brief This is the criterion for halting the search for a minimum.  When the algorithm begins to make less and less progress on eachiteration, it checks the halting criterion: if the stepsize is below epsilon, terminate the iteration and return the current best estimate of the minimum.  Larger values of epsilon (such as 1.0e-4) give quicker running time, but a less accurate estimate of the minimum.  Smaller values of epsilon (such as 1.0e-7) give longer running time, but a more accurate estimate of the minimum. 
   */
  double rho;       // resizing multiplier for the stepsize
  /**
   * \brief initial value for the step length (default 0)
   */
  double lambda;     
  /**
   * \brief  xset = bestx with one parameter, p, changed: xset[p] = bestx[p] + delta[p]
   */
  vector xset;           
  /**
   * \brief last best point.
   */
  vector xbefore;    
  /**
   * \brief last best f-value
   */  
  double fbefore;
  /**
   * \brief 
   */
  double freceive;
  /**
   * \brief which parameter was changed at point the i-th point sent.
   */
  int *par;           
  /**
   * \brief 
   */
  int numiters;
  int numparamset;
  /**
   * \brief the changes tried in best_nearby.
   */
  vector delta;      
  /**
   * \brief the opt value when xset added to datagroup.
   */
  vector previousf;    
  /**
   * \brief number of hosts available.
   */
  int NumberOfHosts;  
  int* param;
  LineSeeker* lineS;
  int* change;
  /**
   * \brief id of the last returned function value.
   */
  int returnId;      
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
