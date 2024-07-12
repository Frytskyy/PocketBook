#include "imageprocessor.h"
#include "../BarchImgCompressorLib/barchimgcompressorlib.h"
#include <QImage>
#include <QLibrary>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QLibrary>
using namespace BarchImgCompressorDecompressor;

typedef eImgCompressionResult (*encodeBarchBitmapFunc)(const std::string&, const std::string&);
typedef eImgCompressionResult (*decodeBarchBitmapFunc)(const std::string&, const std::string&);


ImageProcessor::ImageProcessor(QObject *parent)
    : QObject(parent)
{
}

void ImageProcessor::processImage(const QString &filePath)
{
    emit processingStarted(filePath);

    QLibrary barchLib(tr("..\\..\\..\\..\\BarchImgCompressorLib\\build\\Desktop_Qt_6_7_2_MinGW_64_bit-Release\\release\\BarchImgCompressorLib.dll"));
    if (!barchLib.load()) {
        QMessageBox::warning(nullptr, tr("Error"), tr("Cannot load Barch library."));
        return;
    }

    encodeBarchBitmapFunc encodeBarchBitmapRef = (encodeBarchBitmapFunc)barchLib.resolve("encodeBarchBitmap"); //get func address
    decodeBarchBitmapFunc decodeBarchBitmapRef = (decodeBarchBitmapFunc)barchLib.resolve("decodeBarchBitmap"); //get func address

    // Simplified processing logic
    QString newPath;
    if (filePath.endsWith(".bmp", Qt::CaseInsensitive))
    {
        newPath = filePath + ".barch";

        BarchImgCompressorDecompressor::eImgCompressionResult result = encodeBarchBitmapRef(filePath.toStdString(), newPath.toStdString());

        if (result != BarchImgCompressorDecompressor::comprResOk)
        {
            emit processingFinished(filePath, false, "Failed to encode BMP to BARCH.");
            return;
        }
    } else if (filePath.endsWith(".barch", Qt::CaseInsensitive))
    {
        newPath = filePath + ".bmp";

        BarchImgCompressorDecompressor::eImgCompressionResult result = decodeBarchBitmapRef(filePath.toStdString(), newPath.toStdString());

        if (result != BarchImgCompressorDecompressor::comprResOk)
        {
            emit processingFinished(filePath, false, "Failed to decode BARCH to BMP.");
            return;
        }
    } else {
        emit processingFinished(filePath, false, "Unknown file type.");
        return;
    }

    emit processingFinished(filePath, true, "Processing completed.");
}
