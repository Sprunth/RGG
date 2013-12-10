#ifndef __cmbNucMaterialColors_h
#define __cmbNucMaterialColors_h

#include <QMap>
#include <QString>
#include <QColor>
#include <QPair>

class cmbNucAssembly;
class vtkCompositeDataDisplayAttributes;

struct cmbNucMaterial
{
  cmbNucMaterial()
    : Visible(true) {}
  cmbNucMaterial(const QString& label, const QColor& color)
    : Visible(true), Label(label), Color(color) {}

  QString Label;
  QColor Color;
  bool Visible;
};

class cmbNucMaterialColors
{
public:

  // Get the global instance for the cmbNucMaterialColors.
  static cmbNucMaterialColors* instance();

  cmbNucMaterialColors();
  virtual ~cmbNucMaterialColors();

  QMap<QString, cmbNucMaterial>& MaterialColorMap();

  void AddMaterial(const QString& name, const QString& label,
                   const QColor& color);
  void AddMaterial(const QString& name, const QString& label,
                   double r, double g, double b, double a);
  void AddMaterial(const QString& name, double r, double g, double b, double a);
  void AddMaterial(const QString& name, const QString& label);
  void RemoveMaterial(const QString& name);
  void SetMaterialVisibility(const QString& name, bool visible);

  void GetAssemblyMaterials(
    cmbNucAssembly* assy, QMap<std::string, std::string>& materials);
  void SetBlockMaterialColor(
    vtkCompositeDataDisplayAttributes *attributes, unsigned int flatIdx,
    const std::string& material);

  // open a material-file(.ini) in QSettings' ini format
  bool OpenFile(const QString& name);

  // save materials to a file
  void SaveToFile(const QString& name);

  void CalcRGB(double &r, double &g, double &b);
private:

  static cmbNucMaterialColors* Instance;
  // <Name, <Label, Color> >
  QMap<QString, cmbNucMaterial> MaterialColors;
  double Ulimit, Llimit;  // luminance range when creating colors
};

#endif
