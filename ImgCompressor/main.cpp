#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "filemodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    FileModel fileModel;
    engine.rootContext()->setContextProperty("fileModel", &fileModel);

    // command line handling, the user may provide a path to workw ith as a parameter
    QString directoryPath = QGuiApplication::applicationDirPath();
    if (argc > 1)
    {
        QString arg = QString::fromLocal8Bit(argv[1]);
        QDir    dir(arg);

        if (dir.exists())
            directoryPath = arg;
    }
    fileModel.loadDirectory(directoryPath);

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    engine.load(url);

    return app.exec();
}
