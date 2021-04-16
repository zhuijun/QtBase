#include "Connector.h"
#include <QTimerEvent>
#include <QHostAddress>
#include <QDnsLookup>

#ifdef _WIN32

#include <winsock2.h>
#include <Mstcpip.h>

int SocketTcpAlive(int socket)
{
    int ret = 0;
    struct tcp_keepalive in_keep_alive = { 0 };
    unsigned long ul_in_len = sizeof(struct tcp_keepalive);
    unsigned long ul_bytes_return = 0;

    in_keep_alive.onoff = 1;/*打开keepalive*/
    in_keep_alive.keepalivetime = 15 * 1000;/*距离第一个心跳，15秒内没有数据，视为超时*/
    in_keep_alive.keepaliveinterval = 1000; /*第一个心跳没有返回，每隔1秒重试*/

    ret = WSAIoctl(socket, SIO_KEEPALIVE_VALS, (LPVOID)&in_keep_alive, ul_in_len,
        NULL, 0, &ul_bytes_return, NULL, NULL);
    if (ret == SOCKET_ERROR)
    {
        qCritical("WSAIoctl failed: %d \n", WSAGetLastError());
        return -1;
    }
    return 0;
}
#endif // _WIN32

struct NetPacktHead
{
    int32_t len = 0;
    int32_t id = 0;
    int32_t reserve = 0;
};

Connector::Connector(NetEventHandle* handle, QObject *parent)
:QTcpSocket(parent), m_handle(handle)
{
    memset(m_recvBuffer, 0, sizeof(m_recvBuffer));

    connect(this, &QIODevice::readyRead, this, &Connector::onRead);
    connect(this, &QIODevice::bytesWritten, this, &Connector::onWrite);
    connect(this, &QAbstractSocket::connected, this, &Connector::onConnected);
    connect(this, &QAbstractSocket::disconnected, this, &Connector::onDisconnected);
    connect(this, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
        this, &Connector::displayError);

    // Create a DNS lookup.
    m_dns = new QDnsLookup(this);
    connect(m_dns, &QDnsLookup::finished,
        this, &Connector::handleServers);
}

Connector::~Connector()
{
}


void Connector::myConnect(const QString &hostName, quint16 port, OpenMode mode /*= ReadWrite*/, NetworkLayerProtocol protocol /*= AnyIPProtocol*/)
{
    m_hostName = hostName;
    m_port = port;
    m_mode = mode;
    m_protocol = protocol;

    m_dns->setType(QDnsLookup::A);
    m_dns->setName(hostName);
    m_dns->lookup();

    //QTcpSocket::connectToHost(hostName, port, mode, protocol);
}

void Connector::handleServers()
{
    // Check the lookup succeeded.
    if (m_dns->error() != QDnsLookup::NoError) {
        qWarning("DNS lookup failed");
        return;
    }

    QHostAddress address;
    // Handle the results.
    const auto records = m_dns->hostAddressRecords();
    for (const QDnsHostAddressRecord &record : records) {
        qDebug() << record.name() << record.value();
    }

    for (const QDnsHostAddressRecord &record : records) {
        address = record.value();
        if (!m_tryHostAddress.contains(record.value()))
        {
            m_tryHostAddress.append(address);
            break;
        }
    }
    
    qDebug() << "begin connect" << address;
    QTcpSocket::connectToHost(address, m_port, m_mode);
}

qint64 Connector::send(const char *data, int32_t len, int32_t id, int32_t reserve)
{
    if (state() == QAbstractSocket::ConnectedState)
    {
        NetPacktHead pktHead;
        pktHead.len = len + sizeof(NetPacktHead);
        pktHead.id = id;
        pktHead.reserve = reserve;

        m_sendList.append((char*)&pktHead, sizeof(NetPacktHead));

        if (data != nullptr && len > 0)
        {
            m_sendList.append(data, len);
        }

        return doWrite();
    }
    return 0;
}

qint64 Connector::send(const char *data, int32_t len)
{
    if (state() == QAbstractSocket::ConnectedState)
    {
        m_sendList.append(data, len);

        return doWrite();
    }
    return 0;
}

qint64 Connector::doWrite()
{
    if (m_canWrite && m_sendList.size() > 0)
    {
        qint64 n = write(m_sendList);
        if (n < 0)
        {
            //emit 
            qWarning() << "write failed. ";
        }
        else
        {
            m_canWrite = false;
        }
        return n;
    }
    return 0;
}

void Connector::procPackt()
{
    while (m_recvList.size() >= sizeof(NetPacktHead))
    {
        NetPacktHead* pktHead = (NetPacktHead*)m_recvList.data();
        if (pktHead->len <= m_recvList.size())
        {
            if (m_handle)
            {
                m_handle->onRecv(m_recvList.data(), pktHead->len);
            }
            m_recvList.remove(0, pktHead->len);
        }
        else
        {
            break;
        }
    }
}

void Connector::onRead()
{
    qint64 n = read(m_recvBuffer, sizeof(m_recvBuffer));
    if (n > 0)
    {
        m_recvList.append(m_recvBuffer, n);
    }
    
    if (m_recvList.size() > 0)
    {
        procPackt();
    }
}

void Connector::onWrite(qint64 bytes)
{
    m_canWrite = true;

    if (m_sendList.size() >= bytes)
    {
        m_sendList.remove(0, bytes);

        if (m_sendList.size() > 0)
        {
            doWrite();
        }
    }
    else
    {
        qWarning() << "error onWrite " << bytes;
    }
}

void Connector::onConnected()
{
    //qInfo() << "onConnected ";
    m_canWrite = true;
    m_startup = true;
    if (m_handle)
    {
        m_handle->onConnected();
    }

    onConnectedExt();

#ifdef _WIN32
    SocketTcpAlive(this->socketDescriptor());
#else
    setSocketOption(QAbstractSocket::SocketOption::KeepAliveOption, 1);
#endif // _WIN32

    //QVariant v = socketOption(QAbstractSocket::KeepAliveOption);
    //int i = v.toInt();
    //qDebug() << i;

    QHostAddress ha = peerAddress();
    qInfo() << "onConnected" << ha;
    //m_tryHostAddress.clear();
    m_tryHostAddress.append(ha);
}

void Connector::onDisconnected()
{
    qInfo() << "onDisconnected ";
    if (m_handle)
    {
        m_handle->onDisconnected();
    }

    m_sendList.clear();
    m_recvList.clear();
    m_canWrite = false;

    onDisconnectedExt();
}

void Connector::displayError(QAbstractSocket::SocketError socketError)
{
    qWarning() << "displayError " << socketError;
    if (!m_startup)
    {
        if (m_handle)
        {
            m_handle->onConnectFailed();
        }
    }
}

ConnectorWithReconnectTimer::ConnectorWithReconnectTimer(NetEventHandle * handle, QObject * parent)
    :Connector(handle, parent)
{
}

ConnectorWithReconnectTimer::~ConnectorWithReconnectTimer()
{
}

void ConnectorWithReconnectTimer::onConnectedExt()
{
    if (m_reconnectTimerId > 0)
    {
        killTimer(m_reconnectTimerId);
        m_reconnectTimerId = 0;
    }
}

void ConnectorWithReconnectTimer::onDisconnectedExt()
{
    //定时重连
    if (!m_isQuit)
    {
        if (m_reconnectTimerId == 0)
        {
            m_reconnectTimerId = startTimer(3000);
        }
    }
}

void ConnectorWithReconnectTimer::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_reconnectTimerId)
    {
        myConnect(m_hostName, m_port, m_mode, m_protocol);
        m_handle->onReconnected();
    }
}