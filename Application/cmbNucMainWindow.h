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
class vtkAxesActor;
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
class cmbNucExportDialog;
class cmbNucPreferencesDialog;
class NucMainInternal;

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
  void onFileOpen();
  void onFileOpenMoab();
  void onSaveSelected();
  void onSaveAll();
  void onSaveProjectAs();
  void onSaveSelectedAs();
  void onReloadSelected();
  void onReloadAll();
  void clearAll();
  void clearCore();
  void saveFile(const QString &fileName);
  void saveCoreFile(const QString &fileName);
  void exportVTKFile(const QString &fileName);
  void ResetView();
  void onInteractionTransition(vtkObject *, unsigned long event);
  void useParallelProjection(bool val);

signals:
  void updateGlobalZScale(double scale);
  void checkSave();

protected:
  void initPanels();
  void saveSelected(bool requestFileName);
  void saveAll(bool requestFileName);
  void save(cmbNucAssembly*, bool request_file_name);
  void save(cmbNucCore*, bool request_file_name);
  QString requestInpFileName(QString name, QString type);

protected slots:
  void onObjectSelected(AssyPartObj*, const char* name);
  void onObjectModified(AssyPartObj* obj=NULL);
  void onObjectGeometryChanged(AssyPartObj* obj);

  void onChangeToModelTab();
  void onChangeFromModelTab(int);

  void onSelectionChange();

  void onChangeMeshColorMode(bool b);
  void onChangeMeshEdgeMode(bool b);

  // updates the block colors based on their materials
  void updateCoreMaterialColors();
  void updateAssyMaterialColors(cmbNucAssembly* assy);
  void updatePinCellMaterialColors(PinCell*);

  // Change the title on the property dock based on selected object
  void updatePropertyDockTitle(const QString& title);

  // Runs Assygen, Cubit, and CoreGen
  void exportRGG();

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
  cmbNucExportDialog *ExportDialog;
  cmbNucPreferencesDialog *Preferences;

  NucMainInternal *Internal;

  QPointer<cmbNucInputPropertiesWidget> PropertyWidget;
  QPointer<cmbNucInputListWidget> InputsWidget;

  cmbNucMaterialColors* MaterialColors;
  double ZScale;
};

#endif // cmbNucMainWindow_H

