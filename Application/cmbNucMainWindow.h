#ifndef cmbNucMainWindow_H
#define cmbNucMainWindow_H

#include <QMainWindow>

#include <QPointer>
#include <QMap>
#include <QColor>
#include <vtkSmartPointer.h>

#include "cmbNucPartDefinition.h"
#include "cmbNucImporter.h"

// Forward Qt class declarations
class pqTestUtility;
class Ui_qNucMainWindow;
class vtkActor;
class vtkAxesActor;
class vtkCubeAxesActor;
class vtkCompositePolyDataMapper2;
class vtkPolyDataMapper;
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
class cmbNucLatticeWidget;
class cmbNucGenerateOuterCylinder;
class cmbNucRender;

class cmbNucMainWindow : public QMainWindow
{
  Q_OBJECT

public:
  friend class cmbNucImporter;
  // Constructor/Destructor
  cmbNucMainWindow();
  ~cmbNucMainWindow();

  double getZScale() const { return this->ZScale; }

  //Returns true if it is ok to proceed
  bool checkFilesBeforePreceeding();

  void setScaledBounds();

  bool playTest(QString fname);
  void setUpTests(QString,
                  QStringList testModelCorrectImages,
                  QStringList test2DCorrectImages,
                  QString testMeshCorrectImage,
                  QString testDirectory,
                  QString testOutputDirectory,
                  bool exit);

public slots:
  void onExit();
  void onNewCore();
  void onFileOpen();
  void onImportINPFile();
  void onFileOpenMoab();
  void onSaveSelected();
  void onSaveAll();
  void onSaveSelectedAs();
  void onExportINPFiles();
  void onUpdateINPFiles();
  void clearAll();
  void clearCore();
  void ResetView();
  void Render();
  void onInteractionTransition(vtkObject *, unsigned long event);
  void onInteractionMeshTransition(vtkObject *, unsigned long event);
  void useParallelProjection(bool val);
  void checkForNewCUBH5MFiles();
  void setAxis(bool ison);
  void onClearMesh();
  void playTest();
  void onExportVisibleMesh();
  void meshControls(bool);
  void modelControls(bool);
  void resetMeshCamera();
  void waitForExportingToBeDone();
  void checkExporter();

signals:
  void updateGlobalZScale(double scale);
  void checkSave();

protected:
  void initPanels();
  void saveSelected(bool requestFileName, bool force);
  bool exportINPs();
  void saveXML(cmbNucCore*, bool request_file_name, bool force);
  QString requestXMLFileName(QString name, QString type);
  virtual void closeEvent(QCloseEvent *event);
  void CameraMovedHandlerMesh();
  void CameraMovedHandlerModel();

protected slots:
  void onObjectSelected(AssyPartObj*, const char* name);
  void onObjectModified(AssyPartObj* obj=NULL);
  void onObjectGeometryChanged(AssyPartObj* obj);

  void onStartRecordTest();
  void onStopRecordingTest();
  void onPlayTest();

  void onChangeToModelTab();

  void onSelectionChange();

  void onChangeMeshColorMode();
  void onChangeMeshEdgeMode(bool b);

  void onRaiseMesh();
  void onRaiseModel();

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
  //void retryExport();

  void colorChange();

  void outerLayer(double r, int i);
  void clearOuter();

  void resetCamera();

private:
  // Designer form
  Ui_qNucMainWindow *ui;

  cmbNucImporter * importer;

  void doClearAll(bool needSave = false);

  cmbNucRender * NucMappers;
  vtkSmartPointer<vtkRenderer> Renderer;
  vtkSmartPointer<vtkRenderer> MeshRenderer;
  vtkSmartPointer<vtkCompositePolyDataMapper2> Mapper;
  vtkSmartPointer<vtkCompositePolyDataMapper2> MeshMapper;
  vtkSmartPointer<vtkCubeAxesActor> CubeAxesActor;
  vtkSmartPointer<vtkCubeAxesActor> MeshCubeAxesActor;
  vtkSmartPointer<vtkActor> MeshActor;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKToQt;

  cmbNucCore *NuclearCore;
  cmbNucExportDialog *ExportDialog;
  cmbNucPreferencesDialog *Preferences;

  NucMainInternal *Internal;

  QPointer<cmbNucInputPropertiesWidget> PropertyWidget;
  QPointer<cmbNucInputListWidget> InputsWidget;
  QPointer<cmbNucLatticeWidget> LatticeDraw;

  cmbNucMaterialColors* MaterialColors;
  double ZScale;

  bool isMeshTabVisible();
  bool is3DTabVisible();
  void setCameras(bool coreModel, bool fullMesh);
  bool isCameraIsMoving;

  pqTestUtility * TestUtility;
};

#endif // cmbNucMainWindow_H
