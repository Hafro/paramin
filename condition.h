#ifndef condition_h
#define condition_h

#include "linesearch.h"
#include "paramin.h"

class linesearch;

/* class condition is an abstract class containing
 * only pure virtual functions except for the desturctor */

class condition {
public:
  virtual ~condition();
  virtual int computeCond() = 0;
};

/* class lineSeekerCondition can be used to compute a condition function
 * belonging to class lineSeeker which must return an interger value 0 or 1 */

class lineSeekerCondition : public condition {
public:
  lineSeekerCondition(linesearch* lin);
  virtual ~lineSeekerCondition();
  int computeCond();
private:
  linesearch* lines;
};

#endif
