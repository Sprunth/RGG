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
#include "vtkGeometryFilter.h"
#include <iostream>
#include <QDebug>

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
  //this->Renderer->SetUseDepthPeeling(1);
  //this->Renderer->SetMaximumNumberOfPeels(5);
  this->Renderer->SetBackground(0.1,0.2,0.5);

  this->Mapper = vtkSmartPointer<vtkCompositePolyDataMapper2>::New();
  //this->Mapper->SetScalarVisibility(0);
  this->Actor = vtkSmartPointer<vtkActor>::New();
  this->Actor->SetMapper(this->Mapper.GetPointer());
  this->Actor->GetProperty()->SetShading(1);
  this->Actor->GetProperty()->SetInterpolationToPhong();
  this->Renderer->AddActor(this->Actor);

  MoabReader = vtkMoabReader::New();
  this->GeoFilt = vtkGeometryFilter::New();
  this->GeoFilt->SetInputConnection(MoabReader->GetOutputPort());
  this->Mapper->SetInputConnection(GeoFilt->GetOutputPort());

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
  qDebug() <<file;
  MoabReader->SetFileName(file.toStdString().c_str());
  //vtkMultiBlockDataSet * mesh =	MoabReader->GetOutput();
  MoabReader->Update();
  GeoFilt->Update();
  //mesh =	MoabReader->GetOutput();
  //this->Mapper->SetInputDataObject(mesh);
  /*qDebug() << "number of pieces: " << this->Mapper->GetNumberOfPieces();
  qDebug() << this->Mapper->GetScalarRange()[0] << " " << this->Mapper->GetScalarRange()[1];
  qDebug() << mesh->GetNumberOfBlocks();
  for(unsigned int i = 0; i < mesh->GetNumberOfBlocks(); ++i)
  {
    vtkDataObject * dobj = mesh->GetBlock(i);
    dobj->PrintSelf(std::cout, vtkIndent());
  }*/

  //this->Renderer->ResetCamera();
  //this->Renderer->Render();
  //this->ui->qvtkWidget->update();
  this->show();
  this->Renderer->ResetCamera();
  this->Renderer->Render();
  this->ui->qvtkWidget->update();
}
