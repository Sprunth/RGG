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
class vtkActor;
class vtkCompositePolyDataMapper2;
class vtkEventQtSlotConnect;
class vtkObject;
class vtkRenderer;
class QVTKWidget;
class cmbNucAssembly;
class cmbNucCore;
class cmbNucInputPropertiesWidget;
class cmbNucInputListWidget;
class cmbNucMaterialColors;
class cmbNucNewDialog;

class cmbNucMainWindow : public QMainWindow
{
  Q_OBJECT

public:
  // Constructor/Destructor
  cmbNucMainWindow();
  ~cmbNucMainWindow();

  double getZScale() const { return this->ZScale; }

public slots:
  void onExit();
  void onFileNew();
  void onNewDialogAccept();
  void onFileOpenAssembly();
  void onFileOpenCore();
  void onFileSave();
  void openAssemblyFiles(const QStringList &fileNames);
  void saveFile(const QString &fileName);
  void ResetView();
  void onInteractionTransition(vtkObject *, unsigned long event);
  void useParallelProjection(bool val);

signals:
  void updateGlobalZScale(double scale);

protected:
  void initPanels();

protected slots:
  void onObjectSelected(AssyPartObj*, const char* name);
  void onObjectModified(AssyPartObj* obj=NULL);

  // updates the block colors based on their materials
  void updateMaterialColors();

  // Change the title on the property dock based on selected object
  void updatePropertyDockTitle(const QString& title);

  // shows the preferences dialog
  void onShowPreferences();

  // runs assygen
  void onRunAssygen();

  // called when the z-scale slider or spin box changes
  void zScaleChanged(int value);

private:
  // Designer form
  Ui_qNucMainWindow *ui;

  vtkSmartPointer<vtkRenderer> Renderer;
  vtkSmartPointer<vtkCompositePolyDataMapper2> Mapper;
  vtkSmartPointer<vtkActor> Actor;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKToQt;

  cmbNucCore *NuclearCore;
  cmbNucNewDialog *NewDialog;
  QPointer<cmbNucInputPropertiesWidget> PropertyWidget;
  QPointer<cmbNucInputListWidget> InputsWidget;

  cmbNucMaterialColors* MaterialColors;
  double ZScale;
};

#endif // cmbNucMainWindow_H

