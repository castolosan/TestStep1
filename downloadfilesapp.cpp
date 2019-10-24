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

    // download .json
    QString url = "https://altomobile.blob.core.windows.net/api/files.json";
    QNetworkRequest request= QNetworkRequest(QUrl(url));
    manager->get(request);

    // default path to download
    pathToSaveFile = "/home/castolo/Documents/";

    //Block pause & resume bottons
    ui->pauseButton->setEnabled(false);
    ui->resumeButton_2->setEnabled(false);

    //set initial values
    index2Download = 0;//by default, download first item
    flagPauseFileActive = false;
    // Flag when cancel download
    flagCancelDownload = false;
    sizeFile = 0;
    sizeAtPause=0;//agregado1
}

DownloadFilesApp::~DownloadFilesApp()
{
    delete ui;
}

void DownloadFilesApp::finished(QNetworkReply* reply)
{
    //Check for download the json file
    if(reply->error() != QNetworkReply::NoError){
        //qDebug() << "error " << reply->error(); //uncomment to see error
        QMessageBox::critical(this, tr("Error"),
                     tr("The json file wasn't found."),
                     QMessageBox::Ok);
        //delete reply
        reply->deleteLater();
        //delete manager of json
        manager->deleteLater();
        manager = nullptr;

        //disable download botton
        ui->downloadButton->setEnabled(false);
    }else{

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
            if (fileObject.contains("Length") && fileObject["Length"].isString()){
                fileSizes.append(fileObject["Length"].toString());
            }

        }

        //insert names of files in combo box with file type
        for (int i=0; i<fileNames.size();i++){
            QString nameDisplay;
            nameDisplay = fileNames[i]+ " (File Type: " + fileTypes[i]+")";
            ui->filesComboBox->insertItem(i,nameDisplay);
        }
    }

}

void DownloadFilesApp::on_quitButton_clicked()
{
    QApplication::quit();
}



void DownloadFilesApp::on_filesComboBox_currentIndexChanged()
{
    //change the index of file to download when the filesComboBox changes
    index2Download = (ui->filesComboBox->currentIndex());
}

void DownloadFilesApp::on_downloadButton_clicked()
{
    //get the url of the file
    urlFileDownload = fileLinks[index2Download];
    //get the size of the file
    sizeFile = fileSizes[index2Download].toInt();

    //manager for download the file
    manager = new QNetworkAccessManager(this);

    //get info of the file
    QFileInfo fileInfo(urlFileDownload.path());

    fileName = fileInfo.fileName();

   if (fileName.isEmpty())
           fileName = "index.html";

    //check if the file alredy exists in the folder
   if (QFile::exists(pathToSaveFile + fileName)) {
        if (QMessageBox::question(this, tr("The file exists"),
           tr("There already exists a file called %1 in the current directory. Overwrite?").arg(fileName),
           QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::No)
           //if user dont want to overwrite file
            return;
           QFile::remove(fileName);
       }

    //generate new file
   file = new QFile(pathToSaveFile + fileName);
   file->open(QIODevice::WriteOnly);

    // download button disabled after requesting download
    ui->downloadButton->setEnabled(false);


    //unBlock pause & resume bottons
    ui->pauseButton->setEnabled(true);
    flagCancelDownload = false;

    //call to downoad function
    downloadFunction();

}

void DownloadFilesApp::downloadFunction(){

    //Check if pause download is active
    if(flagPauseFileActive==false){

        //request the file
        currentRequest = QNetworkRequest(urlFileDownload);
        replyNetwork = manager->get(QNetworkRequest(currentRequest));

        //check if the file is present
        if(replyNetwork->error() == QNetworkReply::ContentNotFoundError ){
            //qDebug() << "error, no file " << replyNetwork->error();

            //delete reply
            replyNetwork->deleteLater();
            //delete manager of json
            manager->deleteLater();
            manager = nullptr;
            //check pause & resume bottons
            ui->pauseButton->setEnabled(false);
            ui->downloadButton->setEnabled(true);

            return;
        }


        progressDialog = new QProgressDialog(this);
        progressDialog->setWindowTitle(tr("Progress"));
        progressDialog->setLabelText(tr("Downloading %1.").arg(fileName));


    //if the pause is activated, continue the download from the part that was left
    }else{

        //set the range for continue the download
        QByteArray rangeHeaderValue = "bytes=" + QByteArray::number(sizeAtPause) + "-";

        //set the request for the server
        currentRequest.setRawHeader("Range",rangeHeaderValue);
        replyNetwork = manager->get(currentRequest);


        flagPauseFileActive = false;


    }

    //connect all signals and slots

   //connect and read file
   connect(replyNetwork, SIGNAL(readyRead()),
              this, SLOT(ReadyReadFile()));

   //connect the download bar
   connect(replyNetwork, SIGNAL(downloadProgress(qint64,qint64)),
          this, SLOT(updateProgressBar(qint64,qint64)));

  //connect the signal when download finish with the closefile fucntion
    connect(replyNetwork, SIGNAL(finished()),
           this, SLOT(DownloadFinished()));

    //connect signal when cancel download
    connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelDownload()));

    //connect signals to check for errors
    connect(replyNetwork, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(error(QNetworkReply::NetworkError)));

}


void DownloadFilesApp::ReadyReadFile()
{
    // signal of the QNetworkreplyNetwork to read the file download
    if (file)
        file->write(replyNetwork->readAll());
}


void DownloadFilesApp::DownloadFinished()
{
    // if the download is cancelled, delete the files
    if (flagCancelDownload) {
        if (file) {
            file->close();
            file->remove();
            delete file;
            file = nullptr;
        }
        replyNetwork->deleteLater();
        manager->deleteLater();
        manager = nullptr;
        progressDialog->hide();

        //check pause & resume bottons
        ui->pauseButton->setEnabled(false);
        ui->downloadButton->setEnabled(true);
        ui->resumeButton_2->setEnabled(false);
        return;
    }else{

    // if download continue
    file->flush();
    file->close();
    progressDialog->close();


    //display error message if file size does not match
    if(file->size()!=(sizeFile)){
            qDebug() << "The size of the file does not match " ;
            QMessageBox::critical(this, tr("Error"),
                         tr("Something went wrong. The size of the file does not match."),
                         QMessageBox::Ok);
        }

    //delete pointers and replys
    replyNetwork->deleteLater();
    delete file;

    manager->deleteLater();
    manager = nullptr;

    //qDebug() << "Download finished.";
    //restaure functions of bottons
    ui->downloadButton->setEnabled(true);
    flagPauseFileActive = false;
    flagCancelDownload = false;

    //check pause & resume bottons
    ui->pauseButton->setEnabled(false);
    ui->resumeButton_2->setEnabled(false);
    }
}

void DownloadFilesApp::updateProgressBar(qint64 bytesRead, qint64 totalBytes)
{
    if (flagCancelDownload)
       return;

   progressDialog->setMaximum(static_cast<int>(totalBytes));
   progressDialog->setValue( static_cast<int>(sizeAtPause + bytesRead));

}

void DownloadFilesApp::on_actionEdith_path_to_download_triggered()
{

    QString pathTemp = pathToSaveFile;
    pathTemp = QFileDialog::getExistingDirectory(this, "Choose Path:") + "/";

    QFileInfo my_dir(pathTemp);
    //check if have permissions to write in folder
    if(my_dir.isDir() && my_dir.isWritable()){
      // qDebug() << "The user have sufficient permissions";
       pathToSaveFile = pathTemp;
    }else
        //qDebug() << "The user does not have sufficient permissions";
        QMessageBox::warning(this, tr("Error"),
                     tr("You don't have permissions to write in this directory. Choose another one."),
                     QMessageBox::Ok);
}

void DownloadFilesApp::on_actionExit_triggered()
{
     QApplication::quit();
}

void DownloadFilesApp::cancelDownload()
{

    //active flags
    flagCancelDownload = true;
    flagPauseFileActive = false;
    //disable bottons
    ui->downloadButton->setEnabled(true);
    ui->pauseButton->setEnabled(false);
    ui->resumeButton_2->setEnabled(false);
    //delete progressDialog
    progressDialog->deleteLater();
    replyNetwork->abort();
}

void DownloadFilesApp::on_pauseButton_clicked()
{

    //Check if the server accept pauses in downloads with ranges
    if (replyNetwork->hasRawHeader("Accept-Ranges")){

        flagPauseFileActive = true;
        //unblock resume botton
        ui->pauseButton->setEnabled(false);
        ui->resumeButton_2->setEnabled(true);


        //disconnect and read file
        disconnect(replyNetwork, SIGNAL(readyRead()),
                   this, SLOT(ReadyReadFile()));

        //disconnect the download bar
        disconnect(replyNetwork, SIGNAL(downloadProgress(qint64,qint64)),
               this, SLOT(updateProgressBar(qint64,qint64)));

        //disconnect the signal when download finish with the closefile fucntion
         disconnect(replyNetwork, SIGNAL(finished()),
                this, SLOT(DownloadFinished()));

         //disconnect signal when cancel download
         disconnect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelDownload()));

         //disconect error signal
         disconnect(replyNetwork, SIGNAL(error(QNetworkReply::NetworkError)),
                 this, SLOT(error(QNetworkReply::NetworkError)));


        file->flush();
        sizeAtPause = file->size();
        replyNetwork->abort();



    }else{
            flagPauseFileActive = false;
            QMessageBox::information(this, tr("Info"),
                         tr("The server dont accept pause downloads."),
                         QMessageBox::Ok);
            //qDebug() << "The server dont accept pause downloads.";
        }
}

void DownloadFilesApp::on_resumeButton_2_clicked()
{
    //qDebug() << "Resume download";
    //activate pause
    ui->pauseButton->setEnabled(true);

    downloadFunction();
}



void DownloadFilesApp::error(QNetworkReply::NetworkError code)
{
    qDebug() << __FUNCTION__ << "(" << code << ")";
}
