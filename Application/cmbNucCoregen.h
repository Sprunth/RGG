#ifndef cmbNucCoregen_H
#define cmbNucCoregen_H

#include <QDialog>
#include <QString>
#include <QTreeWidget>
#include <QObject>
#include <vtkSmartPointer.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkPolyData.h>
#include "cmbNucPartDefinition.h"
#include "cmbNucCore.h"
#include "ui_qCoregenModel.h"
#include <vector>

class vtkMoabReader;
class vtkGeometryFilter;

class cmbNucCoregen : public QObject
{
  Q_OBJECT
public:
  cmbNucCoregen(QTreeWidget *);
  ~cmbNucCoregen();

  vtkSmartPointer<vtkDataObject> getData();

  bool colorBlocks() const { return this->color; }

public slots:
  void openFile(QString file);

signals:
  void error(QString);
  void update();

private:
  // Designer form
  vtkSmartPointer<vtkMoabReader> MoabReader;
  vtkSmartPointer<vtkGeometryFilter> GeoFilt;
  vtkSmartPointer<vtkDataObject> Data;
  std::vector< vtkSmartPointer<vtkDataObject> > DataSets;
  QTreeWidget * List;
  bool color;

protected slots:
  void onSelectionChanged();
};

#endif
