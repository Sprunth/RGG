#ifndef cmbNucMainWindow_H
#define cmbNucMainWindow_H

#include <QMainWindow>

#include <vtkSmartPointer.h>

// Forward Qt class declarations
class Ui_qNucMainWindow;
class QVTKWidget;
class vtkActor;
class vtkRenderer;
class vtkCompositePolyDataMapper2;
class cmbNucAssembly;

class cmbNucMainWindow : public QMainWindow
{
  Q_OBJECT

public:
  // Constructor/Destructor
  cmbNucMainWindow();
  ~cmbNucMainWindow() {};

public slots:
  void onExit();
  void onFileOpen();
  void openFile(const QString &fileName);
  void onFileSave();
  void saveFile(const QString &fileName);

private:
  // Designer form
  Ui_qNucMainWindow *ui;

  vtkSmartPointer<vtkRenderer> Renderer;
  vtkSmartPointer<vtkCompositePolyDataMapper2> Mapper;
  vtkSmartPointer<vtkActor> Actor;
  cmbNucAssembly *Assembly;
};

#endif // cmbNucMainWindow_H

