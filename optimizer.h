#ifndef optimizer_h
#define optimizer_h

#include "paraminsearch.h"
#include "paraminhooke.h"
#include "paraminsimann.h"
#include "paraminbfgs.h"

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
  /**
   * 
   */
  void PrintResult(NetInterface* net);
  const vector& getBestX(NetInterface* net);
  double getBestF();
  /**
   * \brief This is the file reader
   */
  void ReadOptInfo(char* optfilename, NetInterface* net);
 private:
  /**
   * \brief Pointer to a Simulated Annealing object/search method
   */
  ParaminSearch* parSA; 
  /**
   * \brief Pointer to a Hooke & Jeeves object/search method
   */
  ParaminSearch* parHJ;
  /**
   * \brief Pointer to a BFGS object/search method
   */
  ParaminSearch* parBFGS;
  /**
   * \brief seed for the randomgenerator
   */
  int seed;
  /**
   *\brief The starting point of the optimisation
   */
  vector startx;
  /**
   * \brief The f-value at startx
   */
  double startf;
  /**
   * \brief Switch for Simulated Annealing, if 0 don't use else use 
   */
  int useSimann;
  /**
   * \brief Switch for Hooke & Jeeves, if 0 don't use else use 
   */
  int useHooke;
  /**
   * \brief Switch for BFGS, if 0 don't use else use 
   */
  int useBfgs;
  /**
   * \brief Name the outputfile
   */
  char* outputfile;
};

#endif
