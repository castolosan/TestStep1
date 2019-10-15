#include "downloadfilesapp.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DownloadFilesApp w;
    w.show();
    return a.exec();
}
