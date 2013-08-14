
#include "cmbNucPinCellEditor.h"

#include <QComboBox>
#include <QTableWidgetItem>
#include <QDebug>

#include <vtkRenderWindow.h>

#include "cmbNucPartDefinition.h"
#include "cmbNucAssembly.h"

class PinCellComponent
{
public:
  enum Type {
    CylinderType,
    FrustumType
  };

  explicit PinCellComponent(const Cylinder *cylinder)
    : type(CylinderType),
      z1(cylinder->z1),
      z2(cylinder->z2),
      r1(cylinder->r),
      r2(cylinder->r)
  {
  }

  explicit PinCellComponent(const Frustum *frustum)
    : type(FrustumType),
      z1(frustum->z1),
      z2(frustum->z2),
      r1(frustum->r1),
      r2(frustum->r2)
  {
  }

  double length() const { return z2 - z1; }
  double radius() const { return r1; }

  enum Type type;
  double z1;
  double z2;
  double r1;
  double r2;
};

bool sort_by_z1(const PinCellComponent &a, const PinCellComponent &b)
{
  return a.z1 < b.z1;
}

cmbNucPinCellEditor::cmbNucPinCellEditor(QWidget *parent)
  : QWidget(parent),
    Ui(new Ui::cmbNucPinCellEditor)
{
  this->Ui->setupUi(this);

  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  this->Ui->qvtkWidget->GetRenderWindow()->AddRenderer(this->Renderer);
  this->Actor = vtkSmartPointer<vtkActor>::New();
  this->Mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->Actor->SetMapper(this->Mapper);
  this->Renderer->AddActor(this->Actor);

  connect(this->Ui->acceptButton, SIGNAL(clicked()), this, SLOT(Apply()));
  connect(this->Ui->rejectButton, SIGNAL(clicked()), this, SLOT(close()));

  connect(this->Ui->addButton, SIGNAL(clicked()), this, SLOT(addComponent()));
  connect(this->Ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteComponent()));
}

cmbNucPinCellEditor::~cmbNucPinCellEditor()
{
  delete this->Ui;
}

void cmbNucPinCellEditor::SetPinCell(PinCell *pincell)
{
  this->PinCellObject = pincell;

  this->Ui->nameLineEdit->setText(pincell->name.c_str());
  this->Ui->labelLineEdit->setText(pincell->label.c_str());
  this->Ui->originX->setText("0");
  this->Ui->originY->setText("0");
  this->Ui->pitchX->setText(QString::number(pincell->pitchX));
  this->Ui->pitchY->setText(QString::number(pincell->pitchY));
  this->Ui->pitchZ->setText(QString::number(pincell->pitchZ));

  this->Ui->piecesTable->setColumnCount(4);
  this->Ui->piecesTable->setHorizontalHeaderLabels(
    QStringList() << "Type" << "Length" << "Radius (base)" << "Radius (top)"
  );

  std::vector<PinCellComponent> components;
  for(size_t i = 0; i < pincell->cylinders.size(); i++){
    components.push_back(PinCellComponent(pincell->cylinders[i]));
  }
  for(size_t i = 0; i < pincell->frustums.size(); i++){
    components.push_back(PinCellComponent(pincell->frustums[i]));
  }
  std::sort(components.begin(), components.end(), sort_by_z1);

  this->Ui->piecesTable->setRowCount(components.size());
  for(size_t i = 0; i < components.size(); i++){
    const PinCellComponent &component = components[i];

    QTableWidgetItem *item = new QTableWidgetItem;
    QComboBox *comboBox = new QComboBox;
    comboBox->addItem("Cylinder");
    comboBox->addItem("Frustum");
    comboBox->setCurrentIndex(component.type == PinCellComponent::CylinderType ? 0 : 1);
    this->Ui->piecesTable->setItem(i, 0, item);
    this->Ui->piecesTable->setCellWidget(i, 0, comboBox);

    item = new QTableWidgetItem;
    item->setText(QString::number(component.length()));
    this->Ui->piecesTable->setItem(i, 1, item);

    item = new QTableWidgetItem;
    item->setText(QString::number(component.r1));
    this->Ui->piecesTable->setItem(i, 2, item);

    item = new QTableWidgetItem;
    item->setText(QString::number(component.r2));
    this->Ui->piecesTable->setItem(i, 3, item);
  }
  this->Ui->piecesTable->resizeColumnsToContents();

  this->Ui->layers->setText("1");

  this->UpdatePolyData();

  // setup 3d view
  this->Renderer->ResetCamera();
  this->Ui->qvtkWidget->update();
}

PinCell* cmbNucPinCellEditor::GetPinCell()
{
  return this->PinCellObject;
}

void cmbNucPinCellEditor::Apply()
{
  this->UpdatePinCell();

  emit this->accepted();
  this->close();
}

void cmbNucPinCellEditor::UpdatePinCell()
{
  // apply pincell attributes
  this->PinCellObject->name = this->Ui->nameLineEdit->text().toStdString();
  this->PinCellObject->label = this->Ui->labelLineEdit->text().toStdString();
  this->PinCellObject->pitchX = this->Ui->pitchX->text().toDouble();
  this->PinCellObject->pitchY = this->Ui->pitchY->text().toDouble();
  this->PinCellObject->pitchZ = this->Ui->pitchZ->text().toDouble();

  // setup components
  for(int i = 0; i < this->Ui->piecesTable->rowCount(); i++){
    QTableWidgetItem *item = this->Ui->piecesTable->item(i, 0);

    QComboBox *comboBox = qobject_cast<QComboBox *>(this->Ui->piecesTable->cellWidget(i, 0));
    qDebug() << "row" << i << " = " << comboBox->currentText();
  }
}

void cmbNucPinCellEditor::UpdatePolyData()
{
  vtkPolyData *polyData = cmbNucAssembly::CreatePinCellPolyData(this->PinCellObject);
  this->Mapper->SetInputData(polyData);
  polyData->Delete();
}

void cmbNucPinCellEditor::addComponent()
{
  int row = this->Ui->piecesTable->rowCount();
  this->Ui->piecesTable->setRowCount(row + 1);

  // type
  QTableWidgetItem *item = new QTableWidgetItem;
  QComboBox *comboBox = new QComboBox;
  comboBox->addItem("Cylinder");
  comboBox->addItem("Frustum");
  this->Ui->piecesTable->setCellWidget(row, 0, comboBox);

  // length
  item = new QTableWidgetItem;
  item->setText("2");
  this->Ui->piecesTable->setItem(row, 1, item);

  // radius (base)
  item = new QTableWidgetItem;
  item->setText("0.5");
  this->Ui->piecesTable->setItem(row, 2, item);

  // radius (top)
  item = new QTableWidgetItem;
  item->setText("0.5");
  this->Ui->piecesTable->setItem(row, 3, item);

  // update view
  this->UpdatePinCell();
  this->UpdatePolyData();
}

void cmbNucPinCellEditor::deleteComponent()
{
}

