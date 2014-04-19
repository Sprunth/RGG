#ifndef _cmbNucMaterial_h_
#define _cmbNucMaterial_h_

#include <QObject>
#include <QString>
#include <QColor>
#include <QPointer>

class cmbNucMaterialColors;

class cmbNucMaterial: public QObject
{
  Q_OBJECT
public:
  friend class cmbNucMaterialColors;

  bool isVisible() const;

  bool isValid() const;

  QString getName() const;

  QString getLabel() const;

  QColor getColor() const;

  void inc();
  void dec();
  bool isUsed();

public slots:
  void setVisible(bool b);
  void setName(QString s);
  void setLabel(QString s);
  void setColor(QColor c);

signals:
  void hasBeenDeleted();
  void invalidName();
  void invalidLabel();
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
