#include "cmbNucConflictDialog.h"
#include "ui_name_conflict_gui.h"

#include <QMainWindow>

cmbNucConflictDialog::cmbNucConflictDialog(QMainWindow* mainWindow, cmbNucPinLibrary * lib, PinCell * p)
: QDialog(mainWindow,  Qt::CustomizeWindowHint), pinLibrary(lib), pin(p)
{
  this->ui = new Ui_name_conflict_gui;

  this->ui->setupUi(this);

  this->ui->NewName->setText(pin->getName().c_str());
  this->ui->NewLabel->setText(pin->getLabel().c_str());
  checkLabels();

  connect( this->ui->Ignore, SIGNAL(toggled(bool)), this, SLOT( selectedIgnore(bool) ) );
  connect( this->ui->OverWrite, SIGNAL(toggled(bool)), this, SLOT(selectedReplace(bool)) );
  connect( this->ui->Rename, SIGNAL(toggled(bool)), this, SLOT(selectedRename(bool)) );
  connect( this->ui->NewLabel, SIGNAL(textEdited(const QString &)), this, SLOT(labelChanged(QString)));
  connect( this->ui->NewName, SIGNAL(textEdited(const QString &)), this, SLOT(nameChanged(QString)));
  connect( this->ui->Apply, SIGNAL(clicked(QAbstractButton *)), this, SLOT(apply()));
}

cmbNucConflictDialog::~cmbNucConflictDialog()
{
  delete ui;
}

void cmbNucConflictDialog::nameChanged(QString v)
{
  bool t = checkLabels();
  if(mode == RENAME) this->ui->Apply->setEnabled(t);
}

void cmbNucConflictDialog::labelChanged(QString v)
{
  bool t = checkLabels();
  if(mode == RENAME) this->ui->Apply->setEnabled(t);
}

void cmbNucConflictDialog::selectedReplace(bool v)
{
  if(v)
  {
    mode = REPLACE;
    this->ui->Apply->setEnabled( true );
    this->ui->NewName->setText(pin->getName().c_str());
    this->ui->NewLabel->setText(pin->getLabel().c_str());
    checkLabels();
    this->ui->NewValue->setEnabled(false);
  }
}

void cmbNucConflictDialog::selectedRename(bool v)
{
  if(v)
  {
    mode = RENAME;
    this->ui->NewValue->setEnabled(true);
    this->ui->Apply->setEnabled(checkLabels());
  }
}

void cmbNucConflictDialog::selectedIgnore(bool v)
{
  if(v)
  {
    mode = IGNORE;
    this->ui->Apply->setEnabled( true );
    this->ui->NewName->setText(pin->getName().c_str());
    this->ui->NewLabel->setText(pin->getLabel().c_str());
    checkLabels();
    this->ui->NewValue->setEnabled(false);
  }
}

void cmbNucConflictDialog::apply()
{
  int rt = 0;
  switch( mode )
  {
    case REPLACE:
      rt = cmbNucPinLibrary::Replace;
      break;
    case IGNORE:
      rt = cmbNucPinLibrary::KeepOriginal;
      break;
    case RENAME:
      rt = cmbNucPinLibrary::KeepOriginal;
      pin->setLabel(this->ui->NewLabel->text().toStdString());
      pin->setName(this->ui->NewName->text().toStdString());
      break;
  }
  this->done( rt );
}

bool cmbNucConflictDialog::checkLabels()
{
  bool r = true;
  std::string name = this->ui->NewName->text().toStdString();
  std::string label = this->ui->NewLabel->text().toStdString();
  if(pinLibrary->nameConflicts(name))
  {
    r = false;
    this->ui->NewName->setStyleSheet("QLineEdit#NewName{color:red}");
  }
  else
  {
    this->ui->NewName->setStyleSheet("QLineEdit#NewName{color:blue}");
  }
  if(pinLibrary->labelConflicts(label))
  {
    r = false;
    this->ui->NewLabel->setStyleSheet("QLineEdit#NewLabel{color:red}");
  }
  else
  {
    this->ui->NewLabel->setStyleSheet("QLineEdit#NewLabel{color:blue}");
  }
  return true;
}
