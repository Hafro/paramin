#include "condition.h"

LineSeekerCondition::LineSeekerCondition(linesearch* lin) {
  lines = lin;
}

int LineSeekerCondition::computeCondition() {
  return lines->computeConditionFunction();
}
