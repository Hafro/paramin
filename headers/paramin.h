#ifndef paramin_h
#define paramin_h

/* All the standard header files needed by paramin are also required
 * by gadget, so the easiest way to include them is to include the
 * gadget header file.  This also gives us all the constants that are
 * defined for gadget, so we only need to define any new ones here */
#include "gadget.h"

/* Also defined are some of the constants used by Paramin */
const double LINACC = 0.00001;
const double BORMACC = 0.001;

/* Update the following line each time upgrades are implemented */
#define PARAMINVERSION "2.1.00-BETA"

#endif
