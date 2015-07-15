#ifndef __cmbNucDefaults_h
#define __cmbNucDefaults_h

#include <QObject>
#include <QPointer>
#include <QString>

#include <vector>

#define EASY_DEFAULT_PARAMS_MACRO()\
  FUN1(double, AxialMeshSize) \
  FUN1(int, EdgeInterval)\
  FUN1(QString, MeshType) \
  FUN1(double, Z0) \
  FUN1(double, Height) \
  FUN1(QString, UserDefined) \
  FUN2(double, DuctThickX, double, DuctThickY, DuctThickness) \

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
signals:
  void calculatePitch();
  void recieveCalculatedPitch(double x, double y);
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
