#ifndef paraminsearch_h
#define paraminsearch_h

#include "netcommunication.h"
#include "linesearch.h"
#include "armijo.h"
#include "paramin.h"
#include "netgradient.h"
#include "commandlineinfo.h"
/**
 * \class ParaminSearch
 * \brief Base class for Paramin search algorithms
 */
class ParaminSearch {
protected:
  /**
   * \brief Used for parallell computations
   */
  NetInterface* net;   // Used for parallell computations
  /**
   * \brief The lowerbound for the variables
   */
  vector lowerbound;
  /**
   * \brief The upperbound for the variables
   */
  vector upperbound;
  /**
   * \brief maximum number of iterations for one minimization
   */ 
  int maxiterations; // maximum number of iterations for one minimization
   /**
   * \brief number of variables.
   */
  int numvar;         // number of variables.
  // Must be carefule as x is also used in bfgs, should change to new name
  /**
   * \brief best point found so far..
   */
  vector bestx;       // x which gives best point???
  /**
   * \brief The optimal value of the function, found so far
   */
  double bestf;       // best point found so far..
  // The optimal value of the function, found so far
  // but where in the algo. Always or just at end..
  int converged;
public:
  /**
   * \brief The default constructor
   */
  ParaminSearch();
  /**
   * \brief 
   * \param 
   */
  ParaminSearch(NetInterface* netInt);
  /**
   * \brief default destructor
   */  
  virtual ~ParaminSearch();
  /**
   * \brief Does a search, implemented in descending classes
   * \param startx is the starting point of the BFGS
   * \param startf is the f-value at startx
   */
  virtual void doSearch(const vector& startx, double startf) = 0;
  /**
   * \brief The file reader
   * \param infile is the CommentStream to read the optimisation parameters from
   * \param text is the latest entry from infile
   */
  virtual void Read(CommentStream& infile, char* text) = 0;
  /**
   * \brief This function randomizes the order of varibles of vec
   * \param vec is an int* of size numvar 
   */
  void randomOrder(int* vec);
  /**
   * \brief This function returns the best point, bestx
   */
  const vector& getBestX();
  /**
   * \brief This function returns the best f-value, bestf (corresponds to bestx)
   */
  double getBestF();
  /**
   * \brief This function calculates exp(x)
   * \param x 
   */
  double expRep(double x);
  /**
   * \brief Returns a randomNumber coming from an even distribution on the interval 0 to 1
   */
  double randomNumber();
  int GetConverged() { return converged;};
};

#endif
