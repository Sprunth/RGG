#include "cmbNucNewDialog.h"
#include "ui_qNucNewDialog.h"

#include "cmbNucMainWindow.h"

cmbNucNewDialog::cmbNucNewDialog(cmbNucMainWindow *mainWindow)
  : QDialog(mainWindow)
{
  this->ui = new Ui_qNucNewDialog;
  this->ui->setupUi(this);
}

cmbNucNewDialog::~cmbNucNewDialog()
{
}

enumGeometryType cmbNucNewDialog::getSelectedGeometry()
{
  int idx = this->ui->geometrySelect->currentIndex();

  if (0 == idx)
    {
    return RECTILINEAR;
    }
  else
    {
    return HEXAGONAL;
    }
}
