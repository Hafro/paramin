#include "condition.h"

LineSeekerCondition::LineSeekerCondition(LineSearch* lin) {
  lines = lin;
}

int LineSeekerCondition::computeCondition() {
  return lines->computeConditionFunction();
}
