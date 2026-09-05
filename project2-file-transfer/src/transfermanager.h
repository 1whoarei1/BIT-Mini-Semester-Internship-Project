#ifndef TRANSFERMANAGER_H
#define TRANSFERMANAGER_H

#include <QByteArray>
#include <QFile>
#include <QObject>
#include <QPointer>
#include <QTcpServer>

#include <memory>

class QSaveFile;
class QTcpSocket;

// 封装 TCP 连接、协议编解码及文件流式读写，使主窗口只负责界面展示和用户交互。
class TransferManager : public QObject
{
    Q_OBJECT

public:
    enum class Mode
    {
        Client,
        Server
    };
    Q_ENUM(Mode)

    explicit TransferManager(QObject *parent = nullptr);
    ~TransferManager() override;

    void configure(Mode mode, const QString &host, quint16 port, const QString &saveDirectory);
    bool start();
    void stop();
    bool sendFile(const QString &filePath);

    bool isActive() const;
    bool isConnected() const;
    quint16 listeningPort() const;

signals:
    // 网络状态和传输结果均通过信号通知界面，避免网络层直接操作 UI 控件。
    void stateChanged(bool active, bool connected, const QString &description);
    void logMessage(const QString &message);
    void sendProgress(qint64 current, qint64 total, const QString &fileName);
    void receiveProgress(qint64 current, qint64 total, const QString &fileName);
    void fileSent(const QString &filePath);
    void fileReceived(const QString &filePath);
    void transferError(const QString &message);

private slots:
    void acceptPendingConnection();
    void readIncomingData();
    void continueSending();

private:
    // 协议帧格式：魔数 + 版本 + 消息类型 + 文件名长度 + 文件大小 + UTF-8 文件名 + 文件数据。
    static QByteArray createHeader(const QByteArray &fileName, quint64 fileSize);
    void attachSocket(QTcpSocket *socket);
    void releaseSocket(bool abortSocket);
    void processReceiveBuffer();
    void finishReceivedFile();
    void failReceive(const QString &message);
    void resetReceiveState();
    void pumpSend();
    void finishSend();
    void failSend(const QString &message);
    void resetSendState();
    QString peerDescription() const;

    Mode mode_ = Mode::Client;
    QString host_ = QStringLiteral("127.0.0.1");
    quint16 port_ = 45454;
    QString saveDirectory_;

    QTcpServer server_;
    QPointer<QTcpSocket> socket_;
    // TCP 没有消息边界，所有到达的数据先累计到缓冲区，再由接收状态机逐段消费。
    QByteArray receiveBuffer_;

    // 接收状态保存于成员变量中，以支持协议头或文件内容跨多次 readyRead 到达。
    bool receivingFile_ = false;
    QString receiveFileName_;
    QString receiveFilePath_;
    quint64 receiveFileSize_ = 0;
    quint64 receiveBytes_ = 0;
    std::unique_ptr<QSaveFile> receiveFile_;

    // 发送状态记录协议头和当前数据块的偏移，处理 QTcpSocket::write 只接收部分数据的情况。
    QFile sendFile_;
    QString sendFilePath_;
    QString sendFileName_;
    QByteArray sendPrefix_;
    qsizetype sendPrefixOffset_ = 0;
    QByteArray pendingSendChunk_;
    qsizetype pendingSendOffset_ = 0;
    qint64 sendFileSize_ = 0;
    qint64 sendPayloadQueued_ = 0;
    bool sendActive_ = false;
};

#endif
