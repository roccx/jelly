#include "JellyCore/ASTContext.h"
#include "JellyCore/ASTFunctions.h"
#include "JellyCore/ASTNodes.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/Lexer.h"
#include "JellyCore/Parser.h"
#include "JellyCore/SourceRange.h"
#include "JellyCore/TempAllocator.h"

// TODO: Write tests for correct scope creation and population!

struct _Parser {
    AllocatorRef allocator;
    AllocatorRef tempAllocator;
    ASTContextRef context;
    ScopeID currentScope;
    LexerRef lexer;
    Token token;
};

static inline Bool _StringIsValidFilePath(StringRef string);
static inline Bool _SourceRangeContainsLineBreakCharacter(SourceRange range);

static inline Bool _ParserIsToken(ParserRef parser, TokenKind kind);
static inline Bool _ParserConsumeToken(ParserRef parser, TokenKind kind);

static inline StringRef _ParserConsumeIdentifier(ParserRef parser);
static inline ASTUnaryOperator _ParserConsumeUnaryOperator(ParserRef parser);
static inline ASTBinaryOperator _ParserConsumeBinaryOperator(ParserRef parser);
static inline ASTPostfixOperator _ParserConsumePostfixOperatorHead(ParserRef parser);
static inline Bool _ParserConsumePostfixOperatorTail(ParserRef parser, ASTPostfixOperator postfix);
static inline ASTNodeRef _ParserParseDirective(ParserRef parser);
static inline ASTBlockRef _ParserParseBlock(ParserRef parser);
static inline ASTIfStatementRef _ParserParseIfStatement(ParserRef parser);
static inline ASTLoopStatementRef _ParserParseLoopStatement(ParserRef parser);
static inline ASTCaseStatementRef _ParserParseCaseStatement(ParserRef parser);
static inline ASTSwitchStatementRef _ParserParseSwitchStatement(ParserRef parser);
static inline ASTControlStatementRef _ParserParseControlStatement(ParserRef parser);
static inline ASTNodeRef _ParserParseStatement(ParserRef parser);
static inline ASTExpressionRef _ParserParsePrimaryExpression(ParserRef parser);
static inline ASTUnaryExpressionRef _ParserParseUnaryExpression(ParserRef parser, ASTUnaryOperator unary, SourceRange location);
static inline ASTExpressionRef _ParserParseExpression(ParserRef parser, ASTOperatorPrecedence minPrecedence, Bool silentDiagnostics);
static inline ASTIdentifierExpressionRef _ParserParseIdentifierExpression(ParserRef parser);
static inline ASTCallExpressionRef _ParserParseCallExpression(ParserRef parser, ASTExpressionRef callee);
static inline ASTConstantExpressionRef _ParserParseConstantExpression(ParserRef parser);
static inline ASTEnumerationDeclarationRef _ParserParseEnumerationDeclaration(ParserRef parser);
static inline ASTFunctionDeclarationRef _ParserParseFunctionDeclaration(ParserRef parser);
static inline ASTFunctionDeclarationRef _ParserParseForeignFunctionDeclaration(ParserRef parser);
static inline ASTFunctionDeclarationRef _ParserParsePrefixFunctionDeclaration(ParserRef parser);
static inline ASTFunctionDeclarationRef _ParserParseInfixFunctionDeclaration(ParserRef parser);
static inline ASTStructureDeclarationRef _ParserParseStructureDeclaration(ParserRef parser);
static inline ASTInitializerDeclarationRef _ParserParseInitializerDeclaration(ParserRef parser);
static inline ASTValueDeclarationRef _ParserParseVariableDeclaration(ParserRef parser);
static inline ASTTypeAliasDeclarationRef _ParserParseTypeAliasDeclaration(ParserRef parser);
static inline ASTValueDeclarationRef _ParserParseEnumerationElementDeclaration(ParserRef parser);
static inline ASTValueDeclarationRef _ParserParseParameterDeclaration(ParserRef parser);
static inline ASTTypeRef _ParserParseType(ParserRef parser);
static inline ASTExpressionRef _ParserParseConditionList(ParserRef parser);
static inline ASTNodeRef _ParserParseTopLevelNode(ParserRef parser);
static inline ASTNodeRef _ParserParseTopLevelInterfaceNode(ParserRef parser);

static inline ScopeID _ParserPushScope(ParserRef parser, SourceRange location, ASTNodeRef node, ScopeKind kind);
static inline void _ParserPopScope(ParserRef parser);

ParserRef ParserCreate(AllocatorRef allocator, ASTContextRef context) {
    ParserRef parser  = AllocatorAllocate(allocator, sizeof(struct _Parser));
    parser->allocator = allocator;
    // TODO: Replace tempAllocator with a pool allocator which resets after finishing a top level parse action.
    parser->tempAllocator = TempAllocatorCreate(allocator);
    parser->context       = context;
    parser->currentScope  = kScopeGlobal;
    parser->lexer         = NULL;
    return parser;
}

void ParserDestroy(ParserRef parser) {
    if (parser->lexer) {
        LexerDestroy(parser->lexer);
    }

    AllocatorDestroy(parser->tempAllocator);
    AllocatorDeallocate(parser->allocator, parser);
}

ASTSourceUnitRef ParserParseSourceUnit(ParserRef parser, StringRef filePath, StringRef source) {
    ASTModuleDeclarationRef module = ASTContextGetModule(parser->context);

    parser->lexer = LexerCreate(parser->allocator, source);
    LexerNextToken(parser->lexer, &parser->token);

    SourceRange location  = parser->token.location;
    ArrayRef declarations = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    while (true) {
        SourceRange leadingTrivia = parser->token.leadingTrivia;
        ASTNodeRef topLevelNode   = _ParserParseTopLevelNode(parser);
        if (!topLevelNode) {
            break;
        }

        if (ArrayGetElementCount(declarations) > 0 && !_SourceRangeContainsLineBreakCharacter(leadingTrivia)) {
            ReportError("Consecutive statements on a line are not allowed");
        }

        if (topLevelNode->tag == ASTTagLinkDirective) {
            ASTArrayAppendElement(module->linkDirectives, topLevelNode);
        } else {
            ArrayAppendElement(declarations, &topLevelNode);
        }
    }

    location.end                = parser->token.location.start;
    ASTSourceUnitRef sourceUnit = ASTContextCreateSourceUnit(parser->context, location, parser->currentScope, filePath, declarations);
    ASTModuleAddSourceUnit(parser->context, module, sourceUnit);

    LexerDestroy(parser->lexer);
    parser->lexer = NULL;

    return sourceUnit;
}

ASTSourceUnitRef ParserParseModuleSourceUnit(ParserRef parser, ASTModuleDeclarationRef module, StringRef filePath, StringRef source) {
    parser->lexer = LexerCreate(parser->allocator, source);
    LexerNextToken(parser->lexer, &parser->token);

    SourceRange location  = parser->token.location;
    ArrayRef declarations = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    while (true) {
        SourceRange leadingTrivia = parser->token.leadingTrivia;
        ASTNodeRef topLevelNode   = _ParserParseTopLevelInterfaceNode(parser);
        if (!topLevelNode) {
            break;
        }

        if (ArrayGetElementCount(declarations) > 0 && !_SourceRangeContainsLineBreakCharacter(leadingTrivia)) {
            ReportError("Consecutive statements on a line are not allowed");
        }

        if (topLevelNode->tag == ASTTagLinkDirective) {
            ASTArrayAppendElement(module->linkDirectives, topLevelNode);
        } else {
            ArrayAppendElement(declarations, &topLevelNode);
        }
    }

    location.end                = parser->token.location.start;
    ASTSourceUnitRef sourceUnit = ASTContextCreateSourceUnit(parser->context, location, parser->currentScope, filePath, declarations);
    ASTModuleAddSourceUnit(parser->context, module, sourceUnit);

    LexerDestroy(parser->lexer);
    parser->lexer = NULL;

    return sourceUnit;
}

// grammar: module-declaration := "module" identifier "{" [ { directive } ] "}"
ASTModuleDeclarationRef ParserParseModuleDeclaration(ParserRef parser, StringRef filePath, StringRef source) {
    parser->lexer = LexerCreate(parser->allocator, source);
    LexerNextToken(parser->lexer, &parser->token);

    if (!_ParserConsumeToken(parser, TokenKindKeywordModule)) {
        ReportError("Expected keyword 'module' in imported interface");
        return NULL;
    }

    StringRef moduleName = _ParserConsumeIdentifier(parser);
    if (!moduleName) {
        ReportError("Expected identifier for module declaration");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindLeftCurlyBracket)) {
        ReportError("Expected '{' after module declaration");
        return NULL;
    }

    SourceRange location    = parser->token.location;
    ArrayRef linkDirectives = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    ArrayRef directives     = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    while (!_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
        SourceRange leadingTrivia = parser->token.leadingTrivia;

        if (!_ParserIsToken(parser, TokenKindDirectiveLoad) && !_ParserIsToken(parser, TokenKindDirectiveLink) &&
            !_ParserIsToken(parser, TokenKindDirectiveInclude)) {
            ReportError("Expected '#load', '#link' or '#include' directive in module");
            return NULL;
        }

        ASTNodeRef directive = _ParserParseDirective(parser);
        if (!directive) {
            return NULL;
        }

        if (ArrayGetElementCount(directives) > 0 && !_SourceRangeContainsLineBreakCharacter(leadingTrivia)) {
            ReportError("Consecutive statements on a line are not allowed");
        }

        // TODO: Check where all directives have to be stored...
        if (directive->tag == ASTTagLinkDirective) {
            ArrayAppendElement(linkDirectives, &directive);
        } else {
            ArrayAppendElement(directives, &directive);
        }
    }

    if (!_ParserConsumeToken(parser, TokenKindRightCurlyBracket)) {
        ReportError("Expected '}' at end of module declaration");
        return NULL;
    }

    if (!_ParserIsToken(parser, TokenKindEndOfFile)) {
        ReportError("Expected end of file after module declaration");
        return NULL;
    }

    location.end                   = parser->token.location.start;
    ASTModuleDeclarationRef module = ASTContextCreateModuleDeclaration(parser->context, location, parser->currentScope,
                                                                       ASTModuleKindLibrary, moduleName, NULL, NULL);
    ASTArrayAppendArray(module->linkDirectives, linkDirectives);
    ASTSourceUnitRef sourceUnit = ASTContextCreateSourceUnit(parser->context, location, parser->currentScope, filePath, directives);
    ASTModuleAddSourceUnit(parser->context, module, sourceUnit);

    LexerDestroy(parser->lexer);
    parser->lexer = NULL;

    return module;
}

static inline Bool _StringIsValidFilePath(StringRef string) {
    // TODO: Implement check for file path validation
    return true;
}

static inline Bool _SourceRangeContainsLineBreakCharacter(SourceRange range) {
    for (const Char *cursor = range.start; cursor < range.end; cursor++) {
        switch (*cursor) {
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
            return true;

        default:
            break;
        }
    }

    return false;
}

static inline Bool _ParserIsToken(ParserRef parser, TokenKind kind) {
    return parser->token.kind == kind;
}

static inline Bool _ParserConsumeToken(ParserRef parser, TokenKind kind) {
    if (parser->token.kind == kind) {
        LexerNextToken(parser->lexer, &parser->token);
        return true;
    }

    return false;
}

/// grammar: identifier := identifier-head { identifier-tail }
/// grammar: identifier-head := "a" ... "z" | "A" ... "Z" | "_"
/// grammar: identifier-tail := identifier-head | "0" ... "9"
static inline StringRef _ParserConsumeIdentifier(ParserRef parser) {
    if (parser->token.kind == TokenKindIdentifier) {
        StringRef result = StringCreateRange(parser->tempAllocator, parser->token.location.start, parser->token.location.end);
        LexerNextToken(parser->lexer, &parser->token);
        return result;
    }

    return NULL;
}

static inline ASTUnaryOperator _ParserConsumeUnaryOperator(ParserRef parser) {
    if (_ParserConsumeToken(parser, TokenKindExclamationMark)) {
        return ASTUnaryOperatorLogicalNot;
    } else if (_ParserConsumeToken(parser, TokenKindTilde)) {
        return ASTUnaryOperatorBitwiseNot;
    } else if (_ParserConsumeToken(parser, TokenKindPlusSign)) {
        return ASTUnaryOperatorUnaryPlus;
    } else if (_ParserConsumeToken(parser, TokenKindMinusSign)) {
        return ASTUnaryOperatorUnaryMinus;
    } else {
        return ASTUnaryOperatorUnknown;
    }
}

static inline ASTBinaryOperator _ParserConsumeBinaryOperator(ParserRef parser) {
    if (_ParserConsumeToken(parser, TokenKindLessThanLessThan)) {
        return ASTBinaryOperatorBitwiseLeftShift;
    } else if (_ParserConsumeToken(parser, TokenKindGreaterThanGreaterThan)) {
        return ASTBinaryOperatorBitwiseRightShift;
    } else if (_ParserConsumeToken(parser, TokenKindAsterisk)) {
        return ASTBinaryOperatorMultiply;
    } else if (_ParserConsumeToken(parser, TokenKindSlash)) {
        return ASTBinaryOperatorDivide;
    } else if (_ParserConsumeToken(parser, TokenKindPercentSign)) {
        return ASTBinaryOperatorReminder;
    } else if (_ParserConsumeToken(parser, TokenKindAmpersand)) {
        return ASTBinaryOperatorBitwiseAnd;
    } else if (_ParserConsumeToken(parser, TokenKindPlusSign)) {
        return ASTBinaryOperatorAdd;
    } else if (_ParserConsumeToken(parser, TokenKindMinusSign)) {
        return ASTBinaryOperatorSubtract;
    } else if (_ParserConsumeToken(parser, TokenKindPipe)) {
        return ASTBinaryOperatorBitwiseOr;
    } else if (_ParserConsumeToken(parser, TokenKindCircumflex)) {
        return ASTBinaryOperatorBitwiseXor;
    } else if (_ParserConsumeToken(parser, TokenKindKeywordIs)) {
        return ASTBinaryOperatorTypeCheck;
    } else if (_ParserConsumeToken(parser, TokenKindKeywordAs)) {
        return ASTBinaryOperatorTypeCast;
    } else if (_ParserConsumeToken(parser, TokenKindKeywordAsExclamationMark)) {
        return ASTBinaryOperatorTypeBitcast;
    } else if (_ParserConsumeToken(parser, TokenKindLessThan)) {
        return ASTBinaryOperatorLessThan;
    } else if (_ParserConsumeToken(parser, TokenKindLessThanEqualsSign)) {
        return ASTBinaryOperatorLessThanEqual;
    } else if (_ParserConsumeToken(parser, TokenKindGreaterThan)) {
        return ASTBinaryOperatorGreaterThan;
    } else if (_ParserConsumeToken(parser, TokenKindGreaterThanEqualsSign)) {
        return ASTBinaryOperatorGreaterThanEqual;
    } else if (_ParserConsumeToken(parser, TokenKindEqualsEqualsSign)) {
        return ASTBinaryOperatorEqual;
    } else if (_ParserConsumeToken(parser, TokenKindExclamationMarkEqualsSign)) {
        return ASTBinaryOperatorNotEqual;
    } else if (_ParserConsumeToken(parser, TokenKindAmpersandAmpersand)) {
        return ASTBinaryOperatorLogicalAnd;
    } else if (_ParserConsumeToken(parser, TokenKindPipePipe)) {
        return ASTBinaryOperatorLogicalOr;
    } else if (_ParserConsumeToken(parser, TokenKindEqualsSign)) {
        return ASTBinaryOperatorAssign;
    } else if (_ParserConsumeToken(parser, TokenKindAsteriskEquals)) {
        return ASTBinaryOperatorMultiplyAssign;
    } else if (_ParserConsumeToken(parser, TokenKindSlashEquals)) {
        return ASTBinaryOperatorDivideAssign;
    } else if (_ParserConsumeToken(parser, TokenKindPercentEquals)) {
        return ASTBinaryOperatorReminderAssign;
    } else if (_ParserConsumeToken(parser, TokenKindPlusEquals)) {
        return ASTBinaryOperatorAddAssign;
    } else if (_ParserConsumeToken(parser, TokenKindMinusEqualsSign)) {
        return ASTBinaryOperatorSubtractAssign;
    } else if (_ParserConsumeToken(parser, TokenKindLessThanLessThanEquals)) {
        return ASTBinaryOperatorBitwiseLeftShiftAssign;
    } else if (_ParserConsumeToken(parser, TokenKindGreaterThanGreaterThanEquals)) {
        return ASTBinaryOperatorBitwiseRightShiftAssign;
    } else if (_ParserConsumeToken(parser, TokenKindAmpersandEquals)) {
        return ASTBinaryOperatorBitwiseAndAssign;
    } else if (_ParserConsumeToken(parser, TokenKindPipeEquals)) {
        return ASTBinaryOperatorBitwiseOrAssign;
    } else if (_ParserConsumeToken(parser, TokenKindCircumflexEquals)) {
        return ASTBinaryOperatorBitwiseXorAssign;
    } else {
        return ASTBinaryOperatorUnknown;
    }
}

static inline ASTPostfixOperator _ParserConsumePostfixOperatorHead(ParserRef parser) {
    if (_ParserConsumeToken(parser, TokenKindDot)) {
        return ASTPostfixOperatorSelector;
    } else if (_ParserConsumeToken(parser, TokenKindLeftParenthesis)) {
        return ASTPostfixOperatorCall;
    } else if (_ParserConsumeToken(parser, TokenKindLeftBracket)) {
        return ASTPostfixOperatorSubscript;
    } else {
        return ASTPostfixOperatorUnknown;
    }
}

static inline Bool _ParserConsumePostfixOperatorTail(ParserRef parser, ASTPostfixOperator postfix) {
    switch (postfix) {
    case ASTPostfixOperatorCall:
        return _ParserConsumeToken(parser, TokenKindRightParenthesis);

    case ASTPostfixOperatorSubscript:
        return _ParserConsumeToken(parser, TokenKindRightBracket);

    case ASTPostfixOperatorUnknown:
    case ASTPostfixOperatorSelector:
        return true;

    default:
        return false;
    }
}

/// grammar: directive         := load-directive | link-directive | import-directive | include-directive
/// grammar: load-directive    := "#load" string-literal
/// grammar: link-directive    := "#link" string-literal
/// grammar: import-directive  := "#import" string-literal
/// grammar: include-directive := "#include" string-literal
static inline ASTNodeRef _ParserParseDirective(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (_ParserConsumeToken(parser, TokenKindDirectiveLoad)) {
        ASTConstantExpressionRef filePath = _ParserParseConstantExpression(parser);
        if (!filePath || filePath->kind != ASTConstantKindString) {
            ReportError("Expected string literal after `#load` directive");
            return NULL;
        }

        assert(filePath->kind == ASTConstantKindString);

        if (!_StringIsValidFilePath(filePath->stringValue)) {
            ReportError("Expected valid file path after `#load` directive");
            return NULL;
        }

        location.end = parser->token.location.start;
        return (ASTNodeRef)ASTContextCreateLoadDirective(parser->context, location, parser->currentScope, filePath);
    }

    if (_ParserConsumeToken(parser, TokenKindDirectiveLink)) {
        Bool isFramework = false;

        if (_ParserIsToken(parser, TokenKindIdentifier)) {
            StringRef identifier = _ParserConsumeIdentifier(parser);
            assert(identifier);

            if (StringIsEqualToCString(identifier, "framework")) {
                isFramework = true;
            } else {
                ReportErrorFormat("Expected keyword 'framework' or string-literal found identifier '%s'", StringGetCharacters(identifier));
                return NULL;
            }
        }

        ASTConstantExpressionRef filePath = _ParserParseConstantExpression(parser);
        if (!filePath || filePath->kind != ASTConstantKindString) {
            ReportError("Expected string literal after `#link` directive");
            return NULL;
        }

        assert(filePath->kind == ASTConstantKindString);

        location.end = parser->token.location.start;
        return (ASTNodeRef)ASTContextCreateLinkDirective(parser->context, location, parser->currentScope, isFramework,
                                                         filePath->stringValue);
    }

    if (_ParserConsumeToken(parser, TokenKindDirectiveImport)) {
        ASTConstantExpressionRef filePath = _ParserParseConstantExpression(parser);
        if (!filePath || filePath->kind != ASTConstantKindString) {
            ReportError("Expected string literal after `#import` directive");
            return NULL;
        }

        assert(filePath->kind == ASTConstantKindString);

        location.end = parser->token.location.start;
        return (ASTNodeRef)ASTContextCreateImportDirective(parser->context, location, parser->currentScope, filePath->stringValue);
    }

    if (_ParserConsumeToken(parser, TokenKindDirectiveInclude)) {
        ASTConstantExpressionRef filePath = _ParserParseConstantExpression(parser);
        if (!filePath || filePath->kind != ASTConstantKindString) {
            ReportError("Expected string literal after `#include` directive");
            return NULL;
        }

        assert(filePath->kind == ASTConstantKindString);

        location.end = parser->token.location.start;
        return (ASTNodeRef)ASTContextCreateIncludeDirective(parser->context, location, parser->currentScope, filePath->stringValue);
    }

    ReportError("Unknown compiler directive");
    return NULL;
}

/// grammar: block := '{' { statement } '}'
static inline ASTBlockRef _ParserParseBlock(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindLeftCurlyBracket)) {
        ReportError("Expected '{' at start of `block-statement`");
        return NULL;
    }

    ArrayRef statements = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    while (!_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
        SourceRange leadingTrivia = parser->token.leadingTrivia;
        ASTNodeRef statement      = _ParserParseStatement(parser);
        if (!statement) {
            return NULL;
        }

        if (ArrayGetElementCount(statements) > 0 && !_SourceRangeContainsLineBreakCharacter(leadingTrivia)) {
            ReportError("Consecutive statements on a line are not allowed");
        }

        ArrayAppendElement(statements, &statement);
    }

    if (!_ParserConsumeToken(parser, TokenKindRightCurlyBracket)) {
        return NULL;
    }

    location.end = parser->token.location.start;
    return ASTContextCreateBlock(parser->context, location, parser->currentScope, statements);
}

/// grammar: if-statement := "if" expression { "," expression } block [ "else" ( if-statement | block ) ]
static inline ASTIfStatementRef _ParserParseIfStatement(ParserRef parser) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SourceRange location       = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordIf)) {
        return NULL;
    }

    ASTExpressionRef condition = _ParserParseConditionList(parser);
    if (!condition) {
        return NULL;
    }

    // TODO: @ScopeLocation The location of scope will not be correct it has to encapsulate the associated node!
    ScopeID thenScope     = _ParserPushScope(parser, parser->token.location, NULL, ScopeKindBranch);
    ASTBlockRef thenBlock = _ParserParseBlock(parser);
    if (!thenBlock) {
        return NULL;
    }
    SymbolTableSetScopeUserdata(symbolTable, thenScope, thenBlock);
    _ParserPopScope(parser);

    ASTBlockRef elseBlock = NULL;
    if (_ParserConsumeToken(parser, TokenKindKeywordElse)) {
        // TODO: @ScopeLocation The location of scope will not be correct it has to encapsulate the associated node!
        ScopeID elseScope = _ParserPushScope(parser, parser->token.location, NULL, ScopeKindBranch);
        location.end      = parser->token.location.start;
        if (_ParserIsToken(parser, TokenKindKeywordIf)) {
            SourceRange location          = parser->token.location;
            ASTIfStatementRef ifStatement = _ParserParseIfStatement(parser);
            if (!ifStatement) {
                return NULL;
            }

            ArrayRef statements = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 1);
            ArrayAppendElement(statements, &ifStatement);

            location.end = parser->token.location.start;
            elseBlock    = ASTContextCreateBlock(parser->context, location, parser->currentScope, statements);
        } else {
            elseBlock = _ParserParseBlock(parser);
            if (!elseBlock) {
                return NULL;
            }
        }

        SymbolTableSetScopeUserdata(symbolTable, elseScope, elseBlock);
        _ParserPopScope(parser);
    } else {
        // TODO: @ScopeLocation The location of scope will not be correct it has to encapsulate the associated node!
        ScopeID elseScope = _ParserPushScope(parser, parser->token.location, NULL, ScopeKindBranch);
        elseBlock         = ASTContextCreateBlock(parser->context, SourceRangeMake(parser->token.location.end, parser->token.location.end),
                                          parser->currentScope, NULL);
        SymbolTableSetScopeUserdata(symbolTable, elseScope, elseBlock);
        _ParserPopScope(parser);
    }

    location.end = parser->token.location.start;
    return ASTContextCreateIfStatement(parser->context, location, parser->currentScope, condition, thenBlock, elseBlock);
}

/// grammar: loop-statement  := while-statement | do-statement
/// grammar: while-statement := "while" expression { "," expression } block
/// grammar: do-statement    := "do" block "while" expression
static inline ASTLoopStatementRef _ParserParseLoopStatement(ParserRef parser) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SourceRange location       = parser->token.location;

    if (_ParserConsumeToken(parser, TokenKindKeywordWhile)) {
        ASTExpressionRef condition = _ParserParseConditionList(parser);
        if (!condition) {
            return NULL;
        }

        // TODO: @ScopeLocation The location of scope will not be correct it has to encapsulate the associated node!
        ScopeID loopScope     = _ParserPushScope(parser, parser->token.location, NULL, ScopeKindLoop);
        ASTBlockRef loopBlock = _ParserParseBlock(parser);
        if (!loopBlock) {

            return NULL;
        }

        _ParserPopScope(parser);

        location.end             = parser->token.location.start;
        ASTLoopStatementRef loop = ASTContextCreateLoopStatement(parser->context, location, parser->currentScope, ASTLoopKindWhile,
                                                                 condition, loopBlock);
        SymbolTableSetScopeUserdata(symbolTable, loopScope, loop);
        return loop;
    }

    if (_ParserConsumeToken(parser, TokenKindKeywordDo)) {
        // TODO: @ScopeLocation The location of scope will not be correct it has to encapsulate the associated node!
        ScopeID loopScope     = _ParserPushScope(parser, parser->token.location, NULL, ScopeKindLoop);
        ASTBlockRef loopBlock = _ParserParseBlock(parser);
        if (!loopBlock) {
            return NULL;
        }

        if (!_ParserConsumeToken(parser, TokenKindKeywordWhile)) {
            ReportError("Expected 'while' after block of 'do' statement");
            return NULL;
        }

        ASTExpressionRef condition = _ParserParseConditionList(parser);
        if (!condition) {
            return NULL;
        }

        _ParserPopScope(parser);

        location.end             = parser->token.location.start;
        ASTLoopStatementRef loop = ASTContextCreateLoopStatement(parser->context, location, parser->currentScope, ASTLoopKindDo, condition,
                                                                 loopBlock);
        SymbolTableSetScopeUserdata(symbolTable, loopScope, loop);
        return loop;
    }

    ReportError("Expected 'while' or 'do' at start of loop-statement!");
    return NULL;
}

/// grammar: case-statement             := conditional-case-statement | else-case-statement
/// grammar: conditional-case-statement := "case" expression ":" statement { line-break statement }
/// grammar: else-case-statement        := "else" ":" statement { line-break statement }
static inline ASTCaseStatementRef _ParserParseCaseStatement(ParserRef parser) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SourceRange location       = parser->token.location;

    ASTCaseKind kind           = ASTCaseKindElse;
    ASTExpressionRef condition = NULL;
    if (_ParserConsumeToken(parser, TokenKindKeywordCase)) {
        kind      = ASTCaseKindConditional;
        condition = _ParserParseExpression(parser, 0, false);
        if (!condition) {
            return NULL;
        }
    } else if (!_ParserConsumeToken(parser, TokenKindKeywordElse)) {
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindColon)) {
        if (kind == ASTCaseKindConditional) {
            ReportError("Expected ':' after 'case'");
        } else {
            ReportError("Expected ':' after 'else'");
        }
        return NULL;
    }

    SourceRange blockLocation = parser->token.location;

    // TODO: @ScopeLocation The location of scope will not be correct it has to encapsulate the associated node!
    ScopeID caseScope   = _ParserPushScope(parser, parser->token.location, NULL, ScopeKindCase);
    ArrayRef statements = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    while (!_ParserIsToken(parser, TokenKindKeywordCase) && !_ParserIsToken(parser, TokenKindKeywordElse) &&
           !_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
        SourceRange leadingTrivia = parser->token.leadingTrivia;
        ASTNodeRef statement      = _ParserParseStatement(parser);
        if (!statement) {
            return NULL;
        }

        if (ArrayGetElementCount(statements) > 0 && !_SourceRangeContainsLineBreakCharacter(leadingTrivia)) {
            ReportError("Consecutive statements on a line are not allowed");
        }

        ArrayAppendElement(statements, &statement);
    }

    if (ArrayGetElementSize(statements) < 1) {
        ReportError("case statement in a switch should contain at least one statement");
    }

    _ParserPopScope(parser);

    blockLocation.end = parser->token.location.start;
    ASTBlockRef body  = ASTContextCreateBlock(parser->context, blockLocation, parser->currentScope, statements);

    location.end                  = parser->token.location.start;
    ASTCaseStatementRef statement = ASTContextCreateCaseStatement(parser->context, location, parser->currentScope, kind, condition, body);
    SymbolTableSetScopeUserdata(symbolTable, caseScope, statement);
    return statement;
}

/// grammar: switch-statement := "switch" expression "{" [ case-statement { line-break case-statement } ] "}"
static inline ASTSwitchStatementRef _ParserParseSwitchStatement(ParserRef parser) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SourceRange location       = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordSwitch)) {
        return NULL;
    }

    ASTExpressionRef argument = _ParserParseExpression(parser, 0, true);
    if (!argument) {
        ReportError("Expected expression in 'switch' statement");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindLeftCurlyBracket)) {
        ReportError("Expected '{' at start of 'switch' statement");
        return NULL;
    }

    // TODO: @ScopeLocation The location of scope will not be correct it has to encapsulate the associated node!
    ScopeID switchScope = _ParserPushScope(parser, parser->token.location, NULL, ScopeKindSwitch);
    ArrayRef statements = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);

    if (_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
        ReportError("'switch' statement body must have at least one 'case' or 'else' block");
    }

    while (!_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
        SourceRange leadingTrivia     = parser->token.leadingTrivia;
        ASTCaseStatementRef statement = _ParserParseCaseStatement(parser);
        if (!statement) {
            if (ArrayGetElementCount(statements) > 0) {
                ReportError("All statements inside a switch must be covered by a 'case' or 'else'");
            }

            break;
        }

        if (ArrayGetElementCount(statements) > 0 && !_SourceRangeContainsLineBreakCharacter(leadingTrivia)) {
            ReportError("Consecutive statements on a line are not allowed");
        }

        ArrayAppendElement(statements, &statement);
    }

    if (!_ParserConsumeToken(parser, TokenKindRightCurlyBracket)) {
        return NULL;
    }

    _ParserPopScope(parser);

    location.end                    = parser->token.location.start;
    ASTSwitchStatementRef statement = ASTContextCreateSwitchStatement(parser->context, location, parser->currentScope, argument,
                                                                      statements);
    SymbolTableSetScopeUserdata(symbolTable, switchScope, statement);
    return statement;
}

/// grammar: control-statement      := break-statement | continue-statement | fallthrough-statement | return-statement
/// grammar: break-statement        := "break"
/// grammar: continue-statement     := "continue"
/// grammar: fallthrough-statement  := "fallthrough"
/// grammar: return-statement       := "return" [ expression ]
static inline ASTControlStatementRef _ParserParseControlStatement(ParserRef parser) {
    SourceRange location = parser->token.location;

    ASTControlKind kind;
    ASTExpressionRef result = NULL;
    if (_ParserConsumeToken(parser, TokenKindKeywordBreak)) {
        kind = ASTControlKindBreak;
    } else if (_ParserConsumeToken(parser, TokenKindKeywordContinue)) {
        kind = ASTControlKindContinue;
    } else if (_ParserConsumeToken(parser, TokenKindKeywordFallthrough)) {
        kind = ASTControlKindFallthrough;
    } else if (_ParserConsumeToken(parser, TokenKindKeywordReturn)) {
        kind = ASTControlKindReturn;

        LexerStateRef state = LexerGetState(parser->lexer);
        result              = _ParserParseExpression(parser, 0, true);
        if (!result) {
            LexerSetState(parser->lexer, state);
            LexerPeekToken(parser->lexer, &parser->token);
        }
    } else {
        ReportError("Expected 'break', 'continue', 'fallthrough' or 'return' at start of control-statement!");
        return NULL;
    }

    location.end = parser->token.location.start;
    return ASTContextCreateControlStatement(parser->context, location, parser->currentScope, kind, result);
}

/// grammar: statement := variable-declaration | control-statement | loop-statement | if-statement | switch-statement | expression
static inline ASTNodeRef _ParserParseStatement(ParserRef parser) {
    if (_ParserIsToken(parser, TokenKindKeywordVar)) {
        ASTValueDeclarationRef value = _ParserParseVariableDeclaration(parser);
        if (!value) {
            return NULL;
        }

        assert(value->kind == ASTValueKindVariable);

        return (ASTNodeRef)value;
    }

    if (_ParserIsToken(parser, TokenKindKeywordBreak) || _ParserIsToken(parser, TokenKindKeywordContinue) ||
        _ParserIsToken(parser, TokenKindKeywordFallthrough) || _ParserIsToken(parser, TokenKindKeywordReturn)) {
        return (ASTNodeRef)_ParserParseControlStatement(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordDo) || _ParserIsToken(parser, TokenKindKeywordWhile)) {
        return (ASTNodeRef)_ParserParseLoopStatement(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordIf)) {
        return (ASTNodeRef)_ParserParseIfStatement(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordSwitch)) {
        return (ASTNodeRef)_ParserParseSwitchStatement(parser);
    }

    ASTExpressionRef expression = _ParserParseExpression(parser, 0, true);
    if (!expression) {
        ReportError("Expected statement");
        return NULL;
    }

    return (ASTNodeRef)expression;
}

/// grammar: atom-expression       := group-expression | literal-expression | identifier-expression | sizeof-expression
/// grammar: group-expression      := "(" expression ")"
/// grammar: literal-expression    := literal
/// grammar: identifier-expression := identifier
/// grammar: sizeof-expression      := "sizeof" "(" type ")"
static inline ASTExpressionRef _ParserParseAtomExpression(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (_ParserConsumeToken(parser, TokenKindLeftParenthesis)) {
        ASTExpressionRef expression = _ParserParseExpression(parser, 0, true);
        if (!expression) {
            return NULL;
        }

        if (!_ParserConsumeToken(parser, TokenKindRightParenthesis)) {
            return NULL;
        }

        return expression;
    }

    if (_ParserIsToken(parser, TokenKindKeywordNil) || _ParserIsToken(parser, TokenKindKeywordTrue) ||
        _ParserIsToken(parser, TokenKindKeywordFalse) || _ParserIsToken(parser, TokenKindLiteralInt) ||
        _ParserIsToken(parser, TokenKindLiteralFloat) || _ParserIsToken(parser, TokenKindLiteralString)) {
        return (ASTExpressionRef)_ParserParseConstantExpression(parser);
    }

    if (_ParserConsumeToken(parser, TokenKindKeywordSizeOf)) {
        if (!_ParserConsumeToken(parser, TokenKindLeftParenthesis)) {
            return NULL;
        }

        ASTTypeRef sizeType = _ParserParseType(parser);
        if (!sizeType) {
            return NULL;
        }

        if (!_ParserConsumeToken(parser, TokenKindRightParenthesis)) {
            return NULL;
        }

        location.end = parser->token.location.start;
        return (ASTExpressionRef)ASTContextCreateSizeOfExpression(parser->context, location, parser->currentScope, sizeType);
    }

    return (ASTExpressionRef)_ParserParseIdentifierExpression(parser);
}

/// grammar: primary-expression     := unary-expression | atom-expression | reference-expression | dereference-expression
/// grammar: reference-expression   := '&' expression
/// grammar: dereference-expression := '*' expression
static inline ASTExpressionRef _ParserParsePrimaryExpression(ParserRef parser) {
    SourceRange location   = parser->token.location;
    ASTUnaryOperator unary = _ParserConsumeUnaryOperator(parser);
    if (unary != ASTUnaryOperatorUnknown) {
        return (ASTExpressionRef)_ParserParseUnaryExpression(parser, unary, location);
    }

    if (_ParserConsumeToken(parser, TokenKindAmpersand)) {
        ASTExpressionRef argument = _ParserParseExpression(parser, 0, false);
        if (!argument) {
            return NULL;
        }

        location.end = parser->token.location.start;
        return (ASTExpressionRef)ASTContextCreateReferenceExpression(parser->context, location, parser->currentScope, argument);
    }

    if (_ParserConsumeToken(parser, TokenKindAsterisk)) {
        ASTExpressionRef argument = _ParserParseExpression(parser, 0, false);
        if (!argument) {
            return NULL;
        }

        location.end = parser->token.location.start;
        return (ASTExpressionRef)ASTContextCreateDereferenceExpression(parser->context, location, parser->currentScope, argument);
    }

    return _ParserParseAtomExpression(parser);
}

/// grammar: unary-expression := prefix-operator expression
/// grammar: prefix-operator  := '!' | '~' | '+' | '-'
static inline ASTUnaryExpressionRef _ParserParseUnaryExpression(ParserRef parser, ASTUnaryOperator unary, SourceRange location) {
    assert(unary != ASTUnaryOperatorUnknown);

    ASTExpressionRef arguments[1];
    arguments[0] = _ParserParsePrimaryExpression(parser);
    if (!arguments[0]) {
        return NULL;
    }

    if (location.end != arguments[0]->base.location.start) {
        ReportError("Unary operator cannot be separated from its operand");
        return NULL;
    }

    location.end = parser->token.location.start;
    return ASTContextCreateUnaryExpression(parser->context, location, parser->currentScope, unary, arguments);
}

/// grammar: expression        := binary-expression | primary-expression
/// grammar: binary-expression := primary-expression infix-operator expression
static inline ASTExpressionRef _ParserParseExpression(ParserRef parser, ASTOperatorPrecedence minPrecedence, Bool silentDiagnostics) {
    SourceRange location = parser->token.location;

    // TODO: Source location of expressions are not initialized correctly!

    ASTExpressionRef result = _ParserParsePrimaryExpression(parser);
    if (!result) {
        if (!silentDiagnostics) {
            ReportError("Expected expression");
        }

        return NULL;
    }

    location.end                     = parser->token.location.start;
    LexerStateRef state              = LexerGetState(parser->lexer);
    ASTBinaryOperator binary         = _ParserConsumeBinaryOperator(parser);
    ASTOperatorPrecedence precedence = ASTGetBinaryOperatorPrecedence(binary);

    ASTPostfixOperator postfix = ASTPostfixOperatorUnknown;
    if (binary == ASTBinaryOperatorUnknown) {
        postfix    = _ParserConsumePostfixOperatorHead(parser);
        precedence = ASTGetPostfixOperatorPrecedence(postfix);
    }

    if (binary == ASTBinaryOperatorUnknown && postfix == ASTPostfixOperatorUnknown) {
        return result;
    }

    if (minPrecedence >= precedence) {
        LexerSetState(parser->lexer, state);
        LexerPeekToken(parser->lexer, &parser->token);
        return result;
    }

    while (minPrecedence < precedence) {
        ASTOperatorPrecedence nextPrecedence = precedence;

        if (binary != ASTBinaryOperatorUnknown) {
            ASTOperatorAssociativity associativity = ASTGetBinaryOperatorAssociativity(binary);
            if (associativity == ASTOperatorAssociativityRight) {
                nextPrecedence = ASTGetOperatorPrecedenceBefore(nextPrecedence);
            }

            if (binary == ASTBinaryOperatorTypeCheck || binary == ASTBinaryOperatorTypeCast || binary == ASTBinaryOperatorTypeBitcast) {
                ASTTypeRef expressionType = _ParserParseType(parser);
                if (!expressionType) {
                    return NULL;
                }

                ASTTypeOperation op;
                switch (binary) {
                case ASTBinaryOperatorTypeCast:
                    op = ASTTypeOperationTypeCast;
                    break;

                case ASTBinaryOperatorTypeBitcast:
                    op = ASTTypeOperationTypeBitcast;
                    break;

                case ASTBinaryOperatorTypeCheck:
                    op = ASTTypeOperationTypeCheck;
                    break;

                default:
                    JELLY_UNREACHABLE("Invalid binary operation given!");
                    return NULL;
                }

                location.end = parser->token.location.start;
                result = (ASTExpressionRef)ASTContextCreateTypeOperationExpression(parser->context, location, parser->currentScope, op,
                                                                                   result, expressionType);
            } else {
                ASTExpressionRef right = _ParserParseExpression(parser, nextPrecedence, silentDiagnostics);
                if (!right) {
                    return NULL;
                }

                location.end = parser->token.location.start;
                if (ASTBinaryOperatorIsAssignment(binary)) {
                    result = (ASTExpressionRef)ASTContextCreateAssignmentExpression(parser->context, location, parser->currentScope, binary,
                                                                                    result, right);
                } else {
                    ASTExpressionRef arguments[2] = {result, right};
                    result = (ASTExpressionRef)ASTContextCreateBinaryExpression(parser->context, location, parser->currentScope, binary,
                                                                                arguments);
                }
            }
        } else if (postfix == ASTPostfixOperatorSelector) {
            StringRef memberName = _ParserConsumeIdentifier(parser);
            if (!memberName) {
                return NULL;
            }

            location.end = parser->token.location.start;
            result       = (ASTExpressionRef)ASTContextCreateMemberAccessExpression(parser->context, location, parser->currentScope, result,
                                                                              memberName);
        } else if (postfix == ASTPostfixOperatorCall) {
            result = (ASTExpressionRef)_ParserParseCallExpression(parser, result);
            if (!result) {
                return NULL;
            }
        } else if (postfix == ASTPostfixOperatorSubscript) {
            ArrayRef arguments = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTExpressionRef), 8);
            while (!_ParserIsToken(parser, TokenKindRightBracket)) {
                ASTExpressionRef argument = _ParserParseExpression(parser, 0, silentDiagnostics);
                if (!argument) {
                    return NULL;
                }

                ArrayAppendElement(arguments, &argument);

                if (!_ParserConsumeToken(parser, TokenKindComma)) {
                    break;
                }
            }

            if (!_ParserConsumePostfixOperatorTail(parser, postfix)) {
                return NULL;
            }

            location.end = parser->token.location.start;
            result       = (ASTExpressionRef)ASTContextCreateSubscriptExpression(parser->context, location, parser->currentScope, result,
                                                                           arguments);
        } else {
            return NULL;
        }

        LexerStateRef state = LexerGetState(parser->lexer);
        binary              = _ParserConsumeBinaryOperator(parser);
        precedence          = ASTGetBinaryOperatorPrecedence(binary);
        postfix             = ASTPostfixOperatorUnknown;

        if (binary == ASTBinaryOperatorUnknown) {
            postfix    = _ParserConsumePostfixOperatorHead(parser);
            precedence = ASTGetPostfixOperatorPrecedence(postfix);
        }

        if (binary == ASTBinaryOperatorUnknown && postfix == ASTPostfixOperatorUnknown) {
            return result;
        }

        if (minPrecedence >= precedence) {
            LexerSetState(parser->lexer, state);
            LexerPeekToken(parser->lexer, &parser->token);
            return result;
        }
    }

    return result;
}

/// grammar: identifier := identifier-head { identifier-tail }
/// grammar: identifier-head := "a" ... "z" | "A" ... "Z" | "_"
/// grammar: identifier-tail := identifier-head | "0" ... "9"
static inline ASTIdentifierExpressionRef _ParserParseIdentifierExpression(ParserRef parser) {
    SourceRange location = parser->token.location;
    StringRef name       = _ParserConsumeIdentifier(parser);
    if (!name) {
        return NULL;
    }

    location.end = parser->token.location.start;
    return ASTContextCreateIdentifierExpression(parser->context, location, parser->currentScope, name);
}

/// grammar: call-expression := expression "(" [ expression { "," expression } ] ")"
static inline ASTCallExpressionRef _ParserParseCallExpression(ParserRef parser, ASTExpressionRef callee) {
    SourceRange location = {callee->base.location.start, parser->token.location.start};

    // We expect that the postfix operator head is already consumed earlier
    //    if (!_ParserConsumePostfixOperatorHead(parser)) {
    //        return NULL;
    //    }

    ArrayRef arguments = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    while (!_ParserIsToken(parser, TokenKindRightParenthesis)) {
        ASTExpressionRef argument = _ParserParseExpression(parser, 0, false);
        if (!argument) {
            return NULL;
        }

        ArrayAppendElement(arguments, &argument);

        if (_ParserIsToken(parser, TokenKindRightParenthesis)) {
            break;
        }

        if (!_ParserConsumeToken(parser, TokenKindComma)) {
            return NULL;
        }
    }

    if (!_ParserConsumePostfixOperatorTail(parser, ASTPostfixOperatorCall)) {
        return NULL;
    }

    location.end = parser->token.location.start;
    return ASTContextCreateCallExpression(parser->context, location, parser->currentScope, callee, arguments);
}

/// grammar: constant-expression := nil-literal | bool-literal | numeric-literal | string-literal
/// grammar: nil-literal         := "nil"
/// grammar: bool-literal        := "true" | "false"
static inline ASTConstantExpressionRef _ParserParseConstantExpression(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (_ParserConsumeToken(parser, TokenKindKeywordNil)) {
        location.end = parser->token.location.start;
        return ASTContextCreateConstantNilExpression(parser->context, location, parser->currentScope);
    }

    if (_ParserConsumeToken(parser, TokenKindKeywordTrue)) {
        location.end = parser->token.location.start;
        return ASTContextCreateConstantBoolExpression(parser->context, location, parser->currentScope, true);
    }

    if (_ParserConsumeToken(parser, TokenKindKeywordFalse)) {
        location.end = parser->token.location.start;
        return ASTContextCreateConstantBoolExpression(parser->context, location, parser->currentScope, false);
    }

    UInt64 intValue = parser->token.intValue;
    if (_ParserConsumeToken(parser, TokenKindLiteralInt)) {
        location.end = parser->token.location.start;
        return ASTContextCreateConstantIntExpression(parser->context, location, parser->currentScope, intValue);
    }

    Float64 floatValue = parser->token.floatValue;
    if (_ParserConsumeToken(parser, TokenKindLiteralFloat)) {
        location.end = parser->token.location.start;
        return ASTContextCreateConstantFloatExpression(parser->context, location, parser->currentScope, floatValue);
    }

    if (_ParserIsToken(parser, TokenKindLiteralString)) {
        assert(parser->token.location.end - parser->token.location.start >= 2);
        StringRef value = StringCreateRange(parser->tempAllocator, parser->token.location.start + 1, parser->token.location.end - 1);
        _ParserConsumeToken(parser, TokenKindLiteralString);
        location.end = parser->token.location.start;
        return ASTContextCreateConstantStringExpression(parser->context, location, parser->currentScope, value);
    }

    return NULL;
}

/// grammar: enum-declaration := "enum" identifier "{" [ enum-element { line-break enum-element } ] "}"
static inline ASTEnumerationDeclarationRef _ParserParseEnumerationDeclaration(ParserRef parser) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SourceRange location       = parser->token.location;
    if (!_ParserConsumeToken(parser, TokenKindKeywordEnum)) {
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        ReportError("Expected `name` of `enum-declaration`");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindLeftCurlyBracket)) {
        ReportError("Expected '{' after `name` of `enum-declaration`");
        return NULL;
    }

    // TODO: @ScopeLocation The location of scope will not be correct it has to encapsulate the associated node!
    ScopeID enumScope = _ParserPushScope(parser, parser->token.location, NULL, ScopeKindEnumeration);
    ArrayRef elements = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    if (!_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
        while (true) {
            SourceRange leadingTrivia      = parser->token.leadingTrivia;
            ASTValueDeclarationRef element = _ParserParseEnumerationElementDeclaration(parser);
            if (!element) {
                return NULL;
            }

            assert(element->kind == ASTValueKindEnumerationElement);

            if (ArrayGetElementCount(elements) > 0 && !_SourceRangeContainsLineBreakCharacter(leadingTrivia)) {
                ReportError("Consecutive statements on a line are not allowed");
            }

            if (element->kind != ASTValueKindEnumerationElement) {
                ReportError("Only `enum-element`(s) are allowed inside of `enum-declaration`");
                return NULL;
            }

            ArrayAppendElement(elements, &element);

            if (_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
                break;
            }
        }
    }

    if (!_ParserConsumeToken(parser, TokenKindRightCurlyBracket)) {
        return NULL;
    }

    _ParserPopScope(parser);

    location.end                             = parser->token.location.start;
    ASTEnumerationDeclarationRef enumeration = ASTContextCreateEnumerationDeclaration(parser->context, location, parser->currentScope, name,
                                                                                      elements);
    enumeration->innerScope                  = enumScope;
    SymbolTableSetScopeUserdata(symbolTable, enumScope, enumeration);
    return enumeration;
}

/// grammar: func-declaration := "func" identifier "(" [ parameter-declaration { "," parameter-declaration } ] ")" "->" type-identifier
/// block
static inline ASTFunctionDeclarationRef _ParserParseFunctionDeclaration(ParserRef parser) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SourceRange location       = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordFunc)) {
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        ReportError("Expected name of 'func'");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindLeftParenthesis)) {
        ReportError("Expected parameter list after name of 'func'");
        return NULL;
    }

    // TODO: @ScopeLocation The location of scope will not be correct it has to encapsulate the associated node!
    ScopeID funcScope   = _ParserPushScope(parser, parser->token.location, NULL, ScopeKindFunction);
    ArrayRef parameters = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);

    if (!_ParserIsToken(parser, TokenKindRightParenthesis)) {
        while (true) {
            ASTValueDeclarationRef parameter = _ParserParseParameterDeclaration(parser);
            if (!parameter) {
                return NULL;
            }

            assert(parameter->kind == ASTValueKindParameter);

            ArrayAppendElement(parameters, &parameter);

            if (_ParserIsToken(parser, TokenKindRightParenthesis)) {
                break;
            }

            if (!_ParserConsumeToken(parser, TokenKindComma)) {
                ReportError("Expected ',' or ')' in parameter list of 'func'");
                return NULL;
            }
        }
    }

    if (!_ParserConsumeToken(parser, TokenKindRightParenthesis)) {
        ReportError("Expected ')' after parameter list of 'func'");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindArrow)) {
        ReportError("Expected '->' after parameter list of 'func'");
        return NULL;
    }

    ASTTypeRef returnType = _ParserParseType(parser);
    if (!returnType) {
        ReportError("Expected type for function result");
        return NULL;
    }

    if (_ParserConsumeToken(parser, TokenKindDirectiveIntrinsic)) {
        ASTConstantExpressionRef intrinsic = _ParserParseConstantExpression(parser);
        if (!intrinsic || intrinsic->kind != ASTConstantKindString) {
            ReportError("Expected string literal after `#intrinsic` directive");
            return NULL;
        }

        StringRef intrinsicName = intrinsic->stringValue;

        _ParserPopScope(parser);

        location.end                       = parser->token.location.start;
        ASTFunctionDeclarationRef function = ASTContextCreateIntrinsicFunctionDeclaration(
            parser->context, location, parser->currentScope, ASTFixityPrefix, name, parameters, returnType, intrinsicName);
        function->innerScope = funcScope;
        SymbolTableSetScopeUserdata(symbolTable, funcScope, function);
        return function;
    } else {
        ASTBlockRef body = _ParserParseBlock(parser);
        if (!body) {
            return NULL;
        }

        _ParserPopScope(parser);

        location.end                       = parser->token.location.start;
        ASTFunctionDeclarationRef function = ASTContextCreateFunctionDeclaration(parser->context, location, parser->currentScope,
                                                                                 ASTFixityPrefix, name, parameters, returnType, body);
        function->innerScope               = funcScope;
        SymbolTableSetScopeUserdata(symbolTable, funcScope, function);
        return function;
    }
}

/// grammar: foreign-func-declaration := "prefix" "func" identifier "(" [ parameter { "," parameter } ] ")" "->" type-identifier
static inline ASTFunctionDeclarationRef _ParserParseForeignFunctionDeclaration(ParserRef parser) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SourceRange location       = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindDirectiveForeign)) {
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindKeywordFunc)) {
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        ReportError("Expected name of 'func'");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindLeftParenthesis)) {
        ReportError("Expected parameter list after name of 'func'");
        return NULL;
    }

    // TODO: @ScopeLocation The location of scope will not be correct it has to encapsulate the associated node!
    ScopeID funcScope   = _ParserPushScope(parser, parser->token.location, NULL, ScopeKindFunction);
    ArrayRef parameters = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);

    if (!_ParserIsToken(parser, TokenKindRightParenthesis)) {
        while (true) {
            ASTValueDeclarationRef parameter = _ParserParseParameterDeclaration(parser);
            if (!parameter) {
                return NULL;
            }

            assert(parameter->kind == ASTValueKindParameter);

            ArrayAppendElement(parameters, &parameter);

            if (_ParserIsToken(parser, TokenKindRightParenthesis)) {
                break;
            }

            if (!_ParserConsumeToken(parser, TokenKindComma)) {
                ReportError("Expected ',' or ')' in parameter list of 'func'");
                return NULL;
            }
        }
    }

    if (!_ParserConsumeToken(parser, TokenKindRightParenthesis)) {
        ReportError("Expected ')' after parameter list of 'func'");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindArrow)) {
        ReportError("Expected '->' after parameter list of 'func'");
        return NULL;
    }

    ASTTypeRef returnType = _ParserParseType(parser);
    if (!returnType) {
        ReportError("Expected type for function result");
        return NULL;
    }

    ASTConstantExpressionRef foreign = _ParserParseConstantExpression(parser);
    if (!foreign || foreign->kind != ASTConstantKindString) {
        ReportError("Expected string literal after signature of foreign function declaration");
        return NULL;
    }

    _ParserPopScope(parser);

    location.end                       = parser->token.location.start;
    ASTFunctionDeclarationRef function = ASTContextCreateForeignFunctionDeclaration(
        parser->context, location, parser->currentScope, ASTFixityNone, name, parameters, returnType, foreign->stringValue);
    function->innerScope = funcScope;
    SymbolTableSetScopeUserdata(symbolTable, funcScope, function);
    return function;
}

/// grammar: prefix-func-declaration := "prefix" "func" unary-operator "(" [ parameter { "," parameter } ] ")" "->" type-identifier block
static inline ASTFunctionDeclarationRef _ParserParsePrefixFunctionDeclaration(ParserRef parser) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SourceRange location       = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordPrefix)) {
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindKeywordFunc)) {
        ReportError("Expected keyword 'func' after 'prefix'");
        return NULL;
    }

    ASTUnaryOperator op = _ParserConsumeUnaryOperator(parser);
    if (op == ASTUnaryOperatorUnknown) {
        ReportError("Expected unary operator of prefix func");
        return NULL;
    }

    StringRef name = ASTGetPrefixOperatorName(parser->tempAllocator, op);

    if (!_ParserConsumeToken(parser, TokenKindLeftParenthesis)) {
        ReportError("Expected parameter list after name of 'func'");
        return NULL;
    }

    // TODO: @ScopeLocation The location of scope will not be correct it has to encapsulate the associated node!
    ScopeID funcScope   = _ParserPushScope(parser, parser->token.location, NULL, ScopeKindFunction);
    ArrayRef parameters = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);

    if (!_ParserIsToken(parser, TokenKindRightParenthesis)) {
        while (true) {
            ASTValueDeclarationRef parameter = _ParserParseParameterDeclaration(parser);
            if (!parameter) {
                return NULL;
            }

            assert(parameter->kind == ASTValueKindParameter);

            ArrayAppendElement(parameters, &parameter);

            if (_ParserIsToken(parser, TokenKindRightParenthesis)) {
                break;
            }

            if (!_ParserConsumeToken(parser, TokenKindComma)) {
                ReportError("Expected ',' or ')' in parameter list of 'func'");
                return NULL;
            }
        }
    }

    if (!_ParserConsumeToken(parser, TokenKindRightParenthesis)) {
        ReportError("Expected ')' after parameter list of 'func'");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindArrow)) {
        ReportError("Expected '->' after parameter list of 'func'");
        return NULL;
    }

    ASTTypeRef returnType = _ParserParseType(parser);
    if (!returnType) {
        ReportError("Expected type for function result");
        return NULL;
    }

    if (ArrayGetElementCount(parameters) != 1) {
        ReportError("Prefix functions must have exactly one argument");
    }

    if (_ParserConsumeToken(parser, TokenKindDirectiveIntrinsic)) {
        ASTConstantExpressionRef intrinsic = _ParserParseConstantExpression(parser);
        if (!intrinsic || intrinsic->kind != ASTConstantKindString) {
            ReportError("Expected string literal after `#intrinsic` directive");
            return NULL;
        }

        StringRef intrinsicName = intrinsic->stringValue;

        _ParserPopScope(parser);

        location.end                       = parser->token.location.start;
        ASTFunctionDeclarationRef function = ASTContextCreateIntrinsicFunctionDeclaration(
            parser->context, location, parser->currentScope, ASTFixityPrefix, name, parameters, returnType, intrinsicName);
        function->innerScope = funcScope;
        SymbolTableSetScopeUserdata(symbolTable, funcScope, function);
        return function;
    } else {
        ASTBlockRef body = _ParserParseBlock(parser);
        if (!body) {
            return NULL;
        }

        _ParserPopScope(parser);

        location.end                       = parser->token.location.start;
        ASTFunctionDeclarationRef function = ASTContextCreateFunctionDeclaration(parser->context, location, parser->currentScope,
                                                                                 ASTFixityPrefix, name, parameters, returnType, body);
        function->innerScope               = funcScope;
        SymbolTableSetScopeUserdata(symbolTable, funcScope, function);
        return function;
    }
}

/// grammar: infix-func-declaration := "infix" "func" binary-operator "(" [ parameter { "," parameter } ] ")" "->" type-identifier block
static inline ASTFunctionDeclarationRef _ParserParseInfixFunctionDeclaration(ParserRef parser) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SourceRange location       = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordPrefix)) {
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindKeywordFunc)) {
        ReportError("Expected keyword 'func' after 'infix'");
        return NULL;
    }

    ASTUnaryOperator op = _ParserConsumeUnaryOperator(parser);
    if (op == ASTUnaryOperatorUnknown) {
        ReportError("Expected unary operator of infix func");
        return NULL;
    }

    StringRef name = ASTGetPrefixOperatorName(parser->tempAllocator, op);

    if (!_ParserConsumeToken(parser, TokenKindLeftParenthesis)) {
        ReportError("Expected parameter list after name of 'func'");
        return NULL;
    }

    // TODO: @ScopeLocation The location of scope will not be correct it has to encapsulate the associated node!
    ScopeID funcScope   = _ParserPushScope(parser, parser->token.location, NULL, ScopeKindFunction);
    ArrayRef parameters = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);

    if (!_ParserIsToken(parser, TokenKindRightParenthesis)) {
        while (true) {
            ASTValueDeclarationRef parameter = _ParserParseParameterDeclaration(parser);
            if (!parameter) {
                return NULL;
            }

            assert(parameter->kind == ASTValueKindParameter);

            ArrayAppendElement(parameters, &parameter);

            if (_ParserIsToken(parser, TokenKindRightParenthesis)) {
                break;
            }

            if (!_ParserConsumeToken(parser, TokenKindComma)) {
                ReportError("Expected ',' or ')' in parameter list of 'func'");
                return NULL;
            }
        }
    }

    if (!_ParserConsumeToken(parser, TokenKindRightParenthesis)) {
        ReportError("Expected ')' after parameter list of 'func'");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindArrow)) {
        ReportError("Expected '->' after parameter list of 'func'");
        return NULL;
    }

    ASTTypeRef returnType = _ParserParseType(parser);
    if (!returnType) {
        ReportError("Expected type for function result");
        return NULL;
    }

    if (ArrayGetElementCount(parameters) != 2) {
        ReportError("Infix functions must have exactly two arguments");
    }

    if (_ParserConsumeToken(parser, TokenKindDirectiveIntrinsic)) {
        ASTConstantExpressionRef intrinsic = _ParserParseConstantExpression(parser);
        if (!intrinsic || intrinsic->kind != ASTConstantKindString) {
            ReportError("Expected string literal after `#intrinsic` directive");
            return NULL;
        }

        StringRef intrinsicName = intrinsic->stringValue;

        _ParserPopScope(parser);

        location.end                       = parser->token.location.start;
        ASTFunctionDeclarationRef function = ASTContextCreateIntrinsicFunctionDeclaration(
            parser->context, location, parser->currentScope, ASTFixityInfix, name, parameters, returnType, intrinsicName);
        function->innerScope = funcScope;
        SymbolTableSetScopeUserdata(symbolTable, funcScope, function);
        return function;
    } else {
        ASTBlockRef body = _ParserParseBlock(parser);
        if (!body) {
            return NULL;
        }

        _ParserPopScope(parser);

        location.end                       = parser->token.location.start;
        ASTFunctionDeclarationRef function = ASTContextCreateFunctionDeclaration(parser->context, location, parser->currentScope,
                                                                                 ASTFixityInfix, name, parameters, returnType, body);
        function->innerScope               = funcScope;
        SymbolTableSetScopeUserdata(symbolTable, funcScope, function);
        return function;
    }
}

/// grammar: struct-declaration := "struct" identifier "{" { value-declaration } "}"
static inline ASTStructureDeclarationRef _ParserParseStructureDeclaration(ParserRef parser) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SourceRange location       = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordStruct)) {
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        ReportError("Expected `name` of `struct-declaration`");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindLeftCurlyBracket)) {
        ReportError("Expected '{' after name of `struct-declaration`");
        return NULL;
    }

    // TODO: @ScopeLocation The location of scope will not be correct it has to encapsulate the associated node!
    ScopeID structScope   = _ParserPushScope(parser, parser->token.location, NULL, ScopeKindStructure);
    ArrayRef values       = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    ArrayRef initializers = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);

    if (!_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
        while (true) {
            SourceRange leadingTrivia = parser->token.leadingTrivia;

            if (_ParserIsToken(parser, TokenKindKeywordVar)) {
                ASTValueDeclarationRef value = _ParserParseVariableDeclaration(parser);
                if (!value) {
                    return NULL;
                }
                assert(value->kind == ASTValueKindVariable);

                ArrayAppendElement(values, &value);
            } else if (_ParserIsToken(parser, TokenKindKeywordInit)) {
                ASTInitializerDeclarationRef initializer = _ParserParseInitializerDeclaration(parser);
                if (!initializer) {
                    return NULL;
                }

                ArrayAppendElement(initializers, &initializer);
            } else {
                ReportError("Expected 'var' or 'init' in structure body");
                return NULL;
            }

            if ((ArrayGetElementCount(values) > 0 || ArrayGetElementCount(initializers)) &&
                !_SourceRangeContainsLineBreakCharacter(leadingTrivia)) {
                ReportError("Consecutive statements on a line are not allowed");
            }

            if (_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
                break;
            }
        }
    }

    if (!_ParserConsumeToken(parser, TokenKindRightCurlyBracket)) {
        return NULL;
    }

    _ParserPopScope(parser);

    location.end                           = parser->token.location.start;
    ASTStructureDeclarationRef declaration = ASTContextCreateStructureDeclaration(parser->context, location, parser->currentScope, name,
                                                                                  values, initializers);
    declaration->innerScope                = structScope;
    SymbolTableSetScopeUserdata(symbolTable, structScope, declaration);
    return declaration;
}

/// grammar: initializer-declaration := "init" "(" [ parameter-declaration { "," parameter-declaration } ] ")" block
static inline ASTInitializerDeclarationRef _ParserParseInitializerDeclaration(ParserRef parser) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SourceRange location       = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordInit)) {
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindLeftParenthesis)) {
        return NULL;
    }

    ScopeID initScope   = _ParserPushScope(parser, location, NULL, ScopeKindInitializer);
    ArrayRef parameters = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    if (!_ParserIsToken(parser, TokenKindRightParenthesis)) {
        while (true) {
            ASTValueDeclarationRef parameter = _ParserParseParameterDeclaration(parser);
            if (!parameter) {
                return NULL;
            }

            assert(parameter->kind == ASTValueKindParameter);

            ArrayAppendElement(parameters, &parameter);

            if (_ParserIsToken(parser, TokenKindRightParenthesis)) {
                break;
            }

            if (!_ParserConsumeToken(parser, TokenKindComma)) {
                ReportError("Expected ',' or ')' in parameter list of 'init'");
                return NULL;
            }
        }
    }

    if (!_ParserConsumeToken(parser, TokenKindRightParenthesis)) {
        return NULL;
    }

    ASTBlockRef body = _ParserParseBlock(parser);
    if (!body) {
        return NULL;
    }

    _ParserPopScope(parser);

    location.end                             = parser->token.location.start;
    ASTInitializerDeclarationRef declaration = ASTContextCreateInitializerDeclaration(parser->context, location, parser->currentScope,
                                                                                      parameters, body);
    declaration->innerScope                  = initScope;
    SymbolTableSetScopeUserdata(symbolTable, initScope, declaration);
    return declaration;
}

/// grammar: variable-declaration := "var" identifier ":" type-identifier [ "=" expression ]
static inline ASTValueDeclarationRef _ParserParseVariableDeclaration(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordVar)) {
        ReportErrorFormat("Expected 'var' found '%.*s'", (Int32)SourceRangeLength(parser->token.location), parser->token.location.start);
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        ReportError("Expected name of variable declaration");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindColon)) {
        ReportError("Expected ':' after name of variable declaration");
        return NULL;
    }

    ASTTypeRef type = _ParserParseType(parser);
    if (!type) {
        ReportError("Expected type of variable declaration");
        return NULL;
    }

    ASTExpressionRef initializer = NULL;
    ASTBinaryOperator binary     = _ParserConsumeBinaryOperator(parser);
    if (binary == ASTBinaryOperatorAssign) {
        initializer = _ParserParseExpression(parser, 0, true);
        if (!initializer) {
            ReportError("Expected expression");
            return NULL;
        }
    } else if (binary != ASTBinaryOperatorUnknown) {
        ReportError("Unexpected binary operator found!");
        return NULL;
    }

    location.end = parser->token.location.start;
    return ASTContextCreateValueDeclaration(parser->context, location, parser->currentScope, ASTValueKindVariable, name, type, initializer);
}

/// grammar: type-alias := "typealias" identifier = type
static inline ASTTypeAliasDeclarationRef _ParserParseTypeAliasDeclaration(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordTypeAlias)) {
        ReportError("Expected keyword 'typealias' at start of type alias!");
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        ReportError("Expected identifier of 'typealias'");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindEqualsSign)) {
        ReportError("Expected '=' after identifier of 'typealias'");
        return NULL;
    }

    ASTTypeRef type = _ParserParseType(parser);
    if (!type) {
        ReportError("Expected type of 'typealias'");
        return NULL;
    }

    location.end = parser->token.location.start;
    return ASTContextCreateTypeAliasDeclaration(parser->context, location, parser->currentScope, name, type);
}

/// grammar: enumeration-element-declaration := "case" identifier [ "=" expression ]
static inline ASTValueDeclarationRef _ParserParseEnumerationElementDeclaration(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordCase)) {
        // We assume that we are in a enumeration declaration here...
        ReportError("Expected 'case' or '}' in enumeration declaration");
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        ReportError("Expected name of enumeration element");
        return NULL;
    }

    ASTExpressionRef initializer = NULL;
    ASTBinaryOperator binary     = _ParserConsumeBinaryOperator(parser);
    if (binary == ASTBinaryOperatorAssign) {
        initializer = _ParserParseExpression(parser, 0, true);
        if (!initializer) {
            ReportError("Expected expression after '='");
            return NULL;
        }
    } else if (binary != ASTBinaryOperatorUnknown) {
        ReportError("Unexpected binary operator found!");
        return NULL;
    }

    // TODO: Add strong typing for enumeration elements!
    ASTTypeRef type = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt);

    location.end = parser->token.location.start;
    return ASTContextCreateValueDeclaration(parser->context, location, parser->currentScope, ASTValueKindEnumerationElement, name, type,
                                            initializer);
}

/// grammar: parameter-declaration := identifier ":" type-identifier
static inline ASTValueDeclarationRef _ParserParseParameterDeclaration(ParserRef parser) {
    SourceRange location = parser->token.location;

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        ReportError("Expected parameter name followed by ':'");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindColon)) {
        ReportError("Expected ':' after name of parameter");
        return NULL;
    }

    ASTTypeRef type = _ParserParseType(parser);
    if (!type) {
        ReportError("Expected type of parameter");
        return NULL;
    }

    location.end = parser->token.location.start;
    return ASTContextCreateValueDeclaration(parser->context, location, parser->currentScope, ASTValueKindParameter, name, type, NULL);
}

/// grammar: type                     := builtin-type | opaque-type | pointer-type | array-type | function-pointer-type | generic-type
/// grammar: builtin-type             := "Void" | "Bool" |
///                                      "Int8" | "Int16" | "Int32" | "Int64" | "Int" |
///                                      "UInt8" | "UInt16" | "UInt32" | "UInt64" | "UInt" |
///                                      "Float32" | "Float64" | "Float"
/// grammar: opaque-type              := identifier
/// grammar: pointer-type             := type "*"
/// grammar: array-type               := type "[" [ expression ] "]"
/// grammar: function-pointer-type    := "(" [ type { "," type } ] ")" "->" type
/// grammar: generic-type             := type "<" expression { "," expression } ">"
static inline ASTTypeRef _ParserParseType(ParserRef parser) {
    SourceRange location = parser->token.location;
    ASTTypeRef result    = NULL;

    if (_ParserConsumeToken(parser, TokenKindKeywordVoid)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindVoid);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordBool)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindBool);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordInt8)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt8);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordInt16)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt16);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordInt32)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt32);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordInt64)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt64);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordInt)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordUInt8)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt8);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordUInt16)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt16);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordUInt32)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt32);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordUInt64)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt64);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordUInt)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordFloat32)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindFloat32);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordFloat64)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindFloat64);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordFloat)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindFloat);
    } else if (_ParserConsumeToken(parser, TokenKindLeftParenthesis)) {
        ArrayRef parameterTypes = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTTypeRef), 8);
        while (!_ParserIsToken(parser, TokenKindRightParenthesis)) {
            ASTTypeRef parameterType = _ParserParseType(parser);
            if (!parameterType) {
                return NULL;
            }

            ArrayAppendElement(parameterTypes, &parameterType);

            if (!_ParserConsumeToken(parser, TokenKindComma)) {
                break;
            }
        }

        if (!_ParserConsumeToken(parser, TokenKindRightParenthesis)) {
            ReportError("Expected ')' after parameter list of type");
            return NULL;
        }

        if (!_ParserConsumeToken(parser, TokenKindArrow)) {
            ReportError("Expected '->' after parameter list of type");
            return NULL;
        }

        ASTTypeRef resultType = _ParserParseType(parser);
        if (!resultType) {
            return NULL;
        }

        location.end = parser->token.location.start;
        result = (ASTTypeRef)ASTContextCreateFunctionType(parser->context, location, parser->currentScope, parameterTypes, resultType);
        result = (ASTTypeRef)ASTContextCreatePointerType(parser->context, location, parser->currentScope, result);
        ArrayDestroy(parameterTypes);
        // Early returning here because trailing '*' or '[]' is ambigous with resultType and will be parsed into it...
        // ..so it is impossible to create an array type of function pointer or an pointer type of function pointer in the grammar.
        // If there will be a requirement for this kind of expressivity in the grammar then adding something like alias types will help
        // solving that problem.
        return result;
    } else {
        StringRef name = _ParserConsumeIdentifier(parser);
        if (!name) {
            return NULL;
        }

        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextCreateOpaqueType(parser->context, location, parser->currentScope, name);

        if (_ParserConsumeToken(parser, TokenKindLessThan)) {
            ScopeID argumentScope = _ParserPushScope(parser, location, result, ScopeKindGenericType);
            ArrayRef arguments    = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTExpressionRef), 8);

            if (!_ParserIsToken(parser, TokenKindGreaterThan)) {
                do {

                    LexerStateRef state       = LexerGetState(parser->lexer);
                    ASTTypeRef type           = _ParserParseType(parser);
                    ASTExpressionRef argument = NULL;
                    if (type) {
                        argument = (ASTExpressionRef)ASTContextCreateTypeExpression(parser->context, location, parser->currentScope, type);
                    } else {
                        LexerSetState(parser->lexer, state);
                        LexerPeekToken(parser->lexer, &parser->token);
                        argument = _ParserParsePrimaryExpression(parser);
                    }

                    if (!argument) {
                        return NULL;
                    }

                    ArrayAppendElement(arguments, &argument);

                } while (_ParserConsumeToken(parser, TokenKindComma));
            }

            _ParserPopScope(parser);

            location.end           = parser->token.location.start;
            ASTGenericTypeRef type = ASTContextCreateGenericType(parser->context, location, parser->currentScope, result, arguments);
            type->argumentScope    = argumentScope;
            result                 = (ASTTypeRef)type;

            if (!_ParserConsumeToken(parser, TokenKindGreaterThan)) {
                return NULL;
            }

            return result;
        }
    }

    while (true) {
        if (_ParserConsumeToken(parser, TokenKindAsterisk)) {
            location.end = parser->token.location.start;
            result       = (ASTTypeRef)ASTContextCreatePointerType(parser->context, location, parser->currentScope, result);
        } else if (_ParserConsumeToken(parser, TokenKindLeftBracket)) {
            ASTExpressionRef size = NULL;
            if (!_ParserIsToken(parser, TokenKindRightBracket)) {
                size = _ParserParseExpression(parser, 0, false);
                if (!size) {
                    return NULL;
                }
            }

            if (!_ParserConsumeToken(parser, TokenKindRightBracket)) {
                return NULL;
            }

            location.end = parser->token.location.start;
            result       = (ASTTypeRef)ASTContextCreateArrayType(parser->context, location, parser->currentScope, result, size);
        } else {
            break;
        }
    }

    return result;
}

static inline ASTExpressionRef _ParserParseConditionList(ParserRef parser) {
    ASTExpressionRef condition = _ParserParseExpression(parser, 0, false);
    if (!condition) {
        return NULL;
    }

    while (_ParserConsumeToken(parser, TokenKindComma)) {
        ASTExpressionRef expression = _ParserParseExpression(parser, 0, false);
        if (!expression) {
            return NULL;
        }

        SourceRange location         = {condition->base.location.start, expression->base.location.end};
        ASTExpressionRef arguments[] = {condition, expression};
        condition                    = (ASTExpressionRef)ASTContextCreateBinaryExpression(parser->context, location, parser->currentScope,
                                                                       ASTBinaryOperatorLogicalAnd, arguments);
    }

    return condition;
}

/// grammar: top-level-node := directive | enum-declaration | func-declaration | struct-declaration | variable-declaration | type-alias
static inline ASTNodeRef _ParserParseTopLevelNode(ParserRef parser) {
    if (parser->token.kind == TokenKindEndOfFile) {
        return NULL;
    }

    if (_ParserIsToken(parser, TokenKindDirectiveLoad) || _ParserIsToken(parser, TokenKindDirectiveLink) ||
        _ParserIsToken(parser, TokenKindDirectiveImport) || _ParserIsToken(parser, TokenKindDirectiveInclude)) {
        return (ASTNodeRef)_ParserParseDirective(parser);
    }

    if (_ParserIsToken(parser, TokenKindDirectiveForeign)) {
        return (ASTNodeRef)_ParserParseForeignFunctionDeclaration(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordEnum)) {
        return (ASTNodeRef)_ParserParseEnumerationDeclaration(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordFunc)) {
        return (ASTNodeRef)_ParserParseFunctionDeclaration(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordPrefix)) {
        return (ASTNodeRef)_ParserParsePrefixFunctionDeclaration(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordInfix)) {
        return (ASTNodeRef)_ParserParseInfixFunctionDeclaration(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordStruct)) {
        return (ASTNodeRef)_ParserParseStructureDeclaration(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordVar)) {
        ASTValueDeclarationRef value = _ParserParseVariableDeclaration(parser);
        if (!value) {
            return NULL;
        }

        assert(value->kind == ASTValueKindVariable);
        return (ASTNodeRef)value;
    }

    if (_ParserIsToken(parser, TokenKindKeywordTypeAlias)) {
        return (ASTNodeRef)_ParserParseTypeAliasDeclaration(parser);
    }

    ReportError("Expected top level node!");
    return NULL;
}

// grammar: top-level-interface-node := load-directive | link-directive | enum-declaration | foreing-func-declaration | struct-declaration
//                                    | variable-declaration | type-alias
static inline ASTNodeRef _ParserParseTopLevelInterfaceNode(ParserRef parser) {
    if (parser->token.kind == TokenKindEndOfFile) {
        return NULL;
    }

    if (_ParserIsToken(parser, TokenKindDirectiveLoad) || _ParserIsToken(parser, TokenKindDirectiveLink)) {
        return (ASTNodeRef)_ParserParseDirective(parser);
    }

    if (_ParserIsToken(parser, TokenKindDirectiveForeign)) {
        return (ASTNodeRef)_ParserParseForeignFunctionDeclaration(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordEnum)) {
        return (ASTNodeRef)_ParserParseEnumerationDeclaration(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordStruct)) {
        return (ASTNodeRef)_ParserParseStructureDeclaration(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordVar)) {
        ASTValueDeclarationRef value = _ParserParseVariableDeclaration(parser);
        if (!value) {
            return NULL;
        }

        assert(value->kind == ASTValueKindVariable);
        return (ASTNodeRef)value;
    }

    if (_ParserIsToken(parser, TokenKindKeywordTypeAlias)) {
        return (ASTNodeRef)_ParserParseTypeAliasDeclaration(parser);
    }

    ReportError("Expected top level interface node!");
    return NULL;
}

static inline ScopeID _ParserPushScope(ParserRef parser, SourceRange location, ASTNodeRef node, ScopeKind kind) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    ScopeID scope              = SymbolTableInsertScope(symbolTable, kind, parser->currentScope, location.start);
    parser->currentScope       = scope;
    return scope;
}

static inline void _ParserPopScope(ParserRef parser) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    ScopeID parent             = SymbolTableGetScopeParent(symbolTable, parser->currentScope);
    assert(parent != kScopeNull);
    parser->currentScope = parent;
}
