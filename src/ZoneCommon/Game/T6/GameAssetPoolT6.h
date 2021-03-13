#pragma once

#include "Pool/ZoneAssetPools.h"
#include "Pool/AssetPool.h"
#include "Game/T6/T6.h"

class GameAssetPoolT6 final : public ZoneAssetPools
{
    int m_priority;

    static constexpr const char* ASSET_TYPE_INVALID = "invalid_asset_type";
    static const char* ASSET_TYPE_NAMES[];

protected:
    XAssetInfoGeneric* AddAssetToPool(asset_type_t type, std::string name, void* asset, std::vector<XAssetInfoGeneric*>& dependencies) override;

public:
    AssetPool<T6::PhysPreset>* m_phys_preset;
    AssetPool<T6::PhysConstraints>* m_phys_constraints;
    AssetPool<T6::DestructibleDef>* m_destructible_def;
    AssetPool<T6::XAnimParts>* m_xanim_parts;
    AssetPool<T6::XModel>* m_xmodel;
    AssetPool<T6::Material>* m_material;
    AssetPool<T6::MaterialTechniqueSet>* m_technique_set;
    AssetPool<T6::GfxImage>* m_image;
    AssetPool<T6::SndBank>* m_sound_bank;
    AssetPool<T6::SndPatch>* m_sound_patch;
    AssetPool<T6::clipMap_t>* m_clip_map;
    AssetPool<T6::ComWorld>* m_com_world;
    AssetPool<T6::GameWorldSp>* m_game_world_sp;
    AssetPool<T6::GameWorldMp>* m_game_world_mp;
    AssetPool<T6::MapEnts>* m_map_ents;
    AssetPool<T6::GfxWorld>* m_gfx_world;
    AssetPool<T6::GfxLightDef>* m_gfx_light_def;
    AssetPool<T6::Font_s>* m_font;
    AssetPool<T6::FontIcon>* m_font_icon;
    AssetPool<T6::MenuList>* m_menu_list;
    AssetPool<T6::menuDef_t>* m_menu_def;
    AssetPool<T6::LocalizeEntry>* m_localize;
    AssetPool<T6::WeaponVariantDef>* m_weapon;
    AssetPool<T6::WeaponAttachment>* m_attachment;
    AssetPool<T6::WeaponAttachmentUnique>* m_attachment_unique;
    AssetPool<T6::WeaponCamo>* m_camo;
    AssetPool<T6::SndDriverGlobals>* m_snd_driver_globals;
    AssetPool<T6::FxEffectDef>* m_fx;
    AssetPool<T6::FxImpactTable>* m_fx_impact_table;
    AssetPool<T6::RawFile>* m_raw_file;
    AssetPool<T6::StringTable>* m_string_table;
    AssetPool<T6::LeaderboardDef>* m_leaderboard;
    AssetPool<T6::XGlobals>* m_xglobals;
    AssetPool<T6::ddlRoot_t>* m_ddl;
    AssetPool<T6::Glasses>* m_glasses;
    AssetPool<T6::EmblemSet>* m_emblem_set;
    AssetPool<T6::ScriptParseTree>* m_script;
    AssetPool<T6::KeyValuePairs>* m_key_value_pairs;
    AssetPool<T6::VehicleDef>* m_vehicle;
    AssetPool<T6::MemoryBlock>* m_memory_block;
    AssetPool<T6::AddonMapEnts>* m_addon_map_ents;
    AssetPool<T6::TracerDef>* m_tracer;
    AssetPool<T6::SkinnedVertsDef>* m_skinned_verts;
    AssetPool<T6::Qdb>* m_qdb;
    AssetPool<T6::Slug>* m_slug;
    AssetPool<T6::FootstepTableDef>* m_footstep_table;
    AssetPool<T6::FootstepFXTableDef>* m_footstep_fx_table;
    AssetPool<T6::ZBarrierDef>* m_zbarrier;

    GameAssetPoolT6(Zone* zone, int priority);
    ~GameAssetPoolT6() override;

    void InitPoolStatic(asset_type_t type, size_t capacity) override;
    void InitPoolDynamic(asset_type_t type) override;

    XAssetInfoGeneric* GetAsset(asset_type_t type, std::string name) const override;
    static const char* AssetTypeNameByType(asset_type_t assetType);
    const char* GetAssetTypeName(asset_type_t assetType) const override;
};
