#include "transfermanager.h"

#include <QDataStream>
#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QSaveFile>
#include <QTcpSocket>

#include <algorithm>
#include <limits>

namespace
{
constexpr quint32 ProtocolMagic = 0x46544631; // "FTF1"
constexpr quint8 ProtocolVersion = 1;
constexpr quint8 FileMessage = 1;
// 固定协议头共 18 字节，各整数统一使用大端序，保证不同主机之间解析结果一致。
constexpr qsizetype FixedHeaderSize = 4 + 1 + 1 + 4 + 8;
constexpr quint32 MaximumFileNameBytes = 4096;
// 每次最多从磁盘读取 64 KiB，并限制 Socket 待发送缓存，避免大文件一次性占满内存。
constexpr qint64 SendChunkSize = 64 * 1024;
constexpr qint64 MaximumPendingSocketBytes = 256 * 1024;
}

TransferManager::TransferManager(QObject *parent) : QObject(parent)
{
    // QTcpServer 收到连接后异步发出 newConnection，槽函数负责取出并绑定客户端 Socket。
    connect(&server_, &QTcpServer::newConnection, this,
            &TransferManager::acceptPendingConnection);
}

TransferManager::~TransferManager()
{
    stop();
}

void TransferManager::configure(Mode mode, const QString &host, quint16 port,
                                const QString &saveDirectory)
{
    if (isActive())
    {
        stop();
    }
    mode_ = mode;
    host_ = host;
    port_ = port;
    saveDirectory_ = QDir::cleanPath(saveDirectory);
}

bool TransferManager::start()
{
    if (isActive())
    {
        emit transferError(QStringLiteral("网络服务已经启动，请先断开。"));
        return false;
    }
    if (!QDir().mkpath(saveDirectory_))
    {
        emit transferError(QStringLiteral("无法创建接收目录：%1").arg(saveDirectory_));
        return false;
    }

    if (mode_ == Mode::Server)
    {
        // 服务端监听全部 IPv4 网卡，便于同机和局域网客户端使用同一套程序连接。
        if (!server_.listen(QHostAddress::AnyIPv4, port_))
        {
            emit transferError(QStringLiteral("监听失败：%1").arg(server_.errorString()));
            return false;
        }
        const QString description = QStringLiteral("正在监听 0.0.0.0:%1")
                                        .arg(server_.serverPort());
        emit logMessage(description);
        emit stateChanged(true, false, description);
        return true;
    }

    // connectToHost 为异步调用，连接结果由 connected/errorOccurred 信号返回。
    auto *socket = new QTcpSocket(this);
    attachSocket(socket);
    emit logMessage(QStringLiteral("正在连接 %1:%2").arg(host_).arg(port_));
    emit stateChanged(true, false,
                      QStringLiteral("正在连接 %1:%2").arg(host_).arg(port_));
    socket->connectToHost(host_, port_);
    return true;
}

void TransferManager::stop()
{
    server_.close();
    releaseSocket(true);
    resetReceiveState();
    receiveBuffer_.clear();
    resetSendState();
    emit stateChanged(false, false, QStringLiteral("未连接"));
}

bool TransferManager::sendFile(const QString &filePath)
{
    if (!isConnected())
    {
        emit transferError(QStringLiteral("尚未连接到对端，无法发送文件。"));
        return false;
    }
    if (sendActive_)
    {
        emit transferError(QStringLiteral("当前文件仍在发送，请稍后再试。"));
        return false;
    }

    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile())
    {
        emit transferError(QStringLiteral("文件不存在：%1").arg(filePath));
        return false;
    }

    sendFile_.setFileName(filePath);
    if (!sendFile_.open(QIODevice::ReadOnly))
    {
        emit transferError(QStringLiteral("无法打开待发送文件：%1").arg(sendFile_.errorString()));
        return false;
    }

    const QByteArray fileNameBytes = fileInfo.fileName().toUtf8();
    if (fileNameBytes.isEmpty() || fileNameBytes.size() > MaximumFileNameBytes)
    {
        sendFile_.close();
        emit transferError(QStringLiteral("文件名为空或过长，无法发送。"));
        return false;
    }

    sendFilePath_ = filePath;
    sendFileName_ = fileInfo.fileName();
    sendFileSize_ = fileInfo.size();
    sendPayloadQueued_ = 0;
    sendPrefix_ = createHeader(fileNameBytes, static_cast<quint64>(sendFileSize_));
    sendPrefixOffset_ = 0;
    pendingSendChunk_.clear();
    pendingSendOffset_ = 0;
    sendActive_ = true;

    emit logMessage(QStringLiteral("开始发送：%1（%2 字节）")
                        .arg(sendFileName_)
                        .arg(sendFileSize_));
    emit sendProgress(0, sendFileSize_, sendFileName_);
    pumpSend();
    return true;
}

bool TransferManager::isActive() const
{
    return server_.isListening()
           || (socket_ && socket_->state() != QAbstractSocket::UnconnectedState);
}

bool TransferManager::isConnected() const
{
    return socket_ && socket_->state() == QAbstractSocket::ConnectedState;
}

quint16 TransferManager::listeningPort() const
{
    return server_.serverPort();
}

QByteArray TransferManager::createHeader(const QByteArray &fileName, quint64 fileSize)
{
    QByteArray header;
    QDataStream stream(&header, QIODevice::WriteOnly);
    // 显式固定字节序和流版本，发送端与接收端必须采用完全相同的编码方式。
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setVersion(QDataStream::Qt_6_2);
    stream << ProtocolMagic << ProtocolVersion << FileMessage
           << static_cast<quint32>(fileName.size()) << fileSize;
    header.append(fileName);
    return header;
}

void TransferManager::acceptPendingConnection()
{
    // 一次 newConnection 信号可能对应多个排队连接，因此必须循环取完等待队列。
    while (server_.hasPendingConnections())
    {
        QTcpSocket *pendingSocket = server_.nextPendingConnection();
        if (isConnected())
        {
            // 本作业界面按单客户端设计，已有连接时明确拒绝后续客户端，防止状态互相覆盖。
            emit logMessage(QStringLiteral("已有客户端连接，已拒绝新的连接请求。"));
            pendingSocket->disconnectFromHost();
            pendingSocket->deleteLater();
            continue;
        }
        attachSocket(pendingSocket);
        const QString description = QStringLiteral("客户端已连接：%1").arg(peerDescription());
        emit logMessage(description);
        emit stateChanged(true, true, description);
    }
}

void TransferManager::attachSocket(QTcpSocket *socket)
{
    // 同一时刻只维护一个有效 Socket；绑定新连接前先清理旧连接及其信号槽。
    releaseSocket(true);
    socket_ = socket;
    socket->setParent(this);

    // 网络层全部采用 Qt 异步信号，避免 waitFor... 一类阻塞调用卡住 GUI 事件循环。
    connect(socket, &QTcpSocket::readyRead, this, &TransferManager::readIncomingData);
    // bytesWritten 表示底层缓存腾出了空间，据此继续泵送下一段文件数据。
    connect(socket, &QTcpSocket::bytesWritten, this, &TransferManager::continueSending);
    connect(socket, &QTcpSocket::connected, this, [this, socket]()
            {
                if (socket_ != socket)
                {
                    return;
                }
                const QString description = QStringLiteral("已连接：%1").arg(peerDescription());
                emit logMessage(description);
                emit stateChanged(true, true, description);
            });
    connect(socket, &QTcpSocket::disconnected, this, [this, socket]()
            {
                if (socket_ != socket)
                {
                    return;
                }
                if (sendActive_)
                {
                    failSend(QStringLiteral("连接已断开，文件发送未完成。"));
                }
                if (receivingFile_)
                {
                    failReceive(QStringLiteral("连接已断开，接收中的文件已取消。"));
                }
                emit logMessage(QStringLiteral("对端已断开连接。"));
                const bool listening = server_.isListening();
                releaseSocket(false);
                emit stateChanged(listening, false,
                                  listening ? QStringLiteral("继续等待客户端连接")
                                            : QStringLiteral("未连接"));
            });
    connect(socket, &QTcpSocket::errorOccurred, this,
            [this, socket](QAbstractSocket::SocketError error)
            {
                if (socket_ != socket || error == QAbstractSocket::RemoteHostClosedError)
                {
                    return;
                }
                const QString message = QStringLiteral("Socket 错误：%1").arg(socket->errorString());
                emit logMessage(message);
                emit transferError(message);
                if (mode_ == Mode::Client && socket->state() == QAbstractSocket::UnconnectedState)
                {
                    releaseSocket(false);
                    emit stateChanged(false, false, QStringLiteral("连接失败"));
                }
            });
}

void TransferManager::releaseSocket(bool abortSocket)
{
    if (!socket_)
    {
        return;
    }

    QTcpSocket *socket = socket_.data();
    // 先清空成员并断开信号，避免 abort()/deleteLater() 触发回调后再次操作已释放状态。
    socket_ = nullptr;
    socket->disconnect(this);
    if (abortSocket)
    {
        socket->abort();
    }
    socket->deleteLater();
}

void TransferManager::readIncomingData()
{
    if (!socket_)
    {
        return;
    }
    // readyRead 只说明“当前有数据”，不保证一次读到一个完整协议帧。
    receiveBuffer_.append(socket_->readAll());
    processReceiveBuffer();
}

void TransferManager::processReceiveBuffer()
{
    // 接收状态机：协议头和文件数据都可能被 TCP 任意拆分，也可能多帧粘在一起。
    while (socket_)
    {
        if (!receivingFile_)
        {
            if (receiveBuffer_.size() < FixedHeaderSize)
            {
                return;
            }

            // 缓冲区达到固定头长度后先窥视解析；文件名尚未收全时不移除任何字节。
            const QByteArray fixedHeader = receiveBuffer_.left(FixedHeaderSize);
            QDataStream stream(fixedHeader);
            stream.setByteOrder(QDataStream::BigEndian);
            stream.setVersion(QDataStream::Qt_6_2);

            quint32 magic = 0;
            quint8 version = 0;
            quint8 messageType = 0;
            quint32 fileNameLength = 0;
            quint64 fileSize = 0;
            stream >> magic >> version >> messageType >> fileNameLength >> fileSize;

            if (stream.status() != QDataStream::Ok || magic != ProtocolMagic
                || version != ProtocolVersion || messageType != FileMessage
                || fileNameLength == 0 || fileNameLength > MaximumFileNameBytes
                || fileSize > static_cast<quint64>(std::numeric_limits<qint64>::max()))
            {
                failReceive(QStringLiteral("收到无效协议头，连接已终止。"));
                return;
            }

            const qsizetype completeHeaderSize = FixedHeaderSize
                                                 + static_cast<qsizetype>(fileNameLength);
            if (receiveBuffer_.size() < completeHeaderSize)
            {
                return;
            }

            receiveBuffer_.remove(0, FixedHeaderSize);
            const QByteArray fileNameBytes = receiveBuffer_.left(fileNameLength);
            receiveBuffer_.remove(0, fileNameLength);

            // 只保留基础文件名，丢弃对端可能携带的目录，避免把文件写出指定保存目录。
            receiveFileName_ = QFileInfo(QString::fromUtf8(fileNameBytes)).fileName();
            if (receiveFileName_.isEmpty())
            {
                failReceive(QStringLiteral("收到的文件名无效。"));
                return;
            }

            receiveFileSize_ = fileSize;
            receiveBytes_ = 0;
            receiveFilePath_ = QDir(saveDirectory_).filePath(receiveFileName_);
            // QSaveFile 先写临时文件，全部接收成功后再原子提交，失败时不会留下半个目标文件。
            receiveFile_ = std::make_unique<QSaveFile>(receiveFilePath_);
            if (!receiveFile_->open(QIODevice::WriteOnly))
            {
                failReceive(QStringLiteral("无法保存接收文件：%1")
                                .arg(receiveFile_->errorString()));
                return;
            }
            receivingFile_ = true;
            emit logMessage(QStringLiteral("开始接收：%1（%2 字节）")
                                .arg(receiveFileName_)
                                .arg(receiveFileSize_));
            emit receiveProgress(0, static_cast<qint64>(receiveFileSize_), receiveFileName_);

            if (receiveFileSize_ == 0)
            {
                finishReceivedFile();
                continue;
            }
        }

        if (receiveBuffer_.isEmpty())
        {
            return;
        }

        // 仅消费当前文件仍需的字节，多余数据留在缓冲区作为下一帧继续解析（处理粘包）。
        const quint64 remaining = receiveFileSize_ - receiveBytes_;
        const qsizetype writeSize = static_cast<qsizetype>(
            std::min<quint64>(remaining, static_cast<quint64>(receiveBuffer_.size())));
        const qint64 written = receiveFile_->write(receiveBuffer_.constData(), writeSize);
        if (written != writeSize)
        {
            failReceive(QStringLiteral("写入接收文件失败：%1")
                            .arg(receiveFile_->errorString()));
            return;
        }

        receiveBuffer_.remove(0, writeSize);
        receiveBytes_ += static_cast<quint64>(written);
        emit receiveProgress(static_cast<qint64>(receiveBytes_),
                             static_cast<qint64>(receiveFileSize_), receiveFileName_);

        if (receiveBytes_ == receiveFileSize_)
        {
            finishReceivedFile();
        }
    }
}

void TransferManager::finishReceivedFile()
{
    const QString completedPath = receiveFilePath_;
    // 只有字节数完全匹配协议头声明的大小时才提交临时文件。
    if (!receiveFile_->commit())
    {
        failReceive(QStringLiteral("提交接收文件失败：%1").arg(receiveFile_->errorString()));
        return;
    }

    emit logMessage(QStringLiteral("接收完成：%1").arg(completedPath));
    resetReceiveState();
    emit fileReceived(completedPath);
}

void TransferManager::failReceive(const QString &message)
{
    if (receiveFile_)
    {
        receiveFile_->cancelWriting();
    }
    // 协议已失去同步或文件不完整时清空状态并断开连接，禁止继续误解析后续字节。
    resetReceiveState();
    receiveBuffer_.clear();
    emit transferError(message);
    emit logMessage(message);
    if (socket_)
    {
        socket_->abort();
    }
}

void TransferManager::resetReceiveState()
{
    if (receiveFile_ && receiveFile_->isOpen())
    {
        receiveFile_->cancelWriting();
    }
    receiveFile_.reset();
    receivingFile_ = false;
    receiveFileName_.clear();
    receiveFilePath_.clear();
    receiveFileSize_ = 0;
    receiveBytes_ = 0;
}

void TransferManager::continueSending()
{
    pumpSend();
}

void TransferManager::pumpSend()
{
    if (!sendActive_ || !isConnected())
    {
        return;
    }

    // 只在 Qt 待发送缓存低于阈值时继续写入，形成由 bytesWritten 驱动的发送流水线。
    while (socket_->bytesToWrite() < MaximumPendingSocketBytes)
    {
        if (sendPrefixOffset_ < sendPrefix_.size())
        {
            // 协议头必须先于文件数据发送，同时处理 write() 只接收部分字节的情况。
            const qint64 written = socket_->write(sendPrefix_.constData() + sendPrefixOffset_,
                                                  sendPrefix_.size() - sendPrefixOffset_);
            if (written < 0)
            {
                failSend(QStringLiteral("发送协议头失败：%1").arg(socket_->errorString()));
                return;
            }
            if (written == 0)
            {
                return;
            }
            sendPrefixOffset_ += written;
            continue;
        }

        if (pendingSendOffset_ < pendingSendChunk_.size())
        {
            const qint64 written = socket_->write(
                pendingSendChunk_.constData() + pendingSendOffset_,
                pendingSendChunk_.size() - pendingSendOffset_);
            if (written < 0)
            {
                failSend(QStringLiteral("发送文件数据失败：%1").arg(socket_->errorString()));
                return;
            }
            if (written == 0)
            {
                return;
            }
            pendingSendOffset_ += written;
            sendPayloadQueued_ += written;
            emit sendProgress(sendPayloadQueued_, sendFileSize_, sendFileName_);
            continue;
        }

        // 当前块全部交给 Socket 后再从磁盘读取下一块，内存占用与文件总大小无关。
        pendingSendChunk_.clear();
        pendingSendOffset_ = 0;
        if (sendPayloadQueued_ >= sendFileSize_)
        {
            break;
        }

        const qint64 remaining = sendFileSize_ - sendPayloadQueued_;
        pendingSendChunk_ = sendFile_.read(std::min(SendChunkSize, remaining));
        if (pendingSendChunk_.isEmpty() && remaining > 0)
        {
            failSend(QStringLiteral("读取待发送文件失败：%1").arg(sendFile_.errorString()));
            return;
        }
    }

    // 文件数据已全部排入且 Socket 缓存清空后，才向界面报告“发送完成”。
    if (sendPrefixOffset_ == sendPrefix_.size() && sendPayloadQueued_ == sendFileSize_
        && socket_->bytesToWrite() == 0)
    {
        finishSend();
    }
}

void TransferManager::finishSend()
{
    const QString completedPath = sendFilePath_;
    emit logMessage(QStringLiteral("发送完成：%1").arg(completedPath));
    resetSendState();
    emit fileSent(completedPath);
}

void TransferManager::failSend(const QString &message)
{
    resetSendState();
    emit transferError(message);
    emit logMessage(message);
}

void TransferManager::resetSendState()
{
    if (sendFile_.isOpen())
    {
        sendFile_.close();
    }
    sendFilePath_.clear();
    sendFileName_.clear();
    sendPrefix_.clear();
    sendPrefixOffset_ = 0;
    pendingSendChunk_.clear();
    pendingSendOffset_ = 0;
    sendFileSize_ = 0;
    sendPayloadQueued_ = 0;
    sendActive_ = false;
}

QString TransferManager::peerDescription() const
{
    if (!socket_)
    {
        return QStringLiteral("未知对端");
    }
    return QStringLiteral("%1:%2")
        .arg(socket_->peerAddress().toString())
        .arg(socket_->peerPort());
}
