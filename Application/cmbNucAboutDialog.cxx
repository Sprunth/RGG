#include "cmbNucAboutDialog.h"
#include "ui_qAboutDialog.h"

#include "macro_helpers.h"

cmbNucAboutDialog::cmbNucAboutDialog(QWidget* Parent)
 :  QDialog(Parent),
    Ui(new Ui::AboutDialog())
{
  this->Ui->setupUi(this);
  this->setObjectName("qtCMBAboutDialog");
  this->setWindowTitle(QApplication::translate("RGG AboutDialog",
                                               "About RGG",
                                               0, QApplication::UnicodeUTF8));
  if(!QString(RGG_VERSION_STR).contains("Devel"))
  {
    this->Ui->buildID->setVisible(false);
    this->Ui->label_2->setVisible(false);
  }
  this->Ui->version->setText(RGG_VERSION_STR);
  this->Ui->buildID->setText("<html><head/><body><p><a "
                             "href=\"https://github.com/Kitware/RGG/commit/"
                             RGG_BUILD_ID_STR
                             "\"><span style=\" text-decoration: underline; "
                             "color:#0000ff;\">" RGG_BUILD_ID_STR
                             "</span></a></p></body></html>");
}

cmbNucAboutDialog::~cmbNucAboutDialog()
{
  delete this->Ui;
}
