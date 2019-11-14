﻿using ZoneCodeGenerator.Domain;
using ZoneCodeGenerator.Parsing.C_Header.Blocks;
using ZoneCodeGenerator.Parsing.Matching;
using ZoneCodeGenerator.Parsing.Matching.Matchers;
using ZoneCodeGenerator.Parsing.Testing;
using ZoneCodeGenerator.Utils;

namespace ZoneCodeGenerator.Parsing.C_Header.Tests
{
    class TestUnion : AbstractTokenTest<IHeaderParserState>
    {
        private const string TypedefToken = "typedef";
        private const string NameToken = "name";
        private const string AlignToken = "align";
        private const string ParentToken = "parent";

        private static readonly TokenMatcher[] matchers = {
            new MatcherGroupOptional(new MatcherLiteral("typedef").WithName(TypedefToken)),
            new MatcherGroupOptional(new MatcherLiteral("const")), 
            new MatcherLiteral("union"),
            new MatcherGroupOptional(new MatcherGroupAnd(
                new MatcherLiteral("__declspec"),
                new MatcherLiteral("("),
                new MatcherLiteral("align"),
                new MatcherLiteral("("),
                new MatcherNumber().WithName(AlignToken),
                new MatcherLiteral(")"),
                new MatcherLiteral(")")
            )), 
            new MatcherGroupOptional(new MatcherName().WithName(NameToken)),
            new MatcherGroupOptional(new MatcherGroupAnd(
                new MatcherLiteral(":"),
                new MatcherTypename().WithName(ParentToken)
            )),
            new MatcherLiteral("{")
        };

        public TestUnion() : base(matchers)
        {

        }

        protected override void ProcessMatch(IHeaderParserState state)
        {
            var isTypedef = HasMatcherTokens(TypedefToken);
            var name = NextMatch(NameToken) ?? "";

            var block = new BlockUnion(state, name, isTypedef);

            if (HasMatcherTokens(AlignToken))
                block.CustomAlignment = int.Parse(NextMatch(AlignToken));

            if (HasMatcherTokens(ParentToken))
            {
                var parentDataTypeName = NextMatch(ParentToken);
                var parentDataType = state.FindType(parentDataTypeName);

                if(parentDataType == null)
                    throw new TestFailedException($"Could not find parent data type '{parentDataTypeName}' for union '{name}'");
                if(!(parentDataType is DataTypeUnion parentUnion))
                    throw new TestFailedException($"Parent data type '{parentDataTypeName}' for union '{name}' must be a union as well.");

                block.Inherit(parentUnion);
            }

            state.PushBlock(block);
        }
    }
}
