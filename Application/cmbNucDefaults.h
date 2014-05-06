#ifndef __cmbNucDefaults_h
#define __cmbNucDefaults_h

#include <QObject>
#include <QPointer>


#define EASY_DEFAULT_PARAMS_MACRO()\
  FUN1(double, RadialMeshSize) \
  FUN1(double, AxialMeshSize) \
  FUN1(int, EdgeInterval)\
  FUN2(QString, RotateXYZ, double, RotateAngle, Rotate)\
  FUN1(QString, MeshType) \
  FUN1(double, Height) \
  FUN2(double, DuctThickX, double, DuctThickY, DuctThickness) \
  FUN2(double, PitchX, double, PitchY, Pitch) \
  FUN1(double, PinRadius)

class cmbNucDefaults : public QObject
{
  Q_OBJECT

public:
  cmbNucDefaults();
  ~cmbNucDefaults();
public:
#define FUN1(T,X)      \
  void set##X(T vin);  \
  bool has##X() const; \
  void clear##X();     \
  bool get##X(T& vout);
#define FUN2(T1, X, T2, Y, L) \
  void set##L(T1 v1, T2 v2);  \
  bool has##L() const;        \
  void clear##L();            \
  bool get##L(T1& v1, T2& v2);
  EASY_DEFAULT_PARAMS_MACRO()
#undef FUN1
#undef FUN2
  void set(cmbNucDefaults const& other);
signals:
  void calculatePitch();
  void calculatePinRadius();
  void recieveCalculatedPitch(double x, double y);
  void recieveRadius(double r);
protected:
#define FUN1(T, X) \
  struct X##Paired\
  {\
    T X; \
    bool valid; \
    X##Paired():valid(false){} \
    void set(T v)\
    { valid = true; X = v; } \
    void set(X##Paired v) \
    { valid = v.valid; X = v.X; }\
  } X;
#define FUN2(T1, X, T2, Y, L) \
  struct L##Paired\
  {\
    T1 X; T2 Y; \
    bool valid; \
    L##Paired():valid(false){} \
    void set(T1 v1, T2 v2)\
    { valid = true; X = v1; Y = v2; } \
    void set(L##Paired v) \
    { valid = v.valid; X = v.X; Y = v.Y;}\
  } L;
  EASY_DEFAULT_PARAMS_MACRO()
#undef FUN1
#undef FUN2
};
#endif
