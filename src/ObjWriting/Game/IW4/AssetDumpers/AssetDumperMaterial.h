#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4/IW4.h"

namespace IW4
{
    class AssetDumperMaterial final : public AbstractAssetDumper<Material>
    {
    protected:
        bool ShouldDump(XAssetInfo<Material>* asset) override;
        void DumpAsset(AssetDumpingContext& context, XAssetInfo<Material>* asset) override;
    };
}
