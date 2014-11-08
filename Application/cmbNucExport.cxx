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
#include <QThreadPool>

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

class cmbNucExporterClient;


class cmbNucExportRunnableWorker: public QRunnable, public remus::worker::Worker
{
public:
  cmbNucExportRunnableWorker( std::string label, remus::common::MeshIOType miotype,
                             remus::worker::ServerConnection const& connection,
                             cmbNucExport * exporter,
                             std::vector<std::string> extra_args = std::vector<std::string>());
  ~cmbNucExportRunnableWorker();
  virtual void run();
  bool pollStatus( remus::common::ExecuteProcess* process,
                  const remus::worker::Job& job );
  RunnableConnection connection;
  std::vector<std::string> ExtraArgs;
  cmbNucExport * exporter;
  bool keepGoing;
  std::string label;
};

class ExporterWorkerFactory: public remus::server::WorkerFactoryBase
{
public:
  ExporterWorkerFactory( cmbNucExport * e, int maxThreadCount )
  :exporter(e)
  {
    this->setMaxWorkerCount(maxThreadCount);
    types[0] = remus::common::MeshIOType("ASSYGEN_IN", "CUBIT_IN");
    types[1] = remus::common::MeshIOType("CUBIT_IN", "COREGEN_IN");
    types[2] = remus::common::MeshIOType("COREGEN_IN", "COREGEN_OUT");
    threadPool.setMaxThreadCount( maxThreadCount );
    threadPool.setExpiryTimeout( -1 );
  }

  virtual remus::proto::JobRequirementsSet workerRequirements(remus::common::MeshIOType type) const
  {
    std::string label;
    if(types[0] == type) label = "Assygen";
    else if(types[1] == type) label = "Cubit";
    else if(types[2] == type) label = "Coregen";
    remus::proto::JobRequirementsSet result;
    result.insert(remus::proto::JobRequirements(remus::common::ContentFormat::Type(),
                                                type, label, std::string("") ));
    return result;
  }

  virtual remus::common::MeshIOTypeSet supportedIOTypes() const
  {
    remus::common::MeshIOTypeSet result;
    result.insert(types[0]);
    result.insert(types[1]);
    result.insert(types[2]);
    return result;
  }

  virtual bool haveSupport(const remus::proto::JobRequirements& reqs) const
  {
    remus::common::MeshIOType type = reqs.meshTypes();
    return type == types[0] || type == types[1] || type == types[2];
  }

  virtual bool createWorker(const remus::proto::JobRequirements& reqs,
                            remus::server::WorkerFactory::FactoryDeletionBehavior /*lifespan*/)
  {
    if(this->currentWorkerCount() >= this->maxWorkerCount()) return false;
    remus::worker::ServerConnection connection = remus::worker::ServerConnection();
    remus::common::MeshIOType type = reqs.meshTypes();
    cmbNucExportRunnableWorker * worker;
    if(type == types[0])
    {
      //create and connect assygen worker
      worker = new cmbNucExportRunnableWorker( "Assygen", type, connection, exporter);
    }
    else if(type == types[1])
    {
      //create and connect cubit worker
      std::vector<std::string> args;
      args.push_back("-nographics");
      args.push_back("-batch");
      worker = new cmbNucExportRunnableWorker( "Cubit", type, connection, exporter, args);
    }
    else if(type == types[2])
    {
      //create and connect coregen worker
      worker = new cmbNucExportRunnableWorker( "Coregen", type, connection, exporter);
    }
    else
    {
      return false;
    }
    QObject::connect( &(worker->connection), SIGNAL(currentMessage(QString)), exporter, SIGNAL(statusMessage(QString)) );
    threadPool.start(worker);
    return true;
  }

  virtual void updateWorkerCount()
  {
  }

  virtual unsigned int currentWorkerCount() const
  {
    unsigned int tpc = threadPool.activeThreadCount();
    return tpc;
  }

  void setMaxThreadCount(int maxThreadCount)
  {
    threadPool.setMaxThreadCount( maxThreadCount );
    this->setMaxWorkerCount(maxThreadCount);
  }

protected:
  remus::common::MeshIOType types[3];
  QThreadPool threadPool;
  cmbNucExport * exporter;
};

struct ExporterInput
{
  std::string ExeDir;
  std::string Function;
  std::string LibPath;
  std::string FileArg;
  std::string OutputFile;
  operator std::string(void) const
  {
    std::stringstream ss;
    ss << ExeDir << ';' << FileArg << ";" <<Function << ";" << LibPath << ';' << OutputFile;
    return ss.str();
  }
  ExporterInput(QString exeDir, QString fun, QString file, QString of)
  :ExeDir(exeDir.trimmed().toStdString()), Function(fun.trimmed().toStdString()),
   FileArg(file.trimmed().toStdString()), OutputFile(of.toStdString())
  {  }
  ExporterInput(const remus::worker::Job& job)
  {
    //qDebug() << remus::worker::to_string(job).c_str();
    std::stringstream ss(job.details("data"));
    getline(ss, ExeDir,   ';');
    getline(ss, FileArg,  ';');
    getline(ss, Function, ';');
    getline(ss, LibPath,  ';');
    getline(ss, OutputFile,  ';');
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

class cmbNucExporterClient
{
public:
  cmbNucExporterClient(std::string label, std::string itype, std::string otype, std::string server = "");
  ~cmbNucExporterClient()
  {delete Client;}
  bool getOutput(ExporterInput const& in, remus::proto::Job ** job);
  remus::proto::JobStatus jobStatus(remus::proto::Job * job)
  {
    return Client->jobStatus(*job);
  }
private:
  remus::client::ServerConnection Connection;
  remus::Client * Client;
  std::string Label;
  std::string input_type;
  std::string output_type;
};

struct JobHolder
{
  JobHolder(QString exeDir, QString fun, QString file, QString of)
  : running(false), done(false), in(exeDir, fun, file, of)
  {
    client = NULL;
    job = NULL;
  }
  ~JobHolder()
  {
    delete job;
  }
  bool running, done;
  std::vector<JobHolder*> dependencies;
  cmbNucExporterClient * client;
  remus::proto::Job * job;
  ExporterInput in;
};

void 
cmbNucExportRunnableWorker::run()
{
  remus::worker::Job job = this->getJob();
  if(!job.valid())
  {
    switch(job.validityReason())
    {
      case remus::worker::Job::INVALID:
        connection.sendErrorMessage("JOB NOT VALID");
        return;
      case remus::worker::Job::TERMINATE_WORKER:
        return;
      case remus::worker::Job::VALID_JOB:
        ((void)0); //have valid job, move on.
    }
  }
  qDebug() << "Recieved valid job for: " << this->label.c_str();

  ExporterInput input(job);

  //remove old output file
  QFile::remove(input.OutputFile.c_str());

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
#elif  __linux
  char* oldEnv = getenv("LD_LIBRARY_PATH");
  std::string env(input.LibPath);
  setenv("LD_LIBRARY_PATH", env.c_str(), 1);
#endif

  qDebug() << "starting exe: " << this->label.c_str();
  remus::common::ExecuteProcess* process = new remus::common::ExecuteProcess( input.Function, args);

  //actually launch the new process
  process->execute(remus::common::ExecuteProcess::Attached);

  //Wait for finish
  qDebug() << "waiting: " << this->label.c_str();
  if(pollStatus(process, job) && QFileInfo(input.OutputFile.c_str()).exists())
  {
    remus::proto::JobResult results = remus::proto::make_JobResult(job.id(),"DUMMY FOR NOW;");
    this->returnResult(results);
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
  qDebug() << "finish: " << this->label.c_str();
}

cmbNucExportRunnableWorker
::cmbNucExportRunnableWorker( std::string label, remus::common::MeshIOType miotype,
                              remus::worker::ServerConnection const& conn,
                              cmbNucExport * e,
                              std::vector<std::string> extra_args )
:remus::worker::Worker( remus::proto::make_JobRequirements(miotype, label, ""), conn),
 exporter(e),
 ExtraArgs(extra_args), keepGoing(true)
{
  this->label = label;
  if(exporter!=NULL) exporter->registerWorker(this);
}

cmbNucExportRunnableWorker
::~cmbNucExportRunnableWorker()
{
  if(exporter!=NULL) exporter->workerDone(this);
}

bool cmbNucExportRunnableWorker
::pollStatus( remus::common::ExecuteProcess* process,
              const remus::worker::Job& job)
{
  typedef remus::common::ProcessPipe ProcessPipe;

  //poll on STDOUT and STDERRR only
  bool validExection=true;
  remus::proto::JobStatus status(job.id(),remus::IN_PROGRESS);
  while(process->isAlive()&& validExection && this->keepGoing)
  {
    //poll till we have a data, waiting for-ever!
    ProcessPipe data = process->poll(2);
    if(data.type == ProcessPipe::STDOUT || data.type == ProcessPipe::STDERR)
    {
      //we have something on the output pipe
      remus::proto::JobProgress progress(data.text);
      status.updateProgress(progress);
      connection.sendCurrentMessage( QString(data.text.c_str()) );
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

cmbNucExporterClient::cmbNucExporterClient(std::string label, std::string itype,
                                           std::string otype, std::string server)
{
  Label = label;
  input_type = itype;
  output_type = otype;
  if ( !server.empty())
    {
    Connection = remus::client::make_ServerConnection(server);
    }
  Client = new remus::Client(Connection);
}

bool
cmbNucExporterClient::getOutput(ExporterInput const& in, remus::proto::Job ** job)
{
  ExporterOutput eo;
  remus::common::MeshIOType mesh_types(input_type, output_type);
  remus::proto::JobRequirements reqs = remus::proto::make_JobRequirements(mesh_types, Label,"");
  if(Client->canMesh(reqs))
  {
    remus::proto::JobContent content =
    remus::proto::make_JobContent(in);

    remus::proto::JobSubmission sub(reqs,content);

    (*job) = new remus::proto::Job(Client->submitJob(sub));
    //qDebug() << "Job Sent";
    return true;
  }
  return false;
}

cmbNucExport::cmbNucExport()
: Server(NULL),
  factory(new ExporterWorkerFactory(this, 4))
{
  assygenClient = new cmbNucExporterClient("Assygen", "ASSYGEN_IN", "CUBIT_IN");
  cubitClient = new cmbNucExporterClient("Cubit","CUBIT_IN", "COREGEN_IN");
  coreClient = new cmbNucExporterClient("Coregen", "COREGEN_IN", "COREGEN_OUT");
}

cmbNucExport::~cmbNucExport()
{
  QMutexLocker locker(&Memory);
  for(std::set<cmbNucExportRunnableWorker*>::iterator iter = workers.begin(); iter != workers.end(); ++iter)
  {
    (*iter)->exporter = NULL;
  }
  delete assygenClient;
  delete cubitClient;
  delete coreClient;
}

void
cmbNucExport::run( const QStringList &assygenFile,
                   const QString coregenFile,
                   const QString CoreGenOutputFile,
                   const QString assygenFileCylinder,
                   const QString cubitFileCylinder,
                   const QString cubitOutputFileCylinder,
                   const QString coregenFileCylinder,
                   const QString coregenResultFileCylinder )
{
  this->clearJobs();
  if(!this->startUpHelper()) return;

  std::vector<JobHolder*> deps = exportCylinder( assygenFileCylinder, cubitFileCylinder, cubitOutputFileCylinder,
                                                    coregenFileCylinder, coregenResultFileCylinder );

  std::vector<JobHolder*> assy = this->runAssyHelper( assygenFile );

  deps.insert(deps.end(), assy.begin(), assy.end());

  this->runCoreHelper( coregenFile, deps, CoreGenOutputFile );
  processJobs();
}

void cmbNucExport::cancelHelper()
{
  emit currentProcess("  CANCELED");
  emit done();
  emit terminate();
  this->deleteServer();
  this->deleteWorkers();
}

void cmbNucExport::failedHelper(QString msg, QString pmsg)
{
  emit errorMessage("ERROR: " + msg );
  emit currentProcess(pmsg + " FAILED");
  emit done();
  clearJobs();
  this->deleteServer();
  this->deleteWorkers();
}

JobHolder* cmbNucExport::makeAssyJob(const QString assygenFile)
{
  QFileInfo fi(assygenFile);
  QString path = fi.absolutePath();
  QString name = fi.completeBaseName();
  QString pass = path + '/' + name;
  JobHolder * assyJob = new JobHolder(path, AssygenExe, name, pass + ".jou");
  jobs_to_do.push_back(assyJob);
  assyJob->client = assygenClient;

  {
    assyJob->in.LibPath = "";
    std::stringstream ss(AssygenLib.toStdString().c_str());
    std::string line;
    while( std::getline(ss, line))
    {
      assyJob->in.LibPath += line + ":";
    }
    QFileInfo libPaths(AssygenExe);
    assyJob->in.LibPath += (libPaths.absolutePath() + ":" + libPaths.absolutePath() + "/../lib").toStdString();
    assyJob->in.LibPath += ":"+ QFileInfo(CubitExe).absolutePath().toStdString();
  }

  return assyJob;
}

std::vector<JobHolder*>
cmbNucExport::runAssyHelper( const QStringList &assygenFile )
{
  std::vector<JobHolder*> dependencies;
  for (QStringList::const_iterator iter = assygenFile.constBegin();
       iter != assygenFile.constEnd(); ++iter)
  {
    QFileInfo fi(*iter);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName();
    QString pass = path + '/' + name;
    QString cubFullFile = path + '/' + name.toLower()+".cub";
    JobHolder * assyJob = makeAssyJob(*iter);

    std::vector<JobHolder*> cjobs =
        runCubitHelper( pass + ".jou", std::vector<JobHolder*>(1, assyJob), cubFullFile );

    dependencies.insert(dependencies.end(), cjobs.begin(), cjobs.end());

  }
  return dependencies;
}


std::vector<JobHolder*>
cmbNucExport::runCubitHelper( const QString cubitFile,
                              std::vector<JobHolder*> depIn,
                              const QString cubitOutputFile )
{
  std::vector<JobHolder*> result;

  if(!cubitFile.isEmpty())
  {
    QFileInfo fi(cubitFile);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName();

    JobHolder * cubitJob = new JobHolder(path, CubitExe, cubitFile, cubitOutputFile);
    cubitJob->client = cubitClient;
    cubitJob->dependencies = depIn;
    jobs_to_do.push_back(cubitJob);
    result.push_back(cubitJob);
  }

  return result;
}

std::vector<JobHolder*>
cmbNucExport::runCoreHelper( const QString coregenFile,
                             std::vector<JobHolder*> depIn,
                             const QString CoreGenOutputFile )
{
  std::vector<JobHolder*> result;
  if(!coregenFile.isEmpty())
  {
    QFileInfo fi(coregenFile);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName();
    QString pass = path + '/' + name;

    JobHolder * coreJob = new JobHolder(path, CoregenExe, pass, CoreGenOutputFile);
    coreJob->client = coreClient;
    coreJob->dependencies = depIn;
    jobs_to_do.push_back(coreJob);
    result.push_back(coreJob);
    {
      coreJob->in.LibPath = "";
      std::stringstream ss(CoregenLib.toStdString().c_str());
      std::string line;
      while( std::getline(ss, line))
      {
        coreJob->in.LibPath += line + ":";
      }

      QFileInfo libPaths(CoregenExe);
      coreJob->in.LibPath += (libPaths.absolutePath() + ":" + libPaths.absolutePath() + "/../lib").toStdString();
    }
  }
  return result;
}

std::vector<JobHolder*>
cmbNucExport::exportCylinder( const QString assygenFile,
                              const QString cubitFile,
                              const QString cubitOutputFile,
                              const QString coregenFile,
                              const QString coregenResultFile )
{
  std::vector<JobHolder*> result;
  if(assygenFile.isEmpty() || cubitFile.isEmpty() || coregenFile.isEmpty())
  {
    return result;
  }
  JobHolder* assyJob = makeAssyJob(assygenFile);
  std::vector<JobHolder*> tmp = this->runCoreHelper( coregenFile, std::vector<JobHolder*>(1,assyJob),
                                                     coregenResultFile );
  result = runCubitHelper(cubitFile, tmp, cubitOutputFile);
  return result;
}


void cmbNucExport::cancel()
{
  //todo: fix this
  {
    QMutexLocker locker(&Memory);
    setKeepGoing(false);
    for(std::set<cmbNucExportRunnableWorker*>::iterator iter = workers.begin(); iter != workers.end(); ++iter)
    {
      (*iter)->keepGoing = false;
    }
  }
  this->deleteWorkers();
  emit cancelled();
}

void cmbNucExport::finish()
{
  this->deleteServer();
  this->deleteWorkers();
  clearJobs();
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

void cmbNucExport::deleteServer()
{
  QMutexLocker locker(&ServerProtect);
  if(this->Server != NULL) this->Server->stopBrokering();
  delete this->Server;
  this->Server = NULL;
}

void cmbNucExport::deleteWorkers()
{
}

bool cmbNucExport::startUpHelper()
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
  emit progress(static_cast<int>(1));
  //start server
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
  emit progress(2);

  return true;
}

void cmbNucExport::setAssygen(QString assygenExe,QString assygenLib)
{
  this->AssygenExe = assygenExe;
  this->AssygenLib = assygenLib;
}

void cmbNucExport::setCoregen(QString coregenExe,QString coregenLib)
{
  this->CoregenExe = coregenExe;
  this->CoregenLib = coregenLib;
}

void cmbNucExport::setNumberOfProcessors(int v)
{
  this->factory->setMaxThreadCount(v);
}

void cmbNucExport::setCubit(QString cubitExe)
{
  this->CubitExe = cubitExe;
}

void cmbNucExport::registerWorker(cmbNucExportRunnableWorker* w)
{
  QMutexLocker locker(&Memory);
  workers.insert(w);
}

void cmbNucExport::workerDone(cmbNucExportRunnableWorker* w)
{
  QMutexLocker locker(&Memory);
  workers.erase(w);
}

void cmbNucExport::clearJobs()
{
  for(unsigned int i = 0; i < jobs_to_do.size(); ++i)
  {
    delete jobs_to_do[i];
    jobs_to_do[i] = NULL;
  }
  jobs_to_do.clear();
}

void cmbNucExport::processJobs()
{
  QEventLoop el;
  emit currentProcess("  Generating Mesh");
  double count = 2;
  while(true)
  {
    if(!keepGoing())
    {
      clearJobs();
      cancelHelper();
      return;
    }
    el.processEvents();
    bool all_finish = true;
    for(unsigned int i = 0; i < jobs_to_do.size(); ++i)
    {
      if(jobs_to_do[i]->done) continue;
      else if(jobs_to_do[i]->running )
      {
        remus::proto::JobStatus jobState = jobs_to_do[i]->client->jobStatus(jobs_to_do[i]->job);

        if(jobState.good())
        {
          all_finish = false;
        }
        else if(jobState.finished())
        {
          if(!jobs_to_do[i]->done)
          {
            emit progress(static_cast<int>((count++)/(jobs_to_do.size()+2)*100));
            emit fileDone();
          }
          jobs_to_do[i]->done = true;
        }
        else if(jobState.status() == remus::INVALID_STATUS)
        {
          failedHelper("Remus ERROR", " Remus Invalid Status");
          return;
        }
        else if(jobState.status() == remus::FAILED)
        {
          failedHelper("Remus ERROR", " Failed to mesh");
          return;
        }
        else if(jobState.status() == remus::EXPIRED)
        {
          failedHelper("Remus ERROR", " Remus Expired");
          return;
        }
        else
        {
          failedHelper("Remus ERROR", " Remus did not finish but was not good");
          return;
        }
      }
      else
      {
        all_finish = false;
        bool taf = true;
        for(unsigned int j = 0; j < this->jobs_to_do[i]->dependencies.size(); ++j)
        {
          taf = taf && this->jobs_to_do[i]->dependencies[j]->done;
        }
        if(taf)
        {
          this->jobs_to_do[i]->running = true;
          qDebug() << "i is starting" << i;
          bool r = jobs_to_do[i]->client->getOutput(jobs_to_do[i]->in, &(jobs_to_do[i]->job));
          if(!r)
          {
            failedHelper("Remus ERROR", " Remus does not support the job");
            return;
          }
        }
      }
    }
    if(all_finish) break;
  }
  this->finish();
}

#endif //#ifndef cmbNucExport_cxx
