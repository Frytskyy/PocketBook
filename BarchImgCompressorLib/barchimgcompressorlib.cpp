#include "barchimgcompressorlib.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <cstdint>

namespace BarchImgCompressorDecompressor {

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;          // BMP file signature, must be 0x4D42 ("BM")
    uint32_t bfSize;          // The size of this file, in bytes.
    uint16_t bfReserved1;     // Reserved for the dark side. Must be 0.
    uint16_t bfReserved2;     // Reserved for the light side. Must be 0.
    uint32_t bfOffBits;       // The distance from the beginning of the file to the actual bitmap data.
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;          // The size of this structure, in bytes.
    int32_t  biWidth;         // The width of the image, in pixels.
    int32_t  biHeight;        // The height of the image, in pixels.
    uint16_t biPlanes;        // The number of color planes in the image. Only 1 is allowed.
    uint16_t biBitCount;      // The number of bits per pixel. For grayscale, it's 8.
    uint32_t biCompression;   // The type of compression used. For uncompressed images, it's 0.
    uint32_t biSizeImage;     // The size of the image data, in bytes.
    int32_t  biXPelsPerMeter; // The horizontal resolution of the image, in pixels per meter. Ex: 11811 for 300 dpi (1 meter = 39.37 inches, and 1 inch = 2.54 centimeters, so 300 dpi = 300 pixels / 2.54 centimeters = 11811 pixels / meter)
    int32_t  biYPelsPerMeter; // The vertical resolution of the image, in pixels per meter. Ex: 11811 for 300 dpi
    uint32_t biClrUsed;       // The number of colors used in the image. For grayscale, it's 0.
    uint32_t biClrImportant;  // The number of important colors in the image. For grayscale, it's 0.
} BITMAPINFOHEADER;
#pragma pack(pop)

eImgCompressionResult readBMPFile(const std::string& filePath, Bitmap*& bitmap) {
    bitmap = nullptr;
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return comprResErrorDiskIO; // File not found, like Obi-Wan on Tatooine
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    // Read the BMP headers
    if (!file.read(reinterpret_cast<char*>(&fileHeader), sizeof(BITMAPFILEHEADER)) ||
        !file.read(reinterpret_cast<char*>(&infoHeader), sizeof(BITMAPINFOHEADER))) {
        return comprResErrorInvalidInputFileFormat;
    }

    // Check if it's a valid 8-bit grayscale BMP
    if (fileHeader.bfType != 0x4D42 || infoHeader.biBitCount != 8) {
        return comprResErrorInvalidInputFileFormat; // This is not the file format you're looking for
    }
    //to-do: we can also check for palette here, for grayscale all RGB should eb equal and sequental from 0 to 255, but yet let's presume all is ok as the time is short for this test task...

    // Extract image dimensions
    int width = infoHeader.biWidth;
    int height = std::abs(infoHeader.biHeight);  // Handle both top-down and bottom-up BMPs, therefore abs

    // Calculate row size and padding
    int rowSize = (width + 3) & (~3);
    int padding = rowSize - width;

    // Allocate memory for the bitmap data
    try {
        bitmap = new Bitmap;
        if (!bitmap) {
            return comprResErrorOutOfRAM;
        }
        bitmap->width = width;
        bitmap->height = height;
        bitmap->data = new unsigned char[width * height]; //note: result data in not compressed yet!
        if (!bitmap->data) {
            delete bitmap;
            bitmap = nullptr;
            return comprResErrorOutOfRAM;
        }
    } catch (const std::bad_alloc&) {
        if (bitmap) {
            delete bitmap;
            bitmap = nullptr;
        }
        return comprResErrorOutOfRAM;
    }

    // Seek to the start of pixel data
    file.seekg(fileHeader.bfOffBits, std::ios::beg);
    if (file.fail()) {
    diskReadErr:
        delete[] bitmap->data;
        delete bitmap;
        bitmap = nullptr;

        return comprResErrorDiskIO;
    }

    // Read the pixel data
    if (infoHeader.biHeight > 0) {
        // Bottom-up BMP
        for (int y = height - 1; y >= 0; --y) {
            if (!file.read(reinterpret_cast<char*>(bitmap->data + y * width), width)) {
                goto diskReadErr;
            }
            file.ignore(padding);
        }
    } else {
        // Top-down BMP
        for (int y = 0; y < height; ++y) {
            if (!file.read(reinterpret_cast<char*>(bitmap->data + y * width), width)) {
                goto diskReadErr;
            }
            file.ignore(padding);
        }
    }

    return comprResOk;
}

// Write BMP file like Luke firing proton torpedoes
eImgCompressionResult writeBMPFile(const std::string& filePath, const Bitmap* bitmap) {
    if (!bitmap || !bitmap->data) {
        return comprResErrorOther;
    }

    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        return comprResErrorDiskIO; // Failed to open file, like a jammed blaster
    }

    int rowSize = (bitmap->width + 3) & (~3);
    int padding = rowSize - bitmap->width;
    int fileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 1024 + rowSize * bitmap->height;

    // Prepare the BMP headers
    BITMAPFILEHEADER fileHeader;
    fileHeader.bfType = 0x4D42;  // 'BM'
    fileHeader.bfSize = fileSize;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 1024;

    BITMAPINFOHEADER infoHeader;
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = bitmap->width;
    infoHeader.biHeight = bitmap->height;  // Positive for bottom-up BMP
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 8;
    infoHeader.biCompression = 0;  // No compression
    infoHeader.biSizeImage = rowSize * bitmap->height;
    infoHeader.biXPelsPerMeter = 11811;  // let's assume it's 300 DPI, hopefully it's unimportant
    infoHeader.biYPelsPerMeter = 11811;  // let's assume it's 300 DPI, hopefully it's unimportant
    infoHeader.biClrUsed = 256;  // Use full 8-bit color palette only
    infoHeader.biClrImportant = 256;

    // Write the headers
    if (!file.write(reinterpret_cast<char*>(&fileHeader), sizeof(BITMAPFILEHEADER)) ||
        !file.write(reinterpret_cast<char*>(&infoHeader), sizeof(BITMAPINFOHEADER))) {
        return comprResErrorDiskIO;
    }

    // Write the grayscale color palette
    for (int i = 0; i < 256; ++i) {
        uint8_t gray = uint8_t(i);
        if (!file.write(reinterpret_cast<char*>(&gray), 1) ||
            !file.write(reinterpret_cast<char*>(&gray), 1) ||
            !file.write(reinterpret_cast<char*>(&gray), 1)) {
            return comprResErrorDiskIO;
        }
        uint8_t zero = 0;
        if (!file.write(reinterpret_cast<char*>(&zero), 1)) {
            return comprResErrorDiskIO;
        }
    }

    // Write the pixel data
    std::vector<uint8_t> paddingBytes(padding, 0);
    for (int y = bitmap->height - 1; y >= 0; --y) {
        if (!file.write(reinterpret_cast<const char*>(bitmap->data + y * bitmap->width), bitmap->width) ||
            !file.write(reinterpret_cast<char*>(paddingBytes.data()), padding)) {

            return comprResErrorDiskIO;
        }
    }

    return comprResOk;
}

// Function to check if a row is empty
bool isRowEmpty(const unsigned char* row, int width)
{
    for (int i = 0; i < width; i++) {
        if (row[i] != 0xff) {
            return false;
        }
    }
    return true;
}

std::vector<uint8_t> encodeBitmapToCompressedBarch(const Bitmap* bitmap) {
    std::vector<uint8_t> encodedData;
    std::vector<bool> rowIndex;

    int currentByte = 0;
    int bitCount = 0;

    auto pushBits = [&](int bits, int count)
    {
        currentByte = (currentByte << count) | bits;
        bitCount += count;
        while (bitCount >= 8)
        {
            encodedData.push_back(currentByte >> (bitCount - 8));
            bitCount -= 8;
        }
    };

    for (int y = 0; y < bitmap->height; ++y)
    {
        bool isEmptyRow = std::all_of(bitmap->data + y * bitmap->width,
                                      bitmap->data + (y + 1) * bitmap->width,
                                      [](uint8_t pixel) { return pixel == 0xFF; });
        rowIndex.push_back(!isEmptyRow);

        if (!isEmptyRow)
        {
            for (int x = 0; x < bitmap->width; x += 4)
            {
                uint8_t* pixels = bitmap->data + y * bitmap->width + x;
                int remainingPixels = std::min(4, bitmap->width - x);

                if (std::all_of(pixels, pixels + remainingPixels, [](uint8_t p) { return p == 0xFF; }))
                {
                    pushBits(0b0, 1);  // All white
                } else if (std::all_of(pixels, pixels + remainingPixels, [](uint8_t p) { return p == 0x00; }))
                {
                    pushBits(0b10, 2);  // All black
                } else
                {
                    pushBits(0b11, 2);  // Mixed
                    for (int i = 0; i < remainingPixels; ++i)
                    {
                        pushBits(pixels[i], 8);
                    }
                }
            }
        }
    }

    // Flush any remaining bits
    if (bitCount > 0) {
        encodedData.push_back(currentByte << (8 - bitCount));
    }

    // Prepend row index to encoded data
    std::vector<uint8_t> rowIndexBytes((rowIndex.size() + 7) / 8, 0);
    for (size_t i = 0; i < rowIndex.size(); ++i)
    {
        if (rowIndex[i])
        {
            rowIndexBytes[i / 8] |= (1 << (i % 8));
        }
    }

    // Prepare the final encoded data
    std::vector<uint8_t> finalEncodedData(sizeof(int) * 2);
    *reinterpret_cast<int*>(&finalEncodedData[0]) = bitmap->width;
    *reinterpret_cast<int*>(&finalEncodedData[sizeof(int)]) = bitmap->height;
    finalEncodedData.insert(finalEncodedData.end(), rowIndexBytes.begin(), rowIndexBytes.end());
    finalEncodedData.insert(finalEncodedData.end(), encodedData.begin(), encodedData.end());

    return finalEncodedData;
}

Bitmap* decodeBarchEncodedData(const std::vector<uint8_t>& encodedData) {
    if (encodedData.size() < 2) {
        return nullptr; // Not enough data to decode
    }

    int width = *reinterpret_cast<const int*>(&encodedData[0]);
    int height = *reinterpret_cast<const int*>(&encodedData[sizeof(int)]);

    Bitmap* bitmap = new Bitmap;
    bitmap->width = width;
    bitmap->height = height;
    bitmap->data = new unsigned char[width * height];

    std::vector<bool> rowIndex(height);
    int rowIndexByteCount = (height + 7) / 8;
    for (int i = 0; i < height; ++i) {
        rowIndex[i] = (encodedData[(sizeof(int) * 2) + i / 8] & (1 << (i % 8))) != 0;
    }

    size_t dataIndex = 2 + rowIndexByteCount;
    int bitIndex = 0;

    auto readBits = [&](int count) -> int
    {
        int result = 0;
        while (count > 0)
        {
            if (dataIndex >= encodedData.size())
            {
                throw std::runtime_error("Unexpected end of data");
            }
            int availableBits = 8 - bitIndex;
            int bitsToRead = std::min(count, availableBits);
            result = (result << bitsToRead) | ((encodedData[dataIndex] >> (availableBits - bitsToRead)) & ((1 << bitsToRead) - 1));
            bitIndex += bitsToRead;
            if (bitIndex == 8)
            {
                dataIndex++;
                bitIndex = 0;
            }
            count -= bitsToRead;
        }
        return result;
    };

    for (int y = 0; y < height; ++y)
    {
        if (rowIndex[y])
        {
            for (int x = 0; x < width; x += 4)
            {
                int code = readBits(1);
                int remainingPixels = std::min(4, width - x);

                if (code == 0b0)
                {
                    std::fill_n(bitmap->data + y * width + x, remainingPixels, 0xFF);
                } else
                {
                    int code2 = readBits(1);
                    if (code2 == 0b0) //full code pattern: 0b10 (four black pixels in a row)
                    {
                        std::fill_n(bitmap->data + y * width + x, remainingPixels, 0x00);
                    } else
                    {
                        for (int i = 0; i < remainingPixels; ++i)
                        {
                            bitmap->data[(y * width) + x + i] = readBits(8);
                        }
                    }
                }
            }
        } else
        {
            //full while row
            std::fill_n(bitmap->data + y * width, width, 0xFF);
        }
    }

    return bitmap;
}

void freeBitmapMemory(Bitmap* bitmap) {
    if (bitmap) {
        delete[] bitmap->data;
        delete bitmap;
    }
}

eImgCompressionResult encodeBarchBitmap(const std::string& inputBMPFilePath, const std::string& outBarchFilePath) {
    Bitmap* bitmap = nullptr;
    eImgCompressionResult fileReadResult = readBMPFile(inputBMPFilePath, bitmap);

    if (fileReadResult != comprResOk) {
        return fileReadResult;
    }

    std::vector<uint8_t> encodedData = encodeBitmapToCompressedBarch(bitmap);
    freeBitmapMemory(bitmap);

    std::ofstream outFile(outBarchFilePath, std::ios::binary);
    if (!outFile) {
        return comprResErrorDiskIO;
    }

    outFile.write(reinterpret_cast<char*>(encodedData.data()), encodedData.size());
    if (!outFile) {
        return comprResErrorDiskIO;
    }

    return comprResOk;
}

// Decode Barch to BMP like R2-D2 projecting a hologram
eImgCompressionResult decodeBarchBitmap(const std::string& inputBarchFilePath, const std::string& outBMPFilePath) {
    std::ifstream inFile(inputBarchFilePath, std::ios::binary);
    if (!inFile) {
        return comprResErrorDiskIO;
    }

    std::vector<uint8_t> encodedData((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    if (encodedData.empty()) {
        return comprResErrorInvalidInputFileFormat;
    }

    Bitmap* bitmap = decodeBarchEncodedData(encodedData);
    if (!bitmap) {
        return comprResErrorInvalidInputFileFormat;
    }

    eImgCompressionResult writeResult = writeBMPFile(outBMPFilePath, bitmap);
    freeBitmapMemory(bitmap);

    return writeResult;
}

} //end of namespace BarchImgCompressorDecompressor

