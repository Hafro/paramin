#ifndef paramin_h
#define paramin_h

/* A list of the standard header files that are needed for Paramin*/
/* Older compilers need these to be declared in the old format    */
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <ctime>

/* This is a nasty hack to use the functions in the std namespace */
/* it would be much better to explicitly state the std namespace  */
/* when using the functions - eg std::strcmp() not just strcmp()  */
/* Older compilers will reject this so it needs to be removed     */
using namespace std;

/* Some compilers define the values for EXIT_SUCCESS and EXIT_FAILURE */
/* but to be sure that they are defined, they are also included here. */
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

/* Also defined are some macros used by Paramin */
#ifndef GADGET_NETWORK
#define min(x, y) ((x) > (y) ? (x) : (y))
#define max(x, y) ((x) < (y) ? (x) : (y))
#define absolute(x) ((x) > 0 ? (x) : -(x))
#endif

/* Also defined are some of the constants used by Paramin */
const int SCALE = 1;
const int MAXITER = 10000;
const int NUMVARS = 200;
const double ROUNDOFF = 1e-12;
const double BIG = 1e10;
const double MACHEPS = 1e-12;
const double LINACC = 0.00001;
const double BORMACC = 0.001;
#ifndef GADGET_NETWORK
const int MaxStrLength = 250;
const char sep = ' ';
#endif

/* Update the following line each time upgrades are implemented */
#define paramin_version "2.0.02-BETA"

#endif
