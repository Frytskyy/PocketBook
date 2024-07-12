#pragma once

#include <QObject>
#include <QThread>
#include <QString>

class ImageProcessor : public QObject
{
    Q_OBJECT
public:
    explicit ImageProcessor(QObject *parent = nullptr);

public slots:
    void processImage(const QString &filePath);

signals:
    void processingStarted(const QString &filePath);
    void processingFinished(const QString &filePath, bool success, const QString &message);
};
