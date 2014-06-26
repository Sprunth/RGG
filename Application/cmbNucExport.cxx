#ifndef cmbNucExport_cxx
#define cmbNucExport_cxx //incase this is included for template instant

#include <remus/client/Client.h>

#include <remus/worker/Worker.h>

#include <remus/server/Server.h>
#include <remus/server/WorkerFactory.h>
#include <remus/common/MeshTypes.h>
#include <remus/common/ExecuteProcess.h>
//#include <remus/client/JobResult.h>
#include <remus/proto/JobResult.h>

#include <QDebug>
#include <QFileInfo>
#include <QStringList>
#include <QDir>
#include <QMutexLocker>
#include <QEventLoop>
#include <QThread>

#include <iostream>
#include <sstream>
#include <fstream>

#include <stdlib.h>

#include "cmbNucExport.h"

namespace
{
class Thread : public QThread
{
public:
  static void msleep(int ms)
  {
    QThread::msleep(ms);
  }
};
}

class DoNothingFactory: public remus::server::WorkerFactory
{
public:
  virtual remus::proto::JobRequirementsSet workerRequirements(remus::common::MeshIOType type) const
  {
    remus::proto::JobRequirementsSet result;
    result.insert(remus::proto::JobRequirements(remus::common::ContentFormat::Type(),
                                                type, std::string(""), std::string("") ));
    return result;
  }

  virtual bool haveSupport(const remus::proto::JobRequirements& reqs) const
  { return true; }

  virtual bool createWorker(const remus::proto::JobRequirements& type,
                            WorkerFactory::FactoryDeletionBehavior lifespan)
  { return false; }
};

struct ExporterInput
{
  std::string ExeDir;
  std::string Function;
  std::string LibPath;
  std::string FileArg;
  operator std::string(void) const
  {
    std::stringstream ss;
    ss << ExeDir << ';' << FileArg << ";" <<Function << ";" << LibPath;
    return ss.str();
  }
  ExporterInput(std::string exeDir, std::string fun, std::string file)
  :ExeDir(exeDir), Function(fun),FileArg(file)
  {
  }
  ExporterInput(QString exeDir, QString fun, QString file)
  :ExeDir(exeDir.trimmed().toStdString()), Function(fun.trimmed().toStdString()),
   FileArg(file.trimmed().toStdString())
  {  }
  ExporterInput(const remus::worker::Job& job)
  {
    qDebug() << remus::worker::to_string(job).c_str();
    std::stringstream ss(job.details("data"));
    getline(ss, ExeDir,   ';');
    getline(ss, FileArg,  ';');
    getline(ss, Function, ';');
    getline(ss, LibPath,  ';');
  }
};

struct ExporterOutput
{
  bool Valid;
  std::string Result;
  ExporterOutput():Valid(false){}
  ExporterOutput(const remus::proto::JobResult& job, bool valid=true)
  : Valid(valid), Result(job.data())
  {}
  operator std::string(void) const
  { return Result; }
};

template<class JOB_REQUEST_IN, class JOB_REQUEST_OUT>
class cmbNucExporterClient
{
public:
  cmbNucExporterClient(std::string label, std::string server = "");
  ~cmbNucExporterClient()
  {delete Client;}
  ExporterOutput getOutput(ExporterInput const& in);
private:
  remus::client::ServerConnection Connection;
  remus::Client * Client;
  std::string Label;
};

typedef cmbNucExporterClient<remus::meshtypes::SceneFile,remus::meshtypes::Model> AssygenExporter;
typedef cmbNucExporterClient<remus::meshtypes::Mesh3D,remus::meshtypes::Mesh3D> CoregenExporter;
typedef cmbNucExporterClient<remus::meshtypes::Model,remus::meshtypes::Mesh3D> CubitExporter;

cmbNucExporterWorker *
cmbNucExporterWorker
::AssygenWorker(remus::worker::ServerConnection const& connection,
                std::vector<std::string> extra_args)
{
  remus::common::MeshIOType miot((remus::meshtypes::SceneFile()), (remus::meshtypes::Model()));
  cmbNucExporterWorker * r =
         new cmbNucExporterWorker("Assygen", miot, connection, extra_args );
  return r;
}

cmbNucExporterWorker *
cmbNucExporterWorker
::CoregenWorker(remus::worker::ServerConnection const& connection,
                std::vector<std::string> extra_args)
{
  remus::common::MeshIOType miot((remus::meshtypes::Mesh3D()), (remus::meshtypes::Mesh3D()));
  cmbNucExporterWorker * r =
        new cmbNucExporterWorker("Coregen",  miot, connection, extra_args );
  return r;
}

cmbNucExporterWorker *
cmbNucExporterWorker
::CubitWorker(remus::worker::ServerConnection const& connection,
              std::vector<std::string> extra_args)
{
  remus::common::MeshIOType miot((remus::meshtypes::Model()), (remus::meshtypes::Mesh3D()));
  cmbNucExporterWorker * r =
         new cmbNucExporterWorker("Cubit", miot, connection, extra_args );
  return r;
}

cmbNucExporterWorker
::cmbNucExporterWorker( std::string label,
                        remus::common::MeshIOType miot,
                        remus::worker::ServerConnection const& connection,
                        std::vector<std::string> extra_args)
:remus::worker::Worker( remus::proto::make_JobRequirements(miot, label, ""), connection),
 ExtraArgs(extra_args)
{
  Name = label;
}

void cmbNucExporterWorker::run()
{
  {
    QMutexLocker locker(&Mutex);
    StillRunning = true;
  }
  while(stillRunning())
  {
    remus::worker::Job job = this->getJob();

    if(!job.valid())
    {
      switch(job.validityReason())
      {
        case remus::worker::Job::INVALID:
          emit errorMessage("JOB NOT VALID");
          continue;
        case remus::worker::Job::TERMINATE_WORKER:
          {
          this->stop();
          }
          continue;
        case remus::worker::Job::VALID_JOB:
          ((void)0); //have valid job, move on.
      }
    }

    ExporterInput input(job);

    //Run in execute directory
    QString current = QDir::currentPath();
    QDir::setCurrent( input.ExeDir.c_str() );

    //make a cleaned up path with no relative
    std::vector<std::string> args;
    for( std::vector<std::string>::const_iterator i = ExtraArgs.begin();
        i < ExtraArgs.end(); ++i )
    {
      args.push_back(*i);
    }
    args.push_back(input.FileArg);

#ifdef __APPLE__
    char* oldEnv = getenv("DYLD_LIBRARY_PATH");
    std::string env(input.LibPath);
    setenv("DYLD_LIBRARY_PATH", env.c_str(), 1);
    qDebug() << env.c_str();
    qDebug() << oldEnv;
#elif  __linux
    char* oldEnv = getenv("LD_LIBRARY_PATH");
    std::string env(input.LibPath);
    setenv("LD_LIBRARY_PATH", env.c_str(), 1);
#endif

    remus::common::ExecuteProcess* process = new remus::common::ExecuteProcess( input.Function, args);

    //actually launch the new process
    process->execute(remus::common::ExecuteProcess::Attached);

    //Wait for finish
    if(pollStatus(process, job))
    {
      remus::proto::JobResult results = remus::proto::make_JobResult(job.id(),"DUMMY FOR NOW;");
      this->returnMeshResults(results);
    }
    else
    {
      remus::proto::JobStatus status(job.id(),remus::FAILED);
      updateStatus(status);
    }
    QDir::setCurrent( current );
    delete process;

#ifdef __APPLE__
    if(oldEnv != NULL)
    {
      setenv("DYLD_LIBRARY_PATH", oldEnv, 1);
    }
#elif  __linux
    if(oldEnv != NULL)
    {
      setenv("LD_LIBRARY_PATH", oldEnv, 1);
    }
#endif
  }
}

void cmbNucExporterWorker::stop()
{
  QMutexLocker locker(&Mutex);
  StillRunning = false;
}

bool cmbNucExporterWorker::stillRunning()
{
  QMutexLocker locker(&Mutex);
  return StillRunning;
}

bool cmbNucExporterWorker
::pollStatus( remus::common::ExecuteProcess* process,
              const remus::worker::Job& job)
{
  typedef remus::common::ProcessPipe ProcessPipe;

  //poll on STDOUT and STDERRR only
  bool validExection=true;
  remus::proto::JobStatus status(job.id(),remus::IN_PROGRESS);
  while(process->isAlive()&& validExection && this->stillRunning())
  {
    //poll till we have a data, waiting for-ever!
    ProcessPipe data = process->poll(2);
    if(data.type == ProcessPipe::STDOUT || data.type == ProcessPipe::STDERR)
    {
      //we have something on the output pipe
      remus::proto::JobProgress progress(data.text);
      status.updateProgress(progress);
      emit currentMessage( QString(data.text.c_str()) );
    }
  }
  if(process->isAlive())
  {
    process->kill();
  }

  //verify we exited normally, not segfault or numeric exception
  validExection &= process->exitedNormally();

  if(!validExection)
  {
    return false;
  }
  return true;
}

template<class JOB_REQUEST_IN, class JOB_REQUEST_OUT>
cmbNucExporterClient<JOB_REQUEST_IN, JOB_REQUEST_OUT>::cmbNucExporterClient(std::string label,
                                                                            std::string server)
{
  Label = label;
  if ( !server.empty())
    {
    Connection = remus::client::make_ServerConnection(server);
    }
  Client = new remus::Client(Connection);
}

template<class JOB_REQUEST_IN, class JOB_REQUEST_OUT>
ExporterOutput
cmbNucExporterClient<JOB_REQUEST_IN, JOB_REQUEST_OUT>::getOutput(ExporterInput const& in)
{
  JOB_REQUEST_IN in_type;
  JOB_REQUEST_OUT out_type;
  ExporterOutput eo;
  remus::common::MeshIOType mesh_types(in_type,out_type);
  remus::proto::JobRequirements reqs =
    remus::proto::make_JobRequirements(mesh_types, Label,"");
  unsigned int i = 0;
  if(Client->canMesh(reqs))
    {
    remus::proto::JobContent content =
    remus::proto::make_JobContent(in);

    remus::proto::JobSubmission sub(reqs,content);

    remus::proto::Job job = Client->submitJob(sub);
    remus::proto::JobStatus jobState = Client->jobStatus(job);

    //wait while the job is running
    QEventLoop el;
    while(jobState.good())
      {
      el.processEvents();
      jobState = Client->jobStatus(job);
      };

    if(jobState.finished())
      {
      remus::proto::JobResult result = Client->retrieveResults(job);
      return ExporterOutput(result);
      }
    else if(jobState.status() == remus::INVALID_STATUS)
      {
      eo.Result = " Remus Invalid Status";
      }
    else if(jobState.status() == remus::FAILED)
      {
      eo.Result = " Remus Failed";
      }
    else if(jobState.status() == remus::EXPIRED)
      {
      eo.Result = " Remus Expired";
      }
    else
      {
      eo.Result = " Remus did not finish but was not good";
      }
    }
  else
    {
    eo.Result = " Remus Server does not support the job request";
    }
  return eo;
}

cmbNucExport::cmbNucExport()
: assygenWorker(NULL),
  coregenWorker(NULL),
  cubitWorker(NULL),
  Server(NULL),
  factory(new DoNothingFactory())
{
}

cmbNucExport::~cmbNucExport()
{
}

void cmbNucExport::run( const QString assygenExe,
                        const QString assygenLib,
                        const QStringList &assygenFile,
                        const QString cubitExe,
                        const QString coregenExe,
                        const QString coregenLib,
                        const QString coregenFile,
                        const QString CoreGenOutputFile )
{
  double total_number_of_file = 2.0*assygenFile.count() + 6;
  double current = 0;

  if(!this->startUpHelper(current, total_number_of_file)) return;

  if(!this->runAssyHelper( assygenExe, assygenLib, assygenFile, cubitExe,
                           current, total_number_of_file) )
    return;

  if(!this->runCoreHelper( coregenExe, coregenLib, coregenFile, CoreGenOutputFile,
                           current, total_number_of_file ))
    return;

  this->finish();
}

void cmbNucExport::runAssy( const QString assygenExe,
                            const QString assygenLib,
                            const QStringList &assygenFile,
                            const QString cubitExe )
{
  double total_number_of_file = 2.0*assygenFile.count() + 5;
  double current = 0;
  if(!this->startUpHelper(current, total_number_of_file)) return;
  if(!this->runAssyHelper( assygenExe, assygenLib, assygenFile, cubitExe,
                          current, total_number_of_file) )
    return;
  this->finish();
}

void cmbNucExport::runCore( const QString coregenExe,
                            const QString coregenLib,
                            const QString coregenFile,
                            const QString CoreGenOutputFile )
{
  double total_number_of_file = 6;
  double current = 0;
  if(!this->startUpHelper(current, total_number_of_file)) return;
  if(!this->runCoreHelper( coregenExe, coregenLib, coregenFile, CoreGenOutputFile,
                          current, total_number_of_file ))
    return;

  this->finish();
}



void cmbNucExport::cancelHelper()
{
  emit currentProcess("  CANCELED");
  emit done();
  this->deleteServer();
  this->deleteWorkers();
}

void cmbNucExport::failedHelper(QString msg, QString pmsg)
{
  emit errorMessage("ERROR: " + msg );
  emit currentProcess(pmsg + " FAILED");
  emit done();
  this->deleteServer();
  this->deleteWorkers();
}

bool cmbNucExport::runAssyHelper( const QString assygenExe,
                                  const QString assygenLib,
                                  const QStringList &assygenFile,
                                  const QString cubitExe,
                                  double & count, double max_count )
{
  AssygenExporter ae("Assygen");
  CubitExporter ce("Cubit");
  for (QStringList::const_iterator i = assygenFile.constBegin();
       i != assygenFile.constEnd(); ++i)
  {
    if(!keepGoing())
    {
      cancelHelper();
      return false;
    }
    QFileInfo fi(*i);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName();
    QString pass = path + '/' + name;
    emit currentProcess("  assygen " + name);
    QFile::remove(pass + ".jou");
    ExporterInput in(path, assygenExe, pass);
    {
      in.LibPath = "";
      std::stringstream ss(assygenLib.toStdString().c_str());
      std::string line;
      unsigned int i = 0;
      while( std::getline(ss, line))
      {
        in.LibPath += line + ":";
      }
      QFileInfo libPaths(assygenExe);
      in.LibPath += (libPaths.absolutePath() + ":" + libPaths.absolutePath() + "/../lib").toStdString();
      in.LibPath += ":"+ QFileInfo(cubitExe).absolutePath().toStdString();
    }

    ExporterOutput lr = ae.getOutput(in);
    count++;
    emit progress(static_cast<int>(count/max_count*100));
    if(!lr.Valid)
    {
      failedHelper("Assygen failed",
                   "  assygen " + name + ": " + lr.Result.c_str());
      return false;
    }
    if(!QFileInfo(pass + ".jou").exists())
    {
      failedHelper("Assygen failed to generate jou file",
                   "  "+name + ".jou does not exist");
      return false;
    }
    if(!keepGoing())
    {
      cancelHelper();
      return false;
    }

    emit currentProcess("  cubit " + name + ".jou");
    QFile::remove(pass + ".cub");
    lr = ce.getOutput(ExporterInput(path, cubitExe, pass + ".jou"));
    count++;
    emit progress(static_cast<int>(count/max_count*100));
    if(!lr.Valid)
    {
      failedHelper("Cubit failed",
                   "  cubit " + name + ".jou: " + lr.Result.c_str());
      return false;
    }
    if(!QFileInfo(pass + ".cub").exists())
    {
      failedHelper( "cubit failed to generate cubit file",
                   "  " + name + ".cub does not exist");
      return false;
    }
    emit fileDone();
    if(!keepGoing())
    {
      cancelHelper();
      return false;
    }
  }
  return true;
}

bool cmbNucExport::checkCoreFile(const QString coregenFile, std::string & message)
{
  QFileInfo fi( coregenFile );
  if(!fi.exists())
  {
    message = " Required File does not exist: " + coregenFile.toStdString();
    return false;
  }
  std::ifstream input(coregenFile.toStdString().c_str());
  int count = 0;
  while(!input.eof())
  {
    std::string line("");
    std::getline(input,line);
    line = line.substr(0, line.find_first_of('!'));
    if(line.empty()) continue;
    std::string fname;
    std::stringstream ss(line);
    std::string tag;
    ss >> tag;
    std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
    if(count > 0)
    {
      count--;
      fname = tag;
    }
    else if(tag == "assemblies")
    {
      ss >> count;
      continue;
    }
    else if(tag == "background")
    {
      ss >> fname;
    }
    else
      continue;
    QFileInfo tmpFI( fname.c_str() );
    if(!QFileInfo(fname.c_str()).exists() &&
       !QFileInfo(fi.absoluteDir(),fname.c_str()).exists() )
    {
      message = " Required File does not exist: " + fname;
      return false;
    }
  }
  return true;
}

bool cmbNucExport::runCoreHelper( const QString coregenExe,
                                  const QString coregenLib,
                                  const QString coregenFile,
                                  const QString CoreGenOutputFile,
                                  double & count, double max_count )
{
  if(!keepGoing())
  {
    cancelHelper();
    return false;
  }
  QFile::remove(CoreGenOutputFile);
  QFileInfo fi(coregenFile);
  QString path = fi.absolutePath();
  QString name = fi.completeBaseName();
  QString pass = path + '/' + name;
  emit currentProcess("  coregen " + name + ".inp");
  std::string msg;

  CoregenExporter coreExport("Coregen");
  ExporterInput in(path, coregenExe, pass);
  {
    in.LibPath = "";
    std::stringstream ss(coregenLib.toStdString().c_str());
    std::string line;
    unsigned int i = 0;
    while( std::getline(ss, line))
    {
      in.LibPath += line + ":";
    }

    QFileInfo libPaths(coregenExe);
    in.LibPath += (libPaths.absolutePath() + ":" + libPaths.absolutePath() + "/../lib").toStdString();
  }
  ExporterOutput r = coreExport.getOutput(in);
  count++;
  emit progress(static_cast<int>(count/max_count*100));
  if(!r.Valid)
  {
    failedHelper("Curegen failed",
                 "  running coregen " + name +
                 ".inp returned a failure mode" + r.Result.c_str());
    return false;
  }
  if(QFileInfo(CoreGenOutputFile).exists())
  {
    emit fileDone();
    emit sendCoreResult(CoreGenOutputFile);
  }
  else
  {
    failedHelper("Curegen failed to generate model file",
                 "  coregen did not generage a model file");
    return false;
  }

  return true;
}

void cmbNucExport::cancel()
{
  {
    QMutexLocker locker(&Memory);
    setKeepGoing(false);
    if(assygenWorker != NULL) assygenWorker->stop();
    if(coregenWorker != NULL) coregenWorker->stop();
    if(cubitWorker != NULL) cubitWorker->stop();
  }
  this->deleteWorkers();
  emit cancelled();
}

void cmbNucExport::finish()
{
  this->deleteServer();
  this->deleteWorkers();
  emit progress(100);
  emit currentProcess("Finished");
  emit done();
  emit successful();
}

bool cmbNucExport::keepGoing() const
{
  QMutexLocker locker(&Mutex);
  return KeepGoing;
}

void cmbNucExport::setKeepGoing(bool b)
{
  QMutexLocker locker(&Mutex);
  KeepGoing = b;
}

void cmbNucExport::constructWorkers()
{
  QMutexLocker locker(&Memory);
  ServerConnection = remus::worker::ServerConnection();
  assygenWorker = cmbNucExporterWorker::AssygenWorker(ServerConnection);
  assygenWorker->moveToThread(&(this->WorkerThreads[0]));
  connect( this, SIGNAL(startWorkers()),
           assygenWorker, SLOT(start()));
  connect( this, SIGNAL(endWorkers()),
          assygenWorker, SLOT(stop()));
  connect( assygenWorker, SIGNAL(currentMessage(QString)),
           this, SIGNAL(statusMessage(QString)) );
  coregenWorker = cmbNucExporterWorker::CoregenWorker(ServerConnection);
  coregenWorker->moveToThread(&(WorkerThreads[1]));
  connect( this, SIGNAL(startWorkers()),
           coregenWorker, SLOT(start()));
  connect( this, SIGNAL(endWorkers()),
          coregenWorker, SLOT(stop()));
  connect( coregenWorker, SIGNAL(currentMessage(QString)),
          this, SIGNAL(statusMessage(QString)) );
  std::vector<std::string> args;
  args.push_back("-nographics");
  args.push_back("-batch");
  cubitWorker = cmbNucExporterWorker::CubitWorker(ServerConnection, args);
  cubitWorker->moveToThread(&WorkerThreads[2]);
  connect( this, SIGNAL(startWorkers()),
          cubitWorker, SLOT(start()));
  connect( this, SIGNAL(endWorkers()),
          cubitWorker, SLOT(stop()));
  connect( cubitWorker, SIGNAL(currentMessage(QString)),
          this, SIGNAL(statusMessage(QString)) );
  WorkerThreads[0].start();
  WorkerThreads[1].start();
  WorkerThreads[2].start();
  emit startWorkers();
}

void cmbNucExport::deleteServer()
{
  QMutexLocker locker(&ServerProtect);
  if(this->Server != NULL) this->Server->stopBrokering();
  delete this->Server;
  this->Server = NULL;
}

void cmbNucExport::deleteWorkers()
{
  QMutexLocker locker(&Memory);
  WorkerThreads[0].quit();
  WorkerThreads[1].quit();
  WorkerThreads[2].quit();
  WorkerThreads[0].wait();
  WorkerThreads[1].wait();
  WorkerThreads[2].wait();
  delete assygenWorker;
  assygenWorker = NULL;
  delete coregenWorker;
  coregenWorker = NULL;
  delete cubitWorker;
  cubitWorker = NULL;
}

bool cmbNucExport::startUpHelper(double & count, double max_count)
{
  {
    QMutexLocker locker(&ServerProtect);
    if(Server != NULL)
    {
      emit errorMessage("Currently running a process");
      emit currentProcess("  Please wait until previous process completes");
      return false;
    }
  }
  this->setKeepGoing(true);
  emit currentProcess("  Started setting up");
  //pop up progress bar
  emit progress(static_cast<int>(count/max_count*100));
  //start server
  factory->setMaxWorkerCount(3);

  //create a default server with the factory
  {
    QMutexLocker locker(&ServerProtect);
    Server = new remus::server::Server(factory);
  }

  //start accepting connections for clients and workers
  emit currentProcess("  Starting server");
  bool valid;
  {
    QMutexLocker locker(&ServerProtect);
    valid = Server->startBrokering(remus::Server::NONE);
  }
  if( !valid )
  {
    QMutexLocker locker(&ServerProtect);
    emit errorMessage("failed to start server");
    emit currentProcess("  failed to start server");
    emit done();
    delete Server;
    Server = NULL;
    return false;
  }

  {
    QMutexLocker locker(&ServerProtect);
    Server->waitForBrokeringToStart();
  }
  count++;
  emit progress(static_cast<int>(count/max_count*100));

  emit currentProcess("  Starting Workers");
  this->constructWorkers();
  count += 3;
  emit progress(static_cast<int>(count/max_count*100));
  return true;
}

#endif //#ifndef cmbNucExport_cxx
