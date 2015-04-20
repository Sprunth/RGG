#ifndef cmbNucExport_H
#define cmbNucExport_H

#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <QObject>
#include <QThread>
#include <QStringList>
#include <QMutex>

class cmbNucExporterWorker;
class cmbNucExporterClient;
struct cmbNucExportInternal;
class cmbNucExportWorkerRunner;
struct JobHolder;

class RunnableConnection : public QObject
{
  Q_OBJECT
public:
  RunnableConnection():terminate(false)
  {}
  bool terminate;
  void sendErrorMessage(QString s)
  { emit errorMessage(s); }
  void sendCurrentMessage(QString s)
  { emit currentMessage(s);}
  void sendDoNextStep(QString outputFile, int index)
  { emit doNextStep(outputFile, index); }
public slots:
  void terminateProcess()
  {
    terminate = true;
  }
signals:
  void errorMessage( QString );
  void currentMessage( QString );
  void doNextStep(QString, int);
};

struct Message
{
  struct AssygenTask
  {
    QString assygenFile;
    QString outFileExtension;
    bool justRunCubit;
    AssygenTask(QString af, QString ofe, bool jrc)
    :assygenFile(af), outFileExtension(ofe), justRunCubit(jrc)
    {}
  };

  struct CylinderTask
  {
    QString assygenFile;
    QString coregenFile;
    QString coregenResultFile;
    QString cubitFile;
    QString cubitOutputFile;
    bool valid;
    CylinderTask(): valid(false){}
    CylinderTask(QString af, QString cf, QString crf, QString cuf, QString cuof)
    : assygenFile(af), coregenFile(cf), coregenResultFile(crf),
    cubitFile(cuf), cubitOutputFile(cuof)
    {
      valid = !(assygenFile.isEmpty() || cubitFile.isEmpty() || coregenFile.isEmpty());
    }
  };
  std::vector<AssygenTask> assemblyTasks;
  CylinderTask cylinderTask;
  QString coregenFile;
  QStringList boundryFiles;
  QStringList CoreGenOutputFile;
  bool keepGoingAfterError;
};

class cmbNucExport : public QObject
{
  Q_OBJECT
  QThread workerThread;
public:

  friend class cmbNucExportWorkerRunner;
  cmbNucExport();
  ~cmbNucExport();
  void setKeepGoing(bool);
  void setAssygen(QString assygenExe,QString assygenLib);
  void setCoregen(QString coregenExe,QString coregenLib);
  void setPostBL(QString postBLExe, QString postBLLib);
  void setNumberOfProcessors(int v);
  void setCubit(QString cubitExe);
  void waitTillDone();
public slots:
  void run( Message const& message );
  void cancel();
signals:
  void done();
  void successful();
  void cancelled();
  void terminate();
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
  std::vector<JobHolder*> runAssyHelper( std::vector<Message::AssygenTask> const& msg );
  std::vector<JobHolder*> runCubitHelper(const QString cubitFile,
                                         std::vector<JobHolder*> debIn,
                                         const QString cubitOutputFile);
  std::vector<JobHolder*> runCoreHelper( const QString coregenFile,
                                         std::vector<JobHolder*> debIn,
                                         const QString CoreGenOutputFile,
                                         bool use_cylinder_version );
  std::vector<JobHolder*> runPostBLHelper( const QStringList boundryControlFiles,
                                           std::vector<JobHolder*> debIn,
                                           const QStringList CoreGenOutputFile );
  std::vector<JobHolder*> exportCylinder( Message::CylinderTask const& msg );
  JobHolder* makeAssyJob(const QString assygenFile);

  void finish();
  void cancelHelper();
  void failedHelper(QString msg, QString pmsg);
  bool startUpHelper();
  bool keepGoing() const;
  bool KeepGoing;
  void deleteServer();
  void stopJobs();
  mutable QMutex Mutex, Memory, ServerProtect;

  QString AssygenExe, AssygenLib;
  QString CylinderCoregenExe;
  QString CoregenExe, CoregenLib;
  QString CubitExe;
  QString PostBLExe, PostBLLib;

  cmbNucExportInternal * internal;

  std::vector<JobHolder*> jobs_to_do;
  cmbNucExporterClient * client;
  void clearJobs();
  void processJobs();
  mutable QMutex end_control;
  bool isDone;
  bool keepGoingAfterError;
};

//Q_DECLARE_METATYPE(Message);

#endif //cmbNucExport_H
