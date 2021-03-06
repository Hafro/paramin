#ifndef condition_h
#define condition_h

#include "linesearch.h"
#include "paramin.h"

class LineSearch;

/**
 * \class Condition
 * \brief class condition is an abstract class containing only pure virtual functions except for the destructor 
*/

class Condition {
public:
  /**
   * \brief default destructor
   */
  virtual ~Condition() {};
  /**
   * \brief Is the search criteria met?
   */
  virtual int computeCondition() = 0;
};

/**
 * \class  LineSeekerCondition
 * \brief class LineSeekerCondition can be used to compute a condition function belonging to class lineSeeker which must return an interger value 0 or 1 
 */

class LineSeekerCondition : public Condition {
public:
  LineSeekerCondition(LineSearch* lin);
  virtual ~LineSeekerCondition() {};
  int computeCondition();
private:
  LineSearch* lines;
};

#endif
