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

namespace remus
{
namespace common
{
class ExecuteProcess;
}
namespace server
{
class Server;
}
}

class cmbNucExporterWorker: public QObject, public remus::worker::Worker
{
  Q_OBJECT
  QThread workerThread;
public:
  static cmbNucExporterWorker *
    AssygenWorker(std::vector<std::string> extra_args = std::vector<std::string>(),
                  remus::worker::ServerConnection const& connection = remus::worker::ServerConnection());
  static cmbNucExporterWorker *
    CoregenWorker(std::vector<std::string> extra_args = std::vector<std::string>(),
                  remus::worker::ServerConnection const& connection = remus::worker::ServerConnection());
  static cmbNucExporterWorker *
    CubitWorker(std::vector<std::string> extra_args = std::vector<std::string>(),
                remus::worker::ServerConnection const& connection = remus::worker::ServerConnection());

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

  cmbNucExporterWorker( remus::common::MeshIOType miotype,
                       std::vector<std::string> extra_args = std::vector<std::string>(),
                       remus::worker::ServerConnection const& connection = remus::worker::ServerConnection());
};

class cmbNucExporterWorker;


class cmbNucExport : public QObject
{
  Q_OBJECT
  QThread workerThread;
public:
  void setKeepGoing(bool);
public slots:
  void run( const QString assygenExe,
            const QStringList &assygenFile,
            const QString cubitExe,
            const QString coregenExe,
            const QString coregenFile,
            const QString CoreGenOutputFile);
  void runAssy( const QString assygenExe,
                const QStringList &assygenFile,
                const QString cubitExe );
  void runCore(const QString coregenExe,
               const QString coregenFile,
               const QString CoreGenOutputFile);
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
  bool runAssyHelper( const QString assygenExe,
                      const QStringList &assygenFile,
                      const QString cubitExe,
                      double & count, double max_count );
  bool runCoreHelper( const QString coregenExe,
                      const QString coregenFile,
                      const QString CoreGenOutputFile,
                      double & count, double max_count );
  void finish();
  void cancelHelper();
  void failedHelper(QString msg, QString pmsg);
  bool startUpHelper(double & count, double max_count);
  bool keepGoing() const;
  bool KeepGoing;
  mutable QMutex Mutex, Memory;
  void constructWorkers();
  void deleteWorkers();
  QThread WorkerThreads[3];
  cmbNucExporterWorker * assygenWorker;
  cmbNucExporterWorker * coregenWorker;
  cmbNucExporterWorker * cubitWorker;
  remus::server::Server * Server;
};

#endif //cmbNucExport_H
