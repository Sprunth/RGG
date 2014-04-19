#include "cmbNucMaterialColors.h"

#include <QSettings>
#include <QFileInfo>

#include "cmbNucAssembly.h"
#include "cmbNucPartDefinition.h"
#include "cmbNucMaterial.h"

#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkMath.h"

// QSettings material Group name
#define GROUP_MATERIAL "MaterialColors"

#define numInitialNewMaterialColors 42

static int initialNewMaterialColors[][3] = 
{
  {66,146,198},
  {241,105,19},
  {65,171,93},
  {239,59,44},
  {128,125,186},
  {115,115,115},  
  {198,219,239},
  {253,208,162},
  {199,233,192},
  {252,187,161},
  {218,218,235},
  {217,217,217}, 
  {8,81,156},
  {166,54,3},
  {0,109,44},
  {165,15,21},
  {84,39,143},
  {37,37,37},  
  {158,202,225},
  {253,174,107},
  {161,217,155},
  {252,146,114},
  {188,189,220},
  {189,189,189},  
  {33,113,181},
  {217,72,1},
  {35,139,69},
  {203,24,29},
  {106,81,163},
  {82,82,82},  
  {107,174,214},
  {253,141,60},
  {116,196,118},
  {251,106,74},
  {158,154,200},
  {150,150,150}, 
  {8,48,10},
  {127,39,4},
  {0,68,27},
  {103,0,13},
  {63,0,125},
  {0,0,0}
};

//----------------------------------------------------------------------------
cmbNucMaterialColors* cmbNucMaterialColors::Instance = 0;

cmbNucMaterialColors* cmbNucMaterialColors::instance()
{
  return cmbNucMaterialColors::Instance;
}

//-----------------------------------------------------------------------------
cmbNucMaterialColors::cmbNucMaterialColors(bool reset_instance)
  : Llimit(0.1), Ulimit(0.9), numNewMaterials(0)
{
  UnknownMaterial = new cmbNucMaterial("!!!!UnknownMaterial!!!!",
                                       "!!!!Unknown!!!!",
                                       QColor::fromRgbF(1.0,1.0,1.0));
  if (!cmbNucMaterialColors::Instance || reset_instance)
    {
    cmbNucMaterialColors::Instance = this;
    }
}

cmbNucMaterialColors::~cmbNucMaterialColors()
{
  this->clear();
  if (cmbNucMaterialColors::Instance == this)
    {
    cmbNucMaterialColors::Instance = 0;
    }
}

//-----------------------------------------------------------------------------
bool cmbNucMaterialColors::OpenFile(const QString& name)
{
  QFileInfo finfo(name);
  if(!finfo.exists())
    {
    return false;
    }

  QSettings settings(name, QSettings::IniFormat);
  if(!settings.childGroups().contains(GROUP_MATERIAL))
    {
    return false;
    }

  settings.beginGroup(GROUP_MATERIAL);
  QString settingKey(GROUP_MATERIAL), strcolor;
  settingKey.append("/");
  QStringList list1;
  foreach(QString mat, settings.childKeys())
    {
    strcolor = settings.value(mat).toString();
    list1 = strcolor.split(",");
    if(list1.size() == 4)
      {
      this->AddMaterial(mat,
        list1.value(0).toDouble(),
        list1.value(1).toDouble(),
        list1.value(2).toDouble(),
        list1.value(3).toDouble());
      }
    else if(list1.size() == 5) // with label
      {
      this->AddMaterial(mat, list1.value(0),
        list1.value(1).toDouble(),
        list1.value(2).toDouble(),
        list1.value(3).toDouble(),
        list1.value(4).toDouble());
      }
    }

  settings.endGroup();
  return true;
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::SaveToFile(const QString& name)
{
  QSettings settings(name, QSettings::IniFormat);
  QString settingKey(GROUP_MATERIAL), strcolor;
  settingKey.append("/");
  QColor color;
  /* TODO COLOR
  foreach(QString mat, this->MaterialColors.keys())
    {
    color = this->MaterialColors[mat].Color;
    strcolor = QString("%1, %2, %3, %4, %5").arg(
      this->MaterialColors[mat].Label).
      arg(color.redF()).arg(color.greenF()).arg(
      color.blueF()).arg(color.alphaF());
    settings.setValue(settingKey+mat,strcolor);
    }
   */
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::clear()
{
  foreach(QPointer<cmbNucMaterial> value, this->NameToMaterial)
  {
    delete value;
  }
  this->NameToMaterial.clear();
  this->LabelToMaterial.clear();
}

//-----------------------------------------------------------------------------
QPointer<cmbNucMaterial>
cmbNucMaterialColors
::getMaterialByName(QString& name)
{
  if(nameUsed(name)) return this->NameToMaterial.find(name).value();
  return UnknownMaterial;
}

//-----------------------------------------------------------------------------
QPointer<cmbNucMaterial>
cmbNucMaterialColors
::getMaterialByLabel(QString& label)
{
  if(labelUsed(label)) return this->LabelToMaterial.find(label).value();
  return UnknownMaterial;
}

//-----------------------------------------------------------------------------
QPointer<cmbNucMaterial> cmbNucMaterialColors::getUnknownMaterial() const
{
  return UnknownMaterial;
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::AddMaterial(const QString& name,
  const QString& label, const QColor& color)
{
  this->AddMaterial(name, label, color.redF(), color.greenF(),
    color.blueF(), color.alphaF());
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::AddMaterial(const QString& name,
  const QString& label, double r, double g, double b, double a)
{
  if(this->nameUsed(name) || this->labelUsed(label))
  {
    return;
  }
  QPointer< cmbNucMaterial > mat = new cmbNucMaterial( name, label,
                                                       QColor::fromRgbF(r, g,
                                                                        b, a) );

  this->NameToMaterial.insert(name, mat);
  this->LabelToMaterial.insert(label, mat);
  connect(mat, SIGNAL(nameHasChanged(QString, QPointer<cmbNucMaterial>)),
          this, SLOT(testAndRename(QString, QPointer<cmbNucMaterial>)));
  connect(mat, SIGNAL(labelHasChanged(QString, QPointer<cmbNucMaterial>)),
          this, SLOT(testAndRelabel(QString, QPointer<cmbNucMaterial>)));
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::AddMaterial(
  const QString& name, double r, double g, double b, double a)
{
  this->AddMaterial(name, name, r, g, b, a);
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::RemoveMaterialByName(const QString& name)
{
  QMap<QString, QPointer<cmbNucMaterial> >::iterator it =
    this->NameToMaterial.find(name);
  if(it != this->NameToMaterial.end())
    {
      if(it.value())
        {
        QString label = it.value()->getLabel();
        delete it.value();
        RemoveMaterialByLabel(label);
        }
    this->NameToMaterial.erase(it);
    }
}

//-----------------------------------------------------------------------------
bool cmbNucMaterialColors::nameUsed( const QString& name ) const
{
  return this->NameToMaterial.find(name) != this->NameToMaterial.find(name);
}

//-----------------------------------------------------------------------------
bool cmbNucMaterialColors::labelUsed( const QString& label ) const
{
  return this->LabelToMaterial.find(label) != this->LabelToMaterial.find(label);
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::RemoveMaterialByLabel(const QString& name)
{
  QMap<QString, QPointer<cmbNucMaterial> >::iterator it =
      this->LabelToMaterial.find(name);
  if(it != this->LabelToMaterial.end())
  {
    if(it.value())
    {
      QString name = it.value()->getName();
      delete it.value();
      RemoveMaterialByName(name);
    }
    this->LabelToMaterial.erase(it);
  }
}


//-----------------------------------------------------------------------------
void cmbNucMaterialColors::SetBlockMaterialColor(
  vtkCompositeDataDisplayAttributes *attributes, unsigned int flatIdx,
  const std::string& material)
{
#if 0
  QString bMaterial = QString(material.c_str()).toLower();
  if(this->MaterialColors.contains(bMaterial))
    {
    QColor bColor = this->MaterialColors[bMaterial].Color;
    bool visible = this->MaterialColors[bMaterial].Visible;
    double color[] = { bColor.redF(), bColor.greenF(), bColor.blueF() };
    attributes->SetBlockColor(flatIdx, color);
    attributes->SetBlockOpacity(flatIdx, bColor.alphaF());
    attributes->SetBlockVisibility(flatIdx, visible);
    }
  else
    {
    double color[] = { 255, 192, 203 }; // pink
    attributes->SetBlockColor(flatIdx, color);
    attributes->SetBlockVisibility(flatIdx, true);
    }
  #endif
}

//----------------------------------------------------------------------------
void cmbNucMaterialColors::CalcRGB(double &r, double &g, double &b)
{
  double l;
  while(1)
    {
    r = vtkMath::Random(0.0, 1.0);
    g = vtkMath::Random(0.0, 1.0);
    b = vtkMath::Random(0.0, 1.0);

    l = (0.11 *b) + (0.3 *r) + (0.59*g);
    if ((l >= this->Llimit) && ( l <= this->Ulimit))
      {
      return;
      }
    }
}

//----------------------------------------------------------------------------
void cmbNucMaterialColors::testAndRename(QString oldn,
                                         QPointer<cmbNucMaterial> material)
{
  QString newN = material->getName();
  if(this->nameUsed(newN))
  {
    material->revertName(oldn);
    return;
  }
  QMap<QString, QPointer<cmbNucMaterial> >::iterator it =
      this->NameToMaterial.find(newN);
  this->NameToMaterial.erase(it);
  this->NameToMaterial.insert(newN, material);
}

//----------------------------------------------------------------------------
void cmbNucMaterialColors::testAndRelabel(QString oldl,
                                          QPointer<cmbNucMaterial> material)
{
  QString newL = material->getLabel();
  if(this->nameUsed(newL))
  {
    material->revertLabel(oldl);
    return;
  }
  QMap<QString, QPointer<cmbNucMaterial> >::iterator it =
      this->LabelToMaterial.find(newL);
  this->LabelToMaterial.erase(it);
  this->LabelToMaterial.insert(newL, material);
}

//----------------------------------------------------------------------------
void cmbNucMaterialColors::AddMaterial(const QString& name,
                                       const QString& label)
{
  QColor color;
  if (this->numNewMaterials < numInitialNewMaterialColors)
    {
    color = QColor( initialNewMaterialColors[this->numNewMaterials][0],
                    initialNewMaterialColors[this->numNewMaterials][1],
                    initialNewMaterialColors[this->numNewMaterials][2]);
    ++this->numNewMaterials;
    }
  else
    {
    double r, g, b;
    this->CalcRGB(r, g, b);
    color = QColor::fromRgbF(r, g, b, 1.0);
    }

  this->AddMaterial(name, label, color);
}
