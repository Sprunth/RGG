#ifndef cmbNucMainWindow_H
#define cmbNucMainWindow_H

#include <QMainWindow>

#include <QPointer>
#include <QMap>
#include <QColor>
#include <vtkSmartPointer.h>
#include "cmbNucPartDefinition.h"

// Forward Qt class declarations
class Ui_qNucMainWindow;
class QVTKWidget;
class vtkActor;
class vtkRenderer;
class vtkCompositePolyDataMapper2;
class cmbNucAssembly;
class cmbNucCore;
class cmbNucInputPropertiesWidget;
class cmbNucInputListWidget;
class cmbNucMaterialColors;

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
  void onFileSave();
  void saveFile(const QString &fileName);
  // read file and return a new Assembly
  cmbNucAssembly* openFile(const QString &fileName);

protected:
  void initPanels();

protected slots:
  void onObjectSelected(AssyPartObj*, const char* name);
  void onObjectModified(AssyPartObj* obj=NULL);

  // updates the block colors based on their materials
  void updateMaterialColors();

private:
  // Designer form
  Ui_qNucMainWindow *ui;

  vtkSmartPointer<vtkRenderer> Renderer;
  vtkSmartPointer<vtkCompositePolyDataMapper2> Mapper;
  vtkSmartPointer<vtkActor> Actor;
  cmbNucCore *NuclearCore;
  QPointer<cmbNucInputPropertiesWidget> PropertyWidget;
  QPointer<cmbNucInputListWidget> InputsWidget;

  cmbNucMaterialColors* MaterialColors;
};

#endif // cmbNucMainWindow_H

