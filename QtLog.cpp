#include "QtLog.h"
#include <QRegExp>

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    emit LogController::GetInstance()->writeLog(msg, type);
}

void LogWorker::doWork(QString msg, int type) 
{
    if (type < m_logLevel)
    {
        return;
    }

    QString text;
    switch (type)
    {
    case QtDebugMsg:
        text = QString("Debug:");
        break;
    case QtWarningMsg:
        text = QString("Warning:");
        break;
    case QtCriticalMsg:
        text = QString("Critical:");
        break;
    case QtFatalMsg:
        text = QString("Fatal:");
        break;
    default:
        text = QString("Info:");
        break;
    }

    //QString context_info = QString("File:(%1) Line:(%2)").arg(QString(context.file)).arg(context.line);
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString current_date = QString("(%1)").arg(current_date_time);
    QString message = QString("%1 %2 %3").arg(text).arg(current_date).arg(msg);

    QString log_dir(QDir::currentPath() + "/log");
    QDir dir(log_dir);
    if (!dir.exists())
    {
        dir.mkdir(log_dir);
    }

    QString date = QDate::currentDate().toString("yyyy-MM-dd");
    if (m_file_name.isEmpty())
    {
        QStringList filenames = dir.entryList(QDir::Files, QDir::Name);
        if (!filenames.isEmpty())
        {
            m_file_name = QString(log_dir + "/" + filenames.back());
        }
        else
        {
            m_file_name = QString(log_dir + "/%1_1_log.txt").arg(date);
        }
    }

    QRegExp rx(date + "_(\\d*)" + "_log.txt");
    int pos = rx.indexIn(m_file_name);
    int count = 0;
    if (pos == -1)
    {
        m_file_name = QString(log_dir + "/%1_1_log.txt").arg(date);
    }
    else
    {
        count = rx.cap(1).toInt();
    }

    do
    {
        QFile file(m_file_name);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            if (file.size() > 20 * 1024 * 1024)
            {
                m_file_name = QString(log_dir + "/%1_%2_log.txt").arg(date).arg(++count);
                file.close();
                continue;
            }
            QTextStream text_stream(&file);
            text_stream << message << "\r\n";
            file.flush();
            file.close();
        }
        break;
    } while (true);

}

LogController * LogController::m_instance = nullptr;