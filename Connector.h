#pragma once

#include <QTcpSocket>
#include <QList>
#include <QDnsLookup>
#include <stdint.h>

class Connector : public QTcpSocket
{
public:
    class NetEventHandle
    {
    public:
        virtual void onRecv(char *data, qint64 len) = 0;
        virtual void onConnected() = 0;
        virtual void onDisconnected() = 0;
        virtual void onReconnected() = 0;
        virtual void onConnectFailed() = 0;
    };

    explicit Connector(NetEventHandle* handle, QObject *parent = Q_NULLPTR);
    virtual ~Connector();

    void myConnect(const QString &hostName, quint16 port, OpenMode mode = ReadWrite, NetworkLayerProtocol protocol = AnyIPProtocol);

    virtual void quit()
    {
        close();
    }

    qint64 send(const char *data, int32_t len, int32_t id, int32_t reserve);
    qint64 send(const char *data, int32_t len);

protected:
    qint64 doWrite();
    void     procPackt();

    virtual void onConnectedExt() {}
    virtual void onDisconnectedExt() {}

private slots:
    void onRead();
    void onWrite(qint64 bytes);
    void onConnected();
    void onDisconnected();
    void displayError(QAbstractSocket::SocketError socketError);
    void handleServers();

protected:

    QString m_hostName;
    quint16 m_port = 0;
    OpenMode m_mode = ReadWrite;
    NetworkLayerProtocol m_protocol = AnyIPProtocol;
    NetEventHandle *m_handle = nullptr;

private:

    QByteArray m_sendList; //发送队列
    QByteArray m_recvList; //接收队列

    char m_recvBuffer[8192];
    bool m_canWrite = false;
    bool m_startup = false;

    QDnsLookup* m_dns = nullptr;
    QList<QHostAddress> m_tryHostAddress;
};

class ConnectorWithReconnectTimer : public Connector
{
public:
    explicit ConnectorWithReconnectTimer(NetEventHandle* handle, QObject *parent = Q_NULLPTR);
    virtual ~ConnectorWithReconnectTimer();

    virtual void quit()
    {
        m_isQuit = true;
        close();
    }

protected:
    virtual void onConnectedExt() override;
    virtual void onDisconnectedExt() override;

    virtual void timerEvent(QTimerEvent *event) override;

private:

    int m_reconnectTimerId = 0;
    bool m_isQuit = false;
};