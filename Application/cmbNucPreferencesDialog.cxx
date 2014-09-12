#include "cmbNucPreferencesDialog.h"
#include "cmbNucMainWindow.h"
#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>

#define NAME_PROJECT "RGGNuclear"
#define EXPORTER_NAME "Exporter"

cmbNucPreferencesDialog::cmbNucPreferencesDialog(cmbNucMainWindow *mainWindow)
: QDialog(mainWindow)
{
  EmitValuesSet = false;
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
  connect(this->ui->customMeshkit, SIGNAL(clicked(bool)),
          this->ui->AssygenGroup, SLOT(setVisible(bool)));
  connect(this->ui->customMeshkit, SIGNAL(clicked(bool)),
          this->ui->CoregenGroup, SLOT(setVisible(bool)));
  connect(this->ui->assygenExecutable, SIGNAL(editingFinished()),
          this, SLOT(checkValues()));
  connect(this->ui->coregenExecutable, SIGNAL(editingFinished()),
          this, SLOT(checkValues()));
  connect(this->ui->cubitExecutable, SIGNAL(editingFinished()),
          this, SLOT(checkValues()));
}

cmbNucPreferencesDialog::~cmbNucPreferencesDialog()
{
  delete this->ui;
}

void cmbNucPreferencesDialog::setPreferences(bool e)
{
  EmitValuesSet = e;
  this->hide();
  //Keeping this because static methods could pull unsaved values
  QSettings settings( NAME_PROJECT, EXPORTER_NAME );
  qDebug() << settings.fileName();
  QString assygenexe = settings.value("assygen_exe").toString();
  this->ui->assygenExecutable->setText(assygenexe);
  QString libs = settings.value("assygen_libs").toString();
  this->ui->AssyGenLib->setText(libs);
  QString coregenexe = settings.value("coregen_exe").toString();
  this->ui->coregenExecutable->setText(coregenexe);
  QString cubitexe = settings.value("cubit_exe").toString();
  this->ui->cubitExecutable->setText(cubitexe);
  libs = settings.value("coregen_libs").toString();
  this->ui->CoreGenLib->setText(libs);
  bool useCustom = settings.value("custom_meshkit",
                                  QVariant(!cmbNucPreferencesDialog::hasPackaged())).toBool();
  this->ui->customMeshkit->setChecked(useCustom);
  this->ui->customMeshkit->setVisible(cmbNucPreferencesDialog::hasPackaged());
  this->ui->AssygenGroup->setVisible(useCustom || !cmbNucPreferencesDialog::hasPackaged());
  this->ui->CoregenGroup->setVisible(useCustom || !cmbNucPreferencesDialog::hasPackaged());
  this->checkValues();
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
  QObject::connect(&dialog, SIGNAL(fileSelected( const QString& )),
                   this, SLOT(checkValues()));
  dialog.exec();
}

void cmbNucPreferencesDialog::browserCubitExecutable()
{
  QFileDialog dialog(this);
  dialog.setFileMode(QFileDialog::ExistingFiles);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  QObject::connect(&dialog, SIGNAL(fileSelected( const QString& )),
                   this->ui->cubitExecutable, SLOT(setText(const QString&)));
  QObject::connect(&dialog, SIGNAL(fileSelected( const QString& )),
                   this, SLOT(checkValues()));
  dialog.exec();
}

void cmbNucPreferencesDialog::browserCoregenExecutable()
{
  QFileDialog dialog(this);
  dialog.setFileMode(QFileDialog::ExistingFiles);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  QObject::connect(&dialog, SIGNAL(fileSelected( const QString& )),
                   this->ui->coregenExecutable, SLOT(setText(const QString&)));
  QObject::connect(&dialog, SIGNAL(fileSelected( const QString& )),
                   this, SLOT(checkValues()));
  dialog.exec();
}

void cmbNucPreferencesDialog::setValues()
{
  QString assygenExe = ui->assygenExecutable->text();
  QString assygenLibs = ui->AssyGenLib->toPlainText();
  QString cubitExe = ui->cubitExecutable->text();
  QString coregenExe = ui->coregenExecutable->text();
  QString coregenLibs = ui->CoreGenLib->toPlainText();
  QSettings settings(NAME_PROJECT, EXPORTER_NAME);
  settings.setValue("assygen_exe", assygenExe);
  settings.setValue("assygen_libs", assygenLibs);
  settings.setValue("coregen_exe", coregenExe);
  settings.setValue("cubit_exe", cubitExe);
  settings.setValue("coregen_libs", coregenLibs);
  settings.setValue("custom_meshkit", this->ui->customMeshkit->isChecked());
  if(EmitValuesSet) emit valuesSet();
}

void cmbNucPreferencesDialog::checkValues()
{
  bool isEnabled = true;
  if(this->ui->customMeshkit->isChecked() || !cmbNucPreferencesDialog::hasPackaged())
  {
    QString assygenExe = ui->assygenExecutable->text();
    QString coregenExe = ui->coregenExecutable->text();
    QFileInfo ainfo(assygenExe);
    QFileInfo cinfo(coregenExe);
    isEnabled &= (!assygenExe.isEmpty() && ainfo.exists() &&
                  ainfo.isExecutable() && !ainfo.isDir()) &&
                 (!coregenExe.isEmpty() && cinfo.exists() &&
                  cinfo.isExecutable() && !cinfo.isDir());
  }
  QString cubitExe = ui->cubitExecutable->text();
#if __APPLE__
  if(QFileInfo(cubitExe).isBundle())
  {
    cubitExe = QFileInfo(cubitExe).dir().absoluteFilePath("Cubit.app/Contents/MacOS/Cubit");
    this->ui->cubitExecutable->setText(cubitExe);
  }
#endif

  isEnabled &= (!cubitExe.isEmpty() && QFileInfo(cubitExe).exists() &&
                !QFileInfo(cubitExe).isDir() && QFileInfo(cubitExe).isExecutable());

  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isEnabled);
}

bool cmbNucPreferencesDialog::isOk()
{
  QSettings settings(NAME_PROJECT, EXPORTER_NAME);
  QString assygenexe = settings.value("assygen_exe").toString();
  QString coregenexe = settings.value("coregen_exe").toString();
  QString cubitexe = settings.value("cubit_exe").toString();
  bool useCustom = settings.value("custom_meshkit", QVariant(false)).toBool();
  bool hasCubit = !cubitexe.isEmpty() && QFileInfo(cubitexe).exists();
  bool hasAssygen = !assygenexe.isEmpty() && QFileInfo(assygenexe).exists();
  bool hasCoregen = !coregenexe.isEmpty() && QFileInfo(coregenexe).exists();
  bool hasPack = cmbNucPreferencesDialog::hasPackaged();
  bool hasRgg = (!useCustom && hasPack) || (useCustom && hasAssygen && hasCoregen);
  return hasRgg && hasCubit;
}

bool cmbNucPreferencesDialog::hasPackaged()
{
  QString assygenExe, coregenExe;
  return getPackaged(assygenExe,coregenExe);
}

bool cmbNucPreferencesDialog::usePackaged()
{
  QSettings settings(NAME_PROJECT, EXPORTER_NAME);
  bool useCustom = settings.value("custom_meshkit", QVariant(false)).toBool();
  return cmbNucPreferencesDialog::hasPackaged() && !useCustom;
}

bool cmbNucPreferencesDialog::getExecutable(QString & assygenExe, QString & assygenLib,
                                            QString & cubitExe,
                                            QString & coregenExe, QString & coregenLib)
{
  QSettings settings(NAME_PROJECT, EXPORTER_NAME);
  bool useCustom = settings.value("custom_meshkit",
                                  QVariant(cmbNucPreferencesDialog::hasPackaged())).toBool();
  if(useCustom || !cmbNucPreferencesDialog::hasPackaged())
  {
    assygenExe = settings.value("assygen_exe").toString();
    assygenLib = settings.value("assygen_libs").toString();
    coregenExe = settings.value("coregen_exe").toString();
    coregenLib = settings.value("coregen_libs").toString();
  }
  else
  {
    getPackaged(assygenExe, coregenExe);
  }
  cubitExe = settings.value("cubit_exe").toString();
  return cmbNucPreferencesDialog::isOk();
}

bool cmbNucPreferencesDialog::getPackaged(QString & assygenExe, QString & coregenExe)
{
#if __APPLE__
  QDir appDir(QCoreApplication::applicationDirPath());
  assygenExe = QDir::cleanPath(appDir.absoluteFilePath("../../meshkit/assygen/Contents/bin/assygen"));
  coregenExe = QDir::cleanPath(appDir.absoluteFilePath("../../meshkit/coregen/Contents/bin/coregen"));
  return (!assygenExe.isEmpty() && QFileInfo(assygenExe).exists()) &&
         (!coregenExe.isEmpty() && QFileInfo(coregenExe).exists());
#elif __linux__
  return false; //for now
#else
  return false;
#endif
}
