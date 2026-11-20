#include "fingerprint_c.h"
#include "tcp_transfer.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QThread>

#include <iostream>

namespace {

bool writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) &&
           file.write(contents) == contents.size();
}

bool createPackage(const QString &directory, QByteArray &contents,
                   QString &scanPath, QString &manifestPath,
                   QString &payloadPath,
                   int contentSize = 300000)
{
    contents.resize(contentSize);
    for (int index = 0; index < contents.size(); ++index) {
        contents[index] = static_cast<char>((index * 37 + 11) & 0xff);
    }

    char fingerprint[33] = {};
    rt_compute_fingerprint_hex(
        reinterpret_cast<const uint8_t *>(contents.constData()),
        static_cast<size_t>(contents.size()), 42, fingerprint);
    const uint32_t crc32 = rt_compute_crc32(
        reinterpret_cast<const uint8_t *>(contents.constData()),
        static_cast<size_t>(contents.size()));

    scanPath = QDir(directory).filePath("scan.json");
    manifestPath = QDir(directory).filePath("package.json");
    payloadPath = QDir(directory).filePath("package.rtpack");
    const QByteArray scan = R"({"scanVersion":1,"files":[]})";
    QByteArray manifest = R"({
      "selectedStrategy":"adaptive",
      "selectiveCompression":{
        "payloadFile":"package.rtpack",
        "payloadChunks":[{
          "path":"sample.bin","seq":0,"fingerprint":")";
    manifest += fingerprint;
    manifest += R"(","sourceOffset":0,"originalSize":)";
    manifest += QByteArray::number(contents.size());
    manifest += R"(,"payloadOffset":8,"storedSize":)";
    manifest += QByteArray::number(contents.size());
    manifest += R"(,"codec":"raw","crc32":)";
    manifest += QByteArray::number(crc32);
    manifest += R"(}]},
      "files":[{
        "path":"sample.bin","size":)";
    manifest += QByteArray::number(contents.size());
    manifest += R"(,"chunks":[{
          "seq":0,"offset":0,"length":)";
    manifest += QByteArray::number(contents.size());
    manifest += R"(,"fingerprint":")";
    manifest += fingerprint;
    manifest += R"(","crc32":)";
    manifest += QByteArray::number(crc32);
    manifest += R"(,"algorithm":"fixed"}]}]
    })";

    QByteArray payload("RTPK", 4);
    payload.append(char(1));
    payload.append(char(0));
    payload.append(char(0));
    payload.append(char(0));
    payload.append(contents);
    return writeFile(scanPath, scan) &&
           writeFile(manifestPath, manifest) &&
           writeFile(payloadPath, payload);
}

bool createDeltaPackage(const QString &directory, const QByteArray &contents,
                        QString &scanPath, QString &manifestPath,
                        QString &payloadPath)
{
    char fingerprint[33] = {};
    rt_compute_fingerprint_hex(
        reinterpret_cast<const uint8_t *>(contents.constData()),
        static_cast<size_t>(contents.size()), 42, fingerprint);
    const uint32_t crc32 = rt_compute_crc32(
        reinterpret_cast<const uint8_t *>(contents.constData()),
        static_cast<size_t>(contents.size()));

    scanPath = QDir(directory).filePath("delta-scan.json");
    manifestPath = QDir(directory).filePath("delta.json");
    payloadPath = QDir(directory).filePath("delta.rtpack");
    const QByteArray scan = R"({"scanVersion":1,"files":["delta"]})";
    QByteArray manifest = R"({
      "selectedStrategy":"adaptive",
      "selectiveCompression":{"payloadFile":"delta.rtpack","payloadChunks":[]},
      "files":[{
        "path":"sample.bin","size":)";
    manifest += QByteArray::number(contents.size());
    manifest += R"(,"chunks":[{
          "seq":0,"offset":0,"length":)";
    manifest += QByteArray::number(contents.size());
    manifest += R"(,"fingerprint":")";
    manifest += fingerprint;
    manifest += R"(","crc32":)";
    manifest += QByteArray::number(crc32);
    manifest += R"(,"algorithm":"fixed"}]}]
    })";

    QByteArray payload("RTPK", 4);
    payload.append(char(1));
    payload.append(char(0));
    payload.append(char(0));
    payload.append(char(0));
    return writeFile(scanPath, scan) &&
           writeFile(manifestPath, manifest) &&
           writeFile(payloadPath, payload);
}

bool sameFileContents(const QString &leftPath, const QString &rightPath)
{
    QFile left(leftPath);
    QFile right(rightPath);
    return left.open(QIODevice::ReadOnly) &&
           right.open(QIODevice::ReadOnly) &&
           left.readAll() == right.readAll();
}

bool runWorker(TcpTransferWorker &worker)
{
    worker.start();
    QElapsedTimer timer;
    timer.start();
    while (worker.isRunning() && timer.elapsed() < 15000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    if (worker.isRunning()) {
        worker.cancel();
        worker.wait();
        std::cerr << "TCP transfer test timed out.\n";
        return false;
    }
    return worker.wait(1000);
}

bool testTransferAndResume()
{
    QTemporaryDir temporary;
    if (!temporary.isValid()) return false;

    const QString packageDirectory = QDir(temporary.path()).filePath("package");
    const QString incomingDirectory = QDir(temporary.path()).filePath("incoming");
    const QString destinationDirectory = QDir(temporary.path()).filePath("destination");
    if (!QDir().mkpath(packageDirectory) ||
        !QDir().mkpath(destinationDirectory)) {
        return false;
    }

    QByteArray original;
    QString scanPath;
    QString manifestPath;
    QString payloadPath;
    if (!createPackage(packageDirectory, original, scanPath, manifestPath,
                       payloadPath)) {
        return false;
    }

    QTcpServer portProbe;
    if (!portProbe.listen(QHostAddress::LocalHost, 0)) return false;
    const quint16 port = portProbe.serverPort();
    portProbe.close();

    TcpTransferOptions receiverOptions;
    receiverOptions.mode = TcpTransferOptions::Mode::ReceivePackage;
    receiverOptions.port = port;
    receiverOptions.pairingCode = "731942A08F356B42";
    receiverOptions.incomingDirectory = incomingDirectory;
    receiverOptions.destinationDirectory = destinationDirectory;
    TcpTransferWorker receiver(receiverOptions, nullptr, {}, {});
    receiver.start();
    QThread::msleep(100);

    TcpTransferOptions senderOptions;
    senderOptions.mode = TcpTransferOptions::Mode::SendPackage;
    senderOptions.host = "127.0.0.1";
    senderOptions.port = port;
    senderOptions.pairingCode = "0000000000000000";
    senderOptions.scanPath = scanPath;
    senderOptions.manifestPath = manifestPath;
    senderOptions.payloadPath = payloadPath;
    TcpTransferWorker rejectedSender(senderOptions, nullptr, {}, {});
    if (!runWorker(rejectedSender) || rejectedSender.succeeded() ||
        !receiver.isRunning()) {
        receiver.cancel();
        receiver.wait();
        return false;
    }

    senderOptions.pairingCode = receiverOptions.pairingCode;
    TcpTransferWorker sender(senderOptions, nullptr, {}, {});
    if (!runWorker(sender) || !sender.succeeded() ||
        !receiver.wait(15000) || !receiver.succeeded()) {
        receiver.cancel();
        receiver.wait();
        std::cerr << "Initial package transfer failed: "
                  << sender.errorMessage().toStdString() << "\n";
        return false;
    }

    QFile restoredFile(QDir(destinationDirectory).filePath("sample.bin"));
    if (!restoredFile.open(QIODevice::ReadOnly) ||
        restoredFile.readAll() != original) {
        std::cerr << "Reconstructed package contents did not match.\n";
        return false;
    }
    const QStringList receivedDirectories = QDir(
        QDir(incomingDirectory).filePath("incoming"))
            .entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (receivedDirectories.size() != 1) return false;
    const QString receivedPackageDirectory = QDir(
        QDir(incomingDirectory).filePath("incoming"))
            .filePath(receivedDirectories.first());
    if (!sameFileContents(scanPath,
                          QDir(receivedPackageDirectory).filePath("scan.json.part")) ||
        !sameFileContents(manifestPath,
                          QDir(receivedPackageDirectory).filePath("manifest.json.part")) ||
        !sameFileContents(payloadPath,
                          QDir(receivedPackageDirectory).filePath("payload.rtpack.part"))) {
        std::cerr << "Received package artifacts did not match their source files.\n";
        return false;
    }

    const QString resumedDestination =
        QDir(temporary.path()).filePath("resumed-destination");
    if (!QDir().mkpath(resumedDestination)) return false;
    receiverOptions.destinationDirectory = resumedDestination;
    TcpTransferWorker resumedReceiver(receiverOptions, nullptr, {}, {});
    resumedReceiver.start();
    QThread::msleep(100);
    TcpTransferWorker resumedSender(senderOptions, nullptr, {}, {});
    if (!runWorker(resumedSender) || !resumedSender.succeeded() ||
        !resumedReceiver.wait(15000) || !resumedReceiver.succeeded()) {
        resumedReceiver.cancel();
        resumedReceiver.wait();
        std::cerr << "Resumed package transfer failed: "
                  << resumedSender.errorMessage().toStdString() << "\n";
        return false;
    }
    if (!resumedSender.summary().contains("Sent 0 new file bytes")) {
        return false;
    }

    QString deltaScanPath;
    QString deltaManifestPath;
    QString deltaPayloadPath;
    if (!createDeltaPackage(packageDirectory, original,
                            deltaScanPath, deltaManifestPath, deltaPayloadPath)) {
        return false;
    }

    const QString storeDestination =
        QDir(temporary.path()).filePath("store-destination");
    if (!QDir().mkpath(storeDestination)) return false;
    receiverOptions.destinationDirectory = storeDestination;
    TcpTransferWorker storeReceiver(receiverOptions, nullptr, {}, {});
    storeReceiver.start();
    QThread::msleep(100);

    senderOptions.scanPath = deltaScanPath;
    senderOptions.manifestPath = deltaManifestPath;
    senderOptions.payloadPath = deltaPayloadPath;
    TcpTransferWorker deltaSender(senderOptions, nullptr, {}, {});
    if (!runWorker(deltaSender) || !deltaSender.succeeded() ||
        !storeReceiver.wait(15000) || !storeReceiver.succeeded()) {
        storeReceiver.cancel();
        storeReceiver.wait();
        std::cerr << "Persistent chunk-store transfer failed: "
                  << deltaSender.errorMessage().toStdString() << "\n";
        return false;
    }
    QFile storedReconstruction(QDir(storeDestination).filePath("sample.bin"));
    return storeReceiver.summary().contains("1 reused") &&
           storedReconstruction.open(QIODevice::ReadOnly) &&
           storedReconstruction.readAll() == original;
}

bool testInterruptedResume()
{
    QTemporaryDir temporary;
    if (!temporary.isValid()) return false;

    const QString packageDirectory = QDir(temporary.path()).filePath("package");
    const QString incomingDirectory = QDir(temporary.path()).filePath("incoming");
    const QString destinationDirectory = QDir(temporary.path()).filePath("destination");
    if (!QDir().mkpath(packageDirectory) ||
        !QDir().mkpath(destinationDirectory)) {
        return false;
    }

    QByteArray original;
    QString scanPath;
    QString manifestPath;
    QString payloadPath;
    if (!createPackage(packageDirectory, original, scanPath, manifestPath, payloadPath,
                       16 * 1024 * 1024)) {
        return false;
    }

    QTcpServer portProbe;
    if (!portProbe.listen(QHostAddress::LocalHost, 0)) return false;
    const quint16 port = portProbe.serverPort();
    portProbe.close();

    TcpTransferOptions receiverOptions;
    receiverOptions.mode = TcpTransferOptions::Mode::ReceivePackage;
    receiverOptions.port = port;
    receiverOptions.pairingCode = "42816377C12D9B58";
    receiverOptions.incomingDirectory = incomingDirectory;
    receiverOptions.destinationDirectory = destinationDirectory;

    TcpTransferWorker *interruptedReceiverPointer = nullptr;
    TcpTransferWorker interruptedReceiver(
        receiverOptions, QCoreApplication::instance(), {},
        [&interruptedReceiverPointer](quint64 completed, quint64 total) {
            if (interruptedReceiverPointer && completed > 1024 &&
                completed < total) {
                interruptedReceiverPointer->cancel();
            }
        });
    interruptedReceiverPointer = &interruptedReceiver;
    interruptedReceiver.start();
    QThread::msleep(100);

    TcpTransferOptions senderOptions;
    senderOptions.mode = TcpTransferOptions::Mode::SendPackage;
    senderOptions.host = "127.0.0.1";
    senderOptions.port = port;
    senderOptions.pairingCode = receiverOptions.pairingCode;
    senderOptions.scanPath = scanPath;
    senderOptions.manifestPath = manifestPath;
    senderOptions.payloadPath = payloadPath;
    TcpTransferWorker interruptedSender(senderOptions, nullptr, {}, {});
    if (!runWorker(interruptedSender)) {
        interruptedReceiver.cancel();
        interruptedReceiver.wait();
        return false;
    }
    if (interruptedReceiver.isRunning()) interruptedReceiver.wait(5000);
    if (interruptedSender.succeeded() || interruptedReceiver.succeeded()) {
        std::cerr << "Transfer was not interrupted as expected.\n";
        return false;
    }

    const QStringList partialDirectories = QDir(
        QDir(incomingDirectory).filePath("incoming"))
            .entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (partialDirectories.size() != 1) return false;
    const QString partialPayload = QDir(
        QDir(QDir(incomingDirectory).filePath("incoming"))
            .filePath(partialDirectories.first()))
            .filePath("payload.rtpack.part");
    const qint64 partialSize = QFileInfo(partialPayload).size();
    if (partialSize <= 0 ||
        partialSize >= static_cast<qint64>(original.size() + 8)) {
        std::cerr << "Interrupted transfer did not retain a partial payload.\n";
        return false;
    }
    const QString partialScan = QDir(
        QDir(QDir(incomingDirectory).filePath("incoming"))
            .filePath(partialDirectories.first()))
            .filePath("scan.json.part");
    if (!sameFileContents(scanPath, partialScan)) {
        std::cerr << "Interrupted transfer did not retain the scan JSON.\n";
        return false;
    }

    TcpTransferWorker resumedReceiver(receiverOptions, nullptr, {}, {});
    resumedReceiver.start();
    QThread::msleep(100);
    TcpTransferWorker resumedSender(senderOptions, nullptr, {}, {});
    if (!runWorker(resumedSender) || !resumedSender.succeeded() ||
        !resumedReceiver.wait(15000) || !resumedReceiver.succeeded()) {
        resumedReceiver.cancel();
        resumedReceiver.wait();
        std::cerr << "Interrupted transfer did not resume: "
                  << resumedSender.errorMessage().toStdString() << "\n";
        return false;
    }
    QFile restored(QDir(destinationDirectory).filePath("sample.bin"));
    return restored.open(QIODevice::ReadOnly) &&
           restored.readAll() == original &&
           !resumedSender.summary().contains("Sent 0 new file bytes");
}

}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    if (!testTransferAndResume() || !testInterruptedResume()) {
        std::cerr << "TCP transfer integration test failed.\n";
        return 1;
    }
    std::cout << "TCP transfer integration test passed.\n";
    return 0;
}
