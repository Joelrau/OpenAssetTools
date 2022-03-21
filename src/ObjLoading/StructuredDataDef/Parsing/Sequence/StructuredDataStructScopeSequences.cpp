#include "StructuredDataStructScopeSequences.h"

#include <algorithm>

#include "Parsing/Simple/Matcher/SimpleMatcherFactory.h"

namespace sdd::struct_scope_sequences
{
    class SequenceStructEntry final : public StructuredDataDefParser::sequence_t
    {
        static constexpr auto TAG_TYPE_INT = 1;
        static constexpr auto TAG_TYPE_BYTE = 2;
        static constexpr auto TAG_TYPE_BOOL = 3;
        static constexpr auto TAG_TYPE_FLOAT = 4;
        static constexpr auto TAG_TYPE_SHORT = 5;
        static constexpr auto TAG_TYPE_STRING = 6;
        static constexpr auto TAG_TYPE_NAMED = 7;

        static constexpr auto CAPTURE_STRING_LENGTH = 1;
        static constexpr auto CAPTURE_TYPE_NAME = 2;
        static constexpr auto CAPTURE_ENTRY_NAME = 3;
        static constexpr auto CAPTURE_ARRAY_SIZE = 4;

        static std::unique_ptr<matcher_t> TypeMatchers(const SimpleMatcherFactory& create)
        {
            return create.Or({
                create.Keyword("int").Tag(TAG_TYPE_INT),
                create.Keyword("byte").Tag(TAG_TYPE_BYTE),
                create.Keyword("bool").Tag(TAG_TYPE_BOOL),
                create.Keyword("float").Tag(TAG_TYPE_FLOAT),
                create.Keyword("short").Tag(TAG_TYPE_SHORT),
                create.And({
                    create.Keyword("string"),
                    create.Char('('),
                    create.Integer().Capture(CAPTURE_STRING_LENGTH),
                    create.Char(')')
                }).Tag(TAG_TYPE_STRING),
                create.Identifier().Tag(TAG_TYPE_NAMED).Capture(CAPTURE_TYPE_NAME)
            });
        }

        static std::unique_ptr<matcher_t> ArrayMatchers(const SimpleMatcherFactory& create)
        {
            return create.And({
                create.Char('['),
                create.Or({
                    create.Integer().Capture(CAPTURE_ARRAY_SIZE),
                    create.Identifier().Capture(CAPTURE_ARRAY_SIZE)
                }),
                create.Char(']')
            });
        }

    public:
        SequenceStructEntry()
        {
            const SimpleMatcherFactory create(this);

            AddMatchers({
                TypeMatchers(create),
                create.Identifier().Capture(CAPTURE_ENTRY_NAME),
                create.OptionalLoop(ArrayMatchers(create)),
                create.Char(';')
            });
        }

    private:
        static CommonStructuredDataType ProcessType(StructuredDataDefParserState* state, SequenceResult<SimpleParserValue>& result, size_t& currentSize, size_t& currentBitAlign)
        {
            const auto typeTag = result.NextTag();

            switch (typeTag)
            {
            case TAG_TYPE_INT:
                currentSize = 32;
                currentBitAlign = 8;
                return CommonStructuredDataType(CommonStructuredDataTypeCategory::INT);
            case TAG_TYPE_BYTE:
                currentSize = 8;
                currentBitAlign = 8;
                return CommonStructuredDataType(CommonStructuredDataTypeCategory::BYTE);
            case TAG_TYPE_BOOL:
                currentSize = 1;
                currentBitAlign = 0;
                return CommonStructuredDataType(CommonStructuredDataTypeCategory::BOOL);
            case TAG_TYPE_FLOAT:
                currentSize = 32;
                currentBitAlign = 8;
                return CommonStructuredDataType(CommonStructuredDataTypeCategory::FLOAT);
            case TAG_TYPE_SHORT:
                currentSize = 16;
                currentBitAlign = 8;
                return CommonStructuredDataType(CommonStructuredDataTypeCategory::SHORT);
            case TAG_TYPE_STRING:
                {
                    currentBitAlign = 8;
                    const auto& stringLengthToken = result.NextCapture(CAPTURE_STRING_LENGTH);
                    const auto stringLength = stringLengthToken.IntegerValue();

                    if (stringLength <= 0)
                        throw ParsingException(stringLengthToken.GetPos(), "String length must be greater than zero");

                    currentSize = stringLength * 8;
                    return {CommonStructuredDataTypeCategory::STRING, static_cast<size_t>(stringLength)};
                }
            case TAG_TYPE_NAMED:
                {
                    currentBitAlign = 8;
                    const auto& typeNameToken = result.NextCapture(CAPTURE_TYPE_NAME);
                    const auto typeName = typeNameToken.IdentifierValue();

                    const auto existingType = state->m_def_types_by_name.find(typeName);
                    if (existingType == state->m_def_types_by_name.end())
                        throw ParsingException(typeNameToken.GetPos(), "No type defined under this name");

                    if (existingType->second.m_category == CommonStructuredDataTypeCategory::STRUCT)
                    {
                        assert(existingType->second.m_info.type_index < state->m_current_def->m_structs.size());
                        const auto* _struct = state->m_current_def->m_structs[existingType->second.m_info.type_index].get();
                        currentSize = _struct->m_size_in_byte * 8;
                    }
                    else if (existingType->second.m_category == CommonStructuredDataTypeCategory::ENUM)
                    {
                        assert(existingType->second.m_info.type_index < state->m_current_def->m_enums.size());
                        currentSize = 16;
                    }
                    else
                    {
                        assert(false);
                        currentSize = 0;
                    }

                    return existingType->second;
                }
            default:
                throw ParsingException(TokenPos(), "Invalid Tag for Type @ ProcessType!!!");
            }
        }

        static CommonStructuredDataType ProcessArray(StructuredDataDefParserState* state, const SimpleParserValue& arrayToken, const CommonStructuredDataType currentType,
                                                     size_t& currentSize, size_t& currentBitAlign)
        {
            currentBitAlign = 8;

            if (arrayToken.m_type == SimpleParserValueType::INTEGER)
            {
                const auto arrayElementCount = arrayToken.IntegerValue();

                if (arrayElementCount <= 0)
                    throw ParsingException(arrayToken.GetPos(), "Array size must be greater than zero");

                currentSize *= arrayElementCount;

                const CommonStructuredDataIndexedArray indexedArray(currentType, arrayElementCount);

                const auto existingIndexedArray = state->m_def_indexed_arrays.find(indexedArray);
                if (existingIndexedArray != state->m_def_indexed_arrays.end())
                    return {CommonStructuredDataTypeCategory::INDEXED_ARRAY, existingIndexedArray->second};

                const auto newIndexedArrayIndex = state->m_current_def->m_indexed_arrays.size();
                state->m_current_def->m_indexed_arrays.push_back(indexedArray);
                return {CommonStructuredDataTypeCategory::INDEXED_ARRAY, newIndexedArrayIndex};
            }

            if (arrayToken.m_type == SimpleParserValueType::IDENTIFIER)
            {
                const auto enumName = arrayToken.IdentifierValue();

                const auto existingType = state->m_def_types_by_name.find(enumName);
                if (existingType == state->m_def_types_by_name.end())
                    throw ParsingException(arrayToken.GetPos(), "No type defined under this name");
                if (existingType->second.m_category != CommonStructuredDataTypeCategory::ENUM)
                    throw ParsingException(arrayToken.GetPos(), "Type for enumed array must be an enum");

                assert(existingType->second.m_info.type_index < state->m_current_def->m_enums.size());
                const auto* _enum = state->m_current_def->m_enums[existingType->second.m_info.type_index].get();

                const auto enumElementCount = _enum->ElementCount();
                assert(enumElementCount > 0);
                currentSize *= enumElementCount;

                const CommonStructuredDataEnumedArray enumedArray(currentType, existingType->second.m_info.type_index);

                const auto existingEnumedArray = state->m_def_enumed_arrays.find(enumedArray);
                if (existingEnumedArray != state->m_def_enumed_arrays.end())
                    return {CommonStructuredDataTypeCategory::ENUM_ARRAY, existingEnumedArray->second};

                const auto newEnumedArrayIndex = state->m_current_def->m_enumed_arrays.size();
                state->m_current_def->m_enumed_arrays.push_back(enumedArray);
                return {CommonStructuredDataTypeCategory::ENUM_ARRAY, newEnumedArrayIndex};
            }

            throw ParsingException(arrayToken.GetPos(), "Invalid Token for Array @ ProcessArray!!!");
        }

    protected:
        void ProcessMatch(StructuredDataDefParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            assert(state->m_current_def != nullptr);
            assert(state->m_current_struct != nullptr);

            size_t currentSize = 0;
            size_t currentAlign = 0;
            auto currentType = ProcessType(state, result, currentSize, currentAlign);

            std::vector<std::reference_wrapper<const SimpleParserValue>> arrayTokens;
            while (result.HasNextCapture(CAPTURE_ARRAY_SIZE))
                arrayTokens.emplace_back(result.NextCapture(CAPTURE_ARRAY_SIZE));

            for (auto i = arrayTokens.rbegin(); i != arrayTokens.rend(); ++i)
                currentType = ProcessArray(state, i->get(), currentType, currentSize, currentAlign);

            if (currentAlign > 0)
                state->m_current_struct_offset_in_bits = (state->m_current_struct_offset_in_bits + currentAlign - 1) / currentAlign * currentAlign;

            state->m_current_struct->m_properties.emplace_back(result.NextCapture(CAPTURE_ENTRY_NAME).IdentifierValue(), currentType, state->m_current_struct_offset_in_bits);
            state->m_current_struct_offset_in_bits += currentSize;
        }
    };

    class SequencePadding final : public StructuredDataDefParser::sequence_t
    {
        static constexpr auto CAPTURE_PADDING_VALUE = 1;

    public:
        SequencePadding()
        {
            const SimpleMatcherFactory create(this);

            AddMatchers({
                create.Keyword("pad"),
                create.Char('('),
                create.Integer().Capture(CAPTURE_PADDING_VALUE),
                create.Char(')'),
                create.Char(';')
            });
        }

    protected:
        void ProcessMatch(StructuredDataDefParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            const auto& paddingValueToken = result.NextCapture(CAPTURE_PADDING_VALUE);
            const auto paddingValue = paddingValueToken.IntegerValue();

            if (paddingValue <= 0)
                throw ParsingException(paddingValueToken.GetPos(), "Padding value must be greater than 0");

            // Align to next byte
            state->m_current_struct_offset_in_bits = (state->m_current_struct_offset_in_bits + 7) / 8 * 8;

            // Add padding value to current size
            state->m_current_struct_offset_in_bits += static_cast<size_t>(paddingValue);
        }
    };

    class SequenceCloseStruct final : public StructuredDataDefParser::sequence_t
    {
    public:
        SequenceCloseStruct()
        {
            const SimpleMatcherFactory create(this);

            AddMatchers({
                create.Char('}'),
                create.Optional(create.Char(';'))
            });
        }

    protected:
        void ProcessMatch(StructuredDataDefParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            assert(state->m_current_struct != nullptr);

            // Set the size of the finalized struct
            if (!state->m_current_struct->m_properties.empty())
                state->m_current_struct->m_size_in_byte = (state->m_current_struct_offset_in_bits + 7) / 8;
            else
                state->m_current_struct->m_size_in_byte = 0u;

            state->m_current_struct_offset_in_bits = 0u;

            // Sort the entries of the struct alphabetically
            std::sort(state->m_current_struct->m_properties.begin(), state->m_current_struct->m_properties.end(),
                      [](const CommonStructuredDataStructProperty& e1, const CommonStructuredDataStructProperty& e2)
                      {
                          return e1.m_name < e2.m_name;
                      });
            state->m_current_struct = nullptr;
        }
    };
}

using namespace sdd;
using namespace struct_scope_sequences;

StructuredDataStructScopeSequences::StructuredDataStructScopeSequences(std::vector<std::unique_ptr<StructuredDataDefParser::sequence_t>>& allSequences,
                                                                       std::vector<StructuredDataDefParser::sequence_t*>& scopeSequences)
    : AbstractScopeSequenceHolder(allSequences, scopeSequences)
{
}

void StructuredDataStructScopeSequences::AddSequences() const
{
    AddSequence(std::make_unique<SequenceCloseStruct>());
    AddSequence(std::make_unique<SequencePadding>());
    AddSequence(std::make_unique<SequenceStructEntry>());
}