
#include "cmbNucPinCellEditor.h"

#include <QComboBox>
#include <QTableWidgetItem>

#include <vtkRenderWindow.h>
#include <vtkClipClosedSurface.h>

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
    Ui(new Ui::cmbNucPinCellEditor),
    AssemblyObject(0)
{
  this->Ui->setupUi(this);

  this->resize(1200, 700);
  this->Ui->splitter->setSizes(QList<int>() << 400 << 1200 - 400);

  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  this->Ui->qvtkWidget->GetRenderWindow()->AddRenderer(this->Renderer);
  this->Actor = vtkSmartPointer<vtkActor>::New();
  this->Mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->Actor->SetMapper(this->Mapper);
  this->Renderer->AddActor(this->Actor);

  this->Ui->layersTable->setRowCount(0);
  this->Ui->layersTable->setColumnCount(2);
  this->Ui->layersTable->setHorizontalHeaderLabels(
    QStringList() << "Material" << "Radius"
  );

  connect(this->Ui->acceptButton, SIGNAL(clicked()), this, SLOT(Apply()));
  connect(this->Ui->rejectButton, SIGNAL(clicked()), this, SLOT(close()));

  connect(this->Ui->addButton, SIGNAL(clicked()), this, SLOT(addComponent()));
  connect(this->Ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteComponent()));

  connect(this->Ui->piecesTable, SIGNAL(cellChanged(int, int)),
          this, SLOT(tableCellChanged(int, int)));
  //connect(this->Ui->piecesTable, SIGNAL(itemChanged(QTableWidgetItem*)),
  //        this, SLOT(tableItemChanged(QTableWidgetItem*)));
  connect(this->Ui->layersSpinBox, SIGNAL(valueChanged(int)),
          this, SLOT(numberOfLayersChanged(int)));
  connect(this->Ui->layersTable, SIGNAL(cellChanged(int, int)),
          this, SLOT(layerTableCellChanged(int, int)));
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

  this->Ui->piecesTable->blockSignals(true);

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

  this->Ui->piecesTable->blockSignals(false);

  int layers = pincell->materials.size();
  if(layers < 1)
    {
    this->numberOfLayersChanged(1);
    }
  else
    {
    this->Ui->layersTable->setRowCount(layers);
    for(int i = 0; i < layers; i++)
      {
      QComboBox *comboBox = new QComboBox;
      this->setupMaterialComboBox(comboBox);
      this->Ui->layersTable->setCellWidget(i, 0, comboBox);

      for(int j = 0; j < comboBox->count(); j++)
        {
        if(comboBox->itemText(j).toStdString() == pincell->materials[i])
          {
          comboBox->setCurrentIndex(j);
          break;
          }
        }

      QTableWidgetItem *item = new QTableWidgetItem;
      item->setText(QString::number(pincell->radii[i]));
      this->Ui->layersTable->setItem(i, 1, item);
      }
    }
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

  // clear previous components
  this->PinCellObject->cylinders.clear();
  this->PinCellObject->frustums.clear();

  // setup components
  double z = 0;
  for(int i = 0; i < this->Ui->piecesTable->rowCount(); i++){
    QTableWidgetItem *item = this->Ui->piecesTable->item(i, 0);

    QComboBox *comboBox = qobject_cast<QComboBox *>(this->Ui->piecesTable->cellWidget(i, 0));

    double x = this->Ui->originX->text().toDouble();
    double y = this->Ui->originY->text().toDouble();
    double l = this->Ui->piecesTable->item(i, 1)->text().toDouble();
    double r1 = this->Ui->piecesTable->item(i, 2)->text().toDouble();
    double r2 = this->Ui->piecesTable->item(i, 3)->text().toDouble();

    if(comboBox->currentText() == "Cylinder"){
        Cylinder *cylinder = new Cylinder;
        cylinder->x = x;
        cylinder->y = y;
        cylinder->z1 = z;
        cylinder->z2 = z + l;
        cylinder->r = r1;
        this->PinCellObject->cylinders.push_back(cylinder);
    }
    else if(comboBox->currentText() == "Frustum"){
        Frustum *frustum = new Frustum;
        frustum->x = x;
        frustum->y = y;
        frustum->z1 = z;
        frustum->z2 = z + l;
        frustum->r1 = r1;
        frustum->r2 = r2;
        this->PinCellObject->frustums.push_back(frustum);
    }
    z += l;
  }

  // setup materials
  int materials = this->Ui->layersTable->rowCount();
  this->PinCellObject->materials.resize(materials);
  this->PinCellObject->radii.resize(materials);
  for(int i = 0; i < materials; i++)
    {
    QComboBox *comboBox = qobject_cast<QComboBox *>(this->Ui->layersTable->cellWidget(i, 0));
    if(comboBox)
      {
      this->PinCellObject->materials[i] = comboBox->currentText().toStdString();
      }

    QTableWidgetItem *item = this->Ui->layersTable->item(i, 1);
    if(item)
      {
      this->PinCellObject->radii[i] = item->text().toDouble();
      }
    }
}

void cmbNucPinCellEditor::UpdatePolyData()
{
  vtkPolyData *polyData = cmbNucAssembly::CreatePinCellPolyData(this->PinCellObject);
  this->Mapper->SetInputData(polyData);
  polyData->Delete();
}

void cmbNucPinCellEditor::UpdateRenderView()
{
  this->Renderer->ResetCamera();
  this->Ui->qvtkWidget->update();
}

void cmbNucPinCellEditor::addComponent()
{
  this->Ui->piecesTable->blockSignals(true);

  int row = this->Ui->piecesTable->rowCount();
  this->Ui->piecesTable->setRowCount(row + 1);

  double default_length = 2.0;
  double default_radius = 0.5;

  if(row >= 1){
      // use radius from top of previous component
      default_radius = this->Ui->piecesTable->item(row - 1, 3)->text().toDouble();
  }

  // type
  QTableWidgetItem *item = new QTableWidgetItem;
  QComboBox *comboBox = new QComboBox;
  comboBox->addItem("Cylinder");
  comboBox->addItem("Frustum");
  connect(comboBox, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(sectionTypeComboBoxChanged(QString)));
  this->Ui->piecesTable->setCellWidget(row, 0, comboBox);

  // length
  item = new QTableWidgetItem;
  item->setText(QString::number(default_length));
  this->Ui->piecesTable->setItem(row, 1, item);

  // radius (base)
  item = new QTableWidgetItem;
  item->setText(QString::number(default_radius));
  this->Ui->piecesTable->setItem(row, 2, item);

  // radius (top)
  item = new QTableWidgetItem;
  item->setText(QString::number(default_radius));
  this->Ui->piecesTable->setItem(row, 3, item);

  this->Ui->piecesTable->blockSignals(false);

  // update view
  this->UpdatePinCell();
  this->UpdatePolyData();
  this->UpdateRenderView();
}

void cmbNucPinCellEditor::deleteComponent()
{
  this->Ui->piecesTable->blockSignals(true);

  int row = this->Ui->piecesTable->currentRow();
  this->Ui->piecesTable->removeRow(row);

  this->Ui->piecesTable->blockSignals(false);

  // update view
  this->UpdatePinCell();
  this->UpdatePolyData();
  this->UpdateRenderView();
}

void cmbNucPinCellEditor::tableCellChanged(int row, int col)
{
  QTableWidgetItem *item = this->Ui->piecesTable->item(row, col);

  if(col == 2 || col == 3){
    // propogate radius changes
    this->Ui->piecesTable->blockSignals(true);

    bool is_cylinder =
      qobject_cast<QComboBox *>(this->Ui->piecesTable->cellWidget(row, 0))->currentText() == "Cylinder";

    // if the component is a cylinder and one of the radii was
    // changed update the other to match
    if(is_cylinder){
      int other_col = col == 2 ? 3 : 2;
      QTableWidgetItem *other_item = this->Ui->piecesTable->item(row, other_col);
      other_item->setText(item->text());
    }

    if(col == 3 || is_cylinder){
        for(int i = row + 1; i < this->Ui->piecesTable->rowCount(); i++){
          bool next_is_cylinder =
            qobject_cast<QComboBox *>(this->Ui->piecesTable->cellWidget(i, 0))->currentText() == "Cylinder";
          if(next_is_cylinder){
              // set both top and base radii and continue propagating
              this->Ui->piecesTable->item(i, 2)->setText(item->text());
              this->Ui->piecesTable->item(i, 3)->setText(item->text());
          }
          else {
              // set base radius only and stop propogating
              this->Ui->piecesTable->item(i, 2)->setText(item->text());
              break;
          }
        }
    }
    if(col == 2 || is_cylinder){
      for(int i = row - 1; i >= 0; i--){
          bool next_is_cylinder =
              qobject_cast<QComboBox *>(this->Ui->piecesTable->cellWidget(i, 0))->currentText() == "Cylinder";
          if(next_is_cylinder){
              // set both top and base radii and continue propagating
              this->Ui->piecesTable->item(i, 2)->setText(item->text());
              this->Ui->piecesTable->item(i, 3)->setText(item->text());
          }
          else {
              // set top radius only and stop propogating
              this->Ui->piecesTable->item(i, 3)->setText(item->text());
              break;
          }
      }
    }

    this->Ui->piecesTable->blockSignals(false);
  }

  // update pin cell and render view
  this->UpdatePinCell();
  this->UpdatePolyData();
  this->UpdateRenderView();
}

void cmbNucPinCellEditor::tableItemChanged(QTableWidgetItem *item)
{
  int row = item->row();
  int col = item->column();

  this->tableCellChanged(row, col);
}

void cmbNucPinCellEditor::numberOfLayersChanged(int layers)
{
  int current = this->Ui->layersTable->rowCount();
  this->Ui->layersTable->setRowCount(layers);

  if(layers > current)
    {
    int row = layers - 1;

    QTableWidgetItem *item = new QTableWidgetItem;
    QComboBox *comboBox = new QComboBox;
    this->setupMaterialComboBox(comboBox);
    this->Ui->layersTable->setCellWidget(row, 0, comboBox);

    item = new QTableWidgetItem;
    item->setText(QString::number(1.0));
    this->Ui->layersTable->setItem(row, 1, item);
    }

  this->PinCellObject->materials.resize(layers);
}

void cmbNucPinCellEditor::sectionTypeComboBoxChanged(const QString &type)
{
  QComboBox *comboBox = qobject_cast<QComboBox *>(sender());
  if(!comboBox){
    return;
  }

//  int row = comboBox->data(Qt::UserRole).toInt();
//  this->tableCellChanged(row, 0);
  this->UpdatePinCell();
  this->UpdatePolyData();
  this->UpdateRenderView();
}

void cmbNucPinCellEditor::setupMaterialComboBox(QComboBox *comboBox)
{
  // copied from cmbNucInputListWidget (should be moved to a common place)
  const char  *defaultMaterials[] = {
       "Fuel1", "Fuel2", "Fuel_uox1", "Fuel_uox2",
       "MOX_43", "MOX_73", "MOX_87", "Water", "Coolant",
       "Water_Rod", "Control_Rod", "BARod16", "BARod18",
       "BARod28", "Graphite", "Cladding", "Gap", "Metal",
       "MaterialBlock", "end"
  };
  for(size_t i = 0; i < sizeof(defaultMaterials) / sizeof(*defaultMaterials); i++)
    {
    comboBox->addItem(defaultMaterials[i]);
    }
}

void cmbNucPinCellEditor::layerTableCellChanged(int row, int col)
{
  this->UpdatePinCell();
  this->UpdatePolyData();
  this->UpdateRenderView();
}
