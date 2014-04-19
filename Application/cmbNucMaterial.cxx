#include "cmbNucMaterial.h"

cmbNucMaterial::cmbNucMaterial()
: Visible(true), NumberReferenced(0)
{}

cmbNucMaterial::cmbNucMaterial(const QString& name,
                               const QString& label, const QColor& color)
: Name(name), Label(label), Color(color), Visible(true), NumberReferenced(0)
{}

cmbNucMaterial::~cmbNucMaterial()
{
  emit hasBeenDeleted();
}

bool cmbNucMaterial::isVisible() const
{
  return this->Visible;
}

QString cmbNucMaterial::getName() const
{
  return this->Name;
}

QString cmbNucMaterial::getLabel() const
{
  return this->Label;
}

QColor cmbNucMaterial::getColor() const
{
  return this->Color;
}

void cmbNucMaterial::setVisible(bool b)
{
  this->Visible = b;
}

void cmbNucMaterial::setName(QString s)
{
  QString old = this->Name;
  if(old != s)
  {
    this->Name = s;
    emit nameHasChanged(old, this);
  }
}

void cmbNucMaterial::setLabel(QString s)
{
  QString old = this->Label;
  if(old != s)
  {
    this->Label = s;
    emit labelHasChanged(old, this);
  }
}

void cmbNucMaterial::setColor(QColor c)
{
  this->Color = c;
}

void cmbNucMaterial::inc()
{
  NumberReferenced++;
}

void cmbNucMaterial::dec()
{
  if(NumberReferenced != 0)
    NumberReferenced--;
}

bool cmbNucMaterial::isUsed()
{
  return NumberReferenced != 0;
}

void cmbNucMaterial::revertName(QString name)
{
  this->Name = name;
  emit invalidName();
}

void cmbNucMaterial::revertLabel(QString label)
{
  this->Label = label;
  emit invalidLabel();
}
