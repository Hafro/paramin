#include "linesearch.h"
#include "mathfunc.h"

// ********************************************************
//  Functions for base class linesearch
// ********************************************************
LineSearch::LineSearch() {
  NUMALPHA = 200;
}

LineSearch::~LineSearch() {
}

double LineSearch::getBestF() {
  return f;
}

vector LineSearch::getBestX() {
  return x;
}

