#ifndef cmbNucMainWindow_H
#define cmbNucMainWindow_H
 
#include <QMainWindow>

#include <vtkSmartPointer.h>

// Forward Qt class declarations
class Ui_qNucMainWindow;
class QVTKWidget;
class vtkActor;
class vtkRenderer;

class cmbNucMainWindow : public QMainWindow
{
  Q_OBJECT
public:
 
  // Constructor/Destructor
  cmbNucMainWindow(); 
  ~cmbNucMainWindow() {};
 
public slots:
 
  virtual void onExit();
  virtual void onFileOpen();

private:
 
  // Designer form
  Ui_qNucMainWindow *ui;

  vtkSmartPointer<vtkRenderer> renderer;
  vtkSmartPointer<vtkActor> actor;
};
 
#endif // cmbNucMainWindow_H

