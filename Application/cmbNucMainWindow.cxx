#include "cmbNucMainWindow.h"

#include "ui_qNucMainWindow.h"
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCompositePolyDataMapper2.h>
#include <QFileDialog>
#include <QStringList>
#include <QDebug>

#include "cmbNucAssembly.h"

#include "vtkAxesActor.h"
#include "vtkProperty.h"
#include "vtkCompositeDataDisplayAttributes.h"

// Constructor
cmbNucMainWindow::cmbNucMainWindow()
{
  this->ui = new Ui_qNucMainWindow;
  this->ui->setupUi(this);

  this->Assembly = 0;

  // VTK/Qt wedded
  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  this->ui->qvtkWidget->GetRenderWindow()->AddRenderer(this->Renderer);
  this->Mapper = vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  this->Actor = vtkSmartPointer<vtkActor>::New();
  this->Actor->SetMapper(this->Mapper.GetPointer());
  this->Actor->GetProperty()->SetShading(1);
  this->Actor->GetProperty()->SetInterpolationToPhong();
  this->Renderer->AddActor(this->Actor);

  vtkCompositeDataDisplayAttributes *attributes = vtkCompositeDataDisplayAttributes::New();
  double color[] = { 1.0, 1.0, 0.0 };
  attributes->SetBlockColor(1, color);
  this->Mapper->SetCompositeDataDisplayAttributes(attributes);
  attributes->Delete();

  // add axes actor
  vtkAxesActor *axesActor = vtkAxesActor::New();
  this->Renderer->AddActor(axesActor);
  axesActor->Delete();

  // Set up action signals and slots
  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(onExit()));
  connect(this->ui->actionOpenFile, SIGNAL(triggered()), this, SLOT(onFileOpen()));
  connect(this->ui->actionSaveFile, SIGNAL(triggered()), this, SLOT(onFileSave()));
}

void cmbNucMainWindow::onExit()
{
  delete this->Assembly;

  qApp->exit();
}

void cmbNucMainWindow::onFileOpen()
{
  QString fileName =
    QFileDialog::getOpenFileName(this,
                                 "Open Assygen File...",
                                 QDir::homePath(),
                                 "INP Files (*.inp)");
  if(!fileName.isEmpty())
    {
    this->setCursor(Qt::BusyCursor);
    this->openFile(fileName);
    this->unsetCursor();
    }
}

void cmbNucMainWindow::openFile(const QString &fileName)
{
  // delete old assembly
  delete this->Assembly;

  // read file and create new assembly
  this->Assembly = new cmbNucAssembly;
  this->Assembly->ReadFile(fileName.toStdString());

  vtkSmartPointer<vtkMultiBlockDataSet> data = this->Assembly->GetData();
  this->Mapper->SetInputDataObject(data);
  this->Renderer->ResetCamera();
}

void cmbNucMainWindow::onFileSave()
{
  QString fileName =
    QFileDialog::getSaveFileName(this,
                                 "Save Assygen File...",
                                 QDir::homePath(),
                                 "INP Files (*.inp)");
  if(!fileName.isEmpty())
    {
    this->setCursor(Qt::BusyCursor);
    this->saveFile(fileName);
    this->unsetCursor();
    }
}

void cmbNucMainWindow::saveFile(const QString &fileName)
{
  if(!this->Assembly)
    {
    qDebug() << "no assembly to save";
    return;
    }

  this->Assembly->WriteFile(fileName.toStdString());
}
