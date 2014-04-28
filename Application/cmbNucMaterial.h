#ifndef _cmbNucMaterial_h_
#define _cmbNucMaterial_h_

#include <QObject>
#include <QString>
#include <QColor>
#include <QPointer>

class cmbNucMaterialColors;

class cmbNucMaterial;

class cmbNucMaterialLayer;

class cmbNucMaterialLayerConnection: public QObject
{
  Q_OBJECT
public:
  cmbNucMaterialLayer * v;
  void emitMaterialChange()
  {
    emit materialChanged();
  }
  void emitColorChange()
  {
    emit colorChanged();
  }
signals:
  void materialChanged();
  void colorChanged();
public slots:
  void materialDeleted();
};

class cmbNucMaterialLayer
{
public:
  friend class cmbNucMaterialLayerConnection;
  cmbNucMaterialLayer();
  cmbNucMaterialLayer( const cmbNucMaterialLayer & ml );
  ~cmbNucMaterialLayer();
  void changeMaterial(QPointer<cmbNucMaterial> m);
  QPointer<cmbNucMaterial> getMaterial() const;
  cmbNucMaterialLayerConnection * GetConnection()
  {
    return Connection;
  }
  double * getThickness();
  double const* getThickness() const;
  bool operator==( const cmbNucMaterialLayer & other ) const;
protected:
  QPointer<cmbNucMaterial> Material;
  double Thickness[2];
  void materialDeleted();
  cmbNucMaterialLayerConnection * Connection;
};

class cmbNucMaterial: public QObject
{
  Q_OBJECT
public:
  friend class cmbNucMaterialColors;

  bool isVisible() const;

  QString getName() const;

  QString getLabel() const;

  QColor getColor() const;

  void inc();
  void dec();
  bool isUsed();

  void emitMaterialChange();
  void emitColorChange();

public slots:
  void setVisible(bool b);
  void setName(QString s);
  void setLabel(QString s);
  void setColor(QColor c);

signals:
  void hasBeenDeleted();
  void invalidName();
  void invalidLabel();
  void materialChanged();
  void colorChanged();
  void nameHasChanged(QString oldN, QPointer<cmbNucMaterial> mat);
  void labelHasChanged(QString oldL, QPointer<cmbNucMaterial> mat);

protected:
  QString Name;
  QString Label;
  QColor Color;
  bool Visible;

  unsigned int NumberReferenced;

  cmbNucMaterial();
  cmbNucMaterial(const QString& name, const QString& label, const QColor& color);

  void revertName(QString name);
  void revertLabel(QString label);

  virtual ~cmbNucMaterial();

};

#endif
