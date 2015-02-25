#ifndef cmbNucCoregen_H
#define cmbNucCoregen_H

#include <QDialog>
#include <QString>
#include <QComboBox>
#include <QCheckBox>
#include <QObject>
#include <QList>
#include <QTreeWidgetItem>
#include <QColor>
#include <vtkSmartPointer.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkPolyData.h>
#include "cmbNucPartDefinition.h"
#include "cmbNucCore.h"
#include "ui_qCoregenModel.h"
#include <vector>
#include <map>

class vtkMoabReader;
class vtkGeometryFilter;

class cmbNucCoregen : public QObject
{
  Q_OBJECT
public:

  cmbNucCoregen();
  ~cmbNucCoregen();

  vtkSmartPointer<vtkDataObject> getData();

  bool isSubSection()
  { return subSection != -1; }

  unsigned int getSelectedType() const
  { return selectedType; }

  void clear();

  void exportVisible(QString outFname,
                     std::vector<std::string> const& remove );

  void getColor(int i, QColor & color, bool & visible);

  unsigned int numberOfParts();

  void computeBounds(vtkBoundingBox * box);

public slots:
  void openFile(QString file);
  void selectionChanged(QTreeWidgetItem *);
  void valueChanged(QTreeWidgetItem *);
  void setColor(int);
  void rootChanged(int r);

signals:
  void error(QString);
  void update();
  void fileOpen(bool);
  void components(QList<QTreeWidgetItem*>);
  void components(QStringList parts, int sel);
  void resetCamera();

private:
  // Designer form
  vtkSmartPointer<vtkMoabReader> MoabReader;
  std::vector< vtkSmartPointer<vtkGeometryFilter> > GeoFilt;
  vtkSmartPointer<vtkDataObject> Data;
  std::vector< vtkSmartPointer<vtkDataObject> > DataSets;
  std::vector< std::vector<bool> > SubPartVisible;
  std::vector< QPointer<cmbNucMaterial> > MeshDisplayedMaterial;
  std::vector< std::string > Names;
  int color;
  unsigned int selectedType;
  int subSection;
  QString FileName;

  void clearMeshDisplayMaterial();

};

#endif
