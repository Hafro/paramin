#include "vector.h"

Vector::Vector() {
  p = NULL;
  dim = 0;
}

Vector::Vector(int n) {
  int i;
  if (n < 1) {
    cerr << "Error in vector - invalid number of parameters in vector\n";
    exit(EXIT_FAILURE);
  }
  dim = n;
  p = new DFP[dim];
  for (i = 0; i < dim; i++)
    p[i] = 0.0;
}

Vector::Vector(DFP* f, int numVar) {
  int i;
  if (numVar < 1) {
    cerr << "Error in vector - invalid number of parameters in vector\n";
    exit(EXIT_FAILURE);
  }
  dim = numVar;
  p = new DFP[dim];
  for (i = 0; i < dim; i++)
    p[i] = f[i];
}

Vector::Vector(const Vector& v) {
  int i;
  if (v.dim == 0) {
    p = NULL;
    dim = 0;
  } else if (v.dim >= 1) {
    dim = v.dim;
    p = new DFP[dim];
    for (i = 0; i < dim; i++)
      p[i] = v[i];
  } else {
    cerr << "Error in vector - invalid number of parameters in vector\n";
    exit(EXIT_FAILURE);
  }
}

Vector::~Vector() {
  if (dim > 0) {
    delete[] p;
    p = NULL;
    dim = 0;
  }
}

Vector& Vector::operator = (const Vector& v) {
  int i;
  if (dim > 0) {
    delete[] p;
    p = NULL;
    dim = 0;
  }
  if (v.dim > 0) {
    p = new DFP [v.dim];
    dim = v.dim;  //What if v is NULL??????
    for (i = 0; i < dim; i++)
      p[i] = v[i];
  } else if (v.dim == 0) {
    p = NULL;
    dim = 0;
  } else {
    cerr << "Error in vector - invalid number of parameters in vector\n";
    exit(EXIT_FAILURE);
  }
  return (*this);
}

// Vector addition, subtraction and multiplication
Vector operator + (const Vector& v1, const Vector& v2) {
  assert(v1.dim == v2.dim);
  assert(v1.dim > 0);

  int i;
  Vector result(v1.dim);
  for (i = 0; i < v1.dim; i++)
    result.p[i] = v1.p[i] + v2.p[i];
  return result;
}

Vector operator - (const Vector& v1, const Vector& v2) {
  assert(v1.dim == v2.dim);
  assert(v1.dim > 0);

  int i;
  Vector result(v1.dim);
  for (i = 0; i < v1.dim; i++)
    result.p[i] = v1.p[i] - v2.p[i];
  return result;
}

Vector operator - (const Vector& v1) {
  assert(v1.dim > 0);

  int i;
  Vector result(v1.dim);
  for (i = 0; i < v1.dim; i++)
    result.p[i] = -v1.p[i];
  return result;
}

Vector operator * (DFP a, const Vector& v1) {
  assert(v1.dim > 0);

  int i;
  Vector result(v1.dim);
  for (i = 0; i < v1.dim; i++)
    result.p[i] = a * v1.p[i];
  return result;
}

Vector operator * (const Vector& v1, DFP a) {
  assert(v1.dim > 0);

  int i;
  Vector result(v1.dim);
  for (i = 0; i < v1.dim; i++)
    result.p[i] = v1.p[i] * a;
  return result;
}

DFP operator * (const Vector& v1, const Vector& v2) {
  assert(v1.dim == v2.dim);
  assert(v1.dim > 0);

  int i;
  DFP result = 0.0;
  for (i = 0; i < v1.dim; i++)
    result += (v1.p[i] * v2.p[i]);
  return result;
}

int operator < (const Vector& v1, const Vector& v2) {
  assert(v1.dim == v2.dim);
  assert(v1.dim > 0);

  int i;
  for (i = 0; i < v1.dim; i++)
    if (v1[i] < v2[i])
      return 1;

  return 0;
}

int operator > (const Vector& v1, const Vector& v2) {
  assert(v1.dim == v2.dim);
  assert(v1.dim > 0);

  int i;
  for (i = 0; i < v1.dim; i++)
    if (v1[i] > v2[i])
      return 1;

  return 0;
}

int operator == (const Vector& v1, const Vector& v2) {
  int i;
  if ((v1.dim == 0) && (v2.dim == 0)) {
    return 1;
  } else if ((v1.dim < 0) || (v2.dim < 0)) {
    return 0;
  } else if (v1.dim != v2.dim) {
    return 0;

  } else {
    for (i = 0; i < v1.dim; i++)
      if (v1[i] != v2[i])
        return 0;
    return 1;
  }
}

int operator != (const Vector& v1, const Vector& v2) {
  return !(v1 == v2);
}

DFP& Vector::operator [] (int i) const {
  assert(dim > 0);
  if ((i < 0) || (i >= dim)) {
    cerr << "Error in vector - invalid reference to vector\n";
    exit(EXIT_FAILURE);
  }
  return (p[i]);
}

DFP Vector::magnitude() {
  assert(dim > 0);
  int i;
  DFP m = 0.0;
  for (i = 0; i < dim; i++)
    m += p[i] * p[i];
  return sqrt(m);
}

Vector normalize(Vector& v1) {
  assert(v1.dimension() > 0);
  int i;
  Vector newVector(v1.dimension());
  DFP m = v1.magnitude();
  if (m == 0) {
    cerr << "Error in vector - cannot normalize vector with zero magnitude\n";
    exit(EXIT_FAILURE);
  }
  for (i = 0; i < v1.dimension(); i++)
    newVector[i] = (v1[i] / m);
  return newVector;
}

istream& operator >> (istream& is, Vector& v1) {
  int i;
  char tmp; // = ' ';
  for (i = 0; i < v1.dim; i++) {
    if (i == v1.dim - 1)
      is >> v1[i];
    else
      is >> v1[i] >> tmp;
  }
  return is;
}

ostream& operator << (ostream& os, const Vector& v) {
  int i;
  for (i = 0; i < v.dim; i++)
    os << v[i] << sep;
  os << endl;
  return os;
}

int Vector::dimension() const {
  return dim;
}

void Vector::setValue(DFP f) {
  int i;
  for (i = 0; i < dim; i++)
    p[i] = f;
}
