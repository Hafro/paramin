#include "vector.h"

vector::vector() {
  p = NULL;
  dim = 0;
}

vector::vector(int n) {
  int i;
  if (n < 1) {
    cerr << "Error in vector - illegal number of parameters in vector\n";
    exit(EXIT_FAILURE);
  }
  dim = n;
  p = new DFP[dim];
  for (i = 0; i < dim; i++)
    p[i] = 0.0;
}

vector::vector(DFP* f, int numVar) {
  int i;
  if (numVar < 1) {
    cerr << "Error in vector - illegal number of parameters in vector\n";
    exit(EXIT_FAILURE);
  }
  dim = numVar;
  p = new DFP[dim];
  for (i = 0; i < dim; i++)
    p[i] = f[i];
}

vector::vector(const vector& v) {
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
    cerr << "Error in vector - illegal number of parameters in vector\n";
    exit(EXIT_FAILURE);
  }
}

vector::~vector() {
  if (dim > 0) {
    delete[] p;
    p = NULL;
    dim = 0;
  }
}

vector& vector::operator = (const vector& v) {
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
    cerr << "Error in vector - illegal number of parameters in vector\n";
    exit(EXIT_FAILURE);
  }
  return (*this);
}

// Vector addition, subtraction and multiplication
vector operator + (const vector& v1, const vector& v2) {
  assert(v1.dim == v2.dim);
  assert(v1.dim > 0);

  int i;
  vector result(v1.dim);
  for (i = 0; i < v1.dim; i++)
    result.p[i] = v1.p[i] + v2.p[i];
  return result;
}

vector operator - (const vector& v1, const vector& v2) {
  assert(v1.dim == v2.dim);
  assert(v1.dim > 0);

  int i;
  vector result(v1.dim);
  for (i = 0; i < v1.dim; i++)
    result.p[i] = v1.p[i] - v2.p[i];
  return result;
}

vector operator - (const vector& v1) {
  assert(v1.dim > 0);

  int i;
  vector result(v1.dim);
  for (i = 0; i < v1.dim; i++)
    result.p[i] = -v1.p[i];
  return result;
}

vector operator * (DFP a, const vector& v1) {
  assert(v1.dim > 0);

  int i;
  vector result(v1.dim);
  for (i = 0; i < v1.dim; i++)
    result.p[i] = a * v1.p[i];
  return result;
}

vector operator * (const vector& v1, DFP a) {
  assert(v1.dim > 0);

  int i;
  vector result(v1.dim);
  for (i = 0; i < v1.dim; i++)
    result.p[i] = v1.p[i] * a;
  return result;
}

DFP operator * (const vector& v1, const vector& v2) {
  assert(v1.dim == v2.dim);
  assert(v1.dim > 0);

  int i;
  DFP result = 0.0;
  for (i = 0; i < v1.dim; i++)
    result += (v1.p[i] * v2.p[i]);
  return result;
}

int operator < (const vector& v1, const vector& v2) {
  assert(v1.dim == v2.dim);
  assert(v1.dim > 0);

  int i;
  for (i = 0; i < v1.dim; i++)
    if (v1[i] < v2[i])
      return 1;

  return 0;
}

int operator > (const vector& v1, const vector& v2) {
  assert(v1.dim == v2.dim);
  assert(v1.dim > 0);

  int i;
  for (i = 0; i < v1.dim; i++)
    if (v1[i] > v2[i])
      return 1;

  return 0;
}

int operator == (const vector& v1, const vector& v2) {
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

int operator != (const vector& v1, const vector& v2) {
  return !(v1 == v2);
}

DFP& vector::operator [] (int i) const {
  assert(dim > 0);
  if ((i < 0) || (i >= dim)) {
    cerr << "Error in vector - illegal reference to vector\n";
    exit(EXIT_FAILURE);
  }
  return (p[i]);
}

DFP vector::magnitude() {
  assert(dim > 0);
  int i;
  DFP m = 0.0;
  for (i = 0; i < dim; i++)
    m += p[i] * p[i];
  return sqrt(m);
}

vector normalize(vector& v1) {
  assert(v1.dimension() > 0);
  int i;
  vector newVector(v1.dimension());
  DFP m = v1.magnitude();
  if (m == 0) {
    cerr << "Error in vector - cannot normalize vector with zero magnitude\n";
    exit(EXIT_FAILURE);
  }
  for (i = 0; i < v1.dimension(); i++)
    newVector[i] = (v1[i] / m);
  return newVector;
}

istream& operator >> (istream& is, vector& v1) {
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

ostream& operator << (ostream& os, const vector& v) {
  int i;
  for (i = 0; i < v.dim; i++)
    os << v[i] << sep;
  os << endl;
  return os;
}

int vector::dimension() const {
  return dim;
}

void vector::setValue(DFP f) {
  int i;
  for (i = 0; i < dim; i++)
    p[i] = f;
}
