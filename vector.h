#ifndef vector_h
#define vector_h

#ifdef GADGET_NETWORK
#include "gadget.h"
#else
#include "paramin.h"
#endif

typedef double DFP;

/** \class vector 
    This class implements a vector[0..dim-1] of type DFP. It facilitates vector manipulation. Class vector contains functions for vector output and input, vector comparison functions, functions for addition, subtraction and mulitplication, functions for finding vector magnitude and normalize vector 
*/

class vector {
public:
  /**
   * \brief This is the default constructor
   */
  vector();
  /**
   * \brief This constructor initializes a Vector of value zero
   * \param n is the size of the Vector
   */
  vector(int n);
  /**
   * \brief This constructor initializes a Vector with f
   * \param numVar is the size of the Vector
   * \param f is a double* of size numVar
   */
  vector(DFP* f, int numVar);
  /**
   * \brief This constructor initializes a Vector with values from another Vector object
   * \param v is a Vector
   */
  vector(const vector& v);
  /**
   * \brief This is the default destructor
   */
  ~vector();
  /**
   * \brief Returns the length of the Vector  
   */
  int dimension() const;
  /**
   * \brief Returns the sum of two Vectors (positionwise)
   * \param v1 
   * \param v2
   */
  friend vector operator + (const vector& v1, const vector& v2);
  /**
   * \brief Returns the subtraction of v2 to v1 (positionwise)
   * \param v1 is the Vector subtracted from  
   * \param v2 is the subtracting Vector 
   */
  friend vector operator - (const vector& v1, const vector& v2);
  /**
   * \brief Returns -V1
   * \param v1 
   */  
  friend vector operator - (const vector& v1);
  /**
   * \brief Returns a*v1
   * \param a is the scalar
   */
  friend vector operator * (DFP a, const vector& v1);
  /**
   * \brief Returns a*v1
   * \param a is the scalar
   */
  friend vector operator * (const vector& v1, DFP a);
  /**
   * \brief Returns the dot product of v1 and v2
   * \param v1 a Vector
   * \param v2 a Vector
   */
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
