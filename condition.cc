#include "condition.h"

condition::~condition() {
}

lineSeekerCondition::lineSeekerCondition(linesearch* lin) {
  lines = lin;
}

lineSeekerCondition::~lineSeekerCondition() {
}

int lineSeekerCondition::computeCond() {
  int con;
  con = lines->computeConditionFunction();
  assert(con == 0 || con == 1);
  return con;
}
