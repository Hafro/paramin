#ifndef vector_h
#define vector_h

#include "paramin.h"

typedef double DFP;

/* class vector implements a vector[0..dim-1] of type DFP. It
 * facilitates vector manipulation. Class vector contains functions for vector
 * output and input, vector comparison functions, functions for
 * addition, subtraction and mulitplication, functions for finding
 * vector magnitude and normalize vector */

class vector {
public:
  vector();
  vector(int n);
  vector(DFP* f, int numVar);
  vector(const vector& v);
  ~vector();
  int dimension() const;
  friend vector operator + (const vector& v1, const vector& v2);
  friend vector operator - (const vector& v1, const vector& v2);
  friend vector operator - (const vector& v1);
  friend vector operator * (DFP a, const vector& v1);
  friend vector operator * (const vector& v1, DFP a);
  friend DFP operator * (const vector& v1, const vector& v2);
  friend int operator < (const vector& v1, const vector& v2);
  friend int operator > (const vector& v1, const vector& v2);
  friend int operator == (const vector& v1, const vector& v2);
  friend int operator != (const vector& v1, const vector& v2);
  vector& operator = (const vector& v);
  DFP& operator [] (int i) const;
  void setValue(DFP f);
  DFP magnitude();
  friend vector normalize(vector& v1);
  friend ostream& operator << (ostream& os, const vector& v1);
  friend istream& operator >> (istream& is, vector& v1);
private:
  DFP* p;
  int dim;
};

#endif
