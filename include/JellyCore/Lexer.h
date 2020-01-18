#ifndef __JELLY_LEXER__
#define __JELLY_LEXER__

#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>
#include <JellyCore/SourceRange.h>
#include <JellyCore/String.h>

JELLY_EXTERN_C_BEGIN

enum _TokenKind {
    TokenKindUnknown,
    TokenKindError,
    TokenKindSlash,
    TokenKindSlashEquals,
    TokenKindEqualsSign,
    TokenKindEqualsEqualsSign,
    TokenKindMinusSign,
    TokenKindMinusEqualsSign,
    TokenKindPlusSign,
    TokenKindPlusEquals,
    TokenKindExclamationMark,
    TokenKindExclamationMarkEqualsSign,
    TokenKindAsterisk,
    TokenKindAsteriskEquals,
    TokenKindPercentSign,
    TokenKindPercentEquals,
    TokenKindDot,
    TokenKindLessThan,
    TokenKindLessThanLessThan,
    TokenKindLessThanLessThanEquals,
    TokenKindLessThanEqualsSign,
    TokenKindGreaterThan,
    TokenKindGreaterThanGreaterThan,
    TokenKindGreaterThanGreaterThanEquals,
    TokenKindGreaterThanEqualsSign,
    TokenKindAmpersand,
    TokenKindAmpersandAmpersand,
    TokenKindAmpersandEquals,
    TokenKindPipe,
    TokenKindPipePipe,
    TokenKindPipeEquals,
    TokenKindCircumflex,
    TokenKindCircumflexEquals,
    TokenKindLeftParenthesis,
    TokenKindRightParenthesis,
    TokenKindColon,
    TokenKindLeftBracket,
    TokenKindRightBracket,
    TokenKindLeftCurlyBracket,
    TokenKindRightCurlyBracket,
    TokenKindComma,
    TokenKindTilde,
    TokenKindArrow,
    TokenKindIdentifier,
    TokenKindKeywordIs,
    TokenKindKeywordAs,
    TokenKindKeywordAsExclamationMark,
    TokenKindKeywordIf,
    TokenKindKeywordElse,
    TokenKindKeywordWhile,
    TokenKindKeywordDo,
    TokenKindKeywordCase,
    TokenKindKeywordSwitch,
    TokenKindKeywordBreak,
    TokenKindKeywordContinue,
    TokenKindKeywordFallthrough,
    TokenKindKeywordReturn,
    TokenKindKeywordNil,
    TokenKindKeywordTrue,
    TokenKindKeywordFalse,
    TokenKindKeywordEnum,
    TokenKindKeywordFunc,
    TokenKindKeywordPrefix,
    TokenKindKeywordInfix,
    TokenKindKeywordStruct,
    TokenKindKeywordVar,
    TokenKindKeywordInit,
    TokenKindKeywordTypeAlias,
    TokenKindKeywordSizeOf,
    TokenKindKeywordVoid,
    TokenKindKeywordBool,
    TokenKindKeywordInt8,
    TokenKindKeywordInt16,
    TokenKindKeywordInt32,
    TokenKindKeywordInt64,
    TokenKindKeywordInt,
    TokenKindKeywordUInt8,
    TokenKindKeywordUInt16,
    TokenKindKeywordUInt32,
    TokenKindKeywordUInt64,
    TokenKindKeywordUInt,
    TokenKindKeywordFloat32,
    TokenKindKeywordFloat64,
    TokenKindKeywordFloat,
    TokenKindKeywordModule,
    TokenKindDirectiveLoad,
    TokenKindDirectiveLink,
    TokenKindDirectiveIntrinsic,
    TokenKindDirectiveForeign,
    TokenKindDirectiveImport,
    TokenKindDirectiveInclude,
    TokenKindLiteralString,
    TokenKindLiteralInt,
    TokenKindLiteralFloat,
    TokenKindEndOfFile,
};
typedef enum _TokenKind TokenKind;

enum _TokenValueKind {
    TokenValueKindNone,
    TokenValueKindBool,
    TokenValueKindInt,
    TokenValueKindFloat,
};
typedef enum _TokenValueKind TokenValueKind;

struct _Token {
    TokenKind kind;
    SourceRange location;

    Index line;
    Index column;
    SourceRange leadingTrivia;
    SourceRange trailingTrivia;

    TokenValueKind valueKind;
    union {
        Bool boolValue;
        UInt64 intValue;
        Float64 floatValue;
    };
};
typedef struct _Token Token;

typedef struct _Lexer *LexerRef;
typedef struct _LexerState *LexerStateRef;

LexerRef LexerCreate(AllocatorRef allocator, StringRef buffer);

void LexerDestroy(LexerRef lexer);

LexerStateRef LexerGetState(LexerRef lexer);
void LexerSetState(LexerRef lexer, LexerStateRef state);

Bool LexerGetSplitsConsecutivePunctuations(LexerRef lexer);
void LexerSetSplitsConsecutivePunctuations(LexerRef lexer, Bool splitsConsecutivePunctuations);

void LexerPeekToken(LexerRef lexer, Token *token);
void LexerNextToken(LexerRef lexer, Token *token);

JELLY_EXTERN_C_END

#endif
