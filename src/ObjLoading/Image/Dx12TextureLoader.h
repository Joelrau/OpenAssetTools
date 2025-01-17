#pragma once

#include <unordered_map>

#include "Utils/ClassUtils.h"
#include "Image/DxgiFormat.h"
#include "Utils/MemoryManager.h"
#include "Image/Texture.h"

class Dx12TextureLoader
{
    static std::unordered_map<ImageFormatId, ImageFormatId> m_conversion_table;

    MemoryManager* m_memory_manager;
    DXGI_FORMAT m_format;
    TextureType m_type;
    bool m_has_mip_maps;
    size_t m_width;
    size_t m_height;
    size_t m_depth;

    _NODISCARD const ImageFormat* GetFormatForDx12Format() const;

public:
    explicit Dx12TextureLoader(MemoryManager* memoryManager);

    Dx12TextureLoader& Format(DXGI_FORMAT format);
    Dx12TextureLoader& Type(TextureType textureType);
    Dx12TextureLoader& HasMipMaps(bool hasMipMaps);
    Dx12TextureLoader& Width(size_t width);
    Dx12TextureLoader& Height(size_t height);
    Dx12TextureLoader& Depth(size_t depth);

    Texture* LoadTexture(const void* data);
};
