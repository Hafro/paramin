#ifndef paraminsearch_h
#define paraminsearch_h

#include "netcommunication.h"
#include "linesearch.h"
#include "armijo.h"
#include "paramin.h"
#include "netgradient.h"
#include "commandlineinfo.h"
#include "optinfo.h"

/**
 * \class ParaminSearch
 * \brief Base class for Paramin search algorithms
 */
// AJ. should chekc if need the upper, lower, x, f..
// think so because used throughout the algos..
// but better check is...
class ParaminSearch : public OptInfo {
protected:
  /**
   * \brief Used for parallell computations
   */
  NetInterface* net;   // Used for parallell computations
  /**
   * \brief The lowerbound for the variables
   */
  DoubleVector lowerbound;
  /**
   * \brief The upperbound for the variables
   */
  DoubleVector upperbound;
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
  DoubleVector bestx;       // x which gives best point???
  /**
   * \brief The optimal value of the function, found so far
   */
  // AJ this is in optinfo?????
  double bestf;       // best point found so far..
  // The optimal value of the function, found so far
  // but where in the algo. Always or just at end..
  // int converged;
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
  //virtual void doSearch(const DoubleVector& startx, double startf) = 0;
  // don't need the startx, startf, as they are in netInterface...
  virtual void OptimiseLikelihood() = 0;
   /**
   * \brief The file reader
   * \param infile is the CommentStream to read the optimisation parameters from
   * \param text is the latest entry from infile
   */
  virtual void read(CommentStream& infile, char* text) = 0;
  /**
   * \brief This function randomizes the order of varibles of vec
   * \param vec is an int* of size numvar 
   */
  void randomOrder(IntVector& vec);
  /**
   * \brief This function returns the best point, bestx
   */
  const DoubleVector& getBestX();
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
  int GetConverged() { return converge;};
  virtual void Print(ofstream& outfile, int prec) = 0;
  void printX(const DoubleVector& vec);
  void printX(ofstream& output, const DoubleVector& vec);
};

#endif
