#include "cmbNucDefaults.h"

cmbNucDefaults::cmbNucDefaults()
{
}

cmbNucDefaults::~cmbNucDefaults()
{
}

#define FUN1(T, X)                   \
void cmbNucDefaults::set##X(T vin)   \
{                                    \
  X.set(vin);                        \
}                                    \
bool cmbNucDefaults::get##X(T& vout) \
{                                    \
  vout = X.X;                        \
  return X.valid;                    \
}
#define FUN2(T1, X, T2, Y, L)               \
void cmbNucDefaults::set##L(T1 v1, T2 v2)   \
{                                           \
  L.set(v1, v2);                            \
}                                           \
bool cmbNucDefaults::get##L(T1& v1, T2& v2) \
{                                           \
  v1 = L.X; v2 = L.Y;                       \
  return L.valid;                           \
}

EASY_DEFAULT_PARAMS_MACRO()
#undef FUN1
#undef FUN2

void cmbNucDefaults::set(cmbNucDefaults const& other)
{
#define FUN1(T, X) X.set(other.X);
#define FUN2(T1, X, T2, Y, L) L.set(other.L);
  EASY_DEFAULT_PARAMS_MACRO()
#undef FUN1
#undef FUN2
}

void cmbNucDefaults::recieveCalculatedPitch(double x, double y)
{
  Pitch.set(x,y);
}

void cmbNucDefaults::recieveRadius(double r)
{
  PinRadius.set(r);
}
