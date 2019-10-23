#ifndef DOWNLOADFILESAPP_H
#define DOWNLOADFILESAPP_H

#include <QMainWindow>
#include <QMainWindow>
#include <QDoubleValidator>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QMessageBox>
//For downloadFiles
#include <QUrl>
#include <QProgressDialog>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>


QT_BEGIN_NAMESPACE
namespace Ui { class DownloadFilesApp; }
QT_END_NAMESPACE

class DownloadFilesApp : public QMainWindow
{
    Q_OBJECT

public:
    DownloadFilesApp(QWidget *parent = nullptr);
    ~DownloadFilesApp();
    void downloadFunction();


private slots:

    void finished(QNetworkReply* reply);

    void cancelDownload();

    void ReadyReadFile();

    void DownloadFinished();

    void updateProgressBar(qint64, qint64);

    void on_quitButton_clicked();

    void on_filesComboBox_currentIndexChanged();

    void on_downloadButton_clicked();

    void on_actionEdith_path_to_download_triggered();

    void on_actionExit_triggered();

    void error(QNetworkReply::NetworkError code);

    void on_pauseButton_clicked();

    void on_resumeButton_2_clicked();

private:
    Ui::DownloadFilesApp *ui;
    QNetworkAccessManager* manager;
    qint32 index2Download;
    QStringList fileLinks;
    QStringList fileSizes;
    QUrl url;
    QUrl urlFileDownload;
    QString pathToSaveFile;
    QProgressDialog *progressDialog;
    QFile *file;
    QNetworkReply *replyNetwork; //Reply for file
    QNetworkRequest currentRequest;
    bool flagCancelDownload;
    QString fileName;
    //add pause
    int sizeFile;
    long long sizeAtPause;
    bool flagPauseFileActive;
};
#endif // DOWNLOADFILESAPP_H
