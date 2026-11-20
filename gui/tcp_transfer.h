#ifndef RETROTRANSFER_TCP_TRANSFER_H
#define RETROTRANSFER_TCP_TRANSFER_H

#include <QThread>
#include <QString>
#include <QPointer>

#include <atomic>
#include <functional>

class QObject;

struct TcpTransferOptions {
    enum class Mode {
        SendPackage,
        ReceivePackage
    };

    Mode mode = Mode::SendPackage;
    QString host;
    quint16 port = 0;
    QString pairingCode;
    QString scanPath;
    QString manifestPath;
    QString payloadPath;
    QString incomingDirectory;
    QString destinationDirectory;
};

class TcpTransferWorker : public QThread
{
public:
    using LogCallback = std::function<void(const QString &)>;
    using ProgressCallback = std::function<void(quint64, quint64)>;

    TcpTransferWorker(const TcpTransferOptions &options,
                      QObject *callbackContext,
                      LogCallback logCallback,
                      ProgressCallback progressCallback,
                      QObject *parent = nullptr);
    ~TcpTransferWorker() override;

    void cancel();
    bool succeeded() const;
    QString errorMessage() const;
    QString summary() const;

protected:
    void run() override;

private:
    void reportLog(const QString &message);

    TcpTransferOptions options_;
    QPointer<QObject> callbackContext_;
    LogCallback logCallback_;
    ProgressCallback progressCallback_;
    std::atomic_bool cancelled_{false};
    bool succeeded_ = false;
    QString errorMessage_;
    QString summary_;
};

#endif
