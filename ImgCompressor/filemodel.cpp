#include "filemodel.h"
#include <QRunnable>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QLibrary>
#include <QCoreApplication>


///////////////////////////////////////////////
//
//     Link with image compression/
//        decompression libruary
//

#include "../BarchImgCompressorLib/barchimgcompressorlib.h"

using namespace BarchImgCompressorDecompressor;

typedef eImgCompressionResult (*encodeBarchBitmapFunc)(const std::string&, const std::string&);
typedef eImgCompressionResult (*decodeBarchBitmapFunc)(const std::string&, const std::string&);


QLibrary g_barchLib;
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


///////////////////////////////////////////////
//
//     class FileModel
//

FileModel::FileModel(QObject *parent)
    : QAbstractListModel(parent)
{
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
        return item.name;
    case SizeRole:
        return item.size;
    case SizeStrRole:
        return item.size_str;
    case StateStrRole:
        return item.state_str;
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

    endResetModel();
}

void FileModel::processFile(int index)
{
    if (index < 0 || index >= m_files.size())
        return;

    const FileItem  &item = m_files.at(index);
    QString         filePath = QDir::currentPath() + "/" + item.name;

    if (item.name.endsWith(".bmp")) {
        encodeFile(filePath, index);
    } else if (item.name.endsWith(".barch")) {
        decodeFile(filePath, index);
    } else {
        updateFileState(index, "Unknown file type");
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
            : m_model(model), m_filePath(filePath), m_index(index) {}

        void run() override
        {
            m_model->updateFileState(m_index, "Encoding...");

            encodeBarchBitmapWrapper(m_filePath.toStdString(), m_filePath.toStdString() + ".packed.barch");

            m_model->updateFileState(m_index, "Encoded!");
        }

    private:

        FileModel  *m_model;
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
            : m_model(model), m_filePath(filePath), m_index(index) {}

        void run() override
        {
            m_model->updateFileState(m_index, "Decoding...");

            decodeBarchBitmapWrapper(m_filePath.toStdString(), m_filePath.toStdString() + ".unpacked.bmp");

            m_model->updateFileState(m_index, "Decoded!");
        }

    private:
        FileModel  *m_model;
        QString     m_filePath;
        int         m_index;
    };

    m_threadPool.start(new DecodeTask(this, filePath, fileIndex));
}

void FileModel::updateFileState(int fileIndex, const QString &state)
{
    if (fileIndex < 0 || fileIndex >= m_files.size())
        return;

    m_files[fileIndex].state_str = state;

    beginResetModel();
    endResetModel();

    m_updateCounter ++;
    emit updateCounterChanged();

/*  QModelIndex modelIndex = createIndex(fileIndex, 0);

    m_updateCounter ++;
    emit dataChanged(modelIndex, modelIndex, {StateStrRole}); */
}
