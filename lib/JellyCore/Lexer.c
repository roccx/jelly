#include "JellyCore/Array.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/Lexer.h"

struct _LexerState {
    LexerRef lexer;
    const Char *cursor;
    Index line;
    Index column;
    Token token;
};

struct _Lexer {
    AllocatorRef allocator;
    StringRef buffer;
    const Char *bufferStart;
    const Char *bufferEnd;
    struct _LexerState state;
    ArrayRef stateStack;
    Bool splitsConsecutivePunctuations;
};

static inline Bool _CharIsAlphaNumeric(Char character);
static inline Bool _CharIsStartOfIdentifier(Char character);
static inline Bool _CharIsContinuationOfIdentifier(Char character);
static inline Bool _CharIsBinaryDigit(Char character);
static inline Bool _CharIsOctalDigit(Char character);
static inline Bool _CharIsDecimalDigit(Char character);
static inline Bool _CharIsHexadecimalDigit(Char character);

static inline void _LexerLexNextToken(LexerRef lexer);

LexerRef LexerCreate(AllocatorRef allocator, StringRef buffer) {
    assert(buffer);
    LexerRef lexer = (LexerRef)AllocatorAllocate(allocator, sizeof(struct _Lexer));
    assert(lexer);
    lexer->allocator                     = allocator;
    lexer->buffer                        = buffer;
    lexer->bufferStart                   = StringGetCharacters(buffer);
    lexer->bufferEnd                     = lexer->bufferStart + StringGetLength(buffer);
    lexer->state.lexer                   = lexer;
    lexer->state.cursor                  = lexer->bufferStart;
    lexer->state.line                    = 1;
    lexer->state.column                  = 0;
    lexer->stateStack                    = ArrayCreateEmpty(allocator, sizeof(struct _LexerState), 8);
    lexer->splitsConsecutivePunctuations = false;
    return lexer;
}

void LexerDestroy(LexerRef lexer) {
    ArrayDestroy(lexer->stateStack);
    AllocatorDeallocate(lexer->allocator, lexer);
}

LexerStateRef LexerGetState(LexerRef lexer) {
    LexerStateRef state = ArrayAppendUninitializedElement(lexer->stateStack);
    memcpy(state, &lexer->state, sizeof(struct _LexerState));
    return state;
}

void LexerSetState(LexerRef lexer, LexerStateRef state) {
    assert(lexer == state->lexer);
    lexer->state = *state;
}

Bool LexerGetSplitsConsecutivePunctuations(LexerRef lexer) {
    return lexer->splitsConsecutivePunctuations;
}

void LexerSetSplitsConsecutivePunctuations(LexerRef lexer, Bool splitsConsecutivePunctuations) {
    lexer->splitsConsecutivePunctuations = splitsConsecutivePunctuations;
}

void LexerPeekToken(LexerRef lexer, Token *token) {
    memcpy(token, &lexer->state.token, sizeof(Token));
}

void LexerNextToken(LexerRef lexer, Token *token) {
    _LexerLexNextToken(lexer);
    memcpy(token, &lexer->state.token, sizeof(Token));
}

static inline Bool _LexerSkipWhitespaceAndNewlines(LexerRef lexer) {
    while (lexer->state.cursor < lexer->bufferEnd) {
        switch (*lexer->state.cursor) {
        case 0x09:
        case 0x20:
            lexer->state.cursor += 1;
            lexer->state.column += 1;
            return true;

        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
            lexer->state.cursor += 1;
            lexer->state.line += 1;
            lexer->state.column = 0;
            return true;

        default:
            return false;
        }
    }

    return false;
}

static inline void _LexerSkipToEndOfLine(LexerRef lexer) {
    while (lexer->state.cursor < lexer->bufferEnd && *lexer->state.cursor != '\n' && *lexer->state.cursor != '\r') {
        lexer->state.cursor += 1;
        lexer->state.column += 1;
    }
}

static inline Bool _LexerSkipMultilineCommentTail(LexerRef lexer) {
    assert((*(lexer->state.cursor - 2) == '/' && *(lexer->state.cursor - 1) == '*') && "Invalid start of multiline comment!");

    while (lexer->state.cursor < lexer->bufferEnd) {
        if (*lexer->state.cursor == '*' && *(lexer->state.cursor + 1) == '/') {
            lexer->state.cursor += 2;
            lexer->state.column += 2;
            return true;
        }

        if (*lexer->state.cursor == '/' && *(lexer->state.cursor + 1) == '*') {
            lexer->state.cursor += 2;
            lexer->state.column += 2;

            if (!_LexerSkipMultilineCommentTail(lexer)) {
                return false;
            }
        }

        switch (*lexer->state.cursor) {
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
            lexer->state.cursor += 1;
            lexer->state.line += 1;
            lexer->state.column = 0;
            break;

        default:
            lexer->state.cursor += 1;
            lexer->state.column += 1;
            break;
        }
    }

    return false;
}

static inline void _LexerSkipAnyLiteralTail(LexerRef lexer) {
    // Consume all possible character continuations of the literal
    while (lexer->state.cursor < lexer->bufferEnd && _CharIsAlphaNumeric(*lexer->state.cursor)) {
        lexer->state.cursor += 1;
        lexer->state.column += 1;
    }
}

static inline TokenKind _LexerLexNumericLiteral(LexerRef lexer) {
    assert(_CharIsDecimalDigit(*(lexer->state.cursor - 1)));

    SourceRange location = {lexer->state.cursor - 1, lexer->state.cursor - 1};

    if (*(lexer->state.cursor - 1) == '0') {
        if (*lexer->state.cursor == 'b' || *lexer->state.cursor == 'B') {
            lexer->state.cursor += 1;
            lexer->state.column += 1;

            if (!_CharIsBinaryDigit(*lexer->state.cursor)) {
                _LexerSkipAnyLiteralTail(lexer);
                return TokenKindError;
            }

            while (lexer->state.cursor < lexer->bufferEnd && _CharIsBinaryDigit(*lexer->state.cursor)) {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
            }

            if (_CharIsContinuationOfIdentifier(*lexer->state.cursor)) {
                _LexerSkipAnyLiteralTail(lexer);
                return TokenKindError;
            }

            location.end              = lexer->state.cursor;
            SourceRange valueLocation = {location.start + 2, location.end};
            if (valueLocation.end - valueLocation.start - 1 > 64) {
                ReportError("Integer literal overflows");
            }

            UInt64 value = 0;
            for (const Char *cursor = valueLocation.start; cursor < valueLocation.end; cursor++) {
                value *= 2;
                value += *cursor - '0';
            }

            lexer->state.token.valueKind = TokenValueKindInt;
            lexer->state.token.intValue  = value;
            return TokenKindLiteralInt;
        }

        if (*lexer->state.cursor == 'o' || *lexer->state.cursor == 'O') {
            lexer->state.cursor += 1;
            lexer->state.column += 1;

            if (!_CharIsOctalDigit(*lexer->state.cursor)) {
                _LexerSkipAnyLiteralTail(lexer);
                return TokenKindError;
            }

            while (lexer->state.cursor < lexer->bufferEnd && _CharIsOctalDigit(*lexer->state.cursor)) {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
            }

            if (_CharIsContinuationOfIdentifier(*lexer->state.cursor)) {
                _LexerSkipAnyLiteralTail(lexer);
                return TokenKindError;
            }

            location.end              = lexer->state.cursor;
            SourceRange valueLocation = {location.start + 2, location.end};
            UInt64 value              = 0;
            Bool overflow             = false;
            for (const Char *cursor = valueLocation.start; cursor < valueLocation.end; cursor++) {
                UInt64 oldValue = value;

                value *= 8;
                value += *cursor - '0';

                if (value < oldValue) {
                    overflow = true;
                }
            }

            if (overflow) {
                ReportError("Integer literal overflows");
            }

            lexer->state.token.valueKind = TokenValueKindInt;
            lexer->state.token.intValue  = value;
            return TokenKindLiteralInt;
        }

        if (*lexer->state.cursor == 'x' || *lexer->state.cursor == 'X') {
            lexer->state.cursor += 1;
            lexer->state.column += 1;

            if (!_CharIsHexadecimalDigit(*lexer->state.cursor)) {
                _LexerSkipAnyLiteralTail(lexer);
                return TokenKindError;
            }

            while (lexer->state.cursor < lexer->bufferEnd && _CharIsHexadecimalDigit(*lexer->state.cursor)) {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
            }

            if (*lexer->state.cursor != '.' && *lexer->state.cursor != 'p' && *lexer->state.cursor != 'P') {
                if (_CharIsContinuationOfIdentifier(*lexer->state.cursor)) {
                    _LexerSkipAnyLiteralTail(lexer);
                    return TokenKindError;
                }

                location.end              = lexer->state.cursor;
                SourceRange valueLocation = {location.start + 2, location.end};
                UInt64 value              = 0;
                Bool overflow             = false;
                for (const Char *cursor = valueLocation.start; cursor < valueLocation.end; cursor++) {
                    UInt64 oldValue = value;

                    value *= 16;

                    if ('0' <= *cursor && *cursor <= '9') {
                        value += *cursor - '0';
                    } else if ('a' <= *cursor && *cursor <= 'f') {
                        value += *cursor - 'a' + 10;
                    } else if ('A' <= *cursor && *cursor <= 'F') {
                        value += *cursor - 'A' + 10;
                    } else {
                        return TokenKindError;
                    }

                    if (value < oldValue) {
                        overflow = true;
                    }
                }

                if (overflow) {
                    ReportError("Integer literal overflows");
                }

                lexer->state.token.valueKind = TokenValueKindInt;
                lexer->state.token.intValue  = value;
                return TokenKindLiteralInt;
            }

            location.end                 = lexer->state.cursor;
            SourceRange headLocation     = {location.start + 2, lexer->state.cursor};
            SourceRange tailLocation     = SourceRangeNull();
            SourceRange exponentLocation = SourceRangeNull();

            if (*lexer->state.cursor == '.') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;

                tailLocation.start = lexer->state.cursor;

                if (!_CharIsHexadecimalDigit(*lexer->state.cursor)) {
                    _LexerSkipAnyLiteralTail(lexer);
                    return TokenKindError;
                }

                while (lexer->state.cursor < lexer->bufferEnd && _CharIsHexadecimalDigit(*lexer->state.cursor)) {
                    lexer->state.cursor += 1;
                    lexer->state.column += 1;
                }

                tailLocation.end = lexer->state.cursor;
            }

            Bool isExponentPositive = true;
            if (*lexer->state.cursor == 'p' || *lexer->state.cursor == 'P') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;

                if (*lexer->state.cursor == '+') {
                    lexer->state.cursor += 1;
                    lexer->state.column += 1;
                } else if (*lexer->state.cursor == '-') {
                    lexer->state.cursor += 1;
                    lexer->state.column += 1;
                    isExponentPositive = false;
                }

                exponentLocation.start = lexer->state.cursor;

                if (!_CharIsHexadecimalDigit(*lexer->state.cursor)) {
                    _LexerSkipAnyLiteralTail(lexer);
                    return TokenKindError;
                }

                while (lexer->state.cursor < lexer->bufferEnd && _CharIsHexadecimalDigit(*lexer->state.cursor)) {
                    lexer->state.cursor += 1;
                    lexer->state.column += 1;
                }

                exponentLocation.end = lexer->state.cursor;
            }

            if (_CharIsContinuationOfIdentifier(*lexer->state.cursor)) {
                _LexerSkipAnyLiteralTail(lexer);
                return TokenKindError;
            }

            location.end = lexer->state.cursor;

            Float64 value = 0;
            for (const Char *cursor = headLocation.start; cursor < headLocation.end; cursor++) {
                value *= 16;

                if ('0' <= *cursor && *cursor <= '9') {
                    value += *cursor - '0';
                } else if ('a' <= *cursor && *cursor <= 'f') {
                    value += *cursor - 'a' + 10;
                } else if ('A' <= *cursor && *cursor <= 'F') {
                    value += *cursor - 'A' + 10;
                } else {
                    return NULL;
                }
            }

            if (tailLocation.start) {
                Float64 fraction = 0;
                for (const Char *cursor = tailLocation.start; cursor < tailLocation.end; cursor++) {
                    fraction *= 16;

                    if ('0' <= *cursor && *cursor <= '9') {
                        fraction += *cursor - '0';
                    } else if ('a' <= *cursor && *cursor <= 'f') {
                        fraction += *cursor - 'a' + 10;
                    } else if ('A' <= *cursor && *cursor <= 'F') {
                        fraction += *cursor - 'A' + 10;
                    } else {
                        return NULL;
                    }
                }

                for (const Char *cursor = tailLocation.start; cursor < tailLocation.end; cursor++) {
                    fraction /= 16;
                }

                value += fraction;
            }

            if (exponentLocation.start) {
                UInt64 exponent = 0;
                for (const Char *cursor = exponentLocation.start; cursor < exponentLocation.end; cursor++) {
                    UInt64 oldValue = exponent;

                    exponent *= 16;

                    if ('0' <= *cursor && *cursor <= '9') {
                        exponent += *cursor - '0';
                    } else if ('a' <= *cursor && *cursor <= 'f') {
                        exponent += *cursor - 'a' + 10;
                    } else if ('A' <= *cursor && *cursor <= 'F') {
                        exponent += *cursor - 'A' + 10;
                    } else {
                        return NULL;
                    }

                    if (exponent < oldValue) {
                        return NULL;
                    }
                }

                Float64 sign = isExponentPositive ? 10 : 0.1;
                while (exponent--) {
                    value *= sign;
                }
            }

            lexer->state.token.valueKind  = TokenValueKindFloat;
            lexer->state.token.floatValue = value;
            return TokenKindLiteralFloat;
        }
    }

    while (lexer->state.cursor < lexer->bufferEnd && _CharIsDecimalDigit(*lexer->state.cursor)) {
        lexer->state.cursor += 1;
        lexer->state.column += 1;
    }

    if (*lexer->state.cursor != '.' && *lexer->state.cursor != 'e' && *lexer->state.cursor != 'E') {
        if (_CharIsContinuationOfIdentifier(*lexer->state.cursor)) {
            _LexerSkipAnyLiteralTail(lexer);
            return TokenKindError;
        }

        location.end  = lexer->state.cursor;
        UInt64 value  = 0;
        Bool overflow = false;
        for (const Char *cursor = location.start; cursor < location.end; cursor++) {
            UInt64 oldValue = value;

            value *= 10;
            value += *cursor - '0';

            if (value < oldValue) {
                overflow = true;
            }
        }

        if (overflow) {
            ReportError("Integer literal overflows");
        }

        lexer->state.token.valueKind = TokenValueKindInt;
        lexer->state.token.intValue  = value;
        return TokenKindLiteralInt;
    }

    SourceRange headLocation     = {location.start, lexer->state.cursor};
    SourceRange tailLocation     = SourceRangeNull();
    SourceRange exponentLocation = SourceRangeNull();

    if (*lexer->state.cursor == '.') {
        lexer->state.cursor += 1;
        lexer->state.column += 1;

        tailLocation.start = lexer->state.cursor;

        if (!_CharIsDecimalDigit(*lexer->state.cursor)) {
            _LexerSkipAnyLiteralTail(lexer);
            return TokenKindError;
        }

        while (lexer->state.cursor < lexer->bufferEnd && _CharIsDecimalDigit(*lexer->state.cursor)) {
            lexer->state.cursor += 1;
            lexer->state.column += 1;
        }

        tailLocation.end = lexer->state.cursor;
    }

    Bool isExponentPositive = true;
    if (*lexer->state.cursor == 'e' || *lexer->state.cursor == 'E') {
        lexer->state.cursor += 1;
        lexer->state.column += 1;

        if (*lexer->state.cursor == '+') {
            lexer->state.cursor += 1;
            lexer->state.column += 1;
        } else if (*lexer->state.cursor == '-') {
            lexer->state.cursor += 1;
            lexer->state.column += 1;
            isExponentPositive = false;
        }

        exponentLocation.start = lexer->state.cursor;

        if (!_CharIsDecimalDigit(*lexer->state.cursor)) {
            _LexerSkipAnyLiteralTail(lexer);
            return TokenKindError;
        }

        while (lexer->state.cursor < lexer->bufferEnd && _CharIsDecimalDigit(*lexer->state.cursor)) {
            lexer->state.cursor += 1;
            lexer->state.column += 1;
        }

        exponentLocation.end = lexer->state.cursor;
    }

    if (_CharIsContinuationOfIdentifier(*lexer->state.cursor)) {
        _LexerSkipAnyLiteralTail(lexer);
        return TokenKindError;
    }

    location.end = lexer->state.cursor;

    Float64 value = 0;
    for (const Char *cursor = headLocation.start; cursor < headLocation.end; cursor++) {
        value *= 10;
        value += *cursor - '0';
    }

    if (tailLocation.start) {
        Float64 fraction = 0;
        for (const Char *cursor = tailLocation.start; cursor < tailLocation.end; cursor++) {
            fraction *= 10;
            fraction += *cursor - '0';
        }

        for (const Char *cursor = tailLocation.start; cursor < tailLocation.end; cursor++) {
            fraction /= 10;
        }

        value += fraction;
    }

    if (exponentLocation.start) {
        UInt64 exponent = 0;
        for (const Char *cursor = exponentLocation.start; cursor < exponentLocation.end; cursor++) {
            UInt64 oldValue = exponent;

            exponent *= 10;
            exponent += *cursor - '0';

            if (exponent < oldValue) {
                return NULL;
            }
        }

        Float64 sign = isExponentPositive ? 10 : 0.1;
        while (exponent--) {
            value *= sign;
        }
    }

    lexer->state.token.valueKind  = TokenValueKindFloat;
    lexer->state.token.floatValue = value;
    return TokenKindLiteralFloat;
}

static inline TokenKind _LexerLexDirective(LexerRef lexer) {
    assert(*(lexer->state.cursor - 1) == '#');

    SourceRange range = SourceRangeMake(lexer->state.cursor, lexer->state.cursor);

    while (lexer->state.cursor < lexer->bufferEnd) {
        if (_CharIsContinuationOfIdentifier(*lexer->state.cursor)) {
            lexer->state.cursor += 1;
            lexer->state.column += 1;
        } else {
            break;
        }
    }

    range.end      = lexer->state.cursor;
    TokenKind kind = TokenKindUnknown;

    if (SourceRangeIsEqual(range, "load")) {
        kind = TokenKindDirectiveLoad;
    } else if (SourceRangeIsEqual(range, "link")) {
        kind = TokenKindDirectiveLink;
    } else if (SourceRangeIsEqual(range, "intrinsic")) {
        kind = TokenKindDirectiveIntrinsic;
    } else if (SourceRangeIsEqual(range, "foreign")) {
        kind = TokenKindDirectiveForeign;
    } else if (SourceRangeIsEqual(range, "import")) {
        kind = TokenKindDirectiveImport;
    } else if (SourceRangeIsEqual(range, "include")) {
        kind = TokenKindDirectiveInclude;
    }

    return kind;
}

static inline TokenKind _LexerLexStringLiteral(LexerRef lexer) {
    assert(*(lexer->state.cursor - 1) == '"');

    SourceRange range = {lexer->state.cursor, lexer->state.cursor};
    Bool valid        = true;
    while (true) {
        if (lexer->state.cursor >= lexer->bufferEnd) {
            return TokenKindError;
        }

        switch (*lexer->state.cursor) {
        case '\n':
        case '\r':
        case '\0':
            return TokenKindError;

        case '"':
            lexer->state.cursor += 1;
            lexer->state.column += 1;

            if (valid) {
                range.end = lexer->state.cursor;
                return TokenKindLiteralString;
            }

            return TokenKindError;

        case '\\':
            lexer->state.cursor += 1;
            lexer->state.column += 1;

            switch (*lexer->state.cursor) {
            case '0':
            case '\\':
            case 'r':
            case 'n':
            case 't':
            case '"':
            case '\'':
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                break;

            default:
                valid = false;
                break;
            }

            break;

        default:
            lexer->state.cursor += 1;
            lexer->state.column += 1;
            break;
        }
    }
}

static inline TokenKind _LexerLexIdentifierOrKeyword(LexerRef lexer) {
    assert(_CharIsStartOfIdentifier(*(lexer->state.cursor - 1)));

    SourceRange range = SourceRangeMake(lexer->state.cursor - 1, lexer->state.cursor - 1);

    while (lexer->state.cursor < lexer->bufferEnd) {
        if (_CharIsContinuationOfIdentifier(*lexer->state.cursor)) {
            lexer->state.cursor += 1;
            lexer->state.column += 1;
        } else {
            break;
        }
    }

    range.end      = lexer->state.cursor;
    TokenKind kind = TokenKindUnknown;

    if (SourceRangeIsEqual(range, "is")) {
        kind = TokenKindKeywordIs;
    } else if (SourceRangeIsEqual(range, "as")) {
        if (*lexer->state.cursor == '!') {
            lexer->state.cursor += 1;
            lexer->state.column += 1;
            range.end = lexer->state.cursor;
            kind      = TokenKindKeywordAsExclamationMark;
        } else {
            kind = TokenKindKeywordAs;
        }
    } else if (SourceRangeIsEqual(range, "if")) {
        kind = TokenKindKeywordIf;
    } else if (SourceRangeIsEqual(range, "else")) {
        kind = TokenKindKeywordElse;
    } else if (SourceRangeIsEqual(range, "while")) {
        kind = TokenKindKeywordWhile;
    } else if (SourceRangeIsEqual(range, "do")) {
        kind = TokenKindKeywordDo;
    } else if (SourceRangeIsEqual(range, "case")) {
        kind = TokenKindKeywordCase;
    } else if (SourceRangeIsEqual(range, "switch")) {
        kind = TokenKindKeywordSwitch;
    } else if (SourceRangeIsEqual(range, "break")) {
        kind = TokenKindKeywordBreak;
    } else if (SourceRangeIsEqual(range, "continue")) {
        kind = TokenKindKeywordContinue;
    } else if (SourceRangeIsEqual(range, "fallthrough")) {
        kind = TokenKindKeywordFallthrough;
    } else if (SourceRangeIsEqual(range, "return")) {
        kind = TokenKindKeywordReturn;
    } else if (SourceRangeIsEqual(range, "nil")) {
        kind = TokenKindKeywordNil;
    } else if (SourceRangeIsEqual(range, "true")) {
        kind = TokenKindKeywordTrue;
    } else if (SourceRangeIsEqual(range, "false")) {
        kind = TokenKindKeywordFalse;
    } else if (SourceRangeIsEqual(range, "enum")) {
        kind = TokenKindKeywordEnum;
    } else if (SourceRangeIsEqual(range, "func")) {
        kind = TokenKindKeywordFunc;
    } else if (SourceRangeIsEqual(range, "prefix")) {
        kind = TokenKindKeywordPrefix;
    } else if (SourceRangeIsEqual(range, "infix")) {
        kind = TokenKindKeywordInfix;
    } else if (SourceRangeIsEqual(range, "struct")) {
        kind = TokenKindKeywordStruct;
    } else if (SourceRangeIsEqual(range, "var")) {
        kind = TokenKindKeywordVar;
    } else if (SourceRangeIsEqual(range, "init")) {
        kind = TokenKindKeywordInit;
    } else if (SourceRangeIsEqual(range, "typealias")) {
        kind = TokenKindKeywordTypeAlias;
    } else if (SourceRangeIsEqual(range, "sizeof")) {
        kind = TokenKindKeywordSizeOf;
    } else if (SourceRangeIsEqual(range, "Void")) {
        kind = TokenKindKeywordVoid;
    } else if (SourceRangeIsEqual(range, "Bool")) {
        kind = TokenKindKeywordBool;
    } else if (SourceRangeIsEqual(range, "Int8")) {
        kind = TokenKindKeywordInt8;
    } else if (SourceRangeIsEqual(range, "Int16")) {
        kind = TokenKindKeywordInt16;
    } else if (SourceRangeIsEqual(range, "Int32")) {
        kind = TokenKindKeywordInt32;
    } else if (SourceRangeIsEqual(range, "Int64")) {
        kind = TokenKindKeywordInt64;
    } else if (SourceRangeIsEqual(range, "Int")) {
        kind = TokenKindKeywordInt;
    } else if (SourceRangeIsEqual(range, "UInt8")) {
        kind = TokenKindKeywordUInt8;
    } else if (SourceRangeIsEqual(range, "UInt16")) {
        kind = TokenKindKeywordUInt16;
    } else if (SourceRangeIsEqual(range, "UInt32")) {
        kind = TokenKindKeywordUInt32;
    } else if (SourceRangeIsEqual(range, "UInt64")) {
        kind = TokenKindKeywordUInt64;
    } else if (SourceRangeIsEqual(range, "UInt")) {
        kind = TokenKindKeywordUInt;
    } else if (SourceRangeIsEqual(range, "Float32")) {
        kind = TokenKindKeywordFloat32;
    } else if (SourceRangeIsEqual(range, "Float64")) {
        kind = TokenKindKeywordFloat64;
    } else if (SourceRangeIsEqual(range, "Float")) {
        kind = TokenKindKeywordFloat;
    } else if (SourceRangeIsEqual(range, "module")) {
        kind = TokenKindKeywordModule;
    } else {
        kind = TokenKindIdentifier;
    }

    return kind;
}

static inline void _LexerLexNextToken(LexerRef lexer) {
    SourceRange leadingTriviaLocation = SourceRangeMake(lexer->state.cursor, lexer->state.cursor);

    lexer->state.token.valueKind = TokenValueKindNone;

    if (lexer->state.cursor >= lexer->bufferEnd) {
        lexer->state.token.kind           = TokenKindEndOfFile;
        lexer->state.token.location       = leadingTriviaLocation;
        lexer->state.token.line           = lexer->state.line;
        lexer->state.token.column         = lexer->state.column;
        lexer->state.token.leadingTrivia  = leadingTriviaLocation;
        lexer->state.token.trailingTrivia = leadingTriviaLocation;
        return;
    }

    while (lexer->state.cursor < lexer->bufferEnd) {
        Bool skipped = _LexerSkipWhitespaceAndNewlines(lexer);

        SourceRange location = SourceRangeMake(lexer->state.cursor, lexer->state.cursor);

        if (*lexer->state.cursor == '/' && *(lexer->state.cursor + 1) == '/') {
            _LexerSkipToEndOfLine(lexer);
        } else if (*lexer->state.cursor == '#' && *(lexer->state.cursor + 1) == '!') {
            _LexerSkipToEndOfLine(lexer);
        } else if (*lexer->state.cursor == '/' && *(lexer->state.cursor + 1) == '*') {
            lexer->state.cursor += 2;
            lexer->state.column += 2;

            if (!_LexerSkipMultilineCommentTail(lexer)) {
                location.end = lexer->state.cursor;

                lexer->state.token.kind           = TokenKindError;
                lexer->state.token.location       = location;
                lexer->state.token.line           = lexer->state.line;
                lexer->state.token.column         = lexer->state.column;
                lexer->state.token.leadingTrivia  = leadingTriviaLocation;
                lexer->state.token.trailingTrivia = SourceRangeMake(lexer->state.cursor, lexer->state.cursor);
                return;
            }
        } else if (!skipped) {
            break;
        }
    }

    leadingTriviaLocation.end = lexer->state.cursor;

    SourceRange tokenLocation = SourceRangeMake(lexer->state.cursor, lexer->state.cursor);
    TokenKind kind            = TokenKindUnknown;
    lexer->state.token.line   = lexer->state.line;
    lexer->state.token.column = lexer->state.column;

    switch (*lexer->state.cursor) {
    case '0' ... '9':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = _LexerLexNumericLiteral(lexer);
        break;

    case '#':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = _LexerLexDirective(lexer);
        break;

    case '"':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = _LexerLexStringLiteral(lexer);
        break;

    case 'a' ... 'z':
    case 'A' ... 'Z':
    case '_':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = _LexerLexIdentifierOrKeyword(lexer);
        break;

    case '\0':
        lexer->state.cursor += 1;
        kind = TokenKindEndOfFile;
        break;

    case '/':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindSlash;
        assert(*lexer->state.cursor != '/');
        assert(*lexer->state.cursor != '*');

        if (!lexer->splitsConsecutivePunctuations) {
            if (*lexer->state.cursor == '=') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindSlashEquals;
            }
        }

        break;

    case '=':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindEqualsSign;

        if (!lexer->splitsConsecutivePunctuations) {
            if (*lexer->state.cursor == '=') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindEqualsEqualsSign;
            }
        }

        break;

    case '-':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindMinusSign;

        if (!lexer->splitsConsecutivePunctuations) {
            if (*lexer->state.cursor == '>') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindArrow;
            } else if (*lexer->state.cursor == '=') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindMinusEqualsSign;
            }
        }

        break;

    case '+':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindPlusSign;

        if (!lexer->splitsConsecutivePunctuations) {
            if (*lexer->state.cursor == '=') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindPlusEquals;
            }
        }

        break;

    case '!':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindExclamationMark;

        if (!lexer->splitsConsecutivePunctuations) {
            if (*lexer->state.cursor == '=') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindExclamationMarkEqualsSign;
            }
        }

        break;

    case '*':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindAsterisk;

        if (!lexer->splitsConsecutivePunctuations) {
            if (*lexer->state.cursor == '=') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindAsteriskEquals;
            }
        }

        break;

    case '%':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindPercentSign;

        if (!lexer->splitsConsecutivePunctuations) {
            if (*lexer->state.cursor == '=') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindPercentEquals;
            }
        }

        break;

    case '.':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindDot;
        break;

    case '<':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindLessThan;

        if (!lexer->splitsConsecutivePunctuations) {
            if (*lexer->state.cursor == '<') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindLessThanLessThan;

                if (*lexer->state.cursor == '=') {
                    lexer->state.cursor += 1;
                    lexer->state.column += 1;
                    kind = TokenKindLessThanLessThanEquals;
                }

            } else if (*lexer->state.cursor == '=') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindLessThanEqualsSign;
            }
        }

        break;

    case '>':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindGreaterThan;

        if (!lexer->splitsConsecutivePunctuations) {
            if (*lexer->state.cursor == '>') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindGreaterThanGreaterThan;

                if (*lexer->state.cursor == '=') {
                    lexer->state.cursor += 1;
                    lexer->state.column += 1;
                    kind = TokenKindGreaterThanGreaterThanEquals;
                }
            } else if (*lexer->state.cursor == '=') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindGreaterThanEqualsSign;
            }
        }

        break;

    case '&':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindAmpersand;

        if (!lexer->splitsConsecutivePunctuations) {
            if (*lexer->state.cursor == '&') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindAmpersandAmpersand;
            } else if (*lexer->state.cursor == '=') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindAmpersandEquals;
            }
        }

        break;

    case '|':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindPipe;

        if (!lexer->splitsConsecutivePunctuations) {
            if (*lexer->state.cursor == '|') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindPipePipe;
            } else if (*lexer->state.cursor == '=') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindPipeEquals;
            }
        }

        break;

    case '^':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindCircumflex;

        if (!lexer->splitsConsecutivePunctuations) {
            if (*lexer->state.cursor == '=') {
                lexer->state.cursor += 1;
                lexer->state.column += 1;
                kind = TokenKindCircumflexEquals;
            }
        }

        break;

    case '(':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindLeftParenthesis;
        break;

    case ')':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindRightParenthesis;
        break;

    case ':':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindColon;
        break;

    case '[':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindLeftBracket;
        break;

    case ']':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindRightBracket;
        break;

    case '{':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindLeftCurlyBracket;
        break;

    case '}':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindRightCurlyBracket;
        break;

    case '~':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindTilde;
        break;

    case ',':
        lexer->state.cursor += 1;
        lexer->state.column += 1;
        kind = TokenKindComma;
        break;

    default:
        lexer->state.cursor += 1;
        kind = TokenKindUnknown;
        break;
    }

    tokenLocation.end = lexer->state.cursor;

    SourceRange trailingTriviaLocation = SourceRangeMake(lexer->state.cursor, lexer->state.cursor);
    if (lexer->state.cursor >= lexer->bufferEnd || lexer->state.cursor == '\0') {
        _LexerSkipWhitespaceAndNewlines(lexer);
        trailingTriviaLocation.end = lexer->state.cursor;
    }

    lexer->state.token.kind           = kind;
    lexer->state.token.location       = tokenLocation;
    lexer->state.token.leadingTrivia  = leadingTriviaLocation;
    lexer->state.token.trailingTrivia = trailingTriviaLocation;
}

static inline Bool _CharIsAlphaNumeric(Char character) {
    switch (character) {
    case 'a' ... 'z':
    case 'A' ... 'Z':
    case '0' ... '9':
        return true;
    default:
        return false;
    }
}

static inline Bool _CharIsStartOfIdentifier(Char character) {
    switch (character) {
    case 'a' ... 'z':
    case 'A' ... 'Z':
    case '_':
        return true;

    default:
        return false;
    }
}

static inline Bool _CharIsContinuationOfIdentifier(Char character) {
    switch (character) {
    case 'a' ... 'z':
    case 'A' ... 'Z':
    case '_':
    case '0' ... '9':
        return true;

    default:
        return false;
    }
}

static inline Bool _CharIsBinaryDigit(Char character) {
    switch (character) {
    case '0' ... '1':
        return true;
    default:
        return false;
    }
}

static inline Bool _CharIsOctalDigit(Char character) {
    switch (character) {
    case '0' ... '7':
        return true;
    default:
        return false;
    }
}

static inline Bool _CharIsDecimalDigit(Char character) {
    switch (character) {
    case '0' ... '9':
        return true;
    default:
        return false;
    }
}

static inline Bool _CharIsHexadecimalDigit(Char character) {
    switch (character) {
    case '0' ... '9':
    case 'a' ... 'f':
    case 'A' ... 'F':
        return true;
    default:
        return false;
    }
}
