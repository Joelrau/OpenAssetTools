#pragma once

#include "AssetPool.h"
#include "GlobalAssetPool.h"
#include "XAssetInfo.h"

#include <cstring>

template <typename T>
class AssetPoolDynamic final : public AssetPool<T>
{
    using AssetPool<T>::m_asset_lookup;

    std::vector<XAssetInfo<T>*> m_assets;
    asset_type_t m_type;

public:
    AssetPoolDynamic(const int priority, const asset_type_t type)
    {
        GlobalAssetPool<T>::LinkAssetPool(this, priority);
        m_type = type;
    }

    AssetPoolDynamic(AssetPoolDynamic<T>&) = delete;
    AssetPoolDynamic(AssetPoolDynamic<T>&&) = delete;
    AssetPoolDynamic<T>& operator =(AssetPoolDynamic<T>&) = delete;
    AssetPoolDynamic<T>& operator =(AssetPoolDynamic<T>&&) = default;

    ~AssetPoolDynamic() override
    {
        GlobalAssetPool<T>::UnlinkAssetPool(this);

        for(auto* entry : m_assets)
        {
            delete entry->Asset();
            delete entry;
        }

        m_assets.clear();
        m_asset_lookup.clear();
    }

    XAssetInfo<T>* AddAsset(std::string name, T* asset, Zone* zone, std::vector<XAssetInfoGeneric*> dependencies, std::vector<scr_string_t> usedScriptStrings) override
    {
        auto* newInfo = new XAssetInfo<T>();
        newInfo->m_type = m_type;
        newInfo->m_name = std::move(name);
        newInfo->m_zone = zone;
        newInfo->m_dependencies = std::move(dependencies);
        newInfo->m_used_script_strings = std::move(usedScriptStrings);

        T* newAsset = new T();
        memcpy(newAsset, asset, sizeof(T));
        newInfo->m_ptr = newAsset;
        
        m_assets.push_back(newInfo);
        m_asset_lookup[newInfo->m_name] = newInfo;
       
        GlobalAssetPool<T>::LinkAsset(this, newInfo);

        return newInfo;
    }
};
