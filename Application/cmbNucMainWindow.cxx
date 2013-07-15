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
#include <QDockWidget>

#include "cmbNucAssembly.h"
#include "cmbNucInputPropertiesWidget.h"
#include "cmbNucInputListWidget.h"
#include "cmbNucAssemblyEditor.h"

#include "vtkAxesActor.h"
#include "vtkProperty.h"
#include "vtkCompositeDataDisplayAttributes.h"

// Constructor
cmbNucMainWindow::cmbNucMainWindow()
{
  this->ui = new Ui_qNucMainWindow;
  this->ui->setupUi(this);
  this->initPanels();

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
  connect(this->ui->actionNew, SIGNAL(triggered()), this, SLOT(onFileNew()));
}
cmbNucMainWindow::~cmbNucMainWindow()
{
  if(this->Assembly)
    {
    delete this->Assembly;
    }
}

void cmbNucMainWindow::initPanels()
{
  this->InputsWidget = new cmbNucInputListWidget(this);
  this->PropertyWidget = new cmbNucInputPropertiesWidget(this);
  this->AssemblyEditor = new cmbNucAssemblyEditor(this);
  this->ui->InputsDock->setWidget(this->InputsWidget);
  this->ui->PropertyDock->setWidget(this->PropertyWidget);
  this->ui->PropertyDock->setEnabled(0);
  this->ui->AssemblyDock->setWidget(this->AssemblyEditor);
  // current, 0=Ducts, 1=Pins, 2=Materials
//  QObject::connect(this->InputsWidget,
//      SIGNAL(partTypeSwitched(enumNucParts)), this,
//      SLOT(onPartTypeSwitched(enumNucParts)));

  QObject::connect(this->InputsWidget,
    SIGNAL(objectSelected(AssyPartObj*, const char*)), this,
    SLOT(onObjectSelected(AssyPartObj*, const char*)));
  QObject::connect(this->PropertyWidget,
    SIGNAL(currentObjectModified(AssyPartObj*)), this,
    SLOT(onAssemblyModified(AssyPartObj*)));
}

void cmbNucMainWindow::onObjectSelected(AssyPartObj* selObj,
  const char* name)
{
  if(!selObj)
    {
    this->ui->PropertyDock->setEnabled(0);
    return;
    }
  this->ui->PropertyDock->setEnabled(1);
  QStringList materials;
  for(size_t i = 0; i < this->Assembly->Materials.size(); i++)
    {
    Material &material = this->Assembly->Materials[i];
    materials.append(material.label.c_str());
    }
  this->PropertyWidget->setObject(selObj, name, materials);
}

void cmbNucMainWindow::onAssemblyModified(AssyPartObj*)
{
  // regenerate assembly view
  this->Mapper->SetInputDataObject(this->Assembly->GetData());
}

void cmbNucMainWindow::onExit()
{
  qApp->exit();
}

void cmbNucMainWindow::onFileNew()
{
  if(this->Assembly)
    {
    this->InputsWidget->setAssembly(NULL);
    delete this->Assembly;
    }

  this->Assembly = new cmbNucAssembly;
  this->Mapper->SetInputDataObject(NULL);
  this->Renderer->ResetCamera();
  this->Renderer->Render();
  this->InputsWidget->setAssembly(this->Assembly);
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
  if(this->Assembly)
    {
    delete this->Assembly;
    }

  // read file and create new assembly
  this->Assembly = new cmbNucAssembly;
  this->Assembly->ReadFile(fileName.toStdString());

  vtkSmartPointer<vtkMultiBlockDataSet> data = this->Assembly->GetData();
  this->Mapper->SetInputDataObject(data);
  this->Renderer->ResetCamera();
  this->Renderer->Render();
  this->InputsWidget->setAssembly(this->Assembly);
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
