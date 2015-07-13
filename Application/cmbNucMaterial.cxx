#include "cmbNucMaterial.h"
#include "cmbNucMaterialColors.h"
#include <QDebug>

void cmbNucMaterialLayerConnection::materialDeleted()
{
  v->materialDeleted();
}

cmbNucMaterialLayer::cmbNucMaterialLayer()
:Material(NULL), Connection(NULL)
{
  this->Connection = new cmbNucMaterialLayerConnection();
  this->Connection->v = this;
  this->Thickness[0] = 1.0;
  this->Thickness[1] = 1.0;
  this->changeMaterial(cmbNucMaterialColors::instance()->getUnknownMaterial());
}

cmbNucMaterialLayer::cmbNucMaterialLayer( const cmbNucMaterialLayer & ml )
:Material(NULL), Connection(NULL)
{
  this->Connection = new cmbNucMaterialLayerConnection();
  this->Connection->v = this;
  this->changeMaterial(ml.Material);
  this->Thickness[0] = ml.Thickness[0];
  this->Thickness[1] = ml.Thickness[1];
}

cmbNucMaterialLayer::~cmbNucMaterialLayer()
{
  if(Material) this->Material->dec();
  delete Connection;
  Connection = NULL;
}

void cmbNucMaterialLayer::changeMaterial(QPointer<cmbNucMaterial> m)
{

  if(m == NULL) m = cmbNucMaterialColors::instance()->getUnknownMaterial();
  if(m != this->Material)
  {
    if(Material)
    {
      this->Material->dec();
      QObject::disconnect( this->Material, SIGNAL(hasBeenDeleted()),
                           this->Connection, SLOT(materialDeleted()) );
      QObject::disconnect( this->Material, SIGNAL(materialChanged()),
                           this->Connection, SIGNAL(materialChanged()) );
      QObject::disconnect( this->Material, SIGNAL(colorChanged()),
                           this->Connection, SIGNAL(colorChanged()) );
    }
    this->Material = m;
    QObject::connect( this->Material, SIGNAL(hasBeenDeleted()),
                      this->Connection, SLOT(materialDeleted()) );
    QObject::connect( this->Material, SIGNAL(materialChanged()),
                      this->Connection, SIGNAL(materialChanged()) );
    QObject::connect( this->Material, SIGNAL(colorChanged()),
                      this->Connection, SIGNAL(colorChanged()) );
    this->Material->inc();
    this->Connection->emitMaterialChange();
  }
}

QPointer<cmbNucMaterial>
cmbNucMaterialLayer::getMaterial() const
{
  return this->Material;
}

void cmbNucMaterialLayer::materialDeleted()
{
  cmbNucMaterialColors * colors = cmbNucMaterialColors::instance();
  if(colors!=NULL)
  {
    this->changeMaterial(colors->getUnknownMaterial());
  }
}

double * cmbNucMaterialLayer::getThickness()
{
  return this->Thickness;
}

double const* cmbNucMaterialLayer::getThickness() const
{
  return this->Thickness;
}

bool cmbNucMaterialLayer::operator==( const cmbNucMaterialLayer & other ) const
{
  return  this->Material == other.Material &&
          this->Thickness[0] == other.Thickness[0] &&
          this->Thickness[1] == other.Thickness[1];
}

void cmbNucMaterialLayer::operator=( const cmbNucMaterialLayer & other )
{
  this->Thickness[0] = other.Thickness[0];
  this->Thickness[1] = other.Thickness[1];
  this->changeMaterial(other.Material);
}

cmbNucMaterial::cmbNucMaterial(const QString& name,
                               const QString& label, const QColor& color)
: Name(name), Label(label), Color(color), Visible(true), NumberReferenced(0)
{
  IsDisplayed[MODEL] = false;
  IsDisplayed[MESH] = false;
}

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
  if(NumberReferenced == 1)
  {
    emit useChanged();
  }
}

void cmbNucMaterial::dec()
{
  if(NumberReferenced != 0)
  {
    NumberReferenced--;
    if(NumberReferenced == 0)
    {
      this->IsDisplayed[MODEL] = false;
      this->IsDisplayed[MESH] = false;
      emit useChanged();
    }
  }
}

bool cmbNucMaterial::isUsed()
{
  return NumberReferenced != 0;
}

void cmbNucMaterial::emitMaterialChange()
{
  emit materialChanged();
}

void cmbNucMaterial::emitColorChange()
{
  emit colorChanged();
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

bool cmbNucMaterial::isDisplayed()
{
  return IsDisplayed[MODEL] || IsDisplayed[MESH];
}

void cmbNucMaterial::clearDisplayed(cmbNucMaterial::DisplayMode mode)
{
  IsDisplayed[mode] = false;
}

void cmbNucMaterial::setDisplayed(cmbNucMaterial::DisplayMode mode)
{
  IsDisplayed[mode] = true;
}
