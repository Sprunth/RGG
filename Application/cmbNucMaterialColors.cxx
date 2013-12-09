#include "cmbNucMaterialColors.h"

#include <QSettings>
#include <QFileInfo>

#include "cmbNucAssembly.h"
#include "cmbNucPartDefinition.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkMath.h"

// QSettings material Group name
#define GROUP_MATERIAL "MaterialColors"

//----------------------------------------------------------------------------
cmbNucMaterialColors* cmbNucMaterialColors::Instance = 0;

cmbNucMaterialColors* cmbNucMaterialColors::instance()
{
  return cmbNucMaterialColors::Instance;
}

//-----------------------------------------------------------------------------
cmbNucMaterialColors::cmbNucMaterialColors(): Llimit(0.1), Ulimit(0.9)
{
  if (!cmbNucMaterialColors::Instance)
    {
    cmbNucMaterialColors::Instance = this;
    }
}
cmbNucMaterialColors::~cmbNucMaterialColors()
{
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
  foreach(QString mat, this->MaterialColors.keys())
    {
    color = this->MaterialColors[mat].Color;
    strcolor = QString("%1, %2, %3, %4, %5").arg(
      this->MaterialColors[mat].Label).
      arg(color.redF()).arg(color.greenF()).arg(
      color.blueF()).arg(color.alphaF());
    settings.setValue(settingKey+mat,strcolor);
    }
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::GetAssemblyMaterials(
  cmbNucAssembly* assy, QMap<std::string, std::string>& materials)
{
  if(!assy)
    {
    return;
    }
  // ducts
  std::string strMat;
  for(size_t i = 0; i < assy->AssyDuct.Ducts.size(); i++)
    {
    Duct *duct = assy->AssyDuct.Ducts[i];
    for(size_t j = 0; j < duct->materials.size(); j++)
      {
      strMat = duct->materials[j];
      if(!strMat.empty() && strMat != " " && !materials.contains(strMat))
        {
        materials[strMat] =
          this->MaterialColors[strMat.c_str()].Label.toStdString();
        }
      }
    }

  // pincells
  for(size_t i = 0; i < assy->PinCells.size(); i++)
    {
    PinCell* pincell = assy->PinCells[i];
    for(size_t j = 0; j < pincell->cylinders.size(); j++)
      {
      Cylinder* cylinder = pincell->cylinders[j];
      for(int material = 0; material < cylinder->materials.size(); material++)
        {
        strMat = cylinder->materials[material];
        if(!strMat.empty() && strMat != " " && !materials.contains(strMat))
          {
          materials[strMat] =
            this->MaterialColors[strMat.c_str()].Label.toStdString();
          }
        }
      }

    for(size_t j = 0; j < pincell->frustums.size(); j++)
      {
      Frustum* frustum = pincell->frustums[j];
      for(int material = 0; material < frustum->materials.size(); material++)
        {
        strMat = frustum->materials[material];
        if(!strMat.empty() && strMat != " " && !materials.contains(strMat))
          {
          materials[strMat] =
            this->MaterialColors[strMat.c_str()].Label.toStdString();
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
QMap<QString, cmbNucMaterial>& cmbNucMaterialColors::MaterialColorMap()
{
  return this->MaterialColors;
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
  this->MaterialColors.insert(name, cmbNucMaterial(label,
    QColor::fromRgbF(r, g, b, a)));
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::AddMaterial(
  const QString& name, double r, double g, double b, double a)
{
  this->AddMaterial(name, name, r, g, b, a);
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::RemoveMaterial(const QString& name)
{
  QMap<QString, cmbNucMaterial>::iterator it =
    this->MaterialColors.find(name);
  if(it != this->MaterialColors.end())
    {
    this->MaterialColors.erase(it);
    }
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::SetMaterialVisibility(
  const QString& name, bool visible)
{
  if(this->MaterialColors.contains(name))
    {
    this->MaterialColors[name].Visible = visible;
    }
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::SetBlockMaterialColor(
  vtkCompositeDataDisplayAttributes *attributes, unsigned int flatIdx,
  const std::string& material)
{
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
void cmbNucMaterialColors::AddMaterial(const QString& name, const QString& label)
{
  double r, g, b;
  this->CalcRGB(r, g, b);
  this->MaterialColors.insert(name, cmbNucMaterial(label,
    QColor::fromRgbF(r, g, b, 1.0)));
}
