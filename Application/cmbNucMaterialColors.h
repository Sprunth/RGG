#ifndef __cmbNucMaterialColors_h
#define __cmbNucMaterialColors_h

#include <QMap>
#include <QString>
#include <QColor>
#include <QPair>
#include <QObject>
#include <QPointer>

class cmbNucAssembly;
class cmbNucMaterial;
class vtkCompositeDataDisplayAttributes;

class cmbNucMaterialColors: public QObject
{
  Q_OBJECT
public:

  // Get the global instance for the cmbNucMaterialColors.
  static cmbNucMaterialColors* instance();

  cmbNucMaterialColors(bool reset_instance = false);
  virtual ~cmbNucMaterialColors();

  void clear();

  QPointer<cmbNucMaterial> getMaterialByName(QString& name);
  QPointer<cmbNucMaterial> getMaterialByLabel(QString& label);

  QPointer<cmbNucMaterial> getUnknownMaterial() const;

  void AddMaterial(const QString& name, const QString& label,
                   const QColor& color);
  void AddMaterial(const QString& name, const QString& label,
                   double r, double g, double b, double a);
  void AddMaterial(const QString& name, double r, double g, double b, double a);
  void AddMaterial(const QString& name, const QString& label);
  void RemoveMaterialByName(const QString& name);
  void RemoveMaterialByLabel(const QString& label);

  bool nameUsed( const QString& name ) const;
  bool labelUsed( const QString& label ) const;

  void SetBlockMaterialColor(
    vtkCompositeDataDisplayAttributes *attributes, unsigned int flatIdx,
    const std::string& materialName);

  // open a material-file(.ini) in QSettings' ini format
  bool OpenFile(const QString& name);

  // save materials to a file
  void SaveToFile(const QString& name);

  void CalcRGB(double &r, double &g, double &b);

signals:
  void materialChanged();

protected slots:
  void testAndRename(QString oldn, QPointer<cmbNucMaterial> material);
  void testAndRelabel(QString oldl, QPointer<cmbNucMaterial> material);
private:

  static cmbNucMaterialColors* Instance;

  QPointer< cmbNucMaterial > UnknownMaterial;

  QMap<QString, QPointer<cmbNucMaterial> > NameToMaterial;
  QMap<QString, QPointer<cmbNucMaterial> > LabelToMaterial;
  double Ulimit, Llimit;  // luminance range when creating colors
  int numNewMaterials;
};

#endif
