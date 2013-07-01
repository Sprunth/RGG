#include "cmbNucMainWindow.h"
 
#include "Ui_qNucMainWindow.h"
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <QFileDialog>
#include <QStringList>

// Constructor
cmbNucMainWindow::cmbNucMainWindow() 
{
  this->ui = new Ui_qNucMainWindow;
  this->ui->setupUi(this);

  // VTK Renderer
  vtkSmartPointer<vtkRenderer> renderer = 
      vtkSmartPointer<vtkRenderer>::New();

  // VTK/Qt wedded
  this->renderer = vtkSmartPointer<vtkRenderer>::New();
  this->ui->qvtkWidget->GetRenderWindow()->AddRenderer(this->renderer);
  this->actor = vtkSmartPointer<vtkActor>::New();
  this->renderer->AddActor(this->actor);

  // Set up action signals and slots
  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(onExit()));
  connect(this->ui->actionOpenFile, SIGNAL(triggered()), this, SLOT(onFileOpen()));
}
 
void cmbNucMainWindow::onExit() 
{
  qApp->exit();
}

void cmbNucMainWindow::onFileOpen()
{
  QFileDialog fileDialog( this, "Open Assygen File...", QDir::homePath(), "*.inp");
  fileDialog.exec();
  if(fileDialog.selectedFiles().count())
    {
    this->setCursor(Qt::BusyCursor);

    // Reading file and create VTK rendering pipeline.

    this->unsetCursor();
    }
}
