#pragma once

#include <cstdint>

struct IwiVersion
{
    char tag[3];
    char version;
};

// IW4
namespace iwi8
{
    struct IwiHeader
    {
        uint32_t flags;
        int8_t format;
        int8_t unused;
        uint16_t dimensions[3];
        uint32_t fileSizeForPicmip[4];
    };
}

// T5
namespace iwi13
{
    struct IwiHeader
    {
        int8_t format;
        int8_t flags;
        uint16_t dimensions[3];
        float gamma;
        uint32_t fileSizeForPicmip[8];
    };
}

// T6
namespace iwi27
{
    struct IwiHeader
    {
        int8_t format;
        int8_t flags;
        uint16_t dimensions[3];
        float gamma;
        int8_t maxGlossForMip[16];
        uint32_t fileSizeForPicmip[8];
    };

    enum class IwiFormat
    {
        IMG_FORMAT_INVALID = 0x0,
        IMG_FORMAT_BITMAP_RGBA = 0x1,
        IMG_FORMAT_BITMAP_RGB = 0x2,
        IMG_FORMAT_BITMAP_LUMINANCE_ALPHA = 0x3,
        IMG_FORMAT_BITMAP_LUMINANCE = 0x4,
        IMG_FORMAT_BITMAP_ALPHA = 0x5,
        IMG_FORMAT_WAVELET_RGBA = 0x6,
        IMG_FORMAT_WAVELET_RGB = 0x7,
        IMG_FORMAT_WAVELET_LUMINANCE_ALPHA = 0x8,
        IMG_FORMAT_WAVELET_LUMINANCE = 0x9,
        IMG_FORMAT_WAVELET_ALPHA = 0xA,
        IMG_FORMAT_DXT1 = 0xB,
        IMG_FORMAT_DXT3 = 0xC,
        IMG_FORMAT_DXT5 = 0xD,
        IMG_FORMAT_DXN = 0xE,
        IMG_FORMAT_BITMAP_RGB565 = 0xF,
        IMG_FORMAT_BITMAP_RGB5A3 = 0x10,
        IMG_FORMAT_BITMAP_C8 = 0x11,
        IMG_FORMAT_BITMAP_RGBA8 = 0x12,
        IMG_FORMAT_A16B16G16R16F = 0x13,
        IMG_FORMAT_COUNT,
    };

    enum IwiFlags
    {
        IMG_FLAG_NOPICMIP = 1 << 0,
        IMG_FLAG_NOMIPMAPS = 1 << 1,
        IMG_FLAG_CUBEMAP = 1 << 2,
        IMG_FLAG_VOLMAP = 1 << 3,
        IMG_FLAG_STREAMING = 1 << 4,
        IMG_FLAG_CLAMP_U = 1 << 6,
        IMG_FLAG_CLAMP_V = 1 << 7,
        IMG_FLAG_FORCE_SYSTEM = 1 << 8,
        IMG_FLAG_DYNAMIC = 1 << 16,
        IMG_FLAG_RENDER_TARGET = 1 << 17,
        IMG_FLAG_MULTISAMPLE = 1 << 18,
    };

}