#ifndef __cmbNucMaterialColors_h
#define __cmbNucMaterialColors_h

#include <QMap>
#include <QString>
#include <QColor>
#include <QPair>

class cmbNucAssembly;
class vtkCompositeDataDisplayAttributes;

class cmbNucMaterialColors
{
public:

  // Get the global instance for the cmbNucMaterialColors.
  static cmbNucMaterialColors* instance();

  cmbNucMaterialColors();
  virtual ~cmbNucMaterialColors();

  QMap<QString, QPair<QString, QColor> >& MaterialColorMap()
  {return this->MaterialColors;}
  void AddMaterial(const QString& name, const QString& label,
    const QColor& color)
    {
    this->AddMaterial(name, label, color.redF(), color.greenF(),
      color.blueF(), color.alphaF());
    }
  void AddMaterial(const QString& name, const QString& label,
    double r, double g, double b, double a)
  {this->MaterialColors.insert(name, qMakePair(label,
    QColor::fromRgbF(r, g, b, a)));}
  void AddMaterial(const QString& name, double r, double g, double b, double a)
  {this->AddMaterial(name, name, r, g, b, a);}
  void RemoveMaterial(const QString& name)
  {
    QMap<QString, QPair<QString, QColor> >::iterator it =
      this->MaterialColors.find(name);
    if(it != this->MaterialColors.end())
      {
      this->MaterialColors.erase(it);
      }
  }
  void GetAssemblyMaterials(
    cmbNucAssembly* assy, QMap<std::string, std::string>& materials);
  void SetBlockMaterialColor(
    vtkCompositeDataDisplayAttributes *attributes, unsigned int flatIdx,
    const std::string& material);

  // open a material-file(.ini) in QSettings' ini format
  bool OpenFile(const QString& name);

  // save materials to a file
  void SaveToFile(const QString& name);

private:

  static cmbNucMaterialColors* Instance;
  // <Name, <Label, Color> >
  QMap<QString, QPair<QString, QColor> > MaterialColors;
};

#endif
