#ifndef BARCHIMGCOMPRESSORLIB_H
#define BARCHIMGCOMPRESSORLIB_H

#include "BarchImgCompressorLib_global.h"
#include <string>
#include <vector>
#include <cstdint>

extern "C" {
namespace BarchImgCompressorDecompressor {

    // Enum for compression result
    enum eImgCompressionResult {
        comprResOk = 0,                      // The Force is strong with this one
        comprResErrorInvalidInputFileFormat, // This is not the format you're looking for
        comprResErrorOutOfRAM,               // Memory shortage, you must have
        comprResErrorDiskIO,                 // The dark side of the disk, this is
        comprResErrorOther                   // I've got a bad feeling about this
    };

    struct Bitmap {
        int width;              // Width of the image
        int height;             // Height of the image
        unsigned char* data;    // compressed pixels
    };

    Q_DECL_EXPORT eImgCompressionResult encodeBarchBitmap(const std::string& inputBMPFilePath, const std::string& outBarchFilePath);
    Q_DECL_EXPORT eImgCompressionResult decodeBarchBitmap(const std::string& inputBarchFilePath, const std::string& outBMPFilePath);

    // Internal functions - for Jedi eyes only [but still, can be used by Sith Lords as they know what they do]
    Q_DECL_EXPORT eImgCompressionResult readBMPFile(const std::string& filePath, Bitmap*& bitmap);
    Q_DECL_EXPORT eImgCompressionResult writeBMPFile(const std::string& filePath, const Bitmap* bitmap);
    Q_DECL_EXPORT std::vector<uint8_t> encodeRebelTransmission(const Bitmap* bitmap);
    Q_DECL_EXPORT Bitmap* decodeImperialIntercept(const std::vector<uint8_t>& encodedData);
    Q_DECL_EXPORT void freeBitmapMemory(Bitmap* bitmap);

} //end of namespace BarchImgCompressorDecompressor
}

#endif // BARCHIMGCOMPRESSORLIB_H
