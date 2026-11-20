
#include "mainwindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
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
#include <QTextCharFormat>
#include <QCheckBox>
#include <QScrollArea>
#include <QFrame>
#include <QToolButton>
#include <QButtonGroup>
#include <QStyle>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QDialog>
#include <QDialogButtonBox>
#include <QRegularExpression>
#include <QFont>
#include <QColor>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

enum class LineIcon {
    Scan,
    Compress,
    Reconstruct,
    Folder,
    OpenFolder,
    File,
    Settings,
    Play,
    Menu
};

QIcon lineIcon(LineIcon icon, const QColor &color = QColor("#94A3B8"))
{
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(color, 3.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    QPainterPath path;
    switch (icon) {
    case LineIcon::Scan:
        path.moveTo(11, 18);
        path.lineTo(33, 18);
        path.lineTo(33, 41);
        path.lineTo(11, 41);
        path.closeSubpath();
        painter.drawPath(path);
        painter.drawLine(16, 25, 28, 25);
        painter.drawLine(16, 31, 26, 31);
        painter.drawEllipse(QRectF(33, 31, 18, 18));
        painter.drawLine(46, 45, 54, 53);
        break;
    case LineIcon::Compress:
        painter.drawRoundedRect(QRectF(12, 13, 40, 38), 5, 5);
        painter.drawLine(21, 22, 43, 22);
        painter.drawLine(21, 29, 43, 29);
        painter.drawLine(32, 34, 32, 45);
        painter.drawLine(26, 40, 32, 46);
        painter.drawLine(38, 40, 32, 46);
        break;
    case LineIcon::Reconstruct:
        path.moveTo(47, 25);
        path.cubicTo(42, 15, 27, 13, 19, 21);
        path.cubicTo(11, 29, 13, 41, 20, 46);
        painter.drawPath(path);
        painter.drawLine(42, 16, 48, 25);
        painter.drawLine(37, 26, 48, 25);
        path = QPainterPath();
        path.moveTo(18, 39);
        path.cubicTo(23, 49, 38, 51, 46, 43);
        path.cubicTo(54, 35, 52, 23, 45, 18);
        painter.drawPath(path);
        painter.drawLine(23, 48, 17, 39);
        painter.drawLine(28, 38, 17, 39);
        break;
    case LineIcon::Folder:
        path.moveTo(10, 19);
        path.lineTo(27, 19);
        path.lineTo(33, 25);
        path.lineTo(52, 25);
        path.lineTo(52, 46);
        path.lineTo(10, 46);
        path.closeSubpath();
        painter.drawPath(path);
        painter.drawLine(11, 25, 11, 19);
        break;
    case LineIcon::OpenFolder:
        path.moveTo(9, 22);
        path.lineTo(25, 22);
        path.lineTo(31, 28);
        path.lineTo(52, 28);
        path.lineTo(47, 46);
        path.lineTo(12, 46);
        path.closeSubpath();
        painter.drawPath(path);
        painter.drawLine(9, 22, 9, 18);
        painter.drawLine(9, 18, 26, 18);
        painter.drawLine(26, 18, 31, 23);
        break;
    case LineIcon::File:
        path.moveTo(18, 10);
        path.lineTo(37, 10);
        path.lineTo(48, 21);
        path.lineTo(48, 53);
        path.lineTo(18, 53);
        path.closeSubpath();
        painter.drawPath(path);
        painter.drawLine(37, 10, 37, 21);
        painter.drawLine(37, 21, 48, 21);
        painter.drawLine(24, 33, 42, 33);
        painter.drawLine(24, 40, 42, 40);
        break;
    case LineIcon::Settings:
        painter.drawEllipse(QRectF(23, 23, 18, 18));
        for (int angle = 0; angle < 360; angle += 45) {
            painter.save();
            painter.translate(32, 32);
            painter.rotate(angle);
            painter.drawLine(0, -22, 0, -27);
            painter.restore();
        }
        painter.drawEllipse(QRectF(10, 10, 44, 44));
        break;
    case LineIcon::Play:
        path.moveTo(23, 15);
        path.lineTo(48, 32);
        path.lineTo(23, 49);
        path.closeSubpath();
        painter.drawPath(path);
        break;
    case LineIcon::Menu:
        painter.drawLine(17, 21, 47, 21);
        painter.drawLine(17, 32, 47, 32);
        painter.drawLine(17, 43, 47, 43);
        break;
    }

    painter.end();
    return QIcon(pixmap);
}

QFrame *createCard(const QString &title, QVBoxLayout *&layout,
                   QWidget *parent = nullptr)
{
    QFrame *card = new QFrame(parent);
    card->setObjectName("card");
    layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 18);
    layout->setSpacing(12);

    if (!title.trimmed().isEmpty()) {
        QLabel *heading = new QLabel(title, card);
        heading->setObjectName("cardTitle");
        layout->addWidget(heading);
    }
    return card;
}

QToolButton *createNavButton(QWidget *parent, LineIcon icon,
                             const QString &tooltip)
{
    QToolButton *button = new QToolButton(parent);
    button->setObjectName("navButton");
    button->setIcon(lineIcon(icon));
    button->setIconSize(QSize(21, 21));
    button->setFixedSize(44, 44);
    button->setToolTip(tooltip);
    button->setCheckable(true);
    button->setAutoRaise(true);
    return button;
}

void addFieldIcon(QLineEdit *field, LineIcon icon)
{
    field->addAction(lineIcon(icon), QLineEdit::LeadingPosition);
}

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("RetroTransfer - Local File Transfer System");
    resize(1280, 820);
    setMinimumSize(1040, 700);

    outputDirectory = QDir(QCoreApplication::applicationDirPath())
                          .absoluteFilePath("..");

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *dashboard = new QWidget();
    dashboard->setObjectName("dashboard");
    scrollArea->setWidget(dashboard);
    setCentralWidget(scrollArea);

    QHBoxLayout *dashboardLayout = new QHBoxLayout(dashboard);
    dashboardLayout->setContentsMargins(0, 0, 0, 0);
    dashboardLayout->setSpacing(0);

    sidebarWidget = new QFrame();
    sidebarWidget->setObjectName("sidebar");
    sidebarWidget->setFixedWidth(68);
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setContentsMargins(12, 18, 12, 18);
    sidebarLayout->setSpacing(14);

    QLabel *brandMark = new QLabel();
    brandMark->setObjectName("brandMark");
    brandMark->setAlignment(Qt::AlignCenter);
    brandMark->setPixmap(lineIcon(LineIcon::Menu, QColor("#4ADE80")).pixmap(24, 24));
    brandMark->setFixedSize(42, 42);
    sidebarLayout->addWidget(brandMark, 0, Qt::AlignHCenter);
    sidebarLayout->addSpacing(14);

    QToolButton *scanNav = createNavButton(
        sidebarWidget, LineIcon::Scan, "Scan and chunk");
    QToolButton *compressNav = createNavButton(
        sidebarWidget, LineIcon::Compress, "Compress");
    QToolButton *reconstructNav = createNavButton(
        sidebarWidget, LineIcon::Reconstruct, "Reconstruct");
    QButtonGroup *navigationGroup = new QButtonGroup(this);
    navigationGroup->setExclusive(true);
    navigationGroup->addButton(scanNav, 0);
    navigationGroup->addButton(compressNav, 1);
    navigationGroup->addButton(reconstructNav, 2);
    sidebarLayout->addWidget(scanNav);
    sidebarLayout->addWidget(compressNav);
    sidebarLayout->addWidget(reconstructNav);
    sidebarLayout->addStretch();
    dashboardLayout->addWidget(sidebarWidget);

    QWidget *mainPanel = new QWidget();
    mainPanel->setObjectName("mainPanel");
    QVBoxLayout *mainPanelLayout = new QVBoxLayout(mainPanel);
    mainPanelLayout->setContentsMargins(26, 22, 26, 22);
    mainPanelLayout->setSpacing(18);
    dashboardLayout->addWidget(mainPanel, 1);

    QWidget *header = new QWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout *headingLayout = new QVBoxLayout();
    headingLayout->setSpacing(3);
    QLabel *title = new QLabel("RetroTransfer");
    title->setObjectName("pageTitle");
    QLabel *subtitle = new QLabel("Project packaging and reconstruction");
    subtitle->setObjectName("muted");
    headingLayout->addWidget(title);
    headingLayout->addWidget(subtitle);
    headerLayout->addLayout(headingLayout);
    headerLayout->addStretch();

    settingsButton = new QToolButton();
    settingsButton->setObjectName("settingsButton");
    settingsButton->setText("Settings");
    settingsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    settingsButton->setIcon(lineIcon(LineIcon::Settings));
    settingsButton->setIconSize(QSize(18, 18));
    settingsButton->setCursor(Qt::PointingHandCursor);
    headerLayout->addWidget(settingsButton);
    mainPanelLayout->addWidget(header);

    QWidget *columns = new QWidget();
    QGridLayout *columnsLayout = new QGridLayout(columns);
    columnsLayout->setContentsMargins(0, 0, 0, 0);
    columnsLayout->setSpacing(18);
    mainPanelLayout->addWidget(columns, 1);

    QWidget *leftColumn = new QWidget();
    leftColumn->setMinimumWidth(410);
    leftColumn->setMaximumWidth(510);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(14);
    columnsLayout->addWidget(leftColumn, 0, 0);

    QWidget *rightColumn = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightColumn);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(14);
    columnsLayout->addWidget(rightColumn, 0, 1);
    columnsLayout->setColumnStretch(0, 5);
    columnsLayout->setColumnStretch(1, 6);

    QVBoxLayout *operationCardLayout = nullptr;
    QFrame *operationCard = createCard("Operation", operationCardLayout);
    operationBox = new QComboBox();
    operationBox->addItem(lineIcon(LineIcon::Scan), "Scan and Chunk");
    operationBox->addItem(lineIcon(LineIcon::Compress), "Compress");
    operationBox->addItem(lineIcon(LineIcon::Reconstruct), "Reconstruct");
    operationCardLayout->addWidget(operationBox);
    leftLayout->addWidget(operationCard);

    QVBoxLayout *sourceLayout = nullptr;
    sourceCard = createCard("Project and inputs", sourceLayout);

    folderWidget = new QWidget();
    QVBoxLayout *folderLayout = new QVBoxLayout(folderWidget);
    folderLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *folderLabel = new QLabel("Project folder");
    folderLabel->setObjectName("fieldLabel");
    folderPath = new QLineEdit();
    folderPath->setPlaceholderText("Select the project to process");
    folderPath->setReadOnly(true);
    addFieldIcon(folderPath, LineIcon::Folder);
    browseButton = new QPushButton("Browse");
    browseButton->setIcon(lineIcon(LineIcon::OpenFolder));
    QHBoxLayout *folderRow = new QHBoxLayout();
    folderRow->setSpacing(8);
    folderRow->addWidget(folderPath, 1);
    folderRow->addWidget(browseButton);
    folderLayout->addWidget(folderLabel);
    folderLayout->addLayout(folderRow);
    sourceLayout->addWidget(folderWidget);

    compressionWidget = new QWidget();
    QVBoxLayout *compressionLayout = new QVBoxLayout(compressionWidget);
    compressionLayout->setContentsMargins(0, 8, 0, 0);
    useBaselineBox = new QCheckBox("Compare against an older project");
    compressionLayout->addWidget(useBaselineBox);
    baselineLabel = new QLabel("Older project folder");
    baselineLabel->setObjectName("fieldLabel");
    baselinePath = new QLineEdit();
    baselinePath->setPlaceholderText("Select the baseline project");
    baselinePath->setReadOnly(true);
    addFieldIcon(baselinePath, LineIcon::Folder);
    browseBaselineButton = new QPushButton("Browse");
    browseBaselineButton->setIcon(lineIcon(LineIcon::OpenFolder));
    QHBoxLayout *baselineRow = new QHBoxLayout();
    baselineRow->setSpacing(8);
    baselineRow->addWidget(baselinePath, 1);
    baselineRow->addWidget(browseBaselineButton);
    compressionLayout->addWidget(baselineLabel);
    compressionLayout->addLayout(baselineRow);
    sourceLayout->addWidget(compressionWidget);

    reconstructionWidget = new QWidget();
    QVBoxLayout *reconstructionLayout = new QVBoxLayout(reconstructionWidget);
    reconstructionLayout->setContentsMargins(0, 8, 0, 0);
    manifestLabel = new QLabel("Chunk manifest");
    manifestLabel->setObjectName("fieldLabel");
    manifestPath = new QLineEdit();
    manifestPath->setPlaceholderText("Select a chunks JSON manifest");
    manifestPath->setReadOnly(true);
    addFieldIcon(manifestPath, LineIcon::File);
    browseManifestButton = new QPushButton("Browse");
    browseManifestButton->setIcon(lineIcon(LineIcon::File));
    QHBoxLayout *manifestRow = new QHBoxLayout();
    manifestRow->setSpacing(8);
    manifestRow->addWidget(manifestPath, 1);
    manifestRow->addWidget(browseManifestButton);
    reconstructionLayout->addWidget(manifestLabel);
    reconstructionLayout->addLayout(manifestRow);

    payloadLabel = new QLabel("Payload (optional)");
    payloadLabel->setObjectName("fieldLabel");
    payloadPath = new QLineEdit();
    payloadPath->setPlaceholderText("Uses the manifest's payload filename if blank");
    payloadPath->setReadOnly(true);
    addFieldIcon(payloadPath, LineIcon::File);
    browsePayloadButton = new QPushButton("Browse");
    browsePayloadButton->setIcon(lineIcon(LineIcon::File));
    QHBoxLayout *payloadRow = new QHBoxLayout();
    payloadRow->setSpacing(8);
    payloadRow->addWidget(payloadPath, 1);
    payloadRow->addWidget(browsePayloadButton);
    reconstructionLayout->addWidget(payloadLabel);
    reconstructionLayout->addLayout(payloadRow);

    destinationLabel = new QLabel("Destination folder");
    destinationLabel->setObjectName("fieldLabel");
    destinationPath = new QLineEdit();
    destinationPath->setPlaceholderText("Select the output project folder");
    destinationPath->setReadOnly(true);
    addFieldIcon(destinationPath, LineIcon::Folder);
    browseDestinationButton = new QPushButton("Browse");
    browseDestinationButton->setIcon(lineIcon(LineIcon::OpenFolder));
    QHBoxLayout *destinationRow = new QHBoxLayout();
    destinationRow->setSpacing(8);
    destinationRow->addWidget(destinationPath, 1);
    destinationRow->addWidget(browseDestinationButton);
    reconstructionLayout->addWidget(destinationLabel);
    reconstructionLayout->addLayout(destinationRow);
    sourceLayout->addWidget(reconstructionWidget);
    leftLayout->addWidget(sourceCard);

    QVBoxLayout *strategyCardLayout = nullptr;
    strategyCard = createCard("Chunking strategy", strategyCardLayout);
    strategyBox = new QComboBox();
    strategyBox->addItem("Adaptive");
    strategyBox->addItem("Fixed");
    strategyBox->addItem("CDC");
    strategyCardLayout->addWidget(strategyBox);
    leftLayout->addWidget(strategyCard);

    QVBoxLayout *outputFilesLayout = nullptr;
    outputFilesCard = createCard("File outputs", outputFilesLayout);
    scanChunkWidget = new QWidget();
    QVBoxLayout *scanChunkLayout = new QVBoxLayout(scanChunkWidget);
    scanChunkLayout->setContentsMargins(0, 0, 0, 0);
    scanChunkLayout->setSpacing(8);
    scanOutputLabel = new QLabel("Scan JSON");
    scanOutputLabel->setObjectName("fieldLabel");
    scanOutputName = new QLineEdit("scan.json");
    addFieldIcon(scanOutputName, LineIcon::File);
    chunksOutputLabel = new QLabel("Chunk manifest JSON");
    chunksOutputLabel->setObjectName("fieldLabel");
    chunksOutputName = new QLineEdit("scan_chunks.json");
    addFieldIcon(chunksOutputName, LineIcon::File);
    scanChunkLayout->addWidget(scanOutputLabel);
    scanChunkLayout->addWidget(scanOutputName);
    scanChunkLayout->addWidget(chunksOutputLabel);
    scanChunkLayout->addWidget(chunksOutputName);
    outputFilesLayout->addWidget(scanChunkWidget);

    payloadOutputWidget = new QWidget();
    QVBoxLayout *payloadOutputLayout = new QVBoxLayout(payloadOutputWidget);
    payloadOutputLayout->setContentsMargins(0, 8, 0, 0);
    payloadOutputLabel = new QLabel("Compressed payload");
    payloadOutputLabel->setObjectName("fieldLabel");
    payloadOutputName = new QLineEdit("missing-chunks.rtpack");
    addFieldIcon(payloadOutputName, LineIcon::File);
    payloadOutputLayout->addWidget(payloadOutputLabel);
    payloadOutputLayout->addWidget(payloadOutputName);
    outputFilesLayout->addWidget(payloadOutputWidget);

    outputDirectoryLabel = new QLabel();
    outputDirectoryLabel->setObjectName("muted");
    outputDirectoryLabel->setWordWrap(true);
    outputFilesLayout->addWidget(outputDirectoryLabel);
    leftLayout->addWidget(outputFilesCard);
    leftLayout->addStretch();

    QVBoxLayout *actionCardLayout = nullptr;
    QFrame *actionCard = createCard("Actions and progress", actionCardLayout);
    startButton = new QPushButton("START TRANSFER");
    startButton->setObjectName("primaryButton");
    startButton->setIcon(lineIcon(LineIcon::Play, QColor("#FFFFFF")));
    startButton->setMinimumHeight(48);
    startButton->setCursor(Qt::PointingHandCursor);
    actionCardLayout->addWidget(startButton);
    statusLabel = new QLabel("[READY] Ready");
    statusLabel->setObjectName("statusLabel");
    actionCardLayout->addWidget(statusLabel);
    progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setFormat("Ready");
    actionCardLayout->addWidget(progressBar);
    rightLayout->addWidget(actionCard);

    QVBoxLayout *consoleCardLayout = nullptr;
    QFrame *consoleCard = createCard("Processing output", consoleCardLayout);
    outputArea = new QTextEdit();
    outputArea->setReadOnly(true);
    outputArea->setLineWrapMode(QTextEdit::NoWrap);
    outputArea->setMinimumHeight(360);
    outputArea->setPlaceholderText("Backend messages will appear here.");
    QFont consoleFont("Consolas");
    consoleFont.setStyleHint(QFont::Monospace);
    outputArea->setFont(consoleFont);
    outputArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    outputArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    consoleCardLayout->addWidget(outputArea, 1);
    rightLayout->addWidget(consoleCard, 1);

    dashboard->setStyleSheet(
        "QWidget#dashboard, QWidget#mainPanel {"
        "background-color: #0B1220; color: #E5E7EB;"
        "font-family: Segoe UI; font-size: 13px;"
        "}"
        "QFrame#sidebar {"
        "background-color: #080D16; border-right: 1px solid #1F2A3A;"
        "}"
        "QLabel#brandMark {"
        "background-color: #12372A; color: #4ADE80; border-radius: 12px;"
        "font-size: 15px; font-weight: 700;"
        "}"
        "QLabel#pageTitle {font-size: 24px; font-weight: 700; color: #F8FAFC;}"
        "QLabel#muted {color: #94A3B8;}"
        "QFrame#card {"
        "background-color: #111A29; border: 1px solid #253247; border-radius: 14px;"
        "}"
        "QLabel#cardTitle {font-size: 15px; font-weight: 600; color: #F1F5F9;}"
        "QLabel#fieldLabel {font-size: 12px; color: #A7B4C7;}"
        "QLineEdit, QComboBox {"
        "background-color: #0B1220; color: #E5E7EB; border: 1px solid #2B3A50;"
        "border-radius: 8px; padding: 8px 10px; min-height: 20px;"
        "}"
        "QLineEdit:focus, QComboBox:focus {border: 1px solid #38BDF8;}"
        "QComboBox QAbstractItemView {"
        "background-color: #111A29; color: #E5E7EB; selection-background-color: #1D4ED8;"
        "}"
        "QPushButton, QToolButton#settingsButton {"
        "background-color: #182336; color: #E5E7EB; border: 1px solid #2B3A50;"
        "border-radius: 8px; padding: 8px 10px;"
        "}"
        "QPushButton:hover, QToolButton#settingsButton:hover {"
        "background-color: #22314A;"
        "}"
        "QToolButton#navButton {background: transparent; border: none; border-radius: 10px;}"
        "QToolButton#navButton:hover {background-color: #172235;}"
        "QToolButton#navButton:checked {background-color: #12372A;}"
        "QPushButton#primaryButton {"
        "background-color: #15803D; color: white; border: none; border-radius: 9px;"
        "font-size: 14px; font-weight: 700; letter-spacing: 1px;"
        "}"
        "QPushButton#primaryButton:hover {background-color: #16A34A;}"
        "QPushButton#primaryButton:disabled {background-color: #334155; color: #94A3B8;}"
        "QLabel#statusLabel {font-size: 13px; font-weight: 600; color: #CBD5E1;}"
        "QProgressBar {"
        "background-color: #0B1220; color: white; border: 1px solid #253247;"
        "border-radius: 7px; text-align: center; min-height: 18px;"
        "}"
        "QProgressBar::chunk {background-color: #22C55E; border-radius: 6px;}"
        "QTextEdit {"
        "background-color: #070B12; color: #C9D1D9; border: 1px solid #30363D;"
        "border-radius: 9px; padding: 10px;"
        "}"
        "QCheckBox {color: #CBD5E1; spacing: 8px;}"
        "QScrollArea {border: none; background-color: #0B1220;}"
    );

    outputDirectoryLabel->setText(
        "Generated files: " + QDir(outputDirectory).filePath("collection"));

    navigationGroup->button(0)->setChecked(true);
    connect(navigationGroup, &QButtonGroup::idClicked,
            this, [this](int index) {
                operationBox->setCurrentIndex(index);
            });
    connect(operationBox, &QComboBox::currentIndexChanged,
            this, [navigationGroup](int index) {
                if (QAbstractButton *button = navigationGroup->button(index)) {
                    button->setChecked(true);
                }
            });
    connect(settingsButton, &QToolButton::clicked,
            this, &MainWindow::showSettingsDialog);
    connect(useBaselineBox, &QCheckBox::toggled,
            this, [this](bool checked) {
                baselineLabel->setVisible(checked);
                baselinePath->setVisible(checked);
                browseBaselineButton->setVisible(checked);
            });
    connect(operationBox, &QComboBox::currentTextChanged,
            this, &MainWindow::updateOperationFields);

    // Create backend process
    backendProcess = new QProcess(this);
    backendProcess->setWorkingDirectory(outputDirectory);

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
                appendConsoleOutput("INFO", output);
            });

    // Display backend errors.
    connect(backendProcess,
            &QProcess::readyReadStandardError,
            this,
            [this]() {
                QString output = QString::fromLocal8Bit(
                    backendProcess->readAllStandardError()
                );
                appendConsoleOutput("ERROR", output);
            });

    // Handle completion.
    connect(backendProcess,
            &QProcess::finished,
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {

                startButton->setEnabled(true);
                browseButton->setEnabled(true);
                strategyBox->setEnabled(true);
                browseBaselineButton->setEnabled(true);
                browseManifestButton->setEnabled(true);
                browsePayloadButton->setEnabled(true);
                browseDestinationButton->setEnabled(true);
                useBaselineBox->setEnabled(true);
                operationBox->setEnabled(true);
                sidebarWidget->setEnabled(true);
                settingsButton->setEnabled(true);
                updateOperationFields();

                progressBar->setRange(0, 100);

                if (exitStatus == QProcess::NormalExit &&
                    exitCode == 0) {
                    progressBar->setValue(100);
                    progressBar->setFormat("Complete");
                    statusLabel->setText("[SUCCESS] " +
                                         operationBox->currentText() + " completed");
                    statusLabel->setStyleSheet("color: #4ADE80; font-weight: 600;");
                    appendConsoleOutput("SUCCESS",
                                        operationBox->currentText() + " completed.");
                } else {
                    progressBar->setValue(0);
                    progressBar->setFormat("Failed");
                    statusLabel->setText("[ERROR] Operation failed; see output");
                    statusLabel->setStyleSheet("color: #F87171; font-weight: 600;");
                }
            });

    // Handle failure to start the backend executable.
    connect(backendProcess,
            &QProcess::errorOccurred,
            this,
            [this](QProcess::ProcessError error) {

                if (error == QProcess::FailedToStart) {
                    appendConsoleOutput("ERROR", "Could not start RetroTransfer.exe.");
                    appendConsoleOutput("ERROR", backendProcess->errorString());

                    startButton->setEnabled(true);
                    browseButton->setEnabled(true);
                    strategyBox->setEnabled(true);
                    browseBaselineButton->setEnabled(true);
                    browseManifestButton->setEnabled(true);
                    browsePayloadButton->setEnabled(true);
                    browseDestinationButton->setEnabled(true);
                    useBaselineBox->setEnabled(true);
                    operationBox->setEnabled(true);
                    sidebarWidget->setEnabled(true);
                    settingsButton->setEnabled(true);
                    updateOperationFields();

                    progressBar->setRange(0, 100);
                    progressBar->setValue(0);
                    progressBar->setFormat("Failed");
                    statusLabel->setText("[ERROR] Backend could not start");
                    statusLabel->setStyleSheet("color: #F87171; font-weight: 600;");
                }
            });

    // Button connections
    connect(browseButton, &QPushButton::clicked,
            this, &MainWindow::browseFolder);

    connect(browseBaselineButton, &QPushButton::clicked,
            this, &MainWindow::browseBaseline);

    connect(browseManifestButton, &QPushButton::clicked,
            this, &MainWindow::browseManifest);

    connect(browsePayloadButton, &QPushButton::clicked,
            this, &MainWindow::browsePayload);

    connect(browseDestinationButton, &QPushButton::clicked,
            this, &MainWindow::browseDestination);

    connect(startButton, &QPushButton::clicked,
            this, &MainWindow::startTransfer);

    updateOperationFields();
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

void MainWindow::browseBaseline()
{
    QString selectedFolder = QFileDialog::getExistingDirectory(
        this, "Select Older Project Folder", ""
    );
    if (!selectedFolder.isEmpty()) {
        baselinePath->setText(selectedFolder);
    }
}

void MainWindow::browseManifest()
{
    QString selectedFile = QFileDialog::getOpenFileName(
        this, "Select Chunk Manifest", "", "JSON files (*.json)"
    );
    if (!selectedFile.isEmpty()) {
        manifestPath->setText(selectedFile);
    }
}

void MainWindow::browsePayload()
{
    QString selectedFile = QFileDialog::getOpenFileName(
        this, "Select Payload", "", "RetroTransfer payloads (*.rtpack)"
    );
    if (!selectedFile.isEmpty()) {
        payloadPath->setText(selectedFile);
    }
}

void MainWindow::browseDestination()
{
    QString selectedFolder = QFileDialog::getExistingDirectory(
        this, "Select Destination Folder", ""
    );
    if (!selectedFolder.isEmpty()) {
        destinationPath->setText(selectedFolder);
    }
}

void MainWindow::showSettingsDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Settings");
    dialog.setMinimumWidth(520);
    dialog.setStyleSheet(
        "QDialog {background-color: #0B1220; color: #E5E7EB;}"
        "QLabel {color: #CBD5E1;}"
        "QLineEdit {background-color: #111A29; color: #E5E7EB;"
        "border: 1px solid #2B3A50; border-radius: 7px; padding: 8px;}"
        "QPushButton {background-color: #182336; color: #E5E7EB;"
        "border: 1px solid #2B3A50; border-radius: 7px; padding: 8px;}"
    );

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *description = new QLabel(
        "Choose where generated files are written. RetroTransfer creates a "
        "\"collection\" folder inside this location."
    );
    description->setWordWrap(true);
    layout->addWidget(description);

    QLineEdit *directoryEdit = new QLineEdit(outputDirectory);
    directoryEdit->setReadOnly(true);
    QPushButton *browseButton = new QPushButton("Browse");
    browseButton->setIcon(lineIcon(LineIcon::OpenFolder));
    QHBoxLayout *directoryRow = new QHBoxLayout();
    directoryRow->addWidget(directoryEdit, 1);
    directoryRow->addWidget(browseButton);
    layout->addLayout(directoryRow);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);

    connect(browseButton, &QPushButton::clicked, &dialog, [&]() {
        const QString selected = QFileDialog::getExistingDirectory(
            &dialog, "Select Output Folder", directoryEdit->text());
        if (!selected.isEmpty()) {
            directoryEdit->setText(selected);
        }
    });
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted &&
        !directoryEdit->text().isEmpty() &&
        QDir(directoryEdit->text()).exists()) {
        outputDirectory = QDir(directoryEdit->text()).absolutePath();
        backendProcess->setWorkingDirectory(outputDirectory);
        outputDirectoryLabel->setText(
            "Generated files: " +
            QDir(outputDirectory).filePath("collection"));
    }
}

void MainWindow::appendConsoleOutput(const QString &tag, const QString &text)
{
    const QStringList lines = text.split(
        QRegularExpression("\\r\\n|\\n|\\r"), Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        return;
    }

    QColor tagColor("#93C5FD");
    if (tag == "PROCESSING") {
        tagColor = QColor("#38BDF8");
    } else if (tag == "SUCCESS") {
        tagColor = QColor("#4ADE80");
    } else if (tag == "ERROR") {
        tagColor = QColor("#F87171");
    }

    QTextCharFormat tagFormat;
    tagFormat.setForeground(tagColor);
    tagFormat.setFontWeight(QFont::DemiBold);
    QTextCharFormat textFormat;
    textFormat.setForeground(QColor("#D1D5DB"));

    QTextCursor cursor(outputArea->document());
    cursor.movePosition(QTextCursor::End);
    for (const QString &line : lines) {
        cursor.insertText("[" + tag + "] ", tagFormat);
        cursor.insertText(line + "\n", textFormat);
    }
    outputArea->setTextCursor(cursor);
    outputArea->ensureCursorVisible();
}

void MainWindow::updateOperationFields()
{
    QString operation = operationBox->currentText();
    bool reconstruct = operation == "Reconstruct";
    bool compress = operation == "Compress";

    folderWidget->setVisible(!reconstruct);
    compressionWidget->setVisible(compress);
    reconstructionWidget->setVisible(reconstruct);
    strategyCard->setVisible(!reconstruct);
    outputFilesCard->setVisible(!reconstruct);
    scanChunkWidget->setVisible(!reconstruct);
    payloadOutputWidget->setVisible(compress);

    folderPath->setPlaceholderText("Select the project to process");

    if (useBaselineBox->isChecked()) {
        baselineLabel->setVisible(compress);
        baselinePath->setVisible(compress);
        browseBaselineButton->setVisible(compress);
    } else {
        baselineLabel->setVisible(false);
        baselinePath->setVisible(false);
        browseBaselineButton->setVisible(false);
    }
}

void MainWindow::startTransfer()
{
    if (backendProcess->state() != QProcess::NotRunning) {
        return;
    }

    QString backendPath =
        QDir(QCoreApplication::applicationDirPath())
            .filePath("RetroTransfer.exe");

    if (!QFileInfo::exists(backendPath)) {
        appendConsoleOutput("ERROR", "RetroTransfer.exe was not found beside the GUI.");
        statusLabel->setText("[ERROR] Backend executable not found");
        statusLabel->setStyleSheet("color: #F87171; font-weight: 600;");
        return;
    }

    QStringList arguments;
    QString operation = operationBox->currentText();
    auto rejectInput = [this](const QString &message) {
        statusLabel->setText("[ERROR] " + message);
        statusLabel->setStyleSheet("color: #F87171; font-weight: 600;");
        appendConsoleOutput("ERROR", message);
    };

    if (operation == "Reconstruct") {
        if (manifestPath->text().isEmpty()) {
            rejectInput("Please select a chunk manifest.");
            return;
        }
        if (destinationPath->text().isEmpty()) {
            rejectInput("Please select a destination folder.");
            return;
        }

        arguments << "--reconstruct" << manifestPath->text();
        if (!payloadPath->text().isEmpty()) {
            arguments << "--payload" << payloadPath->text();
        }
        arguments << "--output" << destinationPath->text();
    } else {
        if (folderPath->text().isEmpty()) {
            rejectInput("Please select a project folder.");
            return;
        }

        QString scanName = scanOutputName->text().trimmed();
        QString chunksName = chunksOutputName->text().trimmed();
        if (scanName.isEmpty() || chunksName.isEmpty()) {
            rejectInput("Scan and chunk filenames are required.");
            return;
        }

        arguments << folderPath->text();
        arguments << "--strategy" << strategyBox->currentText().toLower();
        arguments << "--scan-output" << scanName;
        arguments << "--chunks-output" << chunksName;

        if (operation == "Compress") {
            QString payloadName = payloadOutputName->text().trimmed();
            if (payloadName.isEmpty()) {
                rejectInput("Please enter a payload filename.");
                return;
            }
            arguments << "--payload-output" << payloadName;

            if (useBaselineBox->isChecked()) {
                if (baselinePath->text().isEmpty()) {
                    rejectInput("Please select an older project folder.");
                    return;
                }
                arguments << "--baseline" << baselinePath->text();
            } else {
                arguments << "--compress-all";
            }
        }
    }

    outputArea->clear();
    appendConsoleOutput("PROCESSING", "Starting RetroTransfer backend.");
    appendConsoleOutput("INFO", "Operation: " + operation);
    if (operation != "Reconstruct") {
        appendConsoleOutput("INFO", "Selected folder: " + folderPath->text());
        appendConsoleOutput("INFO", "Selected strategy: " + strategyBox->currentText());
    } else {
        appendConsoleOutput("INFO", "Manifest: " + manifestPath->text());
        appendConsoleOutput("INFO", "Destination: " + destinationPath->text());
    }

    statusLabel->setText("[PROCESSING] Running " + operation + "...");
    statusLabel->setStyleSheet("color: #38BDF8; font-weight: 600;");

    progressBar->setRange(0, 0);
    progressBar->setFormat("Processing");

    startButton->setEnabled(false);
    browseButton->setEnabled(false);
    strategyBox->setEnabled(false);
    browseBaselineButton->setEnabled(false);
    browseManifestButton->setEnabled(false);
    browsePayloadButton->setEnabled(false);
    browseDestinationButton->setEnabled(false);
    useBaselineBox->setEnabled(false);
    operationBox->setEnabled(false);
    sidebarWidget->setEnabled(false);
    settingsButton->setEnabled(false);

    backendProcess->setWorkingDirectory(outputDirectory);
    backendProcess->start(backendPath, arguments);
}
