
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;
class QTextEdit;
class QProgressBar;
class QProcess;
class QCheckBox;
class QToolButton;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QLineEdit *folderPath;
    QLineEdit *baselinePath;
    QLineEdit *manifestPath;
    QLineEdit *payloadPath;
    QLineEdit *destinationPath;
    QLineEdit *scanOutputName;
    QLineEdit *chunksOutputName;
    QLineEdit *payloadOutputName;
    QComboBox *strategyBox;
    QComboBox *operationBox;
    QCheckBox *useBaselineBox;
    QPushButton *browseButton;
    QPushButton *browseBaselineButton;
    QPushButton *browseManifestButton;
    QPushButton *browsePayloadButton;
    QPushButton *browseDestinationButton;
    QPushButton *startButton;
    QLabel *statusLabel;
    QLabel *baselineLabel;
    QLabel *manifestLabel;
    QLabel *payloadLabel;
    QLabel *destinationLabel;
    QLabel *scanOutputLabel;
    QLabel *chunksOutputLabel;
    QLabel *payloadOutputLabel;
    QLabel *outputDirectoryLabel;
    QTextEdit *outputArea;
    QProgressBar *progressBar;
    QWidget *scanChunkWidget;
    QWidget *payloadOutputWidget;
    QWidget *compressionWidget;
    QWidget *reconstructionWidget;
    QWidget *folderWidget;
    QWidget *sidebarWidget;
    QWidget *sourceCard;
    QWidget *outputFilesCard;
    QWidget *strategyCard;
    QToolButton *settingsButton;

    QProcess *backendProcess;
    QString outputDirectory;

    void browseFolder();
    void browseBaseline();
    void browseManifest();
    void browsePayload();
    void browseDestination();
    void updateOperationFields();
    void showSettingsDialog();
    void appendConsoleOutput(const QString &tag, const QString &text);
    void startTransfer();
};

#endif
