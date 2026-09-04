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
    QByteArray receiveBuffer_;

    bool receivingFile_ = false;
    QString receiveFileName_;
    QString receiveFilePath_;
    quint64 receiveFileSize_ = 0;
    quint64 receiveBytes_ = 0;
    std::unique_ptr<QSaveFile> receiveFile_;

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
