#pragma once

#include <map>

#include "Persistence/IDataRepository.h"

class InMemoryRepository final : public IDataRepository
{
    std::vector<EnumDefinition*> m_enums;
    std::vector<StructDefinition*> m_structs;
    std::vector<UnionDefinition*> m_unions;
    std::vector<TypedefDefinition*> m_typedefs;
    std::vector<StructureInformation*> m_structures_information;
    std::vector<const FastFileBlock*> m_fast_file_blocks;
    std::map<std::string, DataDefinition*> m_data_definitions_by_name;
    std::map<std::string, EnumMember*> m_enum_members_by_name;
    std::map<std::string, const FastFileBlock*> m_fast_file_blocks_by_name;
    std::map<const DefinitionWithMembers*, StructureInformation*> m_structure_information_by_definition;
    std::string m_game_name;
    Architecture m_architecture;

public:
    InMemoryRepository();
    ~InMemoryRepository() override;
    InMemoryRepository(const InMemoryRepository& other) = delete;
    InMemoryRepository(InMemoryRepository&& other) noexcept = default;
    InMemoryRepository& operator=(const InMemoryRepository& other) = delete;
    InMemoryRepository& operator=(InMemoryRepository&& other) noexcept = default;

    void Add(std::unique_ptr<EnumDefinition> enumsDefinition) override;
    void Add(std::unique_ptr<StructDefinition> structDefinition) override;
    void Add(std::unique_ptr<UnionDefinition> unionDefinition) override;
    void Add(std::unique_ptr<TypedefDefinition> typedefDefinition) override;
    void Add(std::unique_ptr<StructureInformation> structureInformation) override;
    void Add(std::unique_ptr<FastFileBlock> fastFileBlock) override;

    _NODISCARD const std::string& GetGameName() const override;
    void SetGame(std::string gameName) override;
    _NODISCARD Architecture GetArchitecture() const override;
    void SetArchitecture(Architecture architecture) override;

    _NODISCARD const std::vector<EnumDefinition*>& GetAllEnums() const override;
    _NODISCARD const std::vector<StructDefinition*>& GetAllStructs() const override;
    _NODISCARD const std::vector<UnionDefinition*>& GetAllUnions() const override;
    _NODISCARD const std::vector<TypedefDefinition*>& GetAllTypedefs() const override;
    _NODISCARD const std::vector<StructureInformation*>& GetAllStructureInformation() const override;
    _NODISCARD const std::vector<const FastFileBlock*>& GetAllFastFileBlocks() const override;

    _NODISCARD DataDefinition* GetDataDefinitionByName(const std::string& name) const override;
    _NODISCARD StructureInformation* GetInformationFor(const DefinitionWithMembers* definitionWithMembers) const override;
    _NODISCARD EnumMember* GetEnumMemberByName(const std::string& name) const override;
    _NODISCARD const FastFileBlock* GetFastFileBlockByName(const std::string& name) const override;
};
