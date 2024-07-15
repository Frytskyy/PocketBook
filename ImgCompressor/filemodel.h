#pragma once

#include <QAbstractListModel>
#include <QFileInfo>
#include <QDir>
#include <QThreadPool>
#include <QQmlEngine>


class FileModel : public QAbstractListModel
{
    Q_OBJECT
    //Q_PROPERTY(int updateCounter READ updateCounter NOTIFY updateCounterChanged)

public:

    enum eRoles
    {
        NameRole = Qt::UserRole + 1,
        SizeRole,
        SizeStrRole,
        StateStrRole
    };
    Q_ENUM(eRoles)

    explicit         FileModel(QQmlEngine* pQMLEngine, QObject* pParentObj);

    int              rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant         data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void             loadDirectory(const QString &path);
    Q_INVOKABLE void processFile(int index);

    Q_INVOKABLE void updateFileState(int fileIndex, QString state);

    QQmlEngine*      getEngine() const { return m_pQMLEngine; }

    Q_INVOKABLE void showErrorDialog(QString title, QString message);

private:

    struct FileItem
    {
        QString     m_name;
        qint64      m_size;
        QString     m_size_str;
        QString     m_state_str;
    };

    QQmlEngine     *m_pQMLEngine;
    QList<FileItem> m_files;
    QThreadPool     m_threadPool;

    void            encodeFile(const QString &filePath, int fileIndex);
    void            decodeFile(const QString &filePath, int fileIndex);
};


