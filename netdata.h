#ifndef netdata_h
#define netdata_h

/**
 * \class NetDataVaribles
 * \brief classes NetDataVariables and NetDataResult keep together relevant data to be sent/received in netcommunication 
 */

class NetDataVariables {
public:
  int tag;
  int x_id;
  /**
   * \brief x[0..n-1], where 0 <= n < numVar
   */
  double* x;                     
  /**
   * \brief numVar is the number of variables (>0)
   */
  NetDataVariables(int numVar);  
  /**
   * \brief Default destructor
   */
  ~NetDataVariables();
};

/**
 * \class NetDataResult
 * \brief classes NetDataVariables and NetDataResult keep together relevant data to be sent/received in netcommunication 
 */

class NetDataResult {
public:
  int tag;
  int x_id;
  /**
   * \brief result of f(x) where x is identified by tag x_id.
   */
  double result;                 
  /**
   * \brief identifies which process is sending result
   */
  int who;                       
  NetDataResult();
  ~NetDataResult();
};

#endif
