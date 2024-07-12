#pragma once

#include <QAbstractListModel>
#include <QFileInfo>
#include <QDir>
#include <QThreadPool>

class FileModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int updateCounter READ updateCounter NOTIFY updateCounterChanged)

public:

    enum Roles
    {
        NameRole = Qt::UserRole + 1,
        SizeRole,
        SizeStrRole,
        StateStrRole
    };
    Q_ENUM(Roles)

    explicit         FileModel(QObject *parent = nullptr);

    int              rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant         data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void loadDirectory(const QString &path);
    Q_INVOKABLE void processFile(int index);

    void             updateFileState(int fileIndex, const QString &state);
    int              updateCounter() const { return m_updateCounter; }

signals:

    void             updateCounterChanged();

private:

    struct FileItem
    {
        QString     name;
        qint64      size;
        QString     size_str;
        QString     state_str;
    };

    QList<FileItem> m_files;
    QThreadPool     m_threadPool;
    int             m_updateCounter = 0;

    void            encodeFile(const QString &filePath, int fileIndex);
    void            decodeFile(const QString &filePath, int fileIndex);
};
