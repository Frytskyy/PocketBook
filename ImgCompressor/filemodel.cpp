#include "filemodel.h"
#include "customerrordialog.h"
#include <QCoreApplication>
#include <QQuickWindow>
#include <QRunnable>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QLibrary>
#include <QtAssert>


///////////////////////////////////////////////
//
//     Link with image compression/
//        decompression libruary
//

#include "../BarchImgCompressorLib/barchimgcompressorlib.h"

using namespace BarchImgCompressorDecompressor;

typedef eImgCompressionResult (*encodeBarchBitmapFunc)(const std::string&, const std::string&);
typedef eImgCompressionResult (*decodeBarchBitmapFunc)(const std::string&, const std::string&);


QLibrary              g_barchLib;
decodeBarchBitmapFunc g_encodeBarchBitmapFunc = nullptr;
decodeBarchBitmapFunc g_decodeBarchBitmapFunc = nullptr;

static void loadBarchLibIfNeeded(bool bForceReload = false)
{
    static int s_attempt_n = 0;

    if (!g_barchLib.isLoaded() &&
        (s_attempt_n ++ == 0 || bForceReload))
    {
        g_encodeBarchBitmapFunc = nullptr;
        g_decodeBarchBitmapFunc = nullptr;

        // Define a list of possible paths to search for the library
        QStringList possiblePaths =
            {
                QCoreApplication::applicationDirPath() + "/BarchImgCompressorLib.dll",
                QDir::currentPath() + "/BarchImgCompressorLib.dll",
                "..\\..\\..\\..\\BarchImgCompressorLib\\build\\Desktop_Qt_6_7_2_MinGW_64_bit-Release\\release\\BarchImgCompressorLib.dll"
            };

        for (const QString& path : possiblePaths)
        {
            g_barchLib.setFileName(path);
            if (g_barchLib.load())
            {
                break; // Stop searching if library is successfully loaded
            }
        }

        if (!g_barchLib.isLoaded())
        {
            QMessageBox::warning(nullptr, "Error", "Cannot load Barch library.");
            return;
        }

        g_encodeBarchBitmapFunc = (encodeBarchBitmapFunc)g_barchLib.resolve("encodeBarchBitmap");
        g_decodeBarchBitmapFunc = (decodeBarchBitmapFunc)g_barchLib.resolve("decodeBarchBitmap");

        if (!g_encodeBarchBitmapFunc ||
            !g_decodeBarchBitmapFunc)
        {
            QMessageBox::warning(nullptr, "Error", "Cannot load Barch library [lib loaded OK, but encode/decode functions undefined in it].");
            return;
        }
    }
}

static eImgCompressionResult encodeBarchBitmapWrapper(const std::string& inputBMPFilePath, const std::string& outBarchFilePath)
{
    if (!g_encodeBarchBitmapFunc)
    {
        loadBarchLibIfNeeded();
        if (!g_encodeBarchBitmapFunc)
        {
            assert(false); //should never happen
            return comprResErrorOther;
        }
    }

    return g_encodeBarchBitmapFunc(inputBMPFilePath, outBarchFilePath);
}

static eImgCompressionResult decodeBarchBitmapWrapper(const std::string& inputBarchFilePath, const std::string& outBMPFilePath)
{
    if (!g_decodeBarchBitmapFunc)
    {
        loadBarchLibIfNeeded();
        if (!g_decodeBarchBitmapFunc)
        {
            assert(false); //should never happen
            return comprResErrorOther;
        }
    }

    return g_decodeBarchBitmapFunc(inputBarchFilePath, outBMPFilePath);
}

using namespace BarchImgCompressorDecompressor;

typedef eImgCompressionResult (*encodeBarchBitmapFunc)(const std::string&, const std::string&);
typedef eImgCompressionResult (*decodeBarchBitmapFunc)(const std::string&, const std::string&);


///////////////////////////////////////////////
//
//     class FileModel
//


FileModel::FileModel(QQmlEngine* pQMLEngine, QObject* pParentObj) :
    QAbstractListModel(pParentObj),
    m_pQMLEngine(pQMLEngine)
{
    Q_ASSERT(m_pQMLEngine); //nust be not null
    m_threadPool.setMaxThreadCount(3); //let's allow some parallel threds, but not too many (in assesment it's not critical, but let's presume it's for heavy threads that require a lot of resources)
}

int FileModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_files.size();
}

QVariant FileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const FileItem &item = m_files.at(index.row());

    switch (role) {
    case NameRole:
        return item.m_name;
    case SizeRole:
        return item.m_size;
    case SizeStrRole:
        return item.m_size_str;
    case StateStrRole:
        return item.m_state_str;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> FileModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[NameRole] = "name";
    roles[SizeRole] = "size";
    roles[SizeStrRole] = "size_str";
    roles[StateStrRole] = "state_str";

    return roles;
}

QString ConvertSizeToStr(qint64 size)
{
    QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int i;
    double outputSize = size;

    for (i = 0; i < units.size() - 1; i++, outputSize /= 1024)
        if (outputSize < 1024)
            break;

    return QString("%1 %2").arg(outputSize, 0, 'f', 2).arg(units[i]);
}

void FileModel::loadDirectory(const QString &path)
{
    beginResetModel();
    m_files.clear();

    QDir            dir(path);
    QStringList     filters;

    filters << "*.bmp" << "*.png" << "*.barch";

    QFileInfoList   fileInfoList = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo &fileInfo : fileInfoList)
    {
        m_files.append({fileInfo.fileName(), fileInfo.size(), ConvertSizeToStr(fileInfo.size()), ""});
    }

    if (m_files.empty())
        m_files.append({"", -1, " Empty Folder ", ""}); //add empty entry to show "Empty Folder" row

    endResetModel();
}

void FileModel::processFile(int index)
{
    if (index < 0 || index >= m_files.size())
        return;

    const FileItem  &item = m_files.at(index);
    QString         filePath = QDir::currentPath() + "/" + item.m_name;

    if (!item.m_state_str.isEmpty())
        return; //do not process file processing twice. All clicked files will have status forever after

    if (item.m_size == -1)
    {
        //special case for row which says "Empty Folder", do nothing
    } else if (item.m_name.endsWith(".bmp"))
    {
        encodeFile(filePath, index);
    } else if (item.m_name.endsWith(".barch"))
    {
        decodeFile(filePath, index);
    } else
    {
        updateFileState(index, "Unknown file type");

        showErrorDialog("Unknown file type", "File path: " + filePath);
    }
}

void FileModel::encodeFile(const QString &filePath, int fileIndex)
{
    if (filePath.contains("reconverted"))
    {
        updateFileState(fileIndex, "Refuse to do it again - it was already converted twice, seek for original!");
        return;
    }

    class EncodeTask : public QRunnable
    {
    public:

        EncodeTask(FileModel *model, const QString &filePath, int index)
            : m_pModel(model), m_filePath(filePath), m_index(index) { setAutoDelete(true); }

        void run() override
        {
            QString   outFileName = m_filePath + ".packed.barch";
            QFileInfo outFileInfo(outFileName);

            QMetaObject::invokeMethod(m_pModel, "updateFileState",  Qt::QueuedConnection,
                                      Q_ARG(int, m_index), Q_ARG(QString, "Encoding..."));

            eImgCompressionResult   result = encodeBarchBitmapWrapper(m_filePath.toStdString(), outFileName.toStdString());
            if (result == comprResOk)
            {
                for (int percent = 0; percent <= 100; percent += 10)
                {
                    //tmp: to illustrate that parallel processing is enabled, otherwise it would be too fast to notice anything in a blink of an eye
                    QThread::msleep(400);

                    QMetaObject::invokeMethod(m_pModel, "updateFileState",  Qt::QueuedConnection,
                                              Q_ARG(int, m_index), Q_ARG(QString, "Encoding... " + QString::number(percent) + "%"));
                }

                QMetaObject::invokeMethod(m_pModel, "updateFileState",  Qt::QueuedConnection,
                                          Q_ARG(int, m_index), Q_ARG(QString, "Encoded! " + outFileInfo.fileName()));
            } else
            {
                //Error, show dialog box, as per assesmnt description
                QString msg("Cannot convert image " + m_filePath + " to " + outFileName + ". Error code: " + QString::number(result));

                QMetaObject::invokeMethod(m_pModel, "showErrorDialog",Qt::QueuedConnection,
                                          Q_ARG(QString, "Image ecoding failed"), Q_ARG(QString, msg));

                QMetaObject::invokeMethod(m_pModel, "updateFileState",  Qt::QueuedConnection,
                                          Q_ARG(int, m_index), Q_ARG(QString, "ERROR: Encode failed. " + msg));
            }
        }

    private:

        FileModel  *m_pModel;
        QString     m_filePath;
        int         m_index;
    };

    m_threadPool.start(new EncodeTask(this, filePath, fileIndex));
}

void FileModel::decodeFile(const QString &filePath, int fileIndex)
{
    class DecodeTask : public QRunnable
    {
    public:
        DecodeTask(FileModel *model, const QString &filePath, int index)
            : m_pModel(model), m_filePath(filePath), m_index(index) {}

        void run() override
        {
            //tmp, for debugging: qDebug() << "DecodeTask::run is running in thread: " << QThread::currentThreadId();

            QString   outFileName = m_filePath + ".unpacked.bmp";
            QFileInfo outFileInfo(outFileName);

            QMetaObject::invokeMethod(m_pModel, "updateFileState",  Qt::QueuedConnection,
                                      Q_ARG(int, m_index), Q_ARG(QString, "Decoding..."));

            eImgCompressionResult   result = decodeBarchBitmapWrapper(m_filePath.toStdString(), outFileName.toStdString());
            if (result == comprResOk)
            {
                for (int percent = 0; percent <= 100; percent += 10)
                {
                    //tmp: to illustrate that parallel processing is enabled, otherwise it would be too fast to notice anything in a blink of an eye
                    QThread::msleep(400);
                    QMetaObject::invokeMethod(m_pModel, "updateFileState",  Qt::QueuedConnection,
                                              Q_ARG(int, m_index), Q_ARG(QString, "Decoding... " + QString::number(percent) + "%"));
                }

                QMetaObject::invokeMethod(m_pModel, "updateFileState",  Qt::QueuedConnection,
                                          Q_ARG(int, m_index), Q_ARG(QString, "Decoded! " + outFileInfo.fileName()));
            } else
            {
                //Error, show dialog box, as per assesmnt description
                QString msg("Cannot convert image " + m_filePath + " to " + outFileName + ". Error code: " + QString::number(result));

                QMetaObject::invokeMethod(m_pModel, "showErrorDialog",Qt::QueuedConnection,
                                          Q_ARG(QString, "Image decoding failed"), Q_ARG(QString, msg));

                QMetaObject::invokeMethod(m_pModel, "updateFileState",  Qt::QueuedConnection,
                                          Q_ARG(int, m_index), Q_ARG(QString, "ERROR: Decode failed. " + msg));
            }
        }

    private:

        FileModel  *m_pModel;
        QString     m_filePath;
        int         m_index;
    };

    //tmp, for debugging: qDebug() << "Maximum thread count:" << QThreadPool::globalInstance()->maxThreadCount();
    //tmp, for debugging: qDebug() << "FileModel is running in thread: " << QThread::currentThreadId();

    m_threadPool.start(new DecodeTask(this, filePath, fileIndex));
}

void FileModel::updateFileState(int fileIndex, QString state)
{
    if (fileIndex < 0 || fileIndex >= m_files.size())
        return;

    m_files[fileIndex].m_state_str = state;

    beginResetModel();
    endResetModel();
}

void FileModel::showErrorDialog(QString title, QString message)
{
    SHOW_ERROR_MSG(title, message, m_pQMLEngine, this);
}
