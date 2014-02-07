#ifndef cmbNucExport_cxx
#define cmbNucExport_cxx //incase this is included for template instant

#include <remus/client/Client.h>

#include <remus/worker/Worker.h>

#include <remus/server/Server.h>
#include <remus/server/WorkerFactory.h>

#include <QDebug>
#include <QFileInfo>
#include <QStringList>
#include <QDir>

#include <iostream>

#include "cmbNucExport.h"

struct ExporterInput
{
  std::string ExeDir;
  std::string Function;
  std::string FileArg;
  operator std::string(void) const
  {
    std::stringstream ss;
    ss << ExeDir << ';' << FileArg << ";" <<Function << ";";
    return ss.str();
  }
  ExporterInput(std::string exeDir, std::string fun, std::string file)
  :ExeDir(exeDir), Function(fun),FileArg(file)
  {  }
  ExporterInput(QString exeDir, QString fun, QString file)
  :ExeDir(exeDir.trimmed().toStdString()), Function(fun.trimmed().toStdString()),
   FileArg(file.trimmed().toStdString())
  {  }
  ExporterInput(const remus::worker::Job& job)
  {
    std::stringstream ss(job.details());
    getline(ss, ExeDir,   ';');
    getline(ss, FileArg,  ';');
    getline(ss, Function, ';');
  }
};

struct ExporterOutput
{
  bool Valid;
  std::string Result;
  ExporterOutput():Valid(false){}
  ExporterOutput(const remus::client::JobResult& job, bool valid=true)
  : Valid(valid), Result(job.Data)
  {}
  operator std::string(void) const
  { return Result; }
};

template<class JOB_REQUEST_IN, class JOB_REQUEST_OUT>
class cmbNucExporterClient
{
public:
  cmbNucExporterClient(std::string server = "");
  ExporterOutput getOutput(ExporterInput const& in);
private:
  remus::client::ServerConnection Connection;
};

typedef cmbNucExporterClient<remus::meshtypes::SceneFile,remus::meshtypes::Model> AssygenExporter;
typedef cmbNucExporterClient<remus::meshtypes::Mesh3D,remus::meshtypes::Mesh3D> CoregenExporter;
typedef cmbNucExporterClient<remus::meshtypes::Model,remus::meshtypes::Mesh3D> CubitExporter;

cmbNucExporterWorker *
cmbNucExporterWorker
::AssygenWorker(std::vector<std::string> extra_args,
                remus::worker::ServerConnection const& connection)
{
  return new cmbNucExporterWorker(remus::common::MeshIOType(remus::meshtypes::SceneFile(),
                                                            remus::meshtypes::Model()),
                                  extra_args, connection );
}

cmbNucExporterWorker *
cmbNucExporterWorker
::CoregenWorker(std::vector<std::string> extra_args,
                remus::worker::ServerConnection const& connection)
{
  return new cmbNucExporterWorker(remus::common::MeshIOType(remus::meshtypes::Mesh3D(),
                                                            remus::meshtypes::Mesh3D()),
                                  extra_args, connection );
}

cmbNucExporterWorker *
cmbNucExporterWorker
::CubitWorker(std::vector<std::string> extra_args,
             remus::worker::ServerConnection const& connection)
{
  return new cmbNucExporterWorker(remus::common::MeshIOType(remus::meshtypes::Model(),
                                                            remus::meshtypes::Mesh3D()),
                                  extra_args, connection );
}

cmbNucExporterWorker
::cmbNucExporterWorker( remus::common::MeshIOType miot,
                        std::vector<std::string> extra_args,
                        remus::worker::ServerConnection const& connection)
:remus::worker::Worker(miot, connection),
 ExtraArgs(extra_args)
{
}

void cmbNucExporterWorker::run()
{
  for(;;)
  {
    qDebug("waiting for job");
    remus::worker::Job job = this->getJob();

    if(!job.valid())
    {
      qDebug("job invalid");
      emit errorMessage("JOB NOT VALID");
      break;
    }
    qDebug("got a job");

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

    qDebug() << "FUNCTION: " << input.Function.c_str();
    for( std::vector<std::string>::const_iterator i = args.begin();
        i < args.end(); ++i )
    {
      qDebug() << "Arg: \"" << i->c_str() << "\"";
    }


    remus::common::ExecuteProcess* process = new remus::common::ExecuteProcess( input.Function, args);

    //actually launch the new process
    process->execute(remus::common::ExecuteProcess::Attached);

    //Wait for finish
    if(pollStatus(process, job))
    {
      std::cout << "success" << std::endl;
      remus::worker::JobResult results(job.id(),"DUMMY FOR NOW;");
      this->returnMeshResults(results);
    }
    else
    {
      remus::worker::JobStatus status(job.id(),remus::FAILED);
      updateStatus(status);
    }
    QDir::setCurrent( current );
    delete process;
  }
  qDebug() << "FINISH RUNNING WORKER";
}

bool cmbNucExporterWorker
::pollStatus( remus::common::ExecuteProcess* process,
              const remus::worker::Job& job)
{
  typedef remus::common::ProcessPipe ProcessPipe;

  //poll on STDOUT and STDERRR only
  bool validExection=true;
  remus::worker::JobStatus status(job.id(),remus::IN_PROGRESS);
  while(process->isAlive()&& validExection )
  {
    //poll till we have a data, waiting for-ever!
    ProcessPipe data = process->poll(-1);
    if(data.type == ProcessPipe::STDOUT)
    {
      //we have something on the output pipe
      status.Progress.setMessage(data.text);
      this->updateStatus(status);
    }
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
cmbNucExporterClient<JOB_REQUEST_IN, JOB_REQUEST_OUT>::cmbNucExporterClient(std::string server)
{
  if ( !server.empty())
    {
    Connection = remus::client::make_ServerConnection(server);
    }
}

template<class JOB_REQUEST_IN, class JOB_REQUEST_OUT>
ExporterOutput
cmbNucExporterClient<JOB_REQUEST_IN, JOB_REQUEST_OUT>::getOutput(ExporterInput const& in)
{
  remus::Client c(Connection);
  JOB_REQUEST_IN in_type;
  JOB_REQUEST_OUT out_type;
  remus::client::JobRequest request(in_type, out_type, in);
  if(c.canMesh(request))
    {
    remus::client::Job job = c.submitJob(request);
    remus::client::JobStatus jobState = c.jobStatus(job);

    //wait while the job is running
    while(jobState.good())
      {
      jobState = c.jobStatus(job);
      };

    if(jobState.finished())
      {
      remus::client::JobResult result = c.retrieveResults(job);
      return ExporterOutput(result);
      }
    }
  return ExporterOutput();
}

void cmbNucExport::run( const QString assygenExe,
                        const QStringList &assygenFile,
                        const QString cubitExe,
                        const QString coregenExe,
                        const QString coregenFile)
{
  emit currentProcess("Started setting up");
  //pop up progress bar
  double total_number_of_file = 2.0*assygenFile.count() + 6;
  double current = 0;
  emit progress(static_cast<int>(current/total_number_of_file*100));
  //start server
  remus::server::WorkerFactory factory;
  factory.setMaxWorkerCount(0);

  //create a default server with the factory
  remus::server::Server b(factory);

  //start accepting connections for clients and workers
  emit currentProcess("Starting server");
  qDebug() << "Starting server";
  bool valid = b.startBrokering();
  if( !valid )
  {
    emit errorMessage("failed to start server");
    emit currentProcess("failed to start server");
    emit done();
    return;
  }
  qDebug() << "Valid server";
  qDebug() << "waiting for server to start";
  b.waitForBrokeringToStart();
  current++;
  emit progress(static_cast<int>(current/total_number_of_file*100));
  qDebug() << "Sever started!";

  emit currentProcess("Starting Workers");
  QThread workerThread[3];
  cmbNucExporterWorker * assygenWorker = cmbNucExporterWorker::AssygenWorker();
  assygenWorker->moveToThread(&(workerThread[0]));
  connect( this, SIGNAL(startWorkers()),
           assygenWorker, SLOT(start()));
  cmbNucExporterWorker * coregenWorker = cmbNucExporterWorker::CoregenWorker();
  coregenWorker->moveToThread(&(workerThread[1]));
  connect( this, SIGNAL(startWorkers()),
           coregenWorker, SLOT(start()));
  std::vector<std::string> args;
  args.push_back("-nographics");
  args.push_back("-batch");
  cmbNucExporterWorker * cubitWorker = cmbNucExporterWorker::CubitWorker(args);
  cubitWorker->moveToThread(&workerThread[2]);
  connect( this, SIGNAL(startWorkers()),
           cubitWorker, SLOT(start()));
  workerThread[0].start();
  workerThread[1].start();
  workerThread[2].start();
  emit startWorkers();
  current += 3;
  emit progress(static_cast<int>(current/total_number_of_file*100));

  //Send files
  AssygenExporter ae;
  CubitExporter ce;
  for (QStringList::const_iterator i = assygenFile.constBegin();
       i != assygenFile.constEnd(); ++i)
  {
    QFileInfo fi(*i);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName();
    QString pass = path + '/' + name;
    emit currentProcess("assygen " + name);
    qDebug() << "assygen";
    ExporterOutput lr = ae.getOutput(ExporterInput(path, assygenExe, pass));
    current++;
    emit progress(static_cast<int>(current/total_number_of_file*100));
    if(!lr.Valid)
    {
      emit errorMessage("Assygen failed");
      emit currentProcess("assygen " + name + " FAILED");
      emit done();
      return;
    }

    qDebug() << "cubit";
    emit currentProcess("cubit " + name + ".jou");
    qDebug() <<pass + ".jou";
    lr = ce.getOutput(ExporterInput(path, cubitExe, pass + ".jou"));
    current++;
    emit progress(static_cast<int>(current/total_number_of_file*100));
    if(!lr.Valid)
    {
      emit errorMessage("Cubit failed");
      emit currentProcess("cubit " + name + ".jou FAILED");
      emit done();
      return;
    }
  }
  {
    QFileInfo fi(coregenFile);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName();
    QString pass = path + '/' + name;
    qDebug() << "core";
    emit currentProcess("coregen " + name + ".jou");
    CoregenExporter coreExport;
    ExporterOutput r = coreExport.getOutput(ExporterInput(path, coregenExe, pass));
    current++;
    emit progress(static_cast<int>(current/total_number_of_file*100));
    if(!r.Valid)
    {
      emit errorMessage("ERROR: Curegen failed");
      emit currentProcess("coregen " + name + ".jou FAILED");
      emit done();
      return;
    }
  }
  emit currentProcess("Finishing up export");
  b.stopBrokering();
  qDebug() << "stoping thread 1";
  workerThread[0].quit();
   qDebug() << "stoping thread 2";
  workerThread[1].quit();
   qDebug() << "stoping thread 3";
  workerThread[2].quit();
  qDebug() << "waiting thread 1";
  workerThread[0].wait();
  qDebug() << "waiting thread 2";
  workerThread[1].wait();
  qDebug() << "waiting thread 3";
  workerThread[2].wait();
  qDebug() << "DELETING WORKER 1";
  delete assygenWorker;
  qDebug() << "DELETING WORKER 2";
  delete coregenWorker;
  qDebug() << "DELETING WORKER 3";
  delete cubitWorker;
  current++;
  emit progress(static_cast<int>(current/total_number_of_file*100));
  qDebug() << "finish";
  emit currentProcess("Finished");
  emit done();
}

#endif //#ifndef cmbNucExport_cxx
