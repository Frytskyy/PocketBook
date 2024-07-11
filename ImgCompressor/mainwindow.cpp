#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../BarchImgCompressorLib/barchimgcompressorlib.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QLibrary>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_selectButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open BMP Image"), "", tr("BMP Files (*.bmp)"));

    if (!filePath.isEmpty()) {
        convertImage(filePath);
    }
}

using namespace BarchImgCompressorDecompressor;

typedef eImgCompressionResult (*encodeBarchBitmapFunc)(const std::string&, const std::string&);
typedef eImgCompressionResult (*decodeBarchBitmapFunc)(const std::string&, const std::string&);

void MainWindow::convertImage(const QString &filePath)
{
    QLibrary barchLib(tr("..\\..\\..\\..\\BarchImgCompressorLib\\build\\Desktop_Qt_6_7_2_MinGW_64_bit-Release\\release\\BarchImgCompressorLib.dll"));
    if (!barchLib.load()) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot load Barch library."));
        return;
    }

    encodeBarchBitmapFunc encodeBarchBitmapRef = (encodeBarchBitmapFunc)barchLib.resolve("encodeBarchBitmap"); //get func address
    decodeBarchBitmapFunc decodeBarchBitmapRef = (decodeBarchBitmapFunc)barchLib.resolve("decodeBarchBitmap"); //get func address

    QImage image(filePath);
    if (image.isNull()) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot load image."));
        return;
    }

    QString compressedBarchPath = filePath;

    compressedBarchPath.replace(".bmp", ".barch");

/*
    savePath.replace(".bmp", ".png");

    if (image.save(savePath, "PNG")) {
        QMessageBox::information(this, tr("Success"),
                                 tr("Image converted and saved as: ") + savePath);

        ui->imageLabel->setPixmap(QPixmap::fromImage(image).scaled(
            ui->imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Cannot save image."));
    }
*/

    eImgCompressionResult conversionResult = encodeBarchBitmapRef(filePath.toStdString(), compressedBarchPath.toStdString());
    if (conversionResult == comprResOk) {
        QMessageBox::information(this, tr("Success"),
                                 tr("Image compressed to Barch format and saved as: ") + compressedBarchPath);

        //display the picture
        ui->imageLabel->setPixmap(QPixmap::fromImage(image).scaled(
            ui->imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

        //decode back from barch to .bmp format
        QString decompressedBMPFromBarchPath = filePath;

        decompressedBMPFromBarchPath.replace(".bmp", "_reconverted.bmp");
        eImgCompressionResult conversionResult = decodeBarchBitmapRef(compressedBarchPath.toStdString(), decompressedBMPFromBarchPath.toStdString());
        if (conversionResult == comprResOk) {
            QMessageBox::information(this, tr("Success"),
                                     tr("Backward Barch image decompression to BMP is completed: ") + decompressedBMPFromBarchPath);
        } else {
            QString errorMessage;
            switch (conversionResult) {
            case comprResErrorInvalidInputFileFormat:
                errorMessage = tr("Invalid Barch file format. Normaly should not happen...");
                break;
            case comprResErrorOutOfRAM:
                errorMessage = tr("Out of memory. Cannot unpack Barch to check how it works. Please close some applications and try again.");
                break;
            case comprResErrorDiskIO:
                errorMessage = tr("Disk I/O error. Cannot unpack Barch to check how it works. Please check the disk and try again.");
                break;
            default:
                errorMessage = tr("An unknown error occurred. Cannot unpack Barch to check how it works. Ttry again.");
                break;
            }
            QMessageBox::warning(this, tr("Error"), errorMessage);
        }
    } else {
        QString errorMessage;
        switch (conversionResult) {
        case comprResErrorInvalidInputFileFormat:
            errorMessage = tr("Invalid input file format. Please select a valid BMP file.");
            break;
        case comprResErrorOutOfRAM:
            errorMessage = tr("Out of memory. Please close some applications and try again.");
            break;
        case comprResErrorDiskIO:
            errorMessage = tr("Disk I/O error. Please check the disk and try again.");
            break;
        default:
            errorMessage = tr("An unknown error occurred. Ttry again.");
            break;
        }
        QMessageBox::warning(this, tr("Error"), errorMessage);
    }
}
