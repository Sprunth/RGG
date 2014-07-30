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
class vtkCubeAxesActor;
class vtkCompositePolyDataMapper2;
class vtkEventQtSlotConnect;
class vtkObject;
class vtkRenderer;
class QVTKWidget;
class cmbNucAssembly;
class cmbNucCore;
class PinCell;
class DuctCell;
class Frustum;
class Cylinder;
class cmbNucInputPropertiesWidget;
class cmbNucInputListWidget;
class cmbNucMaterialColors;
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

  //Returns true if it is ok to proceed
  bool checkFilesBeforePreceeding();

  void setScaledBounds();

public slots:
  void onExit();
  void onNewCore();
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
  void saveFile(cmbNucAssembly*);
  void saveFile(cmbNucCore*);
  void saveFile(const QString &fileName);
  void saveCoreFile(const QString &fileName);
  void exportVTKFile(const QString &fileName);
  void ResetView();
  void Render();
  void onInteractionTransition(vtkObject *, unsigned long event);
  void useParallelProjection(bool val);
  void checkForNewCUBH5MFiles();
  void setAxis(bool ison);

signals:
  void updateGlobalZScale(double scale);
  void checkSave();

protected:
  void initPanels();
  void saveSelected(bool requestFileName, bool force);
  void saveAll(bool requestFileName, bool force);
  void save(cmbNucAssembly*, bool request_file_name, bool force);
  void save(cmbNucCore*, bool request_file_name, bool force);
  QString requestInpFileName(QString name, QString type);
  virtual void closeEvent(QCloseEvent *event);

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
  void updateDuctCellMaterialColors(DuctCell*);

  // Change the title on the property dock based on selected object
  void updatePropertyDockTitle(const QString& title);

  // Runs Assygen, Cubit, and CoreGen
  void exportRGG();

  // called when the z-scale slider or spin box changes
  void zScaleChanged(int value);

  void setTitle();

private:
  // Designer form
  Ui_qNucMainWindow *ui;

  void doClearAll(bool needSave = false);

  vtkSmartPointer<vtkRenderer> Renderer;
  vtkSmartPointer<vtkCompositePolyDataMapper2> Mapper;
  vtkSmartPointer<vtkCubeAxesActor> CubeAxesActor;
  vtkSmartPointer<vtkActor> Actor;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKToQt;

  cmbNucCore *NuclearCore;
  cmbNucExportDialog *ExportDialog;
  cmbNucPreferencesDialog *Preferences;

  NucMainInternal *Internal;

  QPointer<cmbNucInputPropertiesWidget> PropertyWidget;
  QPointer<cmbNucInputListWidget> InputsWidget;

  cmbNucMaterialColors* MaterialColors;
  double ZScale;
};

#endif // cmbNucMainWindow_H

