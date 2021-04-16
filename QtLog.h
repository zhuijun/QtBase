#pragma once

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QThread>
#include <QDebug>

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);


class LogWorker : public QObject
{
    Q_OBJECT

public:
    void SetLogLevel(QtMsgType logLevel)
    {
        m_logLevel = logLevel;
    }

public slots:

    void doWork(QString msg, int type);

private:

    QtMsgType m_logLevel = QtMsgType::QtDebugMsg;
    QString m_file_name;
};

class LogController : public QObject
{
    Q_OBJECT
        QThread workerThread;
        LogWorker *worker = nullptr;

public:
    LogController() {
        worker = new LogWorker;
        worker->moveToThread(&workerThread);
        connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
        connect(this, &LogController::writeLog, worker, &LogWorker::doWork, Qt::ConnectionType::QueuedConnection);
    }

    ~LogController() {
        workerThread.quit();
        workerThread.wait();
    }

public:

    static void CreateInstance()
    {
        if (m_instance == nullptr)
        {
            m_instance = new LogController();
        }
    }

    static void DestoryInstance()
    {
        if (m_instance != nullptr)
        {
            delete m_instance;
        }
    }

    static LogController* GetInstance()
    {
        return m_instance;
    }

    bool Start(QtMsgType logLevel)
    {
        if (workerThread.isRunning())
        {
            return false;
        }
        worker->SetLogLevel(logLevel);
        workerThread.start();
        return true;
    }

signals:
    void writeLog(QString msg, int type);

private:

    static LogController * m_instance;
};