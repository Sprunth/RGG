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
class cmbNucMaterialTreeItem;
class vtkCompositeDataDisplayAttributes;

class QComboBox;
class QTreeWidget;

class cmbNucMaterialColors: public QObject
{
  Q_OBJECT
public:

  // Get the global instance for the cmbNucMaterialColors.
  static cmbNucMaterialColors* instance();

  cmbNucMaterialColors(bool reset_instance = false);
  virtual ~cmbNucMaterialColors();

  void clear();

  QPointer<cmbNucMaterial> getMaterialByName(QString const& name) const;
  QPointer<cmbNucMaterial> getMaterialByLabel(QString const& label) const;

  QPointer<cmbNucMaterial> getUnknownMaterial() const;

  QPointer<cmbNucMaterial> getMaterial(QComboBox *comboBox) const;

  QPointer<cmbNucMaterial> AddMaterial(const QString& name,
                                       const QString& label,
                                       const QColor& color);
  QPointer<cmbNucMaterial> AddMaterial(const QString& name,
                                       const QString& label,
                                       double r, double g,
                                       double b, double a);
  QPointer<cmbNucMaterial> AddMaterial(const QString& name,
                                       double r, double g,
                                       double b, double a);
  QPointer<cmbNucMaterial> AddMaterial(const QString& name,
                                       const QString& label);

  void RemoveMaterialByName(const QString& name);
  void RemoveMaterialByLabel(const QString& label);

  bool nameUsed( const QString& name ) const;
  bool labelUsed( const QString& label ) const;

  void SetBlockMaterialColor(vtkCompositeDataDisplayAttributes *attributes,
                             unsigned int flatIdx,
                             QPointer<cmbNucMaterial> material);

  void setUp(QComboBox *comboBox) const;
  void selectIndex(QComboBox *comboBox, QPointer<cmbNucMaterial>) const;

  // open a material-file(.ini) in QSettings' ini format
  bool OpenFile(const QString& name);

  // save materials to a file
  void SaveToFile(const QString& name);

  void CalcRGB(double &r, double &g, double &b);

  void buildTree(QTreeWidget * tree);

public slots:
  void showJustUsed(bool);
  void CreateNewMaterial();
  void deleteSelected();

signals:
  void materialChanged();
  void materialSelected(QPointer<cmbNucMaterial>);

protected slots:
  void testAndRename(QString oldn, QPointer<cmbNucMaterial> material);
  void testAndRelabel(QString oldl, QPointer<cmbNucMaterial> material);
  void sendMaterialFromName(QString const& name);
  void sendMaterialFromLabel(QString const& label);

signals:
  void showJustUsedSig(bool);

private:

  QString generateString(QString prefix, QMap<QString, QPointer<cmbNucMaterial> > const& );

  static cmbNucMaterialColors* Instance;

  cmbNucMaterialTreeItem * addToTree(QPointer<cmbNucMaterial>);

  QTreeWidget* MaterialTree;

  QPointer< cmbNucMaterial > UnknownMaterial;
  cmbNucMaterialTreeItem * UnknownMaterialTreeItem;

  QMap<QString, QPointer<cmbNucMaterial> > NameToMaterial;
  QMap<QString, QPointer<cmbNucMaterial> > LabelToMaterial;
  double Ulimit, Llimit;  // luminance range when creating colors
  int numNewMaterials;
  int newID;
  bool justUsed;
};

#endif
