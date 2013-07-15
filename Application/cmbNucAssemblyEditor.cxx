
#include "cmbNucAssemblyEditor.h"

#include <QVBoxLayout>

cmbNucAssemblyEditor::cmbNucAssemblyEditor(QWidget *parent)
  : QWidget(parent)
{
  this->GraphicsView = new QGraphicsView(this);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(this->GraphicsView);
  setLayout(layout);
}

cmbNucAssemblyEditor::~cmbNucAssemblyEditor()
{
}

void cmbNucAssemblyEditor::SetAssembly(cmbNucAssembly *assembly)
{
  this->Assembly = assembly;
}
