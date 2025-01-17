#include "EventHandlerSetScopeSequences.h"

#include <sstream>
#include <memory>
#include <initializer_list>

#include "Generic/GenericStringPropertySequence.h"
#include "Parsing/Menu/Domain/EventHandler/CommonEventHandlerScript.h"
#include "Parsing/Menu/Domain/EventHandler/CommonEventHandlerSetLocalVar.h"
#include "Parsing/Menu/Matcher/MenuExpressionMatchers.h"
#include "Parsing/Menu/Matcher/MenuMatcherFactory.h"
#include "Parsing/Menu/Matcher/MenuMatcherScriptInt.h"
#include "Parsing/Menu/Matcher/MenuMatcherScriptNumeric.h"

using namespace menu;

namespace menu
{
    enum class ExpectedScriptToken
    {
        NUMERIC,
        INT,
        TEXT
    };

    class ScriptMatcherFactory final : public MenuMatcherFactory
    {
    public:
        explicit ScriptMatcherFactory(const IMatcherForLabelSupplier<SimpleParserValue>* labelSupplier)
            : MenuMatcherFactory(labelSupplier)
        {
        }

        _NODISCARD MatcherFactoryWrapper<SimpleParserValue> ScriptStrictNumeric() const
        {
            return And({
                MatcherFactoryWrapper<SimpleParserValue>(std::make_unique<MenuMatcherScriptNumeric>()).Transform([](const token_list_t& tokens)-> SimpleParserValue
                {
                    const auto& firstToken = tokens[0].get();

                    if (firstToken.m_type == SimpleParserValueType::INTEGER)
                        return SimpleParserValue::String(firstToken.GetPos(), new std::string(std::to_string(firstToken.IntegerValue())));
                    if (firstToken.m_type == SimpleParserValueType::FLOATING_POINT)
                    {
                        std::ostringstream ss;
                        ss << std::noshowpoint << firstToken.FloatingPointValue();
                        return SimpleParserValue::String(firstToken.GetPos(), new std::string(ss.str()));
                    }
                    return SimpleParserValue::String(firstToken.GetPos(), new std::string(firstToken.StringValue()));
                })
            });
        }

        _NODISCARD MatcherFactoryWrapper<SimpleParserValue> ScriptNumeric() const
        {
            return Or({
                ScriptStrictNumeric(),
                Or({
                    Type(SimpleParserValueType::CHARACTER),
                    Type(SimpleParserValueType::STRING),
                    Type(SimpleParserValueType::IDENTIFIER),
                }).Transform([](const token_list_t& tokens) -> SimpleParserValue
                {
                    return SimpleParserValue::Integer(tokens[0].get().GetPos(), static_cast<int>(ExpectedScriptToken::NUMERIC));
                })
            });
        }

        _NODISCARD MatcherFactoryWrapper<SimpleParserValue> ScriptStrictInt() const
        {
            return And({
                MatcherFactoryWrapper<SimpleParserValue>(std::make_unique<MenuMatcherScriptInt>()).Transform([](const token_list_t& tokens)-> SimpleParserValue
                {
                    const auto& firstToken = tokens[0].get();

                    if (firstToken.m_type == SimpleParserValueType::INTEGER)
                        return SimpleParserValue::String(firstToken.GetPos(), new std::string(std::to_string(firstToken.IntegerValue())));
                    return SimpleParserValue::String(firstToken.GetPos(), new std::string(firstToken.StringValue()));
                })
            });
        }

        _NODISCARD MatcherFactoryWrapper<SimpleParserValue> ScriptChar(const char c) const
        {
            return Or({
                Char(c),
                Or({
                    Type(SimpleParserValueType::INTEGER),
                    Type(SimpleParserValueType::FLOATING_POINT),
                    Type(SimpleParserValueType::STRING),
                    Type(SimpleParserValueType::IDENTIFIER),
                }).Transform([](const token_list_t& tokens) -> SimpleParserValue
                {
                    return SimpleParserValue::Integer(tokens[0].get().GetPos(), static_cast<int>(ExpectedScriptToken::INT));
                })
            });
        }

        _NODISCARD MatcherFactoryWrapper<SimpleParserValue> ScriptInt() const
        {
            return Or({
                ScriptStrictInt(),
                Or({
                    Type(SimpleParserValueType::CHARACTER),
                    Type(SimpleParserValueType::FLOATING_POINT),
                    Type(SimpleParserValueType::STRING),
                    Type(SimpleParserValueType::IDENTIFIER),
                }).Transform([](const token_list_t& tokens) -> SimpleParserValue
                {
                    return SimpleParserValue::Integer(tokens[0].get().GetPos(), static_cast<int>(ExpectedScriptToken::INT));
                })
            });
        }

        _NODISCARD MatcherFactoryWrapper<SimpleParserValue> ScriptText() const
        {
            return Or({
                Type(SimpleParserValueType::STRING),
                Type(SimpleParserValueType::IDENTIFIER),
                Or({
                    Type(SimpleParserValueType::CHARACTER),
                    Type(SimpleParserValueType::FLOATING_POINT),
                    Type(SimpleParserValueType::INTEGER),
                }).Transform([](const token_list_t& tokens) -> SimpleParserValue
                {
                    return SimpleParserValue::Integer(tokens[0].get().GetPos(), static_cast<int>(ExpectedScriptToken::TEXT));
                })
            });
        }

        _NODISCARD MatcherFactoryWrapper<SimpleParserValue> ScriptKeyword(std::string keyword) const
        {
            return KeywordIgnoreCase(std::move(keyword));
        }

        _NODISCARD MatcherFactoryWrapper<SimpleParserValue> ScriptColor() const
        {
            return And({
                ScriptStrictNumeric(),
                Optional(ScriptStrictNumeric()),
                Optional(ScriptStrictNumeric()),
                Optional(ScriptStrictNumeric())
            });
        }
    };
}

namespace menu::event_handler_set_scope_sequences
{
    class SequenceCloseBlock final : public MenuFileParser::sequence_t
    {
        static constexpr auto CAPTURE_TOKEN = 1;

    public:
        SequenceCloseBlock()
        {
            const MenuMatcherFactory create(this);

            AddMatchers({
                create.Char('}')
            });
        }

    protected:
        void ProcessMatch(MenuFileParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            auto remainingScript = state->m_current_script.str();
            if (!remainingScript.empty())
                state->m_current_nested_event_handler_set->m_elements.emplace_back(std::make_unique<CommonEventHandlerScript>(std::move(remainingScript)));
            state->m_current_script.str(std::string());

            bool conditionWasAutoSkip;

            do
            {
                conditionWasAutoSkip = false;
                if (!state->m_condition_stack.empty())
                {
                    conditionWasAutoSkip = state->m_condition_stack.top().m_auto_skip;
                    state->m_condition_stack.pop();

                    if (!state->m_condition_stack.empty())
                    {
                        const auto& newConditionState = state->m_condition_stack.top();
                        if (newConditionState.m_in_condition_elements)
                            state->m_current_nested_event_handler_set = newConditionState.m_condition->m_condition_elements.get();
                        else
                            state->m_current_nested_event_handler_set = newConditionState.m_condition->m_else_elements.get();
                    }
                    else
                        state->m_current_nested_event_handler_set = state->m_current_event_handler_set;
                }
                else
                {
                    state->m_current_event_handler_set = nullptr;
                    state->m_current_nested_event_handler_set = nullptr;
                }
            }
            while (conditionWasAutoSkip);
        }
    };

    class SequenceSkipEmptyStatements final : public MenuFileParser::sequence_t
    {
    public:
        SequenceSkipEmptyStatements()
        {
            const MenuMatcherFactory create(this);

            AddMatchers({
                create.Char(';')
            });
        }

    protected:
        void ProcessMatch(MenuFileParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            if (!state->m_current_script_statement_terminated)
            {
                state->m_current_script << "; ";
                state->m_current_script_statement_terminated = true;
            }
        }
    };

    class SequenceSkipScriptToken final : public MenuFileParser::sequence_t
    {
        static constexpr auto CAPTURE_SCRIPT_TOKEN = 1;

    public:
        SequenceSkipScriptToken()
        {
            const ScriptMatcherFactory create(this);

            AddMatchers({
                create.Or({
                    create.Numeric(),
                    create.Text(),
                    create.Type(SimpleParserValueType::CHARACTER),
                }).Capture(CAPTURE_SCRIPT_TOKEN)
            });
        }

    protected:
        void ProcessMatch(MenuFileParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            const auto& capture = result.NextCapture(CAPTURE_SCRIPT_TOKEN);

            switch (capture.m_type)
            {
            case SimpleParserValueType::STRING:
                state->m_current_script << "\"" << capture.StringValue() << "\" ";
                break;

            case SimpleParserValueType::IDENTIFIER:
                state->m_current_script << "\"" << capture.IdentifierValue() << "\" ";
                break;

            case SimpleParserValueType::INTEGER:
                state->m_current_script << "\"" << capture.IntegerValue() << "\" ";
                break;

            case SimpleParserValueType::CHARACTER:
                state->m_current_script << capture.CharacterValue() << " ";
                break;

            case SimpleParserValueType::FLOATING_POINT:
                state->m_current_script << "\"" << capture.FloatingPointValue() << "\" ";
                break;

            default:
                throw ParsingException(capture.GetPos(), "Invalid script token type for skipping");
            }

            state->m_current_script_statement_terminated = false;
        }
    };

    class SequenceGenericScriptStatement : public MenuFileParser::sequence_t
    {
    protected:
        static constexpr auto CAPTURE_SCRIPT_TOKEN = 1;

        SequenceGenericScriptStatement() = default;

    public:
        explicit SequenceGenericScriptStatement(std::initializer_list<Movable<std::unique_ptr<AbstractMatcher<SimpleParserValue>>>> matchers)
        {
            const MenuMatcherFactory create(this);

            AddMatchers({
                create.And(matchers).Capture(CAPTURE_SCRIPT_TOKEN),
                create.Optional(create.Char(';'))
            });
        }

        static std::unique_ptr<SequenceGenericScriptStatement> Create(std::initializer_list<Movable<std::unique_ptr<AbstractMatcher<SimpleParserValue>>>> matchers)
        {
            return std::make_unique<SequenceGenericScriptStatement>(matchers);
        }

    private:
        static std::string ScriptTokenTypeToString(int tokenType)
        {
            switch (static_cast<ExpectedScriptToken>(tokenType))
            {
            case ExpectedScriptToken::INT:
                return "INT";

            case ExpectedScriptToken::NUMERIC:
                return "NUMERIC";

            case ExpectedScriptToken::TEXT:
                return "TEXT";

            default:
                return "UNKNOWN";
            }
        }

    protected:
        void ProcessMatch(MenuFileParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            while (result.HasNextCapture(CAPTURE_SCRIPT_TOKEN))
            {
                const auto& capture = result.NextCapture(CAPTURE_SCRIPT_TOKEN);

                if (capture.m_type == SimpleParserValueType::IDENTIFIER)
                {
                    state->m_current_script << "\"" << capture.IdentifierValue() << "\" ";
                }
                else if (capture.m_type == SimpleParserValueType::STRING)
                {
                    state->m_current_script << "\"" << capture.StringValue() << "\" ";
                }
                else if (capture.m_type == SimpleParserValueType::CHARACTER)
                {
                    state->m_current_script << capture.CharacterValue() << " ";
                }
                else if (capture.m_type == SimpleParserValueType::INTEGER)
                {
                    std::ostringstream ss;
                    ss << "Invalid script token. Expected " << ScriptTokenTypeToString(capture.IntegerValue()) << ".";

                    throw ParsingException(capture.GetPos(), ss.str());
                }
                else
                    throw ParsingException(capture.GetPos(), "Invalid script capture");
            }

            state->m_current_script << "; ";
        }
    };

    class SequenceUiScriptStatement final : public SequenceGenericScriptStatement
    {
    public:
        explicit SequenceUiScriptStatement(std::initializer_list<Movable<std::unique_ptr<AbstractMatcher<SimpleParserValue>>>> matchers)
            : SequenceGenericScriptStatement()
        {
            const ScriptMatcherFactory create(this);

            AddMatchers({
                create.And({
                    create.ScriptKeyword("uiScript"),
                    create.And(matchers)
                }).Capture(CAPTURE_SCRIPT_TOKEN),
                create.Optional(create.Char(';'))
            });
        }

        static std::unique_ptr<SequenceUiScriptStatement> Create(std::initializer_list<Movable<std::unique_ptr<AbstractMatcher<SimpleParserValue>>>> matchers)
        {
            return std::make_unique<SequenceUiScriptStatement>(matchers);
        }
    };

    class AbstractSequenceWithPlayerData : public SequenceGenericScriptStatement
    {
    protected:
        static constexpr auto LABEL_PLAYER_DATA_PATH_ELEMENT = 1;

        static std::unique_ptr<matcher_t> PlayerDataPathMatchers(const ScriptMatcherFactory& create)
        {
            return create.Or({
                create.ScriptKeyword("false"),
                create.ScriptKeyword("true"),
                create.And({
                    create.Or({
                        create.ScriptKeyword("localvarstring"),
                        create.ScriptKeyword("localvarint"),
                        create.ScriptKeyword("localvarbool"),
                    }),
                    create.ScriptChar('('),
                    create.ScriptText(),
                    create.ScriptChar(')'),
                }),
                create.ScriptStrictInt(),
                create.ScriptText()
            });
        }
    };

    class SequenceSetPlayerData final : public AbstractSequenceWithPlayerData
    {
    public:
        explicit SequenceSetPlayerData()
        {
            const ScriptMatcherFactory create(this);

            AddLabeledMatchers(PlayerDataPathMatchers(create), LABEL_PLAYER_DATA_PATH_ELEMENT);
            AddMatchers({
                create.And({
                    create.ScriptKeyword("setPlayerData"),
                    create.ScriptChar('('),
                    create.Label(LABEL_PLAYER_DATA_PATH_ELEMENT),
                    create.Loop(create.And({
                        create.ScriptChar(','),
                        create.Label(LABEL_PLAYER_DATA_PATH_ELEMENT)
                    })),
                    create.ScriptChar(')'),
                }).Capture(CAPTURE_SCRIPT_TOKEN),
                create.Optional(create.Char(';'))
            });
        }
    };

    class SequenceSetPlayerDataSplitscreen final : public AbstractSequenceWithPlayerData
    {
    public:
        explicit SequenceSetPlayerDataSplitscreen()
        {
            const ScriptMatcherFactory create(this);

            AddLabeledMatchers(PlayerDataPathMatchers(create), LABEL_PLAYER_DATA_PATH_ELEMENT);
            AddMatchers({
                create.And({
                    create.ScriptKeyword("setPlayerDataSplitscreen"),
                    create.ScriptChar('('),
                    create.ScriptInt(),
                    create.ScriptChar(','),
                    create.Label(LABEL_PLAYER_DATA_PATH_ELEMENT),
                    create.Loop(create.And({
                        create.ScriptChar(','),
                        create.Label(LABEL_PLAYER_DATA_PATH_ELEMENT)
                    })),
                    create.ScriptChar(')'),
                }).Capture(CAPTURE_SCRIPT_TOKEN),
                create.Optional(create.Char(';'))
            });
        }
    };

    class SequenceLerp final : public SequenceGenericScriptStatement
    {
    public:
        explicit SequenceLerp()
        {
            const ScriptMatcherFactory create(this);

            AddMatchers({
                create.And({
                    create.ScriptKeyword("lerp"),
                    create.Or({
                        create.ScriptKeyword("scale"),
                        create.ScriptKeyword("alpha"),
                        create.ScriptKeyword("x"),
                        create.ScriptKeyword("y"),
                    }),
                    create.ScriptKeyword("from"),
                    create.ScriptNumeric(),
                    create.ScriptKeyword("to"),
                    create.ScriptNumeric(),
                    create.ScriptKeyword("over"),
                    create.ScriptNumeric()
                }).Capture(CAPTURE_SCRIPT_TOKEN),
                create.Optional(create.Char(';'))
            });
        }
    };

    class SequenceSetLocalVar final : public MenuFileParser::sequence_t
    {
        static constexpr auto TAG_BOOL = static_cast<int>(SetLocalVarType::BOOL);
        static constexpr auto TAG_INT = static_cast<int>(SetLocalVarType::INT);
        static constexpr auto TAG_FLOAT = static_cast<int>(SetLocalVarType::FLOAT);
        static constexpr auto TAG_STRING = static_cast<int>(SetLocalVarType::STRING);

        static constexpr auto CAPTURE_VAR_NAME = 1;

    public:
        SequenceSetLocalVar()
        {
            const ScriptMatcherFactory create(this);
            const MenuExpressionMatchers expressionMatchers;

            AddLabeledMatchers(expressionMatchers.Expression(this), MenuExpressionMatchers::LABEL_EXPRESSION);

            AddMatchers({
                create.Or({
                    create.ScriptKeyword("setLocalVarBool").Tag(TAG_BOOL),
                    create.ScriptKeyword("setLocalVarInt").Tag(TAG_INT),
                    create.ScriptKeyword("setLocalVarFloat").Tag(TAG_FLOAT),
                    create.ScriptKeyword("setLocalVarString").Tag(TAG_STRING)
                }),
                create.ScriptText().Capture(CAPTURE_VAR_NAME),
                create.Label(MenuExpressionMatchers::LABEL_EXPRESSION),
                create.Optional(create.Char(';'))
            });
        }

    private:
        static std::string ScriptKeywordForType(const SetLocalVarType type)
        {
            switch (type)
            {
            case SetLocalVarType::BOOL:
                return "setLocalVarBool";
            case SetLocalVarType::INT:
                return "setLocalVarInt";
            case SetLocalVarType::FLOAT:
                return "setLocalVarFloat";
            case SetLocalVarType::STRING:
                return "setLocalVarString";
            default:
                return "unknown";
            }
        }

        static void EmitStaticValue(MenuFileParserState* state, const SimpleExpressionValue& value)
        {
            switch (value.m_type)
            {
            case SimpleExpressionValue::Type::DOUBLE:
                state->m_current_script << value.m_double_value;
                break;

            case SimpleExpressionValue::Type::INT:
                state->m_current_script << value.m_int_value;
                break;

            case SimpleExpressionValue::Type::STRING:
                state->m_current_script << *value.m_string_value;
                break;
            }
        }

        static void CheckStaticValueType(const TokenPos& pos, const SetLocalVarType type, const SimpleExpressionValue& staticValue)
        {
            switch (type)
            {
            case SetLocalVarType::BOOL:
                if (staticValue.m_type != SimpleExpressionValue::Type::INT)
                    throw ParsingException(pos, "Static value must be BOOL");
                break;

            case SetLocalVarType::INT:
                if (staticValue.m_type != SimpleExpressionValue::Type::INT)
                    throw ParsingException(pos, "Static value must be INT");
                break;

            case SetLocalVarType::FLOAT:
                if (staticValue.m_type != SimpleExpressionValue::Type::DOUBLE && staticValue.m_type != SimpleExpressionValue::Type::INT)
                    throw ParsingException(pos, "Static value must be FLOAT");
                break;

            case SetLocalVarType::STRING:
            default:
                break;
            }
        }

        static void EmitStaticSetLocalVar(MenuFileParserState* state, const TokenPos& pos, const SetLocalVarType type, const std::string& varName, std::unique_ptr<ISimpleExpression> expression)
        {
            state->m_current_script << "\"" << ScriptKeywordForType(type) << "\" \"" << varName << "\" \"";
            const auto staticValue = expression->EvaluateStatic();
            CheckStaticValueType(pos, type, staticValue);
            EmitStaticValue(state, staticValue);
            state->m_current_script << "\" ; ";
        }

        static void EmitDynamicSetLocalVar(const MenuFileParserState* state, const SetLocalVarType type, const std::string& varName, std::unique_ptr<ISimpleExpression> expression)
        {
            state->m_current_nested_event_handler_set->m_elements.emplace_back(std::make_unique<CommonEventHandlerSetLocalVar>(type, varName, std::move(expression)));
        }

    protected:
        void ProcessMatch(MenuFileParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            const MenuExpressionMatchers expressionMatchers(state);

            const auto typeTag = static_cast<SetLocalVarType>(result.NextTag());
            const auto& varNameToken = result.NextCapture(CAPTURE_VAR_NAME);
            const auto& varName = MenuMatcherFactory::TokenTextValue(varNameToken);
            auto expression = expressionMatchers.ProcessExpression(result);

            if (!expression)
                throw ParsingException(varNameToken.GetPos(), "No expression");

            if (expression && expression->IsStatic())
                EmitStaticSetLocalVar(state, varNameToken.GetPos(), typeTag, varName, std::move(expression));
            else
                EmitDynamicSetLocalVar(state, typeTag, varName, std::move(expression));
        }
    };

    class SequenceIf final : public MenuFileParser::sequence_t
    {
        static constexpr auto CAPTURE_KEYWORD = 1;

    public:
        SequenceIf()
        {
            const ScriptMatcherFactory create(this);
            const MenuExpressionMatchers expressionMatchers;

            AddLabeledMatchers(expressionMatchers.Expression(this), MenuExpressionMatchers::LABEL_EXPRESSION);

            AddMatchers({
                create.Keyword("if").Capture(CAPTURE_KEYWORD),
                create.Char('('),
                create.Label(MenuExpressionMatchers::LABEL_EXPRESSION),
                create.Char(')'),
                create.Char('{')
            });
        }

    protected:
        void ProcessMatch(MenuFileParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            const MenuExpressionMatchers expressionMatchers(state);
            auto expression = expressionMatchers.ProcessExpression(result);

            if (!expression)
                throw ParsingException(result.NextCapture(CAPTURE_KEYWORD).GetPos(), "Could not parse expression");

            auto remainingScript = state->m_current_script.str();
            if (!remainingScript.empty())
                state->m_current_nested_event_handler_set->m_elements.emplace_back(std::make_unique<CommonEventHandlerScript>(std::move(remainingScript)));
            state->m_current_script.str(std::string());

            auto newCondition = std::make_unique<CommonEventHandlerCondition>(std::move(expression), std::make_unique<CommonEventHandlerSet>(), nullptr);
            auto* newConditionPtr = newCondition.get();
            state->m_current_nested_event_handler_set->m_elements.emplace_back(std::move(newCondition));

            state->m_condition_stack.emplace(newConditionPtr);
            state->m_current_nested_event_handler_set = newConditionPtr->m_condition_elements.get();
        }
    };

    class SequenceElseIf final : public MenuFileParser::sequence_t
    {
        static constexpr auto CAPTURE_KEYWORD = 1;

    public:
        SequenceElseIf()
        {
            const ScriptMatcherFactory create(this);
            const MenuExpressionMatchers expressionMatchers;

            AddLabeledMatchers(expressionMatchers.Expression(this), MenuExpressionMatchers::LABEL_EXPRESSION);

            AddMatchers({
                create.Char('}'),
                create.Keyword("elseif").Capture(CAPTURE_KEYWORD),
                create.Char('('),
                create.Label(MenuExpressionMatchers::LABEL_EXPRESSION),
                create.Char(')'),
                create.Char('{')
            });
        }

    protected:
        void ProcessMatch(MenuFileParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            const MenuExpressionMatchers expressionMatchers(state);
            auto expression = expressionMatchers.ProcessExpression(result);

            if (!expression)
                throw ParsingException(result.NextCapture(CAPTURE_KEYWORD).GetPos(), "Could not parse expression");

            if (state->m_condition_stack.empty())
                throw ParsingException(result.NextCapture(CAPTURE_KEYWORD).GetPos(), "Not in an if statement");

            auto remainingScript = state->m_current_script.str();
            if (!remainingScript.empty())
                state->m_current_nested_event_handler_set->m_elements.emplace_back(std::make_unique<CommonEventHandlerScript>(std::move(remainingScript)));
            state->m_current_script.str(std::string());

            auto& currentCondition = state->m_condition_stack.top();

            assert(currentCondition.m_in_condition_elements == (currentCondition.m_condition->m_else_elements == nullptr));
            if (!currentCondition.m_in_condition_elements)
                throw ParsingException(result.NextCapture(CAPTURE_KEYWORD).GetPos(), "Cannot specify elseif after else");
            currentCondition.m_in_condition_elements = false;

            auto newCondition = std::make_unique<CommonEventHandlerCondition>(std::move(expression), std::make_unique<CommonEventHandlerSet>(), nullptr);
            auto* newConditionPtr = newCondition.get();
            currentCondition.m_condition->m_else_elements = std::make_unique<CommonEventHandlerSet>();
            currentCondition.m_condition->m_else_elements->m_elements.emplace_back(std::move(newCondition));

            state->m_condition_stack.emplace(newConditionPtr, true);
            state->m_current_nested_event_handler_set = newConditionPtr->m_condition_elements.get();
        }
    };

    class SequenceElse final : public MenuFileParser::sequence_t
    {
        static constexpr auto CAPTURE_KEYWORD = 1;

    public:
        SequenceElse()
        {
            const ScriptMatcherFactory create(this);
            const MenuExpressionMatchers expressionMatchers;

            AddMatchers({
                create.Char('}'),
                create.Keyword("else").Capture(CAPTURE_KEYWORD),
                create.Char('{')
            });
        }

    protected:
        void ProcessMatch(MenuFileParserState* state, SequenceResult<SimpleParserValue>& result) const override
        {
            if (state->m_condition_stack.empty())
                throw ParsingException(result.NextCapture(CAPTURE_KEYWORD).GetPos(), "Not in an if statement");

            auto& currentCondition = state->m_condition_stack.top();

            assert(currentCondition.m_in_condition_elements == (currentCondition.m_condition->m_else_elements == nullptr));
            if (!currentCondition.m_in_condition_elements)
                throw ParsingException(result.NextCapture(CAPTURE_KEYWORD).GetPos(), "Cannot specify second else block");

            auto remainingScript = state->m_current_script.str();
            if (!remainingScript.empty())
                state->m_current_nested_event_handler_set->m_elements.emplace_back(std::make_unique<CommonEventHandlerScript>(std::move(remainingScript)));
            state->m_current_script.str(std::string());

            currentCondition.m_in_condition_elements = false;
            currentCondition.m_condition->m_else_elements = std::make_unique<CommonEventHandlerSet>();
            state->m_current_nested_event_handler_set = currentCondition.m_condition->m_else_elements.get();
        }
    };
}

using namespace event_handler_set_scope_sequences;

EventHandlerSetScopeSequences::EventHandlerSetScopeSequences(std::vector<std::unique_ptr<MenuFileParser::sequence_t>>& allSequences, std::vector<MenuFileParser::sequence_t*>& scopeSequences)
    : AbstractScopeSequenceHolder(allSequences, scopeSequences)
{
}

void EventHandlerSetScopeSequences::AddSequences(const FeatureLevel featureLevel, const bool permissive) const
{
    AddSequence(std::make_unique<SequenceSkipEmptyStatements>());
    // If else and stuff

    // Creating factory with no label supplier. Cannot use labels with it.
    const ScriptMatcherFactory create(nullptr);

    if (!permissive)
    {
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("fadeIn"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("fadeOut"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("show"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("hide"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("showMenu"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("hideMenu"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("setColor"), create.ScriptColor()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("open"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("close"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("escape"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("closeForAllPlayers"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("ingameOpen"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("ingameClose"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("setBackground"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("setItemColor"), create.ScriptText(), create.ScriptText(), create.ScriptColor()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("focusFirst")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("setFocus"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("setFocusByDvar"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("setDvar"), create.ScriptText(), create.Or({create.ScriptStrictNumeric(), create.ScriptText()})}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("exec"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("execNow"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("execOnDvarStringValue"), create.ScriptText(), create.ScriptText(), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("execOnDvarIntValue"), create.ScriptText(), create.ScriptInt(), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("execOnDvarFloatValue"), create.ScriptText(), create.ScriptNumeric(), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("execNowOnDvarStringValue"), create.ScriptText(), create.ScriptText(), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("execNowOnDvarIntValue"), create.ScriptText(), create.ScriptInt(), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("execNowOnDvarFloatValue"), create.ScriptText(), create.ScriptNumeric(), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("play"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("scriptMenuResponse"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("respondOnDvarStringValue"), create.ScriptText(), create.ScriptText(), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("respondOnDvarIntValue"), create.ScriptText(), create.ScriptInt(), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("respondOnDvarFloatValue"), create.ScriptText(), create.ScriptNumeric(), create.ScriptText()}));
        AddSequence(std::make_unique<SequenceSetPlayerData>());
        AddSequence(std::make_unique<SequenceSetPlayerDataSplitscreen>());
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("setPlayerDataSp")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("updateMail")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("openMail")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("deleteMail")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("doMailLottery")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("resetStatsConfirm")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("resetStatsCancel")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("setGameMode"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("feederTop")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("feederBottom")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("showGamerCard")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("openForGameType"), create.ScriptText()}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("closeForGameType"), create.ScriptText()}));
        // statClearPerkNew // TODO
        // statSetUsingTable // TODO
        // statClearBitMask // TODO
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("kickPlayer")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("getKickPlayerQuestion")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("partyUpdateMissingMapPackDvar")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("togglePlayerMute")}));
        AddSequence(SequenceGenericScriptStatement::Create({create.ScriptKeyword("resolveError")}));
        AddSequence(std::make_unique<SequenceLerp>());

        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("StartServer")}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("loadArenas")}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("loadGameInfo")}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("clearError")}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("Quit")}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("Controls")}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("Leave")}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("closeingame")}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("update"), create.ScriptText()}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("startSingleplayer")}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("getLanguage")}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("verifyLanguage")}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("updateLanguage")}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("mutePlayer")}));
        AddSequence(SequenceUiScriptStatement::Create(
            {create.ScriptKeyword("openMenuOnDvar"), create.ScriptText(), create.Or({create.ScriptStrictNumeric(), create.ScriptText()}), create.ScriptText()}));
        AddSequence(
            SequenceUiScriptStatement::Create({create.ScriptKeyword("openMenuOnDvarNot"), create.ScriptText(), create.Or({create.ScriptStrictNumeric(), create.ScriptText()}), create.ScriptText()}));
        AddSequence(SequenceUiScriptStatement::Create({
            create.ScriptKeyword("closeMenuOnDvar"), create.ScriptText(), create.Or({create.ScriptStrictNumeric(), create.ScriptText()}), create.ScriptText()
        }));
        AddSequence(SequenceUiScriptStatement::Create(
            {create.ScriptKeyword("closeMenuOnDvarNot"), create.ScriptText(), create.Or({create.ScriptStrictNumeric(), create.ScriptText()}), create.ScriptText()}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("setRecommended")}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("clearLoadErrorsSummary")}));
        AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("clearClientMatchData")}));

        if (featureLevel == FeatureLevel::IW4)
        {
            // IW4x UiScripts
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("LoadMods")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("RunMod")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("ClearMods")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("security_increase_cancel")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("mod_download_cancel")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("LoadFriends")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("JoinFriend")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("downloadDLC"), create.ScriptInt()}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("checkFirstLaunch")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("visitWebsite")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("visitWiki")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("visitDiscord")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("updateui_mousePitch")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("ServerStatus")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("UpdateFilter")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("RefreshFilter")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("RefreshServers")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("JoinServer")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("ServerSort"), create.ScriptInt()}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("CreateListFavorite")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("CreateFavorite")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("CreateCurrentServerFavorite")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("DeleteFavorite")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("nextStartupMessage")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("UpdateClasses")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("loadDemos")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("launchDemo")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("deleteDemo")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("ApplyMap")}));
            AddSequence(SequenceUiScriptStatement::Create({create.ScriptKeyword("ApplyInitialMap")}));
        }
    }

    AddSequence(std::make_unique<SequenceSetLocalVar>());
    AddSequence(std::make_unique<SequenceIf>());
    AddSequence(std::make_unique<SequenceElseIf>());
    AddSequence(std::make_unique<SequenceElse>());
    AddSequence(std::make_unique<SequenceCloseBlock>());

    if (permissive)
    {
        AddSequence(std::make_unique<SequenceSkipScriptToken>());
    }
}
