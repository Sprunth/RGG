#ifndef cmbNucExport_H
#define cmbNucExport_H

#include <string>
#include <sstream>
#include <vector>
#include <QObject>
#include <QThread>

#include <remus/client/ServerConnection.h>
#include <remus/worker/Worker.h>

namespace remus
{
namespace common
{
class ExecuteProcess;
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
  //void stop();
signals:
  void errorMessage( QString );
private:
  //bool stillRunning()
  void run();
  bool pollStatus( remus::common::ExecuteProcess* process,
                  const remus::worker::Job& job);
  std::vector< std::string > ExtraArgs;

  cmbNucExporterWorker( remus::common::MeshIOType miotype,
                       std::vector<std::string> extra_args = std::vector<std::string>(),
                       remus::worker::ServerConnection const& connection = remus::worker::ServerConnection());
};

class cmbNucExport : public QObject
{
  Q_OBJECT
  QThread workerThread;
public slots:
  void run( const QString assygenExe,
            const QStringList &assygenFile,
            const QString cubitExe,
            const QString coregenExe,
            const QString coregenFile);
signals:
  void done();
  void progress(int);
  void currentProcess(QString);
  void errorMessage( QString );
protected:
signals:
  void startWorkers();
  //void endWorkers();
};

#endif //cmbNucExport_H
