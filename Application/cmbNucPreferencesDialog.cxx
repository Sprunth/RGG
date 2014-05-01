#include "cmbNucPreferencesDialog.h"
#include "cmbNucMainWindow.h"
#include <QSettings>
#include <QFileDialog>

cmbNucPreferencesDialog::cmbNucPreferencesDialog(cmbNucMainWindow *mainWindow)
: QDialog(mainWindow)
{
  this->ui = new Ui_Preferences;
  this->ui->setupUi(this);

  connect( this->ui->assygenExeBrowseButton, SIGNAL(clicked()),
          this, SLOT(browserAssygenExecutable()) );
  connect( this->ui->cubitExeBrowseButton, SIGNAL(clicked()),
          this, SLOT(browserCubitExecutable()) );
  connect( this->ui->coregenExeBrowseButton, SIGNAL(clicked()),
          this, SLOT(browserCoregenExecutable()) );
  connect( this->ui->buttonBox, SIGNAL(accepted()),
          this, SLOT(setValues() ));
  connect(this->ui->parallel_Projection, SIGNAL(clicked(bool)),
          this, SIGNAL(actionParallelProjection(bool)));
}

cmbNucPreferencesDialog::~cmbNucPreferencesDialog()
{
}

void cmbNucPreferencesDialog::setPreferences()
{
  this->hide();
  QSettings settings("CMBNuclear", "CMBNuclear");
  QString assygenexe = settings.value("EXPORTER/assygen_exe").toString();
  this->ui->assygenExecutable->setText(assygenexe);
  QString coregenexe = settings.value("EXPORTER/coregen_exe").toString();
  this->ui->coregenExecutable->setText(coregenexe);
  QString cubitexe = settings.value("EXPORTER/cubit_exe").toString();
  this->ui->cubitExecutable->setText(cubitexe);
  this->show();
}

void cmbNucPreferencesDialog::browserAssygenExecutable()
{
  QFileDialog dialog(this);
  dialog.setFileMode(QFileDialog::ExistingFiles);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  QStringList fileNames;
  QObject::connect(&dialog, SIGNAL(fileSelected( const QString& )),
                   this->ui->assygenExecutable, SLOT(setText(const QString&)));
  dialog.exec();
}

void cmbNucPreferencesDialog::browserCubitExecutable()
{
  QFileDialog dialog(this);
  dialog.setFileMode(QFileDialog::ExistingFiles);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  QObject::connect(&dialog, SIGNAL(fileSelected( const QString& )),
                   this->ui->cubitExecutable, SLOT(setText(const QString&)));
  dialog.exec();
}

void cmbNucPreferencesDialog::browserCoregenExecutable()
{
  QFileDialog dialog(this);
  dialog.setFileMode(QFileDialog::ExistingFiles);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  QObject::connect(&dialog, SIGNAL(fileSelected( const QString& )),
                   this->ui->coregenExecutable, SLOT(setText(const QString&)));
  dialog.exec();
}

void cmbNucPreferencesDialog::setValues()
{
  QString assygenExe = ui->assygenExecutable->text();
  QString cubitExe = ui->cubitExecutable->text();
  QString coregenExe = ui->coregenExecutable->text();
  QSettings settings("CMBNuclear", "CMBNuclear");
  settings.setValue("EXPORTER/assygen_exe", assygenExe);
  settings.setValue("EXPORTER/coregen_exe", coregenExe);
  settings.setValue("EXPORTER/cubit_exe", cubitExe);

}
