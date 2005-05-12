#include "linesearch.h"
#include "mathfunc.h"

// ********************************************************
//  Functions for base class linesearch
// ********************************************************
LineSearch::LineSearch() {
  numAlpha = 200;
}

LineSearch::~LineSearch() {
}

double LineSearch::getBestF() {
  return f;
}

Vector LineSearch::getBestX() {
  return x;
}

