#pragma once

#include <string>
#include <vector>

namespace techset
{
    enum class ShaderSelector
    {
        VERTEX_SHADER,
        PIXEL_SHADER
    };

    class ShaderArgument
    {
    public:
        std::string m_argument_name;
        bool m_argument_index_specified;
        size_t m_argument_index;

        ShaderArgument();
        explicit ShaderArgument(std::string argumentName);
        ShaderArgument(std::string argumentName, size_t argumentIndex);
    };

    class ShaderArgumentCodeSource
    {
    public:
        std::vector<std::string> m_accessors;
        bool m_index_accessor_specified;
        size_t m_index_accessor;

        ShaderArgumentCodeSource();
        explicit ShaderArgumentCodeSource(std::vector<std::string> accessors);
        ShaderArgumentCodeSource(std::vector<std::string> accessors, size_t indexAccessor);
    };

    class ShaderArgumentLiteralSource
    {
    public:
        float m_value[4];

        ShaderArgumentLiteralSource();
        ShaderArgumentLiteralSource(float v0, float v1, float v2, float v3);
        explicit ShaderArgumentLiteralSource(float value[4]);

        friend bool operator<(const ShaderArgumentLiteralSource& lhs, const ShaderArgumentLiteralSource& rhs);
        friend bool operator<=(const ShaderArgumentLiteralSource& lhs, const ShaderArgumentLiteralSource& rhs);
        friend bool operator>(const ShaderArgumentLiteralSource& lhs, const ShaderArgumentLiteralSource& rhs);
        friend bool operator>=(const ShaderArgumentLiteralSource& lhs, const ShaderArgumentLiteralSource& rhs);
    };

    class ShaderArgumentMaterialSource
    {
    public:
        bool m_is_hash;
        size_t m_hash;
        std::string m_name;

        ShaderArgumentMaterialSource();
        explicit ShaderArgumentMaterialSource(size_t hash);
        explicit ShaderArgumentMaterialSource(std::string name);
    };

    class ITechniqueDefinitionAcceptor
    {
    protected:
        ITechniqueDefinitionAcceptor() = default;
    public:
        virtual ~ITechniqueDefinitionAcceptor() = default;
        ITechniqueDefinitionAcceptor(const ITechniqueDefinitionAcceptor& other) = default;
        ITechniqueDefinitionAcceptor(ITechniqueDefinitionAcceptor&& other) noexcept = default;
        ITechniqueDefinitionAcceptor& operator=(const ITechniqueDefinitionAcceptor& other) = default;
        ITechniqueDefinitionAcceptor& operator=(ITechniqueDefinitionAcceptor&& other) noexcept = default;

        virtual void AcceptNextPass() = 0;
        virtual bool AcceptEndPass(std::string& errorMessage) = 0;

        virtual bool AcceptStateMap(const std::string& stateMapName, std::string& errorMessage) = 0;

        virtual bool AcceptVertexShader(const std::string& vertexShaderName, std::string& errorMessage) = 0;
        virtual bool AcceptPixelShader(const std::string& pixelShaderName, std::string& errorMessage) = 0;

        virtual bool AcceptShaderConstantArgument(ShaderSelector shader, ShaderArgument shaderArgument, ShaderArgumentCodeSource source, std::string& errorMessage) = 0;
        virtual bool AcceptShaderSamplerArgument(ShaderSelector shader, ShaderArgument shaderArgument, ShaderArgumentCodeSource source, std::string& errorMessage) = 0;
        virtual bool AcceptShaderLiteralArgument(ShaderSelector shader, ShaderArgument shaderArgument, ShaderArgumentLiteralSource source, std::string& errorMessage) = 0;
        virtual bool AcceptShaderMaterialArgument(ShaderSelector shader, ShaderArgument shaderArgument, ShaderArgumentMaterialSource source, std::string& errorMessage) = 0;

        virtual bool AcceptVertexStreamRouting(const std::string& destination, const std::string& source, std::string& errorMessage) = 0;
    };
}
