//
// MIT License
//
// Copyright (c) 2018 Murat Yilmaz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once

#include <AST/AST.h>
#include <Basic/Basic.h>
#include <Parse/Parse.h>

// @Incomplete Parse all leading and trailing trivia of a token in a useful fashion without storing the contents
//             build a request api for the contents of trivia tokens like comments...
//             rewrite the AST Printer to emit the original source file given into the compiler
//             but appending the contents of #load files if the AST is populated further with source files...

namespace jelly {
namespace Parse {

    class Lexer;

    class Parser {
        Lexer* lexer;

        jelly::DiagnosticEngine* diagnostic;
        jelly::Parse::Token token;
        bool silentErrors = false;

        template<typename ...Args>
        void report(Diagnostic::Level level, const char* format, Args&&... args) {
            if (silentErrors) { return; }

            diagnostic->report(level, format, args...);
        }

        void consumeToken();
        bool consumeToken(Token::Kind kind);
        bool consumeIdentifier(jelly::AST::Identifier& identifier);
        bool consumeOperator(jelly::AST::Fixity fixity, jelly::AST::Operator& op);

        bool tryConsumeOperator(jelly::AST::Operator op);

        jelly::AST::Expression* tryParseExpression(jelly::AST::Precedence precedence = 0);
        jelly::AST::MemberAccessExpression* tryParseMemberAccessExpression(jelly::AST::Expression* left);

        jelly::AST::Expression* parseConditionList();
        jelly::AST::Expression* parseAtomExpression();
        jelly::AST::Expression* parsePrimaryExpression();
        jelly::AST::Declaration* parseTopLevelDeclaration();
        jelly::AST::ArrayTypeRef* parseArrayTypeRef(jelly::AST::TypeRef* elementTypeRef);
        jelly::AST::BlockStatement* parseBlock();
        jelly::AST::BoolLiteral* parseBoolLiteral();
        jelly::AST::BreakStatement* parseBreak();
        jelly::AST::CallExpression* parseCallExpression(jelly::AST::Expression* callee);
        jelly::AST::SubscriptExpression* parseSubscriptExpression(jelly::AST::Expression* left);
        jelly::AST::CaseStatement* parseCaseStatement();
        jelly::AST::ConditionalCaseStatement* parseConditionalCaseStatement();
        jelly::AST::ConstantDeclaration* parseConstant();
        jelly::AST::ContinueStatement* parseContinue();
        jelly::AST::DeferStatement* parseDefer();
        jelly::AST::DoStatement* parseDoStatement();
        jelly::AST::ElseCaseStatement* parseElseCaseStatement();
        jelly::AST::EnumerationDeclaration* parseEnumeration();
        jelly::AST::EnumerationElementDeclaration* parseEnumerationElement();
        jelly::AST::Expression* parseExpression(jelly::AST::Precedence precedence = 0);
        jelly::AST::FallthroughStatement* parseFallthrough();
        jelly::AST::FloatLiteral* parseFloatLiteral();
        jelly::AST::FunctionDeclaration* parseFunction();
        jelly::AST::Expression* parseGroupExpression();
        jelly::AST::GuardStatement* parseGuard();
        jelly::AST::IdentifierExpression* parseIdentifierExpression();
        jelly::AST::IfStatement* parseIf();
        jelly::AST::IntLiteral* parseIntLiteral();
        jelly::AST::LoadDeclaration* parseLoadDeclaration();
        jelly::AST::NilLiteral* parseNilLiteral();
        jelly::AST::OpaqueTypeRef* parseOpaqueTypeRef();
        jelly::AST::ParameterDeclaration* parseParameter();
        jelly::AST::PointerTypeRef* parsePointerTypeRef(jelly::AST::TypeRef* pointeeTypeRef);
        jelly::AST::ReturnStatement* parseReturn();
        jelly::AST::Statement* parseStatement();
        jelly::AST::StringLiteral* parseStringLiteral();
        jelly::AST::StructureDeclaration* parseStructure();
        jelly::AST::SwitchStatement* parseSwitchStatement();
        jelly::AST::TypeOfTypeRef* parseTypeOfTypeRef();
        jelly::AST::TypeRef* parseTypeRef();
        jelly::AST::UnaryExpression* parseUnaryExpression();
        jelly::AST::ValueDeclaration* parseValueDeclaration();
        jelly::AST::VariableDeclaration* parseVariable();
        jelly::AST::WhileStatement* parseWhileStatement();

    public:

        Parser(Lexer* lexer, jelly::DiagnosticEngine* diag);

        jelly::AST::Context* getContext() const;

        void parseAllTopLevelNodes();
    };
}
}
