#ifndef netdata_h
#define netdata_h

/**
 * \class NetDataVariables
 * \brief This is the class used to keep a vector of relevant data to be sent/received using PVM communication 
 */
class NetDataVariables {
public:
  int tag;
  int x_id;
  /**
   * \brief This is the vector used to store the variables to be sent/received
   */
  double* x;                     
  /**
   * \brief This is the default NetDataVariables constructor
   * \param numVar is the number of variables to be stored
   */
  NetDataVariables(int numVar);  
  /**
   * \brief This is the default NetDataVariables destructor
   */
  ~NetDataVariables();
};

/**
 * \class NetDataResult
 * \brief This is the class used to keep relevant data to be sent/received using PVM communication 
 */
class NetDataResult {
public:
  int tag;
  int x_id;
  /**
   * \brief This is the variable to be sent/received
   */
  double result;                 
  /**
   * \brief This is the identifier of the process that is sending the data
   */
  int who;                       
  /**
   * \brief This is the default NetDataResult constructor
   */
  NetDataResult();
  /**
   * \brief This is the default NetDataResult destructor
   */
  ~NetDataResult();
};

#endif
