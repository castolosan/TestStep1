#include "downloadfilesapp.h"
#include "ui_downloadfilesapp.h"

DownloadFilesApp::DownloadFilesApp(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DownloadFilesApp)
{
    ui->setupUi(this);
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
    this, SLOT(finished(QNetworkReply*)));

    // url to download .json
    QString url = "https://altomobile.blob.core.windows.net/api/files.json";

    QNetworkRequest request= QNetworkRequest(QUrl(url));
    manager->get(request);
    // default path to download
    pathToSaveFile = "/home/castolo/Documents/";

}

DownloadFilesApp::~DownloadFilesApp()
{
    delete ui;
}

void DownloadFilesApp::finished(QNetworkReply* reply)
{

    QStringList fileNames;//only display in app
    QStringList fileTypes;//only display in app

    //Extract info from json
    QByteArray response = reply->readAll();
    //qDebug() << response;//uncomment to see response
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
    QJsonArray jsonArray = jsonResponse.array();
    //iterate over the array of the json
    for (int index = 0; index < jsonArray.size(); ++index) {
        QJsonObject fileObject = jsonArray[index].toObject();
        if (fileObject.contains("FileName") && fileObject["FileName"].isString()){
            fileNames.append(fileObject["FileName"].toString());
        }
        if (fileObject.contains("BlobType") && fileObject["BlobType"].isString()){
            fileTypes.append(fileObject["BlobType"].toString());
        }
        if (fileObject.contains("DownloadURI") && fileObject["DownloadURI"].isString()){
            fileLinks.append(fileObject["DownloadURI"].toString());
        }
    }

    //insert names of files in combo box with file type
    for (int i=0; i<fileNames.size();i++){
        QString nameDisplay;
        nameDisplay = fileNames[i]+ " (File Type: " + fileTypes[i]+")";
        ui->filesComboBox->insertItem(i,nameDisplay);
    }

    //Extract the link of selected file
    urlFileDownload = fileLinks[index2Download];
}

void DownloadFilesApp::on_quitButton_clicked()
{
    QApplication::quit();
}



void DownloadFilesApp::on_filesComboBox_currentIndexChanged(int index)
{
    //change the file to download
    index2Download = (ui->filesComboBox->currentIndex());
}

void DownloadFilesApp::on_downloadButton_clicked()
{
    progressDialog = new QProgressDialog(this);
    urlFileDownload = fileLinks[index2Download];

    //manager for the file
    manager2 = new QNetworkAccessManager(this);

   QFileInfo fileInfo(urlFileDownload.path());
   QString fileName = fileInfo.fileName();
   //qDebug() << "FileName: " <<fileName;

   if (fileName.isEmpty())
           fileName = "index.html";


   if (QFile::exists(pathToSaveFile + fileName)) {
        if (QMessageBox::question(this, tr("The file exists"),
           tr("There already exists a file called %1 in "
           "the current directory. Overwrite?").arg(fileName),
           QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
           == QMessageBox::No)
           return;
           QFile::remove(fileName);
       }


   file = new QFile(pathToSaveFile + fileName);
       if (!file->open(QIODevice::WriteOnly)) {
           QMessageBox::information(this, tr("Message"),
                         tr("Unable to save the file %1: %2.")
                         .arg(fileName).arg(file->errorString()));
           delete file;
           file = nullptr;
           return;
       }

    // Flag when cancel download
    flagCancelDownload = false;

    progressDialog->setWindowTitle(tr("Progress"));
    progressDialog->setLabelText(tr("Downloading %1.").arg(fileName));

    // download button disabled after requesting download
    ui->downloadButton->setEnabled(false);

    //call to downoad function
    downloadFunction();
    //connect signal when cancel download
    connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelDownload()));
}

void DownloadFilesApp::downloadFunction(){

   reply2 = manager2->get(QNetworkRequest(urlFileDownload));

   //connect and read file
   connect(reply2, SIGNAL(readyRead()),
              this, SLOT(ReadyReadFile()));

   //connect the download bar
   connect(reply2, SIGNAL(downloadProgress(qint64,qint64)),
          this, SLOT(updateProgressBar(qint64,qint64)));

  //connect the signal when download finish with the closefile fucntion
    connect(reply2, SIGNAL(finished()),
           this, SLOT(DownloadFinished()));
}


void DownloadFilesApp::ReadyReadFile()
{
    // signal of the QNetworkReply2 to read the file download
    if (file)
        file->write(reply2->readAll());
}


void DownloadFilesApp::DownloadFinished()
{
    // if cancel download, delete files
    if (flagCancelDownload) {
        if (file) {
            file->close();
            file->remove();
            delete file;
            file = nullptr;
        }
        reply2->deleteLater();
        progressDialog->hide();
        return;
    }
    // if download continue
    file->flush();
    file->close();

    reply2->deleteLater();
    reply2 = nullptr;
    delete file;
    file = nullptr;
    manager2 = nullptr;
    //qDebug() << "Download finished.";
    ui->downloadButton->setEnabled(true);
}

void DownloadFilesApp::updateProgressBar(qint64 bytesRead, qint64 totalBytes)
{
    if (flagCancelDownload)
       return;

    progressDialog->setMaximum(totalBytes);
    progressDialog->setValue(bytesRead);
}

void DownloadFilesApp::on_actionEdith_path_to_download_triggered()
{
    pathToSaveFile = QFileDialog::getExistingDirectory(this, "Choose Path:") + "/";
}

void DownloadFilesApp::on_actionExit_triggered()
{
     QApplication::quit();
}

void DownloadFilesApp::cancelDownload()
{
    flagCancelDownload = true;
    reply2->abort();
    ui->downloadButton->setEnabled(true);
}
