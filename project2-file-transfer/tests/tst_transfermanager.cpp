#include "transfermanager.h"

#include <QDataStream>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

namespace
{
QByteArray makeFrame(const QString &fileName, const QByteArray &data)
{
    const QByteArray nameBytes = fileName.toUtf8();
    QByteArray frame;
    QDataStream stream(&frame, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setVersion(QDataStream::Qt_6_2);
    stream << quint32(0x46544631) << quint8(1) << quint8(1)
           << quint32(nameBytes.size()) << quint64(data.size());
    frame.append(nameBytes);
    frame.append(data);
    return frame;
}
}

class TransferManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void transfersImageAndUtf8NamedTextFile();
    void handlesFragmentedAndCoalescedFrames();
};

void TransferManagerTest::transfersImageAndUtf8NamedTextFile()
{
    QTemporaryDir sourceDirectory;
    QTemporaryDir receiveDirectory;
    QVERIFY(sourceDirectory.isValid());
    QVERIFY(receiveDirectory.isValid());

    TransferManager server;
    TransferManager client;
    server.configure(TransferManager::Mode::Server, QString(), 0, receiveDirectory.path());
    QVERIFY(server.start());
    QVERIFY(server.listeningPort() != 0);

    client.configure(TransferManager::Mode::Client, QStringLiteral("127.0.0.1"),
                     server.listeningPort(), sourceDirectory.path());
    QVERIFY(client.start());
    QTRY_VERIFY_WITH_TIMEOUT(server.isConnected() && client.isConnected(), 5000);

    QImage sourceImage(640, 480, QImage::Format_RGB32);
    quint32 randomState = 0x12345678;
    for (int y = 0; y < sourceImage.height(); ++y)
    {
        for (int x = 0; x < sourceImage.width(); ++x)
        {
            randomState = randomState * 1664525U + 1013904223U;
            sourceImage.setPixel(x, y, qRgb((randomState >> 16) & 0xff,
                                            (randomState >> 8) & 0xff,
                                            randomState & 0xff));
        }
    }

    const QString imagePath = sourceDirectory.filePath(QStringLiteral("测试图片.png"));
    QVERIFY(sourceImage.save(imagePath, "PNG"));
    QFile sourceImageFile(imagePath);
    QVERIFY(sourceImageFile.open(QIODevice::ReadOnly));
    const QByteArray sourceImageBytes = sourceImageFile.readAll();
    QVERIFY(sourceImageBytes.size() > 64 * 1024);

    QSignalSpy receivedSpy(&server, &TransferManager::fileReceived);
    QSignalSpy sentSpy(&client, &TransferManager::fileSent);
    QVERIFY(client.sendFile(imagePath));
    QTRY_COMPARE_WITH_TIMEOUT(receivedSpy.count(), 1, 10000);
    QTRY_COMPARE_WITH_TIMEOUT(sentSpy.count(), 1, 10000);

    const QString receivedImagePath = receiveDirectory.filePath(QStringLiteral("测试图片.png"));
    QFile receivedImageFile(receivedImagePath);
    QVERIFY(receivedImageFile.open(QIODevice::ReadOnly));
    QCOMPARE(receivedImageFile.readAll(), sourceImageBytes);
    const QImage receivedImage(receivedImagePath);
    QVERIFY(!receivedImage.isNull());
    QCOMPARE(receivedImage.size(), sourceImage.size());

    const QByteArray textData = QStringLiteral("第一行：Qt Socket 文件传输\n第二行：中文不乱码。\n")
                                    .toUtf8();
    const QString textPath = sourceDirectory.filePath(QStringLiteral("测试文本.txt"));
    QFile textFile(textPath);
    QVERIFY(textFile.open(QIODevice::WriteOnly));
    QCOMPARE(textFile.write(textData), static_cast<qint64>(textData.size()));
    textFile.close();

    QVERIFY(client.sendFile(textPath));
    QTRY_COMPARE_WITH_TIMEOUT(receivedSpy.count(), 2, 10000);
    QTRY_COMPARE_WITH_TIMEOUT(sentSpy.count(), 2, 10000);

    QFile receivedText(receiveDirectory.filePath(QStringLiteral("测试文本.txt")));
    QVERIFY(receivedText.open(QIODevice::ReadOnly));
    QCOMPARE(receivedText.readAll(), textData);
}

void TransferManagerTest::handlesFragmentedAndCoalescedFrames()
{
    QTemporaryDir receiveDirectory;
    QVERIFY(receiveDirectory.isValid());

    TransferManager server;
    server.configure(TransferManager::Mode::Server, QString(), 0, receiveDirectory.path());
    QVERIFY(server.start());

    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, server.listeningPort());
    QVERIFY(socket.waitForConnected(3000));
    QTRY_VERIFY_WITH_TIMEOUT(server.isConnected(), 3000);

    const QByteArray firstData("header split across packets\n");
    const QByteArray secondData = QByteArray::fromHex("89504e470d0a1a0a000102030405");
    const QByteArray firstFrame = makeFrame(QStringLiteral("first.txt"), firstData);
    const QByteArray secondFrame = makeFrame(QStringLiteral("second.png"), secondData);

    QSignalSpy receivedSpy(&server, &TransferManager::fileReceived);
    QCOMPARE(socket.write(firstFrame.left(3)), qint64(3));
    QVERIFY(socket.waitForBytesWritten(3000));
    QTest::qWait(20);
    const QByteArray coalescedTail = firstFrame.mid(3) + secondFrame;
    QCOMPARE(socket.write(coalescedTail), static_cast<qint64>(coalescedTail.size()));
    QVERIFY(socket.waitForBytesWritten(3000));

    QTRY_COMPARE_WITH_TIMEOUT(receivedSpy.count(), 2, 5000);

    QFile firstFile(receiveDirectory.filePath(QStringLiteral("first.txt")));
    QFile secondFile(receiveDirectory.filePath(QStringLiteral("second.png")));
    QVERIFY(firstFile.open(QIODevice::ReadOnly));
    QVERIFY(secondFile.open(QIODevice::ReadOnly));
    QCOMPARE(firstFile.readAll(), firstData);
    QCOMPARE(secondFile.readAll(), secondData);
}

QTEST_MAIN(TransferManagerTest)
#include "tst_transfermanager.moc"
