#ifndef cmbNucMainWindow_H
#define cmbNucMainWindow_H

#include <QMainWindow>

#include <QPointer>
#include <vtkSmartPointer.h>
#include "cmbNucPartDefinition.h"

// Forward Qt class declarations
class Ui_qNucMainWindow;
class QVTKWidget;
class vtkActor;
class vtkRenderer;
class vtkCompositePolyDataMapper2;
class cmbNucAssembly;
class cmbNucInputPropertiesWidget;
class cmbNucInputListWidget;

class cmbNucMainWindow : public QMainWindow
{
  Q_OBJECT

public:
  // Constructor/Destructor
  cmbNucMainWindow();
  ~cmbNucMainWindow();

public slots:
  void onExit();
  void onFileNew();
  void onFileOpen();
  void openFile(const QString &fileName);
  void onFileSave();
  void saveFile(const QString &fileName);

protected:
  void initPanels();

protected slots:
  void onObjectSelected(AssyPartObj*, const char* name);
  void onAssemblyModified(AssyPartObj*);

  // updates the block colors based on their materials
  void updateMaterialColors();

private:
  // Designer form
  Ui_qNucMainWindow *ui;

  vtkSmartPointer<vtkRenderer> Renderer;
  vtkSmartPointer<vtkCompositePolyDataMapper2> Mapper;
  vtkSmartPointer<vtkActor> Actor;
  cmbNucAssembly *Assembly;
  QPointer<cmbNucInputPropertiesWidget> PropertyWidget;
  QPointer<cmbNucInputListWidget> InputsWidget;

};

#endif // cmbNucMainWindow_H

