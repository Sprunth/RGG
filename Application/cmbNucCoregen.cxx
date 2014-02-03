#include "cmbNucCoregen.h"
#include "cmbNucMainWindow.h"
#include <vtkActor.h>
#include <vtkAlgorithm.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkCompositeDataDisplayAttributes.h>
#include <vtkCompositePolyDataMapper2.h>
#include <vtkCompositeDataPipeline.h>
#include <vtkDataObjectTreeIterator.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkInformation.h>
#include <vtkInteractorObserver.h>
#include <vtkNew.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkXMLMultiBlockDataWriter.h>
#include "vtk_moab_reader/vtkMoabReader.h"
#include "vtkCmbLayeredConeSource.h"
#include <iostream>

cmbNucCoregen::cmbNucCoregen(cmbNucMainWindow* mainWindow)
: QDialog(mainWindow)
{
  this->ui = new Ui_qCoregenModel;
  this->ui->setupUi(this);

  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkRenderWindow *renderWindow = this->ui->qvtkWidget->GetRenderWindow();
  renderWindow->AddRenderer(this->Renderer);
  this->VTKToQt = vtkSmartPointer<vtkEventQtSlotConnect>::New();

  renderWindow->SetAlphaBitPlanes(1);
  renderWindow->SetMultiSamples(0);
  this->Renderer->SetUseDepthPeeling(1);
  this->Renderer->SetMaximumNumberOfPeels(5);

  this->Mapper = vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  this->Mapper->SetScalarVisibility(0);
  this->Actor = vtkSmartPointer<vtkActor>::New();
  this->Actor->SetMapper(this->Mapper.GetPointer());
  this->Actor->GetProperty()->SetShading(1);
  this->Actor->GetProperty()->SetInterpolationToPhong();
  this->Renderer->AddActor(this->Actor);

  MoabReader = vtkMoabReader::New();

  vtkCmbLayeredConeSource *cone = vtkCmbLayeredConeSource::New();
  cone->SetNumberOfLayers(3);
  cone->SetHeight(20.0);
  cone->Update();
  this->Mapper->SetInputDataObject(MoabReader->GetOutput());
  cone->Delete();

  vtkCompositeDataDisplayAttributes *attributes = vtkCompositeDataDisplayAttributes::New();
  this->Mapper->SetCompositeDataDisplayAttributes(attributes);
  attributes->Delete();

}

cmbNucCoregen::~cmbNucCoregen()
{
  delete this->ui;
}

void cmbNucCoregen::openFile(QString file)
{
  //std::cout <<file.toStdString().c_str() << std::endl;
  MoabReader->SetFileName(file.toStdString().c_str());
  vtkMultiBlockDataSet * mesh =	MoabReader->GetOutput();
  std::cout << mesh << std::endl;
  //this->Mapper->SetInputDataObject(mesh);
  this->Renderer->ResetCamera();
  this->Renderer->Render();
  this->ui->qvtkWidget->update();
  /*this->PropertyWidget->setGeometryType(this->NewDialog->getSelectedGeometry());
  this->PropertyWidget->setObject(NULL, NULL);
  this->PropertyWidget->setAssembly(NULL);
  this->NuclearCore->CoreLattice.SetGeometryType(
                                               this->PropertyWidget->getGeometryType());
  this->InputsWidget->onNewAssembly();
  this->Renderer->ResetCamera();
  this->Renderer->Render();*/
  this->show();
}
