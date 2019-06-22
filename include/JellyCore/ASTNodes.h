#ifndef __JELLY_ASTNODES__
#define __JELLY_ASTNODES__

#include <JellyCore/Array.h>
#include <JellyCore/Base.h>
#include <JellyCore/SourceRange.h>
#include <JellyCore/String.h>
#include <JellyCore/SymbolTable.h>

JELLY_EXTERN_C_BEGIN

enum _ASTTag {
    ASTTagSourceUnit,
    ASTTagLinkedList,
    ASTTagLoadDirective,
    ASTTagBlock,
    ASTTagIfStatement,
    ASTTagLoopStatement,
    ASTTagCaseStatement,
    ASTTagSwitchStatement,
    ASTTagControlStatement,
    ASTTagUnaryExpression,
    ASTTagBinaryExpression,
    ASTTagIdentifierExpression,
    ASTTagMemberAccessExpression,
    ASTTagCallExpression,
    ASTTagConstantExpression,
    ASTTagModuleDeclaration,
    ASTTagEnumerationDeclaration,
    ASTTagFunctionDeclaration,
    ASTTagStructureDeclaration,
    ASTTagOpaqueDeclaration,
    ASTTagValueDeclaration,
    ASTTagOpaqueType,
    ASTTagPointerType,
    ASTTagArrayType,
    ASTTagBuiltinType,

    AST_TAG_COUNT
};
typedef enum _ASTTag ASTTag;

typedef Index ASTOperatorPrecedence;

enum _ASTOperatorAssociativity {
    ASTOperatorAssociativityNone,
    ASTOperatorAssociativityLeft,
    ASTOperatorAssociativityRight,
};
typedef enum _ASTOperatorAssociativity ASTOperatorAssociativity;

typedef struct _ASTNode *ASTNodeRef;
typedef struct _ASTNode *ASTExpressionRef;
typedef struct _ASTNode *ASTDeclarationRef;
typedef struct _ASTNode *ASTTypeRef;

typedef struct _ASTSourceUnit *ASTSourceUnitRef;
typedef struct _ASTLinkedList *ASTLinkedListRef;
typedef struct _ASTLoadDirective *ASTLoadDirectiveRef;
typedef struct _ASTBlock *ASTBlockRef;
typedef struct _ASTIfStatement *ASTIfStatementRef;
typedef struct _ASTLoopStatement *ASTLoopStatementRef;
typedef struct _ASTCaseStatement *ASTCaseStatementRef;
typedef struct _ASTSwitchStatement *ASTSwitchStatementRef;
typedef struct _ASTControlStatement *ASTControlStatementRef;
typedef struct _ASTUnaryExpression *ASTUnaryExpressionRef;
typedef struct _ASTBinaryExpression *ASTBinaryExpressionRef;
typedef struct _ASTIdentifierExpression *ASTIdentifierExpressionRef;
typedef struct _ASTMemberAccessExpression *ASTMemberAccessExpressionRef;
typedef struct _ASTCallExpression *ASTCallExpressionRef;
typedef struct _ASTConstantExpression *ASTConstantExpressionRef;
typedef struct _ASTModuleDeclaration *ASTModuleDeclarationRef;
typedef struct _ASTEnumerationDeclaration *ASTEnumerationDeclarationRef;
typedef struct _ASTFunctionDeclaration *ASTFunctionDeclarationRef;
typedef struct _ASTStructureDeclaration *ASTStructureDeclarationRef;
typedef struct _ASTOpaqueDeclaration *ASTOpaqueDeclarationRef;
typedef struct _ASTValueDeclaration *ASTValueDeclarationRef;
typedef struct _ASTOpaqueType *ASTOpaqueTypeRef;
typedef struct _ASTPointerType *ASTPointerTypeRef;
typedef struct _ASTArrayType *ASTArrayTypeRef;
typedef struct _ASTBuiltinType *ASTBuiltinTypeRef;

struct _ASTNode {
    ASTTag tag;
    SourceRange location;
    ScopeRef scope;
};

struct _ASTLinkedList {
    struct _ASTNode base;

    ASTNodeRef node;
    ASTLinkedListRef next;
};

struct _ASTSourceUnit {
    struct _ASTNode base;

    StringRef filePath;
    ASTLinkedListRef declarations;
};

struct _ASTLoadDirective {
    struct _ASTNode base;

    ASTConstantExpressionRef filePath;
};

struct _ASTBlock {
    struct _ASTNode base;

    ASTLinkedListRef statements;
};

struct _ASTIfStatement {
    struct _ASTNode base;

    ASTExpressionRef condition;
    ASTBlockRef thenBlock;
    ASTBlockRef elseBlock;
};

enum _ASTLoopKind {
    ASTLoopKindDo,
    ASTLoopKindWhile,
};
typedef enum _ASTLoopKind ASTLoopKind;

struct _ASTLoopStatement {
    struct _ASTNode base;

    ASTLoopKind kind;
    ASTExpressionRef condition;
    ASTBlockRef loopBlock;
};

enum _ASTCaseKind {
    ASTCaseKindConditional,
    ASTCaseKindElse,
};
typedef enum _ASTCaseKind ASTCaseKind;

struct _ASTCaseStatement {
    struct _ASTNode base;

    ASTCaseKind kind;
    ASTExpressionRef condition;
    ASTBlockRef body;
};

struct _ASTSwitchStatement {
    struct _ASTNode base;

    ASTExpressionRef argument;
    ASTLinkedListRef cases;
};

enum _ASTControlKind {
    ASTControlKindBreak,
    ASTControlKindContinue,
    ASTControlKindFallthrough,
    ASTControlKindReturn,
};
typedef enum _ASTControlKind ASTControlKind;

struct _ASTControlStatement {
    struct _ASTNode base;

    ASTControlKind kind;
    ASTExpressionRef result;
};

enum _ASTUnaryOperator {
    ASTUnaryOperatorUnknown,
    ASTUnaryOperatorLogicalNot,
    ASTUnaryOperatorBitwiseNot,
    ASTUnaryOperatorUnaryPlus,
    ASTUnaryOperatorUnaryMinus,
};
typedef enum _ASTUnaryOperator ASTUnaryOperator;

struct _ASTUnaryExpression {
    struct _ASTNode base;

    ASTUnaryOperator op;
    ASTExpressionRef arguments[1];
};

enum _ASTBinaryOperator {
    ASTBinaryOperatorUnknown,
    ASTBinaryOperatorBitwiseLeftShift,
    ASTBinaryOperatorBitwiseRightShift,
    ASTBinaryOperatorMultiply,
    ASTBinaryOperatorDivide,
    ASTBinaryOperatorReminder,
    ASTBinaryOperatorBitwiseAnd,
    ASTBinaryOperatorAdd,
    ASTBinaryOperatorSubtract,
    ASTBinaryOperatorBitwiseOr,
    ASTBinaryOperatorBitwiseXor,
    ASTBinaryOperatorTypeCheck,
    ASTBinaryOperatorTypeCast,
    ASTBinaryOperatorLessThan,
    ASTBinaryOperatorLessThanEqual,
    ASTBinaryOperatorGreaterThan,
    ASTBinaryOperatorGreaterThanEqual,
    ASTBinaryOperatorEqual,
    ASTBinaryOperatorNotEqual,
    ASTBinaryOperatorLogicalAnd,
    ASTBinaryOperatorLogicalOr,
    ASTBinaryOperatorAssign,
    ASTBinaryOperatorMultiplyAssign,
    ASTBinaryOperatorDivideAssign,
    ASTBinaryOperatorReminderAssign,
    ASTBinaryOperatorAddAssign,
    ASTBinaryOperatorSubtractAssign,
    ASTBinaryOperatorBitwiseLeftShiftAssign,
    ASTBinaryOperatorBitwiseRightShiftAssign,
    ASTBinaryOperatorBitwiseAndAssign,
    ASTBinaryOperatorBitwiseOrAssign,
    ASTBinaryOperatorBitwiseXorAssign,
};
typedef enum _ASTBinaryOperator ASTBinaryOperator;

struct _ASTBinaryExpression {
    struct _ASTNode base;

    ASTBinaryOperator op;
    ASTExpressionRef arguments[2];
};

enum _ASTPostfixOperator {
    ASTPostfixOperatorUnknown,
    ASTPostfixOperatorSelector,
    ASTPostfixOperatorCall,
};
typedef enum _ASTPostfixOperator ASTPostfixOperator;

struct _ASTIdentifierExpression {
    struct _ASTNode base;

    StringRef name;
    ASTDeclarationRef declaration;
};

struct _ASTMemberAccessExpression {
    struct _ASTNode base;

    ASTExpressionRef argument;
    StringRef memberName;
};

struct _ASTCallExpression {
    struct _ASTNode base;

    ASTExpressionRef callee;
    ASTLinkedListRef arguments;
};

enum _ASTConstantKind {
    ASTConstantKindNil,
    ASTConstantKindBool,
    ASTConstantKindInt,
    ASTConstantKindFloat,
    ASTConstantKindString,
};
typedef enum _ASTConstantKind ASTConstantKind;

struct _ASTConstantExpression {
    struct _ASTNode base;

    ASTConstantKind kind;
    union {
        Bool boolValue;
        UInt64 intValue;
        Float64 floatValue;
        StringRef stringValue;
    };
};

struct _ASTModuleDeclaration {
    struct _ASTNode base;

    ASTLinkedListRef sourceUnits;
    ASTLinkedListRef importedModules;
};

struct _ASTEnumerationDeclaration {
    struct _ASTNode base;

    StringRef name;
    ASTLinkedListRef elements;
};

struct _ASTFunctionDeclaration {
    struct _ASTNode base;

    StringRef name;
    ASTLinkedListRef parameters;
    ASTTypeRef returnType;
    ASTBlockRef body;
};

struct _ASTStructureDeclaration {
    struct _ASTNode base;

    StringRef name;
    ASTLinkedListRef values;
};

struct _ASTOpaqueDeclaration {
    struct _ASTNode base;

    StringRef name;
};

enum _ASTValueKind {
    ASTValueKindVariable,
    ASTValueKindParameter,
    ASTValueKindEnumerationElement,
};
typedef enum _ASTValueKind ASTValueKind;

struct _ASTValueDeclaration {
    struct _ASTNode base;

    ASTValueKind kind;
    StringRef name;
    ASTTypeRef type;
    ASTExpressionRef initializer;
};

struct _ASTOpaqueType {
    struct _ASTNode base;

    StringRef name;
    ASTDeclarationRef declaration;
};

struct _ASTPointerType {
    struct _ASTNode base;

    ASTTypeRef pointeeType;
};

struct _ASTArrayType {
    struct _ASTNode base;

    ASTTypeRef elementType;
    ASTExpressionRef size;
};

enum _ASTBuiltinTypeKind {
    ASTBuiltinTypeKindError,
    ASTBuiltinTypeKindVoid,
    ASTBuiltinTypeKindBool,
    ASTBuiltinTypeKindInt8,
    ASTBuiltinTypeKindInt16,
    ASTBuiltinTypeKindInt32,
    ASTBuiltinTypeKindInt64,
    ASTBuiltinTypeKindInt128,
    ASTBuiltinTypeKindInt,
    ASTBuiltinTypeKindUInt8,
    ASTBuiltinTypeKindUInt16,
    ASTBuiltinTypeKindUInt32,
    ASTBuiltinTypeKindUInt64,
    ASTBuiltinTypeKindUInt128,
    ASTBuiltinTypeKindUInt,
    ASTBuiltinTypeKindFloat16,
    ASTBuiltinTypeKindFloat32,
    ASTBuiltinTypeKindFloat64,
    ASTBuiltinTypeKindFloat80,
    ASTBuiltinTypeKindFloat128,
    ASTBuiltinTypeKindFloat,

    AST_BUILTIN_TYPE_KIND_COUNT,
};
typedef enum _ASTBuiltinTypeKind ASTBuiltinTypeKind;

struct _ASTBuiltinType {
    struct _ASTNode base;

    ASTBuiltinTypeKind kind;
    StringRef name;
};

JELLY_EXTERN_C_END

#endif
