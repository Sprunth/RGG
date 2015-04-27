#include "cmbNucPreferencesDialog.h"
#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QMainWindow>

#define NAME_PROJECT "RGGNuclear"
#define EXPORTER_NAME "Exporter"

cmbNucPreferencesDialog::cmbNucPreferencesDialog(QMainWindow *mainWindow)
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
  connect( this->ui->postBLBrowseButton, SIGNAL(clicked()),
          this, SLOT(browserPostBLExectuable()) );
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
  connect(this->ui->postBLExecutable, SIGNAL(editingFinished()),
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
  QString postBL_exe = settings.value("PostBL_exe").toString();
  libs = settings.value("PostBL_libs").toString();
  this->ui->postBLExecutable->setText(postBL_exe);
  this->ui->postBLLib->setText(libs);

  bool useCustom = settings.value("custom_meshkit",
                                  QVariant(!cmbNucPreferencesDialog::hasPackaged())).toBool();
  int nop = settings.value("number_of_processors", QVariant(3)).toInt();
  this->ui->numberOfProccessors->setValue(nop);
  this->ui->customMeshkit->setChecked(useCustom);
  this->ui->customMeshkit->setVisible(cmbNucPreferencesDialog::hasPackaged());
  this->ui->AssygenGroup->setVisible(useCustom || !cmbNucPreferencesDialog::hasPackaged());
  this->ui->CoregenGroup->setVisible(useCustom || !cmbNucPreferencesDialog::hasPackaged());
  this->ui->PostBLGroup->setVisible(useCustom || !cmbNucPreferencesDialog::hasPackaged());
  this->checkValues();
  this->show();
}

void cmbNucPreferencesDialog::browserExectuable( QLineEdit* line)
{
  QFileDialog dialog(this);
  dialog.setFileMode(QFileDialog::ExistingFiles);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  QStringList fileNames;
  QObject::connect(&dialog, SIGNAL(fileSelected( const QString& )),
                   line, SLOT(setText(const QString&)));
  QObject::connect(&dialog, SIGNAL(fileSelected( const QString& )),
                   this, SLOT(checkValues()));
  dialog.exec();
}

void cmbNucPreferencesDialog::browserAssygenExecutable()
{
  this->browserExectuable(this->ui->assygenExecutable);
}

void cmbNucPreferencesDialog::browserCubitExecutable()
{
  this->browserExectuable(this->ui->cubitExecutable);
}

void cmbNucPreferencesDialog::browserCoregenExecutable()
{
  this->browserExectuable(this->ui->coregenExecutable);
}

void cmbNucPreferencesDialog::browserPostBLExectuable()
{
  this->browserExectuable(this->ui->postBLExecutable);
}


void cmbNucPreferencesDialog::setValues()
{
  QString assygenExe = ui->assygenExecutable->text();
  QString assygenLibs = ui->AssyGenLib->toPlainText();
  QString cubitExe = ui->cubitExecutable->text();
  QString coregenExe = ui->coregenExecutable->text();
  QString coregenLibs = ui->CoreGenLib->toPlainText();
  QString postBLExe = ui->postBLExecutable->text();
  QString postBLLibs = ui->postBLLib->toPlainText();
  int numberOfProcessors = ui->numberOfProccessors->value();
  QSettings settings(NAME_PROJECT, EXPORTER_NAME);
  settings.setValue("assygen_exe", assygenExe);
  settings.setValue("assygen_libs", assygenLibs);
  settings.setValue("coregen_exe", coregenExe);
  settings.setValue("cubit_exe", cubitExe);
  settings.setValue("coregen_libs", coregenLibs);
  settings.setValue("PostBL_exe", postBLExe);
  settings.setValue("PostBL_libs", postBLLibs);
  settings.setValue("custom_meshkit", this->ui->customMeshkit->isChecked());
  settings.setValue("number_of_processors", numberOfProcessors);
  if(EmitValuesSet) emit valuesSet();
}

void cmbNucPreferencesDialog::checkValues()
{
  bool enabled = true;
  if(this->ui->customMeshkit->isChecked() || !cmbNucPreferencesDialog::hasPackaged())
  {
    QString assygenExe = ui->assygenExecutable->text();
    QString coregenExe = ui->coregenExecutable->text();
    QString postBLExe = ui->postBLExecutable->text();
    QFileInfo ainfo(assygenExe);
    QFileInfo cinfo(coregenExe);
    QFileInfo pinfo(postBLExe);
    enabled &= (!assygenExe.isEmpty() && ainfo.exists() &&
                 ainfo.isExecutable() && !ainfo.isDir()) &&
               (!coregenExe.isEmpty() && cinfo.exists() &&
                 cinfo.isExecutable() && !cinfo.isDir()) &&
               (!postBLExe.isEmpty() && pinfo.exists() &&
                 pinfo.isExecutable() && !pinfo.isDir());
  }
  QString cubitExe = ui->cubitExecutable->text();
#if __APPLE__
  if(QFileInfo(cubitExe).isBundle())
  {
    cubitExe = QFileInfo(cubitExe).dir().absoluteFilePath("Cubit.app/Contents/MacOS/Cubit");
    this->ui->cubitExecutable->setText(cubitExe);
  }
#endif

  enabled &= (!cubitExe.isEmpty() && QFileInfo(cubitExe).exists() &&
              !QFileInfo(cubitExe).isDir() && QFileInfo(cubitExe).isExecutable());

  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}

bool cmbNucPreferencesDialog::isOk()
{
  QSettings settings(NAME_PROJECT, EXPORTER_NAME);
  QString assygenexe = settings.value("assygen_exe").toString();
  QString coregenexe = settings.value("coregen_exe").toString();
  QString cubitexe = settings.value("cubit_exe").toString();
  QString postBLExe = settings.value("PostBL_exe").toString();
  bool useCustom = settings.value("custom_meshkit", QVariant(false)).toBool();
  bool hasCubit = !cubitexe.isEmpty() && QFileInfo(cubitexe).exists();
  bool hasAssygen = !assygenexe.isEmpty() && QFileInfo(assygenexe).exists();
  bool hasCoregen = !coregenexe.isEmpty() && QFileInfo(coregenexe).exists();
  bool hasPostBL = !postBLExe.isEmpty() && QFileInfo(postBLExe).exists();
  bool hasPack = cmbNucPreferencesDialog::hasPackaged();
  bool hasRgg = (!useCustom && hasPack) || ((!hasPack || useCustom) && hasAssygen && hasCoregen);
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
                                            QString & coregenExe, QString & coregenLib,
                                            int & numberOfProcessors )
{
  qDebug() << "Get exe";
  QSettings settings(NAME_PROJECT, EXPORTER_NAME);
  bool useCustom = settings.value("custom_meshkit", QVariant(false)).toBool();
  numberOfProcessors = settings.value("number_of_processors", QVariant(3)).toInt();
  if(useCustom || !cmbNucPreferencesDialog::hasPackaged())
  {
    assygenExe = settings.value("assygen_exe").toString();
    assygenLib = settings.value("assygen_libs").toString();
    coregenExe = settings.value("coregen_exe").toString();
    coregenLib = settings.value("coregen_libs").toString();
  }
  else
  {
    qDebug() << "use packaged exe";
    cmbNucPreferencesDialog::getPackaged(assygenExe, coregenExe);
    qDebug() << "====> Got:" << assygenExe << coregenExe;
  }
  cubitExe = settings.value("cubit_exe").toString();
  qDebug() << assygenExe << cubitExe << coregenExe;
  return cmbNucPreferencesDialog::isOk();
}

bool cmbNucPreferencesDialog::getPostBL(QString & exe, QString & lib)
{
  QSettings settings(NAME_PROJECT, EXPORTER_NAME);
  bool useCustom = settings.value("custom_meshkit", QVariant(false)).toBool();
  if(useCustom || !cmbNucPreferencesDialog::hasPackaged())
  {
    exe = settings.value("PostBL_exe").toString();
    lib = settings.value("PostBL_libs").toString();
  }
  else
  {
    qDebug() << "use packaged exe";
    cmbNucPreferencesDialog::getPackaged(exe);
  }
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
  QDir appDir(QCoreApplication::applicationDirPath());
  assygenExe = QDir::cleanPath(appDir.absoluteFilePath("assygen"));
  coregenExe = QDir::cleanPath(appDir.absoluteFilePath("coregen"));
  if ((!assygenExe.isEmpty() && QFileInfo(assygenExe).exists()) &&
      (!coregenExe.isEmpty() && QFileInfo(coregenExe).exists()))
  {
    return true;
  }
  assygenExe = QDir::cleanPath(appDir.absoluteFilePath("../bin/assygen"));
  coregenExe = QDir::cleanPath(appDir.absoluteFilePath("../bin/coregen"));
  qDebug() << assygenExe << coregenExe;
  if ((!assygenExe.isEmpty() && QFileInfo(assygenExe).exists()) &&
         (!coregenExe.isEmpty() && QFileInfo(coregenExe).exists()))
  {
    return true;
  }
  assygenExe = QDir::cleanPath(appDir.absoluteFilePath("../meshkit/assygen"));
  coregenExe = QDir::cleanPath(appDir.absoluteFilePath("../meshkit/coregen"));
  qDebug() << assygenExe << coregenExe;
  return ((!assygenExe.isEmpty() && QFileInfo(assygenExe).exists()) &&
          (!coregenExe.isEmpty() && QFileInfo(coregenExe).exists()));
#else
  return false;
#endif
}

bool cmbNucPreferencesDialog::getPackaged(QString & postBL)
{
#if __APPLE__
  QDir appDir(QCoreApplication::applicationDirPath());
  postBL = QDir::cleanPath(appDir.absoluteFilePath("../../meshkit/postbl/Contents/bin/PostBL"));
  return (!postBL.isEmpty() && QFileInfo(postBL).exists());
#elif __linux__
  QDir appDir(QCoreApplication::applicationDirPath());
  postBL = QDir::cleanPath(appDir.absoluteFilePath("PostBL"));
  if (!postBL.isEmpty() && QFileInfo(postBL).exists())
  {
    return true;
  }
  postBL = QDir::cleanPath(appDir.absoluteFilePath("../bin/PostBL"));
  if (!postBL.isEmpty() && QFileInfo(postBL).exists())
  {
    return true;
  }
  postBL = QDir::cleanPath(appDir.absoluteFilePath("../meshkit/PostBL"));
  return (!postBL.isEmpty() && QFileInfo(postBL).exists());
#else
  return false;
#endif
}

bool cmbNucPreferencesDialog::getPostBLInpFileGenerator(QString & exe)
{
  if(cmbNucPreferencesDialog::usePackaged())
  {
    QDir appDir(QCoreApplication::applicationDirPath());
    exe = QDir::cleanPath(appDir.absoluteFilePath("cmbGeneratePostBLFile"));
    if (!exe.isEmpty() && QFileInfo(exe).exists())
    {
      return true;
    }
    exe = QDir::cleanPath(appDir.absoluteFilePath("../bin/cmbGeneratePostBLFile"));
    if (!exe.isEmpty() && QFileInfo(exe).exists())
    {
      return true;
    }
    exe = QDir::cleanPath(appDir.absoluteFilePath("../meshkit/cmbGeneratePostBLFile"));
    return (!exe.isEmpty() && QFileInfo(exe).exists());
  }
  //else
#if __APPLE__
  QDir appDir(QCoreApplication::applicationDirPath());
  exe = QDir::cleanPath(appDir.absoluteFilePath("../../../cmbGeneratePostBLFile"));
  return (!exe.isEmpty() && QFileInfo(exe).exists());
#else
  QDir appDir(QCoreApplication::applicationDirPath());
  exe = QDir::cleanPath(appDir.absoluteFilePath("cmbGeneratePostBLFile"));
  if (!exe.isEmpty() && QFileInfo(exe).exists())
  {
    return true;
  }
#endif
  return false;
}
