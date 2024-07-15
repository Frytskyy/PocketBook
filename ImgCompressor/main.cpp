#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QWidget>
#include <QtAssert>
#include "filemodel.h"


int main(int argc, char *argv[])
{
    QGuiApplication         app(argc, argv);
    QQmlApplicationEngine   engine;
    QString                 directoryPath;

    if (argc > 1)
    {
        // command line handling, the user may provide a path to work with as a parameter
        QString argStrCombined;

        for (int i = 1; i < argc; ++i)
        {
            argStrCombined += QString::fromLocal8Bit(argv[i]);
            if (i < argc - 1)
                argStrCombined += " ";
        }

        int dbgExtraIndex = argStrCombined.indexOf("-qmljsdebugger=");

        if (dbgExtraIndex != -1)
            argStrCombined = argStrCombined.left(dbgExtraIndex); //get rid from debugging-related arguments

        argStrCombined = argStrCombined.trimmed();
        if (argStrCombined.startsWith('"') &&
            argStrCombined.endsWith('"'))
        {
            //get rid from surrounding double quotes
            argStrCombined.remove(0, 1);
            argStrCombined.chop(1);
        }

        //
        QDir    dir(argStrCombined);

        if (dir.exists())
            directoryPath = argStrCombined;
    }
    if (directoryPath.isEmpty())
        directoryPath = QGuiApplication::applicationDirPath();

    const QUrl url(QStringLiteral("qrc:/main.qml"));

    engine.load(url);

    // Show path we are working with in main window title
    QObject      *pRootObject = engine.rootObjects().first();
    Q_ASSERT(pRootObject);
    QQuickWindow *pMainWindow = qobject_cast<QQuickWindow *>(pRootObject);

    if (!pMainWindow)
    {
        qFatal("Error: Root item has to be a Window.");
        Q_ASSERT(false); //must never happen
        return -1;
    }

    FileModel     fileModel(&engine, pRootObject);

    engine.rootContext()->setContextProperty("fileModel", &fileModel);
    fileModel.loadDirectory(directoryPath);

    pMainWindow->setTitle("Barch Image encoder/decoder - " + directoryPath);

    return app.exec();
}
