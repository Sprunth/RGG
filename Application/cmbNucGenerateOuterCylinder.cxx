#include "cmbNucGenerateOuterCylinder.h"

#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include "cmbNucPartsTreeItem.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucDefaults.h"
#include "Ui_CylinderCreateGui.h"

#include "cmbNucMainWindow.h"
#include "inpFileIO.h"

#include <QFileDialog>
#include <QSettings>

QString GetRandomString(int length)
{
  const QString possibleCharacters("abcdefghijklmnopqrstuvwxyz0123456789");

  QString randomString;
  for(int i=0; i<length; ++i)
  {
    int index = qrand() % possibleCharacters.length();
    QChar nextChar = possibleCharacters.at(index);
    randomString.append(nextChar);
  }
  return randomString;
}

cmbNucGenerateOuterCylinder
::cmbNucGenerateOuterCylinder(cmbNucMainWindow* mainWindow)
: Core(NULL), MainWindow(mainWindow),
  QDialog(mainWindow, Qt::CustomizeWindowHint |
                      Qt::WindowTitleHint |
                      Qt::WindowMinMaxButtonsHint)
{
  ui = new Ui_CylinderCreateGui;
  this->ui->setupUi(this);
  Exporter = new cmbNucExport();
  Exporter->moveToThread(&Thread);

  this->ui->Generate->setEnabled(false);

  connect( this->Exporter, SIGNAL(statusMessage(QString)),
           this->ui->ProcessOutput, SLOT(appendPlainText(const QString&)) );
  connect( this, SIGNAL(process( const QString, const QString, const QString,
                                 const QString, const QString, const QString,
                                 const QString, const QString, const QString,
                                 const QString )),
           this->Exporter, SLOT(runCylinder(const QString, const QString, const QString,
                                            const QString, const QString, const QString,
                                            const QString, const QString, const QString,
                                            const QString)));
  connect(this->Exporter, SIGNAL(successful()),
          this, SLOT(done()));
  connect( this->ui->Set, SIGNAL(clicked()),
           this, SLOT(SetFileName()) );
  connect( this->ui->Generate, SIGNAL(clicked()),
          this, SLOT(Generate()));
  connect( this->ui->Cancel, SIGNAL(clicked()),
          this, SLOT(cancel()));

  connect(this->ui->OuterEdgeInterval, SIGNAL(valueChanged(int)),
          this, SLOT(updateCylinder()));
  connect(this->ui->RadiusBox, 	SIGNAL(valueChanged(double)),
          this, SLOT(updateCylinder()));

  Thread.start();
}

cmbNucGenerateOuterCylinder
::~cmbNucGenerateOuterCylinder()
{
  deleteTempFiles();
  delete ui;
  delete Exporter;
}

void
cmbNucGenerateOuterCylinder
::exportFile(cmbNucCore * core)
{
  Core = core;
  double initRadius = Core->getLattice().Grid.size() * Core->AssyemblyPitchX;
  this->ui->RadiusBox->setValue(initRadius);
  int ei;
  Core->GetDefaults()->getEdgeInterval(ei);
  this->ui->OuterEdgeInterval->setValue(Core->getLattice().Grid.size()*ei*12);
  deleteTempFiles();
  random = GetRandomString(8);
  this->show();
  updateCylinder();
}

void cmbNucGenerateOuterCylinder
::SetFileName()
{
  this->ui->Generate->setEnabled(true);
  QString defaultLoc;
  QString name(Core->FileName.c_str());
  if(!name.isEmpty())
  {
    QFileInfo fi(name);
    QDir dir = fi.dir();
    if(dir.path() == ".")
    {
      QDir tdir = QSettings("CMBNuclear", "CMBNuclear").value("cache/lastDir",
                                                              QDir::homePath()).toString();
      defaultLoc = tdir.path();
    }
    else
    {
      defaultLoc = dir.path();
    }
  }
  else
  {
    QDir tdir = QSettings("CMBNuclear", "CMBNuclear").value("cache/lastDir",
                                                            QDir::homePath()).toString();
    defaultLoc = tdir.path();
  }
  QFileDialog saveQD( this, "Save Outer Cylinder File...", defaultLoc, "cub Files (*.cub)");
  saveQD.setOptions(QFileDialog::DontUseNativeDialog); //There is a bug on the mac were one does not seem to be able to set the default name.
  saveQD.setAcceptMode(QFileDialog::AcceptSave);
  saveQD.selectFile("outer_cylinder.cub");
  QString fileName;
  if(saveQD.exec()== QDialog::Accepted)
  {
    FileName = saveQD.selectedFiles().first();
  }
  else
  {
    return;
  }

  QFileInfo info(FileName);
  this->ui->FileName->setText(info.fileName());
}

void
cmbNucGenerateOuterCylinder
::Generate()
{
  deleteTempFiles();
  emit clearCylinder();
  this->ui->FileName->setText("");
  this->ui->ProcessOutput->clear();
  this->ui->Generate->setEnabled(false);
  //Generate temp inp file of outer cores of an assembly
  QFileInfo fi(FileName);
  cmbNucAssembly * temp = this->Core->GetUsedAssemblies()[0];
  QString fname = QString(temp->getLabel().c_str()) + random + ".inp";
  fname = fname.toLower();
  QString fullPath =fi.dir().absoluteFilePath(fname);
  inpFileWriter::write(fullPath.toStdString(), *temp, false, true);

  //Generate temp inp file of type geometry of core
  QString corename = QString("core") + random + ".inp";
  fullPath =fi.dir().absoluteFilePath(corename);
  inpFileWriter::writeGSH(fullPath.toStdString(), *Core, fname.toStdString());

  //Generate temp jou file for coregen
  QString jouname = QString("cylinder") + random + ".jou";
  fullPath =fi.dir().absoluteFilePath(jouname);
  std::ofstream output(fullPath.toStdString().c_str());
  output << "{include(\"" << QFileInfo(fname).completeBaseName().toStdString() << ".template.jou\")}\n";
  output << "{rings = " << Core->getLattice().Grid.size() << "}\n";
  output << "#{OUTER_CYL_EDGE_INTERVAL = " << this->ui->OuterEdgeInterval->text().toInt() << "}\n";
  output << "#{rd = " << this->ui->RadiusBox->text().toDouble() << "}\n";
  output << "#{tol = 1e-2}\n";
  output << "#{TOTAL_VOLS_LARGE = 4000}\n";
  output << "{xmove = rings*PITCH}\n";
  output << "{y0 = (rings-1)*PITCH*cosd(30)}\n";
  output << "create cylinder radius {rd} height {Z_HEIGHT}\n";
  output << "move vol 1 x {xmove} y {-y0}  z {Z_HEIGHT/2}\n";
  output << "group 'one' equals vol 1\n";
  output << "import '" <<  QFileInfo(corename).completeBaseName().toStdString() << ".sat'\n";
  output << "group 'gall' equals vol 2 to {TOTAL_VOLS_LARGE}\n";
  output << "subtract vol 2 to 4000 from vol 1\n";
  output << "export acis '" << QFileInfo(jouname).completeBaseName().toStdString() << ".sat' over\n";
  output << "surface with z_coord > {Z_MID -.1*Z_HEIGHT} and z_coord < {Z_MID + .1*Z_HEIGHT} size {AXIAL_MESH_SIZE}\n";
  output << "curve with z_coord > {Z_HEIGHT - tol} and length < {PITCH} interval {TOP_EDGE_INTERVAL}\n";
  output << "curve with z_coord > {Z_HEIGHT - tol} and length > {PITCH} interval {OUTER_CYL_EDGE_INTERVAL}\n";
  output << "mesh vol all\n";
  output << "block 9999 vol all\n";
  output << "save as '" << fi.fileName().toStdString() << "' over\n";
  output.close();

  QSettings settings("CMBNuclear", "CMBNuclear");
  QString assygenExe = settings.value("EXPORTER/assygen_exe").toString();
  QString assyGenLibs = settings.value("EXPORTER/assygen_libs").toString();
  QString coregenExe = settings.value("EXPORTER/coregen_exe").toString();
  QString cubitExe = settings.value("EXPORTER/cubit_exe").toString();
  QString coreGenLibs = settings.value("EXPORTER/coregen_libs").toString();

  emit process(assygenExe, assyGenLibs, fi.dir().absoluteFilePath(fname),
               cubitExe, fi.dir().absoluteFilePath(jouname), QString(FileName),
               coregenExe, coreGenLibs, fi.dir().absoluteFilePath(corename),
               fi.dir().absoluteFilePath(QFileInfo(corename).completeBaseName() + ".sat") );
}

void cmbNucGenerateOuterCylinder
::done()
{
  emit finished(FileName);
  deleteTempFiles();
  this->hide();
}

void cmbNucGenerateOuterCylinder
::cancel()
{
  this->hide();
  this->Exporter->cancel();
  deleteTempFiles();
  emit clearCylinder();
}

void cmbNucGenerateOuterCylinder
::deleteTempFiles()
{
  if(random.isEmpty()) return;
  QString path = QFileInfo(FileName).dir().absolutePath();;
  QDir dir(path);
  dir.setNameFilters(QStringList() << ("*" + random + ".*"));
  dir.setFilter(QDir::Files);
  foreach(QString dirFile, dir.entryList())
  {
    dir.remove(dirFile);
  }
}

void cmbNucGenerateOuterCylinder
::updateCylinder()
{
  int i = this->ui->OuterEdgeInterval->text().toInt();
  double r = this->ui->RadiusBox->text().toDouble();
  emit drawCylinder(r, i);
}
