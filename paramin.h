#ifndef paramin_h
#define paramin_h

/* A list of the standard header files that are needed */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stream.h>
#include <strstream.h>
#include <iostream.h>
#include <fstream.h>
#include <time.h>
#include <math.h>
#include <ctype.h>

/* Some compilers define the values for EXIT_SUCCESS and EXIT_FAILURE */
/* but to be sure that they are defined, they are also included here. */
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

/* Also defined are some macros used by Paramin */
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define ABS(x) ((x) > 0 ? (x) : -(x))

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
#define paramin_version "2.0.01"

#endif
