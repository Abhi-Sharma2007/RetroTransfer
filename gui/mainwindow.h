
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;
class QTextEdit;
class QProgressBar;
class QProcess;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QLineEdit *folderPath;
    QComboBox *strategyBox;
    QPushButton *browseButton;
    QPushButton *startButton;
    QLabel *statusLabel;
    QTextEdit *outputArea;
    QProgressBar *progressBar;

    QProcess *backendProcess;

    void browseFolder();
    void startTransfer();
};

#endif
