#ifndef paramin_h
#define paramin_h

/* A list of the standard header files that are needed */
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <ctime>
#include <signal.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/param.h>

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

/* Also defined are some of the constants used by Paramin */
const double LINACC = 0.00001;
const double BORMACC = 0.001;
#ifndef GADGET_NETWORK
const double rathersmall = 1e-10;
const double verysmall = 1e-100;
const double verybig = 1e+10;
const int MaxStrLength = 250;
const char sep = ' ';
const int NUMVARS = 350;
#endif

/* Update the following line each time upgrades are implemented */
#define PARAMINVERSION "2.1.00-BETA"

#endif
