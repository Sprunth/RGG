#ifndef cmbNucCoregen_H
#define cmbNucCoregen_H

#include <QDialog>
#include <QString>
#include <QThread>
#include <vtkSmartPointer.h>
#include "cmbNucPartDefinition.h"
#include "cmbNucCore.h"
#include "ui_qCoregenModel.h"

class vtkActor;
class vtkCompositePolyDataMapper2;
class vtkEventQtSlotConnect;
class vtkObject;
class vtkRenderer;
class QVTKWidget;
class cmbNucMainWindow;
class vtkMoabReader;

class cmbNucCoregen : public QDialog
{
  Q_OBJECT
public:
  cmbNucCoregen(cmbNucMainWindow* mainWindow);
  ~cmbNucCoregen();
  vtkSmartPointer<vtkMoabReader> MoabReader;

public slots:
  void openFile(QString file);

signals:
  void error(QString);

private:
  // Designer form
  Ui_qCoregenModel *ui;
  vtkSmartPointer<vtkRenderer> Renderer;
  vtkSmartPointer<vtkCompositePolyDataMapper2> Mapper;
  vtkSmartPointer<vtkActor> Actor;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKToQt;
};

#endif
