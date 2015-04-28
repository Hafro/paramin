#ifndef vector_h
#define vector_h

#include "paramin.h"

typedef double DFP;

/** \class Vector 
    This class implements a Vector[0..dim-1] of type DFP. It facilitates Vector manipulation. Class Vector contains functions for Vector output and input, Vector comparison functions, functions for addition, subtraction and mulitplication, functions for finding Vector magnitude and normalize Vector 
*/

class Vector {
public:
  /**
   * \brief This is the default constructor
   */
  Vector();
  /**
   * \brief This constructor initializes a Vector of value zero
   * \param n is the size of the Vector
   */
  Vector(int n);
  /**
   * \brief This constructor initializes a Vector with f
   * \param numVar is the size of the Vector
   * \param f is a double* of size numVar
   */
  Vector(DFP* f, int numVar);
  /**
   * \brief This constructor initializes a Vector with values from another Vector object
   * \param v is a Vector
   */
  Vector(const Vector& v);
  /**
   * \brief This is the default destructor
   */
  ~Vector();
  /**
   * \brief Returns the length of the Vector  
   */
  int dimension() const;
  /**
   * \brief Returns the sum of two Vectors (positionwise)
   * \param v1 
   * \param v2
   */
  friend Vector operator + (const Vector& v1, const Vector& v2);
  /**
   * \brief Returns the subtraction of v2 to v1 (positionwise)
   * \param v1 is the Vector subtracted from  
   * \param v2 is the subtracting Vector 
   */
  friend Vector operator - (const Vector& v1, const Vector& v2);
  /**
   * \brief Returns -V1
   * \param v1 
   */  
  friend Vector operator - (const Vector& v1);
  /**
   * \brief Returns a*v1
   * \param a is the scalar
   */
  friend Vector operator * (DFP a, const Vector& v1);
  /**
   * \brief Returns a*v1
   * \param a is the scalar
   */
  friend Vector operator * (const Vector& v1, DFP a);
  /**
   * \brief Returns the dot product of v1 and v2
   * \param v1 a Vector
   * \param v2 a Vector
   */
  friend DFP operator * (const Vector& v1, const Vector& v2);

  friend int operator < (const Vector& v1, const Vector& v2);
  friend int operator > (const Vector& v1, const Vector& v2);
  friend int operator == (const Vector& v1, const Vector& v2);
  friend int operator != (const Vector& v1, const Vector& v2);
  Vector& operator = (const Vector& v);
  DFP& operator [] (int i) const;
  void setValue(DFP f);
  DFP magnitude();
  friend Vector normalize(Vector& v1);
  friend ostream& operator << (ostream& os, const Vector& v1);
  friend istream& operator >> (istream& is, Vector& v1);
private:
  DFP* p;
  int dim;
};

#endif
