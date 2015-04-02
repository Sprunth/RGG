#ifndef cmbNucExport_cxx
#define cmbNucExport_cxx //incase this is included for template instant

#include <remus/client/Client.h>

#include <remus/worker/Worker.h>

#include <remus/server/Server.h>
#include <remus/server/WorkerFactory.h>
#include <remus/common/MeshTypes.h>
#include <remus/common/ExecuteProcess.h>
#include <remus/proto/JobResult.h>
#include <remus/client/ServerConnection.h>
#include <remus/worker/Worker.h>
#include <remus/server/Server.h>

#include <QDebug>
#include <QFileInfo>
#include <QStringList>
#include <QDir>
#include <QMutexLocker>
#include <QEventLoop>
#include <QThread>
#include <QThreadPool>
#include <QProcess>
#include <QTime>
#include <QMetaType>

#include <iostream>
#include <sstream>
#include <fstream>

#include <stdlib.h>

#include "cmbNucExport.h"

//#define USE_REMUS_EXECUTE_PROCESS

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

class ExporterWorkerFactory;

struct cmbNucExportInternal
{
  remus::server::ServerPorts serverPorts;
  remus::server::Server * Server;
  remus::worker::ServerConnection ServerConnection;
  remus::worker::ServerConnection make_ServerConnection();
  boost::shared_ptr<ExporterWorkerFactory> factory;
  cmbNucExportInternal(cmbNucExport* exporter);
  ~cmbNucExportInternal()
  {
  }
};


class cmbNucExporterClient;

class cmbNucExporterWorker: public remus::worker::Worker
{
public:
  enum status{FAILED, TERMINATE, RETRY, OK};
  cmbNucExporterWorker( std::string label, remus::common::MeshIOType miotype,
                        RunnableConnection & rconn,
                        remus::worker::ServerConnection const& sconn,
                        cmbNucExport * exporter,
                        std::vector<std::string> extra_args = std::vector<std::string>());
  ~cmbNucExporterWorker();
  RunnableConnection & connection;
  void process();
#ifdef USE_REMUS_EXECUTE_PROCESS
  status pollStatus( remus::common::ExecuteProcess* process,
                     const remus::worker::Job& job );
#else
  void sendMessages( QProcess & qp);
  status pollStatus( QProcess & process,
                     const remus::worker::Job& job );
#endif
  std::string label;
  cmbNucExport * exporter;
  std::vector<std::string> ExtraArgs;
  bool keepGoing;
  unsigned int numberOfTries;
  std::string FileName;
  int pid;
};

class cmbNucExportWorkerRunner: public QRunnable
{
public:
  cmbNucExportWorkerRunner( std::string l, remus::common::MeshIOType miotype,
                            //remus::worker::ServerConnection const& conn,
                            cmbNucExport * e,
                            std::vector<std::string> extra_args = std::vector<std::string>())
  :IOType(miotype), ExtraArgs(extra_args), exporter(e), label(l)
  {
  }
  ~cmbNucExportWorkerRunner()
  {}

  virtual void run()
  {
    remus::worker::ServerConnection serverConnection = this->exporter->internal->make_ServerConnection();
    cmbNucExporterWorker worker(this->label, this->IOType, this->connection,
                                serverConnection, this->exporter, this->ExtraArgs);
    worker.process();
    qDebug() << label.c_str() << "is done running";
  }

  remus::common::MeshIOType IOType;
  RunnableConnection connection;
  std::vector<std::string> ExtraArgs;
  cmbNucExport * exporter;
  std::string label;
};

class ExporterWorkerFactory: public remus::server::WorkerFactoryBase
{
public:
  ExporterWorkerFactory( cmbNucExport * e, int maxThreadCount )
  :exporter(e)
  {
    this->setMaxWorkerCount(maxThreadCount);
    types[0] = remus::proto::make_JobRequirements(remus::common::MeshIOType("ASSYGEN_IN", "CUBIT_IN"), "Assygen", "");
    types[1] = remus::proto::make_JobRequirements(remus::common::MeshIOType("CUBIT_IN", "COREGEN_IN"), "Cubit", "");
    types[2] = remus::proto::make_JobRequirements(remus::common::MeshIOType("COREGEN_IN", "COREGEN_OUT"), "Coregen", "");
    threadPool.setMaxThreadCount( maxThreadCount );
    threadPool.setExpiryTimeout( -1 );
  }

  virtual remus::proto::JobRequirementsSet workerRequirements(remus::common::MeshIOType type) const
  {
    remus::proto::JobRequirementsSet result;
    if(types[0].meshTypes() == type) result.insert(types[0]);
    else if(types[1].meshTypes() == type) result.insert(types[1]);
    else if(types[2].meshTypes() == type) result.insert(types[2]);
    return result;
  }

  virtual remus::common::MeshIOTypeSet supportedIOTypes() const
  {
    remus::common::MeshIOTypeSet result;
    result.insert(types[0].meshTypes());
    result.insert(types[1].meshTypes());
    result.insert(types[2].meshTypes());
    return result;
  }

  virtual bool haveSupport(const remus::proto::JobRequirements& reqs) const
  {
    return reqs == types[0] || reqs == types[1] || reqs == types[2];
  }

  virtual bool createWorker(const remus::proto::JobRequirements& reqs,
                            remus::server::WorkerFactory::FactoryDeletionBehavior /*lifespan*/)
  {
    if(this->currentWorkerCount() >= this->maxWorkerCount()) return false;
    //remus::worker::ServerConnection connection = remus::worker::ServerConnection();
    cmbNucExportWorkerRunner * runner;
    if(reqs == types[0])
    {
      //create and connect assygen worker
      runner = new cmbNucExportWorkerRunner( "Assygen", reqs.meshTypes(), /*connection,*/ exporter);
    }
    else if(reqs == types[1])
    {
      //create and connect cubit worker
      std::vector<std::string> args;
      args.push_back("-nographics");
      args.push_back("-batch");
      runner = new cmbNucExportWorkerRunner( "Cubit", reqs.meshTypes(), /*connection,*/ exporter, args);
    }
    else if(reqs == types[2])
    {
      //create and connect coregen worker
      runner = new cmbNucExportWorkerRunner( "Coregen", reqs.meshTypes(), /*connection,*/ exporter);
    }
    else
    {
      return false;
    }
    QObject::connect( &(runner->connection), SIGNAL(currentMessage(QString)), exporter, SIGNAL(statusMessage(QString)) );
    qDebug() << "Current thread count: " << threadPool.activeThreadCount() << "out of" << threadPool.maxThreadCount();
    runner->setAutoDelete(true);
    threadPool.start(runner);
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

  void wait()
  {
    qDebug() << "Waiting for workers to finish" << threadPool.activeThreadCount();
    threadPool.waitForDone();
  }

protected:
  remus::proto::JobRequirements types[3];
  QThreadPool threadPool;
  cmbNucExport * exporter;
};

cmbNucExportInternal
::cmbNucExportInternal(cmbNucExport* exporter)
 : serverPorts(/*zmq::socketInfo<zmq::proto::inproc>("export_client_channel"),
               zmq::socketInfo<zmq::proto::inproc>("export_worker_channel")*/),
 Server(NULL),
 factory(new ExporterWorkerFactory(exporter, 4))
{
}

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
  cmbNucExporterClient(remus::client::ServerConnection conn);
  ~cmbNucExporterClient()
  {delete Client;}
  bool getOutput(std::string label, std::string itype, std::string otype,
                 ExporterInput const& in, remus::proto::Job & job);
  remus::proto::JobStatus jobStatus(remus::proto::Job & job)
  {
    return Client->jobStatus(job);
  }
  void terminate(remus::proto::Job & job)
  {
    Client->terminate(job);
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
  : running(false), done(false), error(false), job(remus::proto::make_invalidJob()), in(exeDir, fun, file, of)
  {
  }
  ~JobHolder()
  {
  }
  bool running, done, error;
  std::vector<JobHolder*> dependencies;
  std::string label, itype, otype;
  remus::proto::Job job;
  ExporterInput in;
};

cmbNucExporterWorker
::cmbNucExporterWorker( std::string l, remus::common::MeshIOType miotype,
                        RunnableConnection & rconn,
                        remus::worker::ServerConnection const& conn,
                        cmbNucExport * e,
                        std::vector<std::string> extra_args )
:remus::worker::Worker( remus::proto::make_JobRequirements(miotype, l, ""), conn),
 connection(rconn), exporter(e), ExtraArgs(extra_args),
 keepGoing(true), pid(0)
{
  this->label = l;
}

void
cmbNucExporterWorker
::process()
{
#if 0
  remus::worker::Job job = takePendingJob();
  //remus::worker::Job job = this->getJob();
  int count = 0;
  while(!job.valid())
  {
    if(count++>=10)
    {
      qDebug() << "!!!!!!!!!!!DID NOT GET A VALID JOB!!!!!!!!!!!!!!!";
      return;
    }
    Thread::msleep(100);
    job = takePendingJob();
  }
#else
  int count = 0;
  this->askForJobs(1);
  while(this->pendingJobCount() == 0)
  {
    //qDebug() << count <<"waiting for jobs: " << this->pendingJobCount();
    if(count++>=100)
    {
      connection.sendErrorMessage("JOB NEVER CAME");
      qDebug() << "!!!!!!!!!!!DID NOT GET A VALID JOB!!!!!!!!!!!!!!!";
      return;
    }
    QThread::yieldCurrentThread();
    Thread::msleep(100);
  }
  remus::worker::Job job = takePendingJob();
#endif
  if(!job.valid())
  {
    switch(job.validityReason())
    {
      case remus::worker::Job::INVALID:
        connection.sendErrorMessage("JOB NOT VALID");
        return;
      case remus::worker::Job::TERMINATE_WORKER:
        std::cout << "TERMINATE_WORKER" << std::endl;
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
  QStringList qargs;
  for( std::vector<std::string>::const_iterator i = ExtraArgs.begin();
      i < ExtraArgs.end(); ++i )
  {
    args.push_back(*i);
    qargs << i->c_str();
  }
  args.push_back(input.FileArg);
  qargs << input.FileArg.c_str();
  FileName = input.FileArg;
  numberOfTries = 0;

#ifdef __APPLE__
  char* oldEnv = getenv("DYLD_LIBRARY_PATH");
  std::string env(input.LibPath);
  setenv("DYLD_LIBRARY_PATH", env.c_str(), 1);
#elif  __linux
  char* oldEnv = getenv("LD_LIBRARY_PATH");
  std::string env(input.LibPath);
  setenv("LD_LIBRARY_PATH", env.c_str(), 1);
#endif

  bool retry = true;
  while(retry)
  {
    qDebug() << "starting exe: " << this->label.c_str() << numberOfTries;
#ifdef USE_REMUS_EXECUTE_PROCESS
    remus::common::ExecuteProcess* ep = new remus::common::ExecuteProcess( input.Function, args);
    //actually launch the new process
    ep->execute(remus::common::ExecuteProcess::Attached);
#else
    qDebug() << qargs;
    QProcess ep;
    ep.start(input.Function.c_str(), qargs);
    QThread::yieldCurrentThread();
#endif

    //Wait for finish
    qDebug() << "waiting for execuable to finish: " << this->label.c_str() << numberOfTries;
    switch(pollStatus(ep, job))
    {
      case OK:
        if(QFileInfo(input.OutputFile.c_str()).exists())
        {
          qDebug() << "Done waiting.  Is ok: " << this->label.c_str();
          remus::proto::JobResult results = remus::proto::make_JobResult(job.id(),"DUMMY FOR NOW;");
          this->returnResult(results);
          break;
        }
        qDebug() << input.OutputFile.c_str() << "Does not exist retrying: " << this->label.c_str();
      case RETRY:
        QThread::yieldCurrentThread();
        if(numberOfTries++ < 3)
        {
          Thread::msleep(30);
          //At times the exectuable crashes, we retry it 3 times.
          qDebug() << "Done waiting.  Crashed, retrying: " << this->label.c_str();
          continue;
        }
      case FAILED:
      {
        qDebug() << "Done waiting.  Job Failed: " << this->label.c_str();
        remus::proto::JobStatus status(job.id(),remus::FAILED);
        updateStatus(status);
        break;
      }
      case TERMINATE:
      {
        ((void)0); //return no message.
      }
    }
    retry = false;
  }
#ifdef USE_REMUS_EXECUTE_PROCESS
  delete ep;
#endif

  QDir::setCurrent( current );

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

cmbNucExporterWorker
::~cmbNucExporterWorker()
{
}

#ifdef USE_REMUS_EXECUTE_PROCESS
cmbNucExporterWorker::status cmbNucExporterWorker
::pollStatus( remus::common::ExecuteProcess* ep,
              const remus::worker::Job& job)
{
  typedef remus::common::ProcessPipe ProcessPipe;

  //poll on STDOUT and STDERRR only
  bool validExection=true;
  remus::proto::JobStatus status(job.id(),remus::IN_PROGRESS);
  while(ep->isAlive()&& validExection && !this->jobShouldBeTerminated(job))
  {
    //poll till we have a data, waiting for-ever!
    ProcessPipe data = ep->poll(0);
    if(data.type == ProcessPipe::STDOUT || data.type == ProcessPipe::STDERR)
    {
      //we have something on the output pipe
      remus::proto::JobProgress progress(data.text);
      status.updateProgress(progress);
      connection.sendCurrentMessage( QString(data.text.c_str()) );
    }
  }
  if(ep->isAlive())
  {
    qDebug() << "Killing process";
    ep->kill();
    /*ProcessPipe data =*/ ep->poll(4); //clear pipe
  }
  ep->poll(4);

  if(this->jobShouldBeTerminated(job) )
  {
    return TERMINATE;
  }

  //verify we exited normally, not segfault or numeric exception
  validExection &= ep->exitedNormally();

  if(!validExection)
  {
    return FAILED;
  }
  return OK;
}
#else
void cmbNucExporterWorker
::sendMessages( QProcess & qp)
{
  QByteArray tmp = qp.readAll();
  if(tmp.isEmpty()) return;
  QList<QByteArray> lines = tmp.split('\n');
  foreach ( const QByteArray &line, lines)
  {
    connection.sendCurrentMessage( QString("Process ") + QString::number(pid) + QString(": ") + QString(line) );
  }
}

cmbNucExporterWorker::status cmbNucExporterWorker
::pollStatus( QProcess & qp,
              const remus::worker::Job& job )
{
  if (!qp.waitForStarted(-1))
  {
    qDebug() << "waiting for start failed"<< this->label.c_str();
    return FAILED;
  }
#ifdef _WIN32
  //Currently, we are not running meshkit in windows.  
  //pid is different in windows, instead it is a struct.  Because
  //I am being lazy, I will delay getting this working until we
  //need it.  Bwaha
  //pid = qp.pid()->dwProcessId;
  pid++; //TODO: change this to something better
#else
  pid = qp.pid();
#endif
  if(qp.state() != QProcess::Running)
  {
    qDebug() << pid << "started but Is Not Running";
  }
  qDebug() << "process " << pid << "started" << this->label.c_str() << this->FileName.c_str();
  while(!qp.waitForFinished(50) && !this->jobShouldBeTerminated(job))
  {
    sendMessages(qp);
  }
  sendMessages(qp);
  if(qp.state() == QProcess::Running)
  {
    qDebug() << "Killing process:" << pid;
    //qp.kill();
    qp.terminate();
    qp.waitForFinished();
  }
  if(this->jobShouldBeTerminated(job))
  {
    return TERMINATE;
  }
  if( qp.exitStatus() == QProcess::NormalExit)
  {
    qDebug() << "exited normally process:" << pid << " ID: " << this->label.c_str();
    return OK;
  }
  qDebug() << pid << "exit"<< qp.exitStatus() << "error:" <<  qp.error() << "ID:" << this->label.c_str();
  QStringList qargs;
  for( std::vector<std::string>::const_iterator i = ExtraArgs.begin();
      i < ExtraArgs.end(); ++i )
  {
    qargs << i->c_str();
  }
  qDebug() << pid << qargs << FileName.c_str();

  if(qp.error() == QProcess::Crashed)
  {
    qDebug() << "Crashed:" << pid << " ID: " << this->label.c_str();
    return RETRY;
  }
  qDebug() << "Failed:" << pid << " ID: " << this->label.c_str();
  return FAILED;
}
#endif

cmbNucExporterClient::cmbNucExporterClient(remus::client::ServerConnection conn)
{
  Connection = conn;
  Client = new remus::Client(Connection);
}

bool
cmbNucExporterClient::getOutput(std::string label, std::string it, std::string ot,
                                ExporterInput const& in, remus::proto::Job & job)
{
  ExporterOutput eo;
  remus::common::MeshIOType mesh_types(it, ot);
  remus::proto::JobRequirements reqs = remus::proto::make_JobRequirements(mesh_types, label,"");
  if(Client->canMesh(reqs))
  {
    remus::proto::JobContent content =
    remus::proto::make_JobContent(in);

    remus::proto::JobSubmission sub(reqs,content);

    job = Client->submitJob(sub);
    return true;
  }
  return false;
}

cmbNucExport::cmbNucExport()
: client(NULL)
{
  internal = new cmbNucExportInternal(this);
  this->isDone = false;
  this->keepGoingAfterError = false;
  qRegisterMetaType< Message >();
}

cmbNucExport::~cmbNucExport()
{
  QMutexLocker locker(&Memory);
  delete client;
  delete internal;
}

void
cmbNucExport::run( Message const& message )
{
  {
    QMutexLocker locker(&end_control);
    this->isDone = false;
    this->keepGoingAfterError = message.keepGoingAfterError;
  }
  this->clearJobs();
  if(!this->startUpHelper()) return;

  std::vector<JobHolder*> deps = exportCylinder( message.cylinderTask );

  std::vector<JobHolder*> assy = this->runAssyHelper( message.assemblyTasks );

  deps.insert(deps.end(), assy.begin(), assy.end());

  this->runCoreHelper( message.coregenFile, deps, message.CoreGenOutputFile, false );
  processJobs();
  {
    QMutexLocker locker(&end_control);
    this->isDone = true;
  }
}

void cmbNucExport::cancelHelper()
{
  emit currentProcess("  CANCELED");
  emit done();
  emit terminate();
  clearJobs();
  this->deleteServer();
}

void cmbNucExport::failedHelper(QString msg, QString pmsg)
{
  emit errorMessage("ERROR: " + msg );
  emit currentProcess(pmsg + " FAILED");
  if(!keepGoingAfterError)
  {
    emit done();
    clearJobs();
    this->deleteServer();
  }
}

JobHolder* cmbNucExport::makeAssyJob(const QString assygenFile)
{
  QFileInfo fi(assygenFile);
  QString path = fi.absolutePath();
  QString name = fi.completeBaseName();
  QString pass = path + '/' + name;
  JobHolder * assyJob = new JobHolder(path, AssygenExe, name, pass + ".jou");
  jobs_to_do.push_back(assyJob);
  assyJob->label = "Assygen";
  assyJob->itype = "ASSYGEN_IN";
  assyJob->otype = "CUBIT_IN";

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
cmbNucExport::runAssyHelper( std::vector<Message::AssygenTask> const& msg )
{
  std::vector<JobHolder*> dependencies;
  for (std::vector<Message::AssygenTask>::const_iterator iter = msg.begin();
       iter != msg.end(); ++iter)
  {
    Message::AssygenTask const& m = *iter;
    QFileInfo fi(m.assygenFile);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName();
    QString pass = path + '/' + name;
    QString cubFullFile = path + '/' + name.toLower()+m.outFileExtension;
    JobHolder * assyJob = makeAssyJob(m.assygenFile);

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
    cubitJob->label = "Cubit";
    cubitJob->itype = "CUBIT_IN";
    cubitJob->otype = "COREGEN_IN";
    cubitJob->dependencies = depIn;
    jobs_to_do.push_back(cubitJob);
    result.push_back(cubitJob);
  }

  return result;
}

std::vector<JobHolder*>
cmbNucExport::runCoreHelper( const QString coregenFile,
                             std::vector<JobHolder*> depIn,
                             const QString CoreGenOutputFile,
                             bool use_cylinder_version )
{
  std::vector<JobHolder*> result;
  if(!coregenFile.isEmpty())
  {
    QFileInfo fi(coregenFile);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName();
    QString pass = path + '/' + name;

    QString exe = CoregenExe, lib = CoregenLib;
    if(use_cylinder_version)
    {
      exe = CylinderCoregenExe;
      lib = AssygenLib;
    }

    JobHolder * coreJob = new JobHolder(path, exe, pass, CoreGenOutputFile);
    coreJob->label = "Coregen";
    coreJob->itype = "COREGEN_IN";
    coreJob->otype = "COREGEN_OUT";
    coreJob->dependencies = depIn;
    jobs_to_do.push_back(coreJob);
    result.push_back(coreJob);
    {
      coreJob->in.LibPath = "";
      std::stringstream ss(lib.toStdString().c_str());
      std::string line;
      while( std::getline(ss, line))
      {
        coreJob->in.LibPath += line + ":";
      }

      QFileInfo libPaths(exe);
      coreJob->in.LibPath += (libPaths.absolutePath() + ":" + libPaths.absolutePath() + "/../lib").toStdString();
      if(use_cylinder_version)
        coreJob->in.LibPath += ":"+ QFileInfo(CubitExe).absolutePath().toStdString();
      qDebug() << coreJob->in.LibPath.c_str();
    }
  }
  return result;
}

std::vector<JobHolder*>
cmbNucExport::exportCylinder( Message::CylinderTask const& msg )
{
  std::vector<JobHolder*> result;
  if(!msg.valid)
  {
    return result;
  }
  JobHolder* assyJob = makeAssyJob(msg.assygenFile);
  std::vector<JobHolder*> tmp = this->runCoreHelper( msg.coregenFile, std::vector<JobHolder*>(1,assyJob),
                                                     msg.coregenResultFile, true );
  qDebug() << msg.cubitFile << msg.cubitOutputFile;
  result = runCubitHelper(msg.cubitFile, tmp, msg.cubitOutputFile);
  return result;
}


void cmbNucExport::cancel()
{
  //todo: fix this
  setKeepGoing(false);
  emit cancelled();
}

void cmbNucExport::finish()
{
  this->deleteServer();
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
  qDebug() << "stop brocking";
  if(this->internal->Server != NULL)
  {
     this->internal->Server->stopBrokering();
    if( this->internal->Server->isBrokering())
    {
       this->internal->Server->waitForBrokeringToFinish();
    }
  }
  delete this->client;
  this->client = NULL;
  qDebug() << "deleting server";
  delete  this->internal->Server;
  this->internal->Server = NULL;
  this->internal->factory->wait();
  qDebug() << "Done deleting server";
}

bool cmbNucExport::startUpHelper()
{
  {
    QMutexLocker locker(&ServerProtect);
    if(this->internal->Server != NULL)
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
    this->internal->Server = new remus::server::Server(this->internal->serverPorts, this->internal->factory);
  }

  //start accepting connections for clients and workers
  emit currentProcess("  Starting server");
  bool valid;
  {
    QMutexLocker locker(&ServerProtect);
    valid = this->internal->Server->startBrokering(remus::Server::NONE);
    remus::client::ServerConnection conn = remus::client::make_ServerConnection(this->internal->serverPorts.client().endpoint());
    conn.context(this->internal->serverPorts.context());
    client = new cmbNucExporterClient(conn);
  }
  if( !valid )
  {
    QMutexLocker locker(&ServerProtect);
    emit errorMessage("failed to start server");
    emit currentProcess("  failed to start server");
    emit done();
    delete this->internal->Server;
    delete client;
    this->internal->Server = NULL;
    client = NULL;
    return false;
  }

  {
    QMutexLocker locker(&ServerProtect);
    this->internal->Server->waitForBrokeringToStart();
  }
  emit progress(2);

  return true;
}

void cmbNucExport::setAssygen(QString assygenExe,QString assygenLib)
{
  this->AssygenExe = assygenExe;
  QFileInfo fi(assygenExe);
  QString path = fi.absolutePath();
  this->CylinderCoregenExe =  path + "/coregen";
  qDebug() << "=========>" <<this->CylinderCoregenExe;
  this->AssygenLib = assygenLib;
}

void cmbNucExport::setCoregen(QString coregenExe,QString coregenLib)
{
  this->CoregenExe = coregenExe;
  this->CoregenLib = coregenLib;
}

void cmbNucExport::setNumberOfProcessors(int v)
{
  this->internal->factory->setMaxThreadCount(v);
}

void cmbNucExport::setCubit(QString cubitExe)
{
  this->CubitExe = cubitExe;
}

void cmbNucExport::clearJobs()
{
  this->stopJobs();
  for(unsigned int i = 0; i < jobs_to_do.size(); ++i)
  {
    delete jobs_to_do[i];
    jobs_to_do[i] = NULL;
  }
  jobs_to_do.clear();
}

void cmbNucExport::stopJobs()
{
  for(unsigned int i = 0; i < jobs_to_do.size(); ++i)
  {
    if(jobs_to_do[i]->running && !jobs_to_do[i]->done )
    {
      client->terminate(jobs_to_do[i]->job);
    }
  }
}

void cmbNucExport::processJobs()
{
  QEventLoop el;
  bool inErrorState = false;
  emit currentProcess("  Generating Mesh");
  double count = 2;
  while(true)
  {
    if(!keepGoing())
    {
      this->stopJobs();
      cancelHelper();
      return;
    }
    el.processEvents();
    bool all_finish = true;
    for(unsigned int i = 0; i < jobs_to_do.size(); ++i)
    {
      bool error_occured = false;
      if(jobs_to_do[i]->done) continue;
      else if(jobs_to_do[i]->running )
      {
        QTime time;
        time.start();
        while(time.elapsed()<100)
        {
          if(!keepGoing())
          {
            this->stopJobs();
            cancelHelper();
            return;
          }
          el.processEvents();
        }
        ServerProtect.lock();
        remus::proto::JobStatus jobState = client->jobStatus(jobs_to_do[i]->job);
        ServerProtect.unlock();

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
          error_occured = inErrorState = true;
        }
        else if(jobState.status() == remus::FAILED)
        {
          failedHelper("Remus ERROR", " Failed to mesh");
          error_occured = inErrorState = true;
        }
        else if(jobState.status() == remus::EXPIRED)
        {
          all_finish = false;
          qDebug() << "REMUS EXPIRED resubmit job";
          bool r = client->getOutput( jobs_to_do[i]->label, jobs_to_do[i]->itype,
                                      jobs_to_do[i]->otype, jobs_to_do[i]->in,
                                      jobs_to_do[i]->job );
          if(!r)
          {
            failedHelper("Remus ERROR", " Remus does not support the job");
            inErrorState = true;
          }
          else
          {
            continue;
          }
        }
        else
        {
          failedHelper("Remus ERROR", " Remus did not finish but was not good");
          error_occured = inErrorState = true;
        }
        if(error_occured && !keepGoingAfterError)
        {
          return;
        }
        else if(error_occured)
        {
          this->jobs_to_do[i]->error = true;
          jobs_to_do[i]->done = true;
        }
      }
      else
      {
        all_finish = false;
        bool taf = true;
        bool error = false;
        for(unsigned int j = 0; j < this->jobs_to_do[i]->dependencies.size(); ++j)
        {
          taf = taf && this->jobs_to_do[i]->dependencies[j]->done;
          error = error || this->jobs_to_do[i]->dependencies[j]->error;
        }
        if(error)
        {
          if(!this->jobs_to_do[i]->error)
          {
            qDebug() << i << "One or more the the dependencies had an error";
            this->jobs_to_do[i]->error = true;
            this->jobs_to_do[i]->done = true;
          }
        }
        else if(taf)
        {
          QMutexLocker locker(&ServerProtect);
          this->jobs_to_do[i]->running = true;
          qDebug() << "i is starting" << i;
          bool r = client->getOutput( jobs_to_do[i]->label, jobs_to_do[i]->itype,
                                      jobs_to_do[i]->otype, jobs_to_do[i]->in,
                                      jobs_to_do[i]->job );
          if(!r)
          {
            failedHelper("Remus ERROR", " Remus does not support the job");
            inErrorState = true;
            if(!keepGoingAfterError)
            {
              return;
            }
            else
            {
              this->jobs_to_do[i]->error = true;
              jobs_to_do[i]->done = true;
            }
          }
        }
      }
    }
    if(all_finish)
    {
      break;
    }
  }
  qDebug() << "All jobs have been finished";
  if(inErrorState)
  {
    emit done();
    clearJobs();
    this->deleteServer();
  }
  else
  {
    this->finish();
  }
}

remus::worker::ServerConnection
cmbNucExportInternal::make_ServerConnection()
{
  remus::worker::ServerConnection conn = remus::worker::make_ServerConnection(this->serverPorts.worker().endpoint());
  conn.context(this->serverPorts.context());
  return conn;
}

void
cmbNucExport::waitTillDone()
{
  QEventLoop el;
  {
    QMutexLocker locker(&end_control);
  }
  while( !this->isDone )
  {
    el.processEvents();
  }
  {
    this->isDone = false;
  }
}

Q_DECLARE_METATYPE(Message);

#endif //#ifndef cmbNucExport_cxx
