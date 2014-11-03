#ifndef cmbNucExport_H
#define cmbNucExport_H

#include <string>
#include <sstream>
#include <vector>
#include <QObject>
#include <QThread>
#include <QMutex>

#include <remus/client/ServerConnection.h>
#include <remus/worker/Worker.h>
#include <remus/server/Server.h>

namespace remus
{
namespace common
{
class ExecuteProcess;
}
namespace server
{
class Server;
class WorkerFactory;
}
}

class cmbNucExporterWorker: public QObject, public remus::worker::Worker
{
  Q_OBJECT
  QThread workerThread;
public:
  static cmbNucExporterWorker *
    AssygenWorker(remus::worker::ServerConnection const& connection,
                  std::vector<std::string> extra_args = std::vector<std::string>());
  static cmbNucExporterWorker *
    CoregenWorker(remus::worker::ServerConnection const& connection,
                  std::vector<std::string> extra_args = std::vector<std::string>());
  static cmbNucExporterWorker *
    CubitWorker(remus::worker::ServerConnection const& connection,
                std::vector<std::string> extra_args = std::vector<std::string>());

public slots:
  void start()
  { this->run(); }
  void stop();
signals:
  void errorMessage( QString );
  void currentMessage( QString );
private:
  bool stillRunning();
  void run();
  bool StillRunning;
  mutable QMutex Mutex;
  std::string Name;

  bool pollStatus( remus::common::ExecuteProcess* process,
                  const remus::worker::Job& job);
  std::vector< std::string > ExtraArgs;

  cmbNucExporterWorker( std::string label, remus::common::MeshIOType miotype,
                        remus::worker::ServerConnection const& connection,
                        std::vector<std::string> extra_args = std::vector<std::string>());
};

class cmbNucExporterWorker;


class cmbNucExport : public QObject
{
  Q_OBJECT
  QThread workerThread;
public:
  cmbNucExport();
  ~cmbNucExport();
  void setKeepGoing(bool);
  void setAssygen(QString assygenExe,QString assygenLib);
  void setCoregen(QString coregenExe,QString coregenLib);
  void setCubit(QString cubitExe);
public slots:
  void run( const QStringList &assygenFile,
            const QString coregenFile,
            const QString CoreGenOutputFile);
  void run( const QStringList &assygenFile,
            const QString coregenFile,
            const QString CoreGenOutputFile,
            const QString assygenFileCylinder,
            const QString cubitFileCylinder,
            const QString cubitOutputFileCylinder,
            const QString coregenFileCylinder,
            const QString coregenResultFileCylinder );
  void runAssy( const QStringList &assygenFile );
  void runCore(const QString coregenFile,
               const QString CoreGenOutputFile);
  void runCore(const QString coregenFile,
               const QString CoreGenOutputFile,
               const QString assygenFileCylinder,
               const QString cubitFileCylinder,
               const QString cubitOutputFileCylinder,
               const QString coregenFileCylinder,
               const QString coregenResultFileCylinder);
  void cancel();
signals:
  void done();
  void successful();
  void cancelled();
  void sendCoreResult(QString);
  void progress(int);
  void currentProcess(QString);
  void errorMessage( QString );
  void fileDone();
  void statusMessage(QString);
protected:
signals:
  void startWorkers();
  void endWorkers();
private:
  bool runAssyHelper( const QStringList &assygenFile,
                      double & count, double max_count );
  bool runCubitHelper(const QString cubitFile,
                      const QString cubitOutputFile,
                      double & count, double max_count);
  bool runCoreHelper( const QString coregenFile,
                      const QString CoreGenOutputFile,
                      double & count, double max_count );
  bool exportCylinder( const QString assygenFile,
                       const QString cubitFile,
                       const QString cubitOutputFile,
                       const QString coregenFile,
                       const QString coregenResultFile,
                       double & total_number_of_file,
                       double & current );
  void finish();
  void cancelHelper();
  void failedHelper(QString msg, QString pmsg);
  bool startUpHelper(double & count, double max_count);
  bool keepGoing() const;
  bool KeepGoing;
  void deleteServer();
  mutable QMutex Mutex, Memory, ServerProtect;
  void constructWorkers();
  void deleteWorkers();
  QThread WorkerThreads[3];
  cmbNucExporterWorker * assygenWorker;
  cmbNucExporterWorker * coregenWorker;
  cmbNucExporterWorker * cubitWorker;
  remus::server::Server * Server;
  remus::worker::ServerConnection ServerConnection;
  boost::shared_ptr<remus::server::WorkerFactory> factory;
  QString AssygenExe, AssygenLib;
  QString CoregenExe, CoregenLib;
  QString CubitExe;
};

#endif //cmbNucExport_H
