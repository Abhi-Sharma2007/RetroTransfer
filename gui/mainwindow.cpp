
#include "mainwindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QFileDialog>
#include <QProcess>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QTextCursor>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("RetroTransfer - Local File Transfer System");
    resize(950, 650);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(30, 25, 30, 25);
    mainLayout->setSpacing(18);

    // Header
    QLabel *title = new QLabel("RETRO TRANSFER");
    title->setStyleSheet(
        "font-size: 28px;"
        "font-weight: bold;"
        "color:  #F8FAFC;"
    );

    QLabel *subtitle = new QLabel(
        "LOCAL FILE TRANSFER SYSTEM  |  VERSION 1.0"
    );

    subtitle->setStyleSheet(
        "font-size: 12px;"
        "color: #94A3B8;"
    );

    mainLayout->addWidget(title);
    mainLayout->addWidget(subtitle);

    // Folder selection
    QLabel *folderLabel = new QLabel("Select Folder");

    folderPath = new QLineEdit();
    folderPath->setPlaceholderText("Choose a folder to process...");
    folderPath->setReadOnly(true);

    browseButton = new QPushButton("Browse Folder");

    QHBoxLayout *folderLayout = new QHBoxLayout();
    folderLayout->addWidget(folderPath);
    folderLayout->addWidget(browseButton);

    mainLayout->addWidget(folderLabel);
    mainLayout->addLayout(folderLayout);

    // Chunking strategy
    QLabel *strategyLabel = new QLabel("Chunking Strategy");

    strategyBox = new QComboBox();
    strategyBox->addItem("Adaptive");
    strategyBox->addItem("Fixed");
    strategyBox->addItem("CDC");

    mainLayout->addWidget(strategyLabel);
    mainLayout->addWidget(strategyBox);

    // Start button
    startButton = new QPushButton("START TRANSFER");
    startButton->setMinimumHeight(42);

    startButton->setStyleSheet(
        "QPushButton {"
        "background-color: #2563EB;"
        "color: white;"
        "font-weight: bold;"
        "border-radius: 6px;"
        "padding: 8px;"
        "}"
        "QPushButton:hover {"
        "background-color: #1D4ED8;"
        "}"
    );

    mainLayout->addWidget(startButton);

    // Status
    statusLabel = new QLabel("Status: Ready");
    statusLabel->setStyleSheet(
        "font-weight: bold;"
        "color: #15803D;"
    );

    mainLayout->addWidget(statusLabel);

    // Progress bar
    progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);

    mainLayout->addWidget(progressBar);

    // Processing output
    QLabel *outputLabel = new QLabel("Processing Output");

    outputArea = new QTextEdit();
    outputArea->setReadOnly(true);
    outputArea->setPlaceholderText(
        "Backend processing results will appear here."
    );

    mainLayout->addWidget(outputLabel);
    mainLayout->addWidget(outputArea);

   
// Dark Theme Styling
centralWidget->setStyleSheet(
    "QWidget {"
    "background-color: #0D1117;"
    "color: #F8FAFC;"
    "font-family: Segoe UI;"
    "font-size: 14px;"
    "}"

    "QLabel {"
    "color: #E2E8F0;"
    "}"

    "QLineEdit, QComboBox {"
    "background-color: #161B22;"
    "color: #F8FAFC;"
    "border: 1px solid #30363D;"
    "border-radius: 6px;"
    "padding: 9px;"
    "}"

    "QComboBox QAbstractItemView {"
    "background-color: #161B22;"
    "color: #F8FAFC;"
    "selection-background-color: #2563EB;"
    "}"

    "QTextEdit {"
    "background-color: #080B10;"
    "color: #C9D1D9;"
    "border: 1px solid #30363D;"
    "border-radius: 7px;"
    "padding: 10px;"
    "}"

    "QPushButton {"
    "background-color: #21262D;"
    "color: #F8FAFC;"
    "border: 1px solid #30363D;"
    "border-radius: 6px;"
    "padding: 9px;"
    "}"

    "QPushButton:hover {"
    "background-color: #30363D;"
    "}"

    "QProgressBar {"
    "background-color: #161B22;"
    "color: white;"
    "border: 1px solid #30363D;"
    "border-radius: 5px;"
    "text-align: center;"
    "}"

    "QProgressBar::chunk {"
    "background-color: #2563EB;"
    "border-radius: 5px;"
    "}"
);


    // Create backend process
    backendProcess = new QProcess(this);

    // Keep backend output files in the project root.
    QString projectDirectory =
        QDir(QCoreApplication::applicationDirPath())
            .absoluteFilePath("..");

    backendProcess->setWorkingDirectory(projectDirectory);

#ifdef Q_OS_WIN
    // Prevent a separate console window from appearing.
    backendProcess->setCreateProcessArgumentsModifier(
        [](QProcess::CreateProcessArguments *arguments) {
            arguments->flags |= CREATE_NO_WINDOW;
        }
    );
#endif

    // Display backend standard output.
    connect(backendProcess,
            &QProcess::readyReadStandardOutput,
            this,
            [this]() {
                QString output = QString::fromLocal8Bit(
                    backendProcess->readAllStandardOutput()
                );

                outputArea->moveCursor(QTextCursor::End);
                outputArea->insertPlainText(output);
            });

    // Display backend errors.
    connect(backendProcess,
            &QProcess::readyReadStandardError,
            this,
            [this]() {
                QString output = QString::fromLocal8Bit(
                    backendProcess->readAllStandardError()
                );

                outputArea->moveCursor(QTextCursor::End);
                outputArea->insertPlainText(output);
            });

    // Handle completion.
    connect(backendProcess,
            &QProcess::finished,
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {

                startButton->setEnabled(true);
                browseButton->setEnabled(true);
                strategyBox->setEnabled(true);

                progressBar->setRange(0, 100);

                if (exitStatus == QProcess::NormalExit &&
                    exitCode == 0) {

                    progressBar->setValue(100);

                    statusLabel->setText(
                        "Status: Transfer completed successfully!"
                    );

                    statusLabel->setStyleSheet(
                        "color: #15803D; font-weight: bold;"
                    );

                } else {

                    statusLabel->setText(
                        "Status: Transfer failed. Check processing output."
                    );

                    statusLabel->setStyleSheet(
                        "color: #DC2626; font-weight: bold;"
                    );
                }
            });

    // Handle failure to start the backend executable.
    connect(backendProcess,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError error) {

                if (error == QProcess::FailedToStart) {

                    outputArea->append(
                        "\nERROR: Could not start RetroTransfer.exe."
                    );

                    outputArea->append(
                        backendProcess->errorString()
                    );

                    startButton->setEnabled(true);
                    browseButton->setEnabled(true);
                    strategyBox->setEnabled(true);

                    progressBar->setRange(0, 100);
                    progressBar->setValue(0);

                    statusLabel->setText(
                        "Status: Backend could not start."
                    );

                    statusLabel->setStyleSheet(
                        "color: #DC2626; font-weight: bold;"
                    );
                }
            });

    // Button connections
    connect(browseButton, &QPushButton::clicked,
            this, &MainWindow::browseFolder);

    connect(startButton, &QPushButton::clicked,
            this, &MainWindow::startTransfer);
}

void MainWindow::browseFolder()
{
    QString selectedFolder = QFileDialog::getExistingDirectory(
        this,
        "Select Folder",
        ""
    );

    if (!selectedFolder.isEmpty()) {
        folderPath->setText(selectedFolder);
    }
}

void MainWindow::startTransfer()
{
    if (folderPath->text().isEmpty()) {

        statusLabel->setText(
            "Status: Please select a folder first."
        );

        statusLabel->setStyleSheet(
            "color: #DC2626; font-weight: bold;"
        );

        return;
    }

    if (backendProcess->state() != QProcess::NotRunning) {
        return;
    }

    QString backendPath =
        QDir(QCoreApplication::applicationDirPath())
            .filePath("RetroTransfer.exe");

    if (!QFileInfo::exists(backendPath)) {

        outputArea->append(
            "ERROR: RetroTransfer.exe was not found beside the GUI."
        );

        return;
    }

    QString strategy = strategyBox->currentText().toLower();

    QStringList arguments;
    arguments << folderPath->text();
    arguments << "--strategy";
    arguments << strategy;

    outputArea->clear();

    outputArea->append("Starting RetroTransfer backend...");
    outputArea->append("Selected folder: " + folderPath->text());
    outputArea->append("Selected strategy: " + strategy);
    outputArea->append("------------------------------------");

    statusLabel->setText("Status: Processing...");
    statusLabel->setStyleSheet(
        "color: #2563EB; font-weight: bold;"
    );

    progressBar->setRange(0, 0);

    startButton->setEnabled(false);
    browseButton->setEnabled(false);
    strategyBox->setEnabled(false);

    backendProcess->start(backendPath, arguments);
}
