#ifndef condition_h
#define condition_h

#include "linesearch.h"
#include "paramin.h"

class linesearch;

/* class condition is an abstract class containing
 * only pure virtual functions except for the destructor */

class condition {
public:
  virtual ~condition() {};
  virtual int computeCondition() = 0;
};

/* class LineSeekerCondition can be used to compute a condition function
 * belonging to class lineSeeker which must return an interger value 0 or 1 */

class LineSeekerCondition : public condition {
public:
  LineSeekerCondition(linesearch* lin);
  virtual ~LineSeekerCondition() {};
  int computeCondition();
private:
  linesearch* lines;
};

#endif
