#ifndef optimizer_h
#define optimizer_h

// #include "paraminsearch.h"
#include "paraminhooke.h"
#include "paraminsimann.h"
#include "paraminbfgs.h"
#include "optinfoptrvector.h"

class Optimizer {
public:
  Optimizer(CommandLineInfo* info, NetInterface* net);
  /**
   * \brief Default destructor
   */
  ~Optimizer();
  /**
   * \brief Starts the optimisation
   */
  void OptimizeFunc();
  // void printResult(NetInterface* net);
  // const DoubleVector& getBestX(NetInterface* net);
  void printResult();
  // void DoubleVector& getBestX();
  // double getBestF();
  void printX(const DoubleVector& vec);
  /**
   * \brief This is the file reader
   */
  // void readOptInfo(char* optfilename, NetInterface* net);
  void readOptInfo(char* optfilename);
  void getScore(DoubleVector& x, double fx);
  double getBestF();
private:
  NetInterface* netInt;
  /**
   * \brief Pointer to a Simulated Annealing object/search method
   */
   OptInfoPtrVector optvec;
   // ParaminSearch* parSA; 
  /**
   * \brief Pointer to a Hooke & Jeeves object/search method
   */
  // ParaminSearch* parHJ;
  /**
   * \brief Pointer to a BFGS object/search method
   */
  // ParaminSearch* parBFGS;
  /**
   * \brief seed for the randomgenerator
   */
  int seed;
  /**
   *\brief The starting point of the optimisation
   */
  // DoubleVector startx;
  /**
   * \brief The f-value at startx
   */
  // double startf;
  /**
   * \brief Switch for Simulated Annealing, if 0 don't use else use 
   */
  // int useSA;
  /**
   * \brief Switch for Hooke & Jeeves, if 0 don't use else use 
   */
  // int useHJ;
  /**
   * \brief Switch for BFGS, if 0 don't use else use 
   */
  // int useBFGS;
  /**
   * \brief Name the outputfile
   */
  char* outputfile;
};

#endif
