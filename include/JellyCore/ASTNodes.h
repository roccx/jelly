#ifndef __JELLY_ASTNODES__
#define __JELLY_ASTNODES__

#include <JellyCore/ASTArray.h>
#include <JellyCore/Array.h>
#include <JellyCore/Base.h>
#include <JellyCore/SourceRange.h>
#include <JellyCore/String.h>
#include <JellyCore/SymbolTable.h>

JELLY_EXTERN_C_BEGIN

typedef void *IRRef;

enum _ASTTag {
    ASTTagSourceUnit,
    ASTTagLinkedList,
    ASTTagArray,
    ASTTagLoadDirective,
    ASTTagLinkDirective,
    ASTTagImportDirective,
    ASTTagIncludeDirective,
    ASTTagBlock,
    ASTTagIfStatement,
    ASTTagLoopStatement,
    ASTTagCaseStatement,
    ASTTagSwitchStatement,
    ASTTagControlStatement,
    ASTTagReferenceExpression,
    ASTTagDereferenceExpression,
    ASTTagUnaryExpression,
    ASTTagBinaryExpression,
    ASTTagIdentifierExpression,
    ASTTagMemberAccessExpression,
    ASTTagAssignmentExpression,
    ASTTagCallExpression,
    ASTTagConstantExpression,
    ASTTagSizeOfExpression,
    ASTTagSubscriptExpression,
    ASTTagTypeOperationExpression,
    ASTTagTypeExpression,
    ASTTagModuleDeclaration,
    ASTTagEnumerationDeclaration,
    ASTTagFunctionDeclaration,
    ASTTagForeignFunctionDeclaration,
    ASTTagIntrinsicFunctionDeclaration,
    ASTTagStructureDeclaration,
    ASTTagInitializerDeclaration,
    ASTTagValueDeclaration,
    ASTTagTypeAliasDeclaration,
    ASTTagOpaqueType,
    ASTTagPointerType,
    ASTTagArrayType,
    ASTTagBuiltinType,
    ASTTagEnumerationType,
    ASTTagFunctionType,
    ASTTagStructureType,
    ASTTagGenericType,

    AST_TAG_COUNT
};
typedef enum _ASTTag ASTTag;

// TODO: Add error flag for validity checks
enum _ASTFlags {
    ASTFlagsNone                       = 0,
    ASTFlagsStructureHasCyclicStorage  = 1 << 0,
    ASTFlagsStatementIsAlwaysReturning = 1 << 1,
    ASTFlagsSwitchIsExhaustive         = 1 << 2,
    ASTFlagsIsValidated                = 1 << 3,
    ASTFlagsBlockHasTerminator         = 1 << 4,
    ASTFlagsIsValuePointer             = 1 << 5,
    ASTFlagsIsConstantEvaluable        = 1 << 6,
    ASTFlagsIsPointerArithmetic        = 1 << 7,
    ASTFlagsCallIsInitialization       = 1 << 8,
    ASTFlagsArrayTypeIsStatic          = 1 << 9,
};
typedef enum _ASTFlags ASTFlags;

typedef Index ASTOperatorPrecedence;

enum _ASTOperatorAssociativity {
    ASTOperatorAssociativityNone,
    ASTOperatorAssociativityLeft,
    ASTOperatorAssociativityRight,
};
typedef enum _ASTOperatorAssociativity ASTOperatorAssociativity;

typedef struct _ASTNode *ASTNodeRef;
typedef struct _ASTExpression *ASTExpressionRef;
typedef struct _ASTDeclaration *ASTDeclarationRef;
typedef struct _ASTNode *ASTTypeRef;

typedef struct _ASTSourceUnit *ASTSourceUnitRef;
typedef struct _ASTLinkedList *ASTLinkedListRef;
typedef struct _ASTLoadDirective *ASTLoadDirectiveRef;
typedef struct _ASTLinkDirective *ASTLinkDirectiveRef;
typedef struct _ASTImportDirective *ASTImportDirectiveRef;
typedef struct _ASTIncludeDirective *ASTIncludeDirectiveRef;
typedef struct _ASTBlock *ASTBlockRef;
typedef struct _ASTTypeAliasDeclaration *ASTTypeAliasDeclarationRef;
typedef struct _ASTIfStatement *ASTIfStatementRef;
typedef struct _ASTLoopStatement *ASTLoopStatementRef;
typedef struct _ASTCaseStatement *ASTCaseStatementRef;
typedef struct _ASTSwitchStatement *ASTSwitchStatementRef;
typedef struct _ASTControlStatement *ASTControlStatementRef;
typedef struct _ASTReferenceExpression *ASTReferenceExpressionRef;
typedef struct _ASTDereferenceExpression *ASTDereferenceExpressionRef;
typedef struct _ASTUnaryExpression *ASTUnaryExpressionRef;
typedef struct _ASTBinaryExpression *ASTBinaryExpressionRef;
typedef struct _ASTIdentifierExpression *ASTIdentifierExpressionRef;
typedef struct _ASTMemberAccessExpression *ASTMemberAccessExpressionRef;
typedef struct _ASTAssignmentExpression *ASTAssignmentExpressionRef;
typedef struct _ASTCallExpression *ASTCallExpressionRef;
typedef struct _ASTConstantExpression *ASTConstantExpressionRef;
typedef struct _ASTSizeOfExpression *ASTSizeOfExpressionRef;
typedef struct _ASTSubscriptExpression *ASTSubscriptExpressionRef;
typedef struct _ASTTypeOperationExpression *ASTTypeOperationExpressionRef;
typedef struct _ASTTypeExpression *ASTTypeExpressionRef;
typedef struct _ASTModuleDeclaration *ASTModuleDeclarationRef;
typedef struct _ASTEnumerationDeclaration *ASTEnumerationDeclarationRef;
typedef struct _ASTFunctionDeclaration *ASTFunctionDeclarationRef;
typedef struct _ASTStructureDeclaration *ASTStructureDeclarationRef;
typedef struct _ASTInitializerDeclaration *ASTInitializerDeclarationRef;
typedef struct _ASTValueDeclaration *ASTValueDeclarationRef;
typedef struct _ASTOpaqueType *ASTOpaqueTypeRef;
typedef struct _ASTPointerType *ASTPointerTypeRef;
typedef struct _ASTArrayType *ASTArrayTypeRef;
typedef struct _ASTBuiltinType *ASTBuiltinTypeRef;
typedef struct _ASTEnumerationType *ASTEnumerationTypeRef;
typedef struct _ASTFunctionType *ASTFunctionTypeRef;
typedef struct _ASTStructureType *ASTStructureTypeRef;
typedef struct _ASTGenericType *ASTGenericTypeRef;

struct _ASTNode {
    ASTTag tag;
    ASTFlags flags;
    SourceRange location;
    ScopeID scope;
    ASTNodeRef substitute;
    ASTNodeRef primary;

    IRRef irValue;
    IRRef irType;
};

struct _ASTExpression {
    struct _ASTNode base;

    ASTTypeRef type;
    ASTTypeRef expectedType;
};

struct _ASTLinkedList {
    struct _ASTNode base;

    void *node;
    ASTLinkedListRef next;
};

struct _ASTArray {
    struct _ASTNode base;

    void *context;
    Index elementCount;
    ASTLinkedListRef list;
};

struct _ASTSourceUnit {
    struct _ASTNode base;

    StringRef filePath;
    ASTArrayRef declarations;
};

struct _ASTLoadDirective {
    struct _ASTNode base;

    ASTConstantExpressionRef filePath;
};

struct _ASTLinkDirective {
    struct _ASTNode base;

    Bool isFramework;
    StringRef library;
};

struct _ASTImportDirective {
    struct _ASTNode base;

    StringRef modulePath;
};

struct _ASTIncludeDirective {
    struct _ASTNode base;

    StringRef headerPath;
};

struct _ASTBlock {
    struct _ASTNode base;

    ASTArrayRef statements;
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

    IRRef irEntry;
    IRRef irExit;
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
    ASTSwitchStatementRef enclosingSwitch;
    ASTFunctionDeclarationRef comparator;

    IRRef irNext;
};

struct _ASTSwitchStatement {
    struct _ASTNode base;

    ASTExpressionRef argument;
    ASTArrayRef cases;

    IRRef irExit;
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
    ASTNodeRef enclosingNode;
};

struct _ASTReferenceExpression {
    struct _ASTExpression base;

    ASTExpressionRef argument;
};

struct _ASTDereferenceExpression {
    struct _ASTExpression base;

    ASTExpressionRef argument;
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
    struct _ASTExpression base;

    ASTUnaryOperator op;
    ASTExpressionRef arguments[1];
    ASTFunctionDeclarationRef opFunction;
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
    ASTBinaryOperatorTypeBitcast,
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
    struct _ASTExpression base;

    ASTBinaryOperator op;
    ASTExpressionRef arguments[2];
    ASTFunctionDeclarationRef opFunction;
};

enum _ASTPostfixOperator {
    ASTPostfixOperatorUnknown,
    ASTPostfixOperatorSelector,
    ASTPostfixOperatorCall,
    ASTPostfixOperatorSubscript,
};
typedef enum _ASTPostfixOperator ASTPostfixOperator;

struct _ASTIdentifierExpression {
    struct _ASTExpression base;

    StringRef name;
    ASTArrayRef candidateDeclarations;
    ASTDeclarationRef resolvedDeclaration;
};

struct _ASTMemberAccessExpression {
    struct _ASTExpression base;

    ASTExpressionRef argument;
    StringRef memberName;
    Int memberIndex;
    Index pointerDepth;
    ASTDeclarationRef resolvedDeclaration;
};

struct _ASTAssignmentExpression {
    struct _ASTExpression base;

    ASTBinaryOperator op;
    ASTExpressionRef variable;
    ASTExpressionRef expression;
};

enum _ASTFixity {
    ASTFixityNone,
    ASTFixityPrefix,
    ASTFixityInfix,
    ASTFixityPostfix,
};
typedef enum _ASTFixity ASTFixity;

struct _ASTCallExpression {
    struct _ASTExpression base;

    ASTFixity fixity;
    ASTExpressionRef callee;
    ASTArrayRef arguments;
    union {
        ASTUnaryOperator unary;
        ASTBinaryOperator binary;
    } op;
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
    struct _ASTExpression base;

    ASTConstantKind kind;
    Int8 minimumBitWidth;
    union {
        Bool boolValue;
        UInt64 intValue;
        Float64 floatValue;
        StringRef stringValue;
    };
};

struct _ASTSizeOfExpression {
    struct _ASTExpression base;

    ASTTypeRef sizeType;
};

struct _ASTSubscriptExpression {
    struct _ASTExpression base;

    ASTExpressionRef expression;
    ASTArrayRef arguments;
};

enum _ASTTypeOperation {
    ASTTypeOperationTypeCheck,
    ASTTypeOperationTypeCast,
    ASTTypeOperationTypeBitcast,
};
typedef enum _ASTTypeOperation ASTTypeOperation;

struct _ASTTypeOperationExpression {
    struct _ASTExpression base;

    ASTTypeOperation op;
    ASTExpressionRef expression;
    ASTTypeRef argumentType;
};

struct _ASTTypeExpression {
    struct _ASTExpression base;

    ASTTypeRef referencedType;
};

// TODO: Move the innerScope value from the subtypes to base declaration!
struct _ASTDeclaration {
    struct _ASTNode base;

    StringRef name;
    StringRef mangledName;
    ASTTypeRef type;
};

enum _ASTModuleKind {
    ASTModuleKindExecutable,
    ASTModuleKindLibrary,
    ASTModuleKindInterface,
};
typedef enum _ASTModuleKind ASTModuleKind;

struct _ASTModuleDeclaration {
    struct _ASTDeclaration base;

    ASTModuleKind kind;
    // TODO: @SourceUnitTree Replace sourceUnits with a single ASTSourceUnitRef and move additional source units into ASTSourceUnitRef where
    // they get loaded...
    ASTArrayRef sourceUnits;

    // TODO: Move scope to ASTSourceUnitRef after resolving @SourceUnitTree
    ScopeID innerScope;
    ASTArrayRef importedModules;
    ASTArrayRef linkDirectives;

    StringRef entryPointName;
    ASTFunctionDeclarationRef entryPoint;
};

struct _ASTEnumerationDeclaration {
    struct _ASTDeclaration base;

    ASTArrayRef elements;
    ScopeID innerScope;
};

struct _ASTFunctionDeclaration {
    struct _ASTDeclaration base;

    ASTFixity fixity;
    ASTArrayRef parameters;
    ASTTypeRef returnType;
    ASTBlockRef body;
    ScopeID innerScope;
    StringRef foreignName;
    StringRef intrinsicName;
};

struct _ASTStructureDeclaration {
    struct _ASTDeclaration base;

    ASTArrayRef values;
    ASTArrayRef initializers;
    ScopeID innerScope;
};

struct _ASTInitializerDeclaration {
    struct _ASTDeclaration base;

    ASTArrayRef parameters;
    ASTBlockRef body;
    ScopeID innerScope;
    ASTStructureDeclarationRef structure;
    ASTValueDeclarationRef implicitSelf;
};

enum _ASTValueKind {
    ASTValueKindVariable,
    ASTValueKindParameter,
    ASTValueKindEnumerationElement,
};
typedef enum _ASTValueKind ASTValueKind;

struct _ASTValueDeclaration {
    struct _ASTDeclaration base;

    ASTValueKind kind;
    ASTExpressionRef initializer;
};

struct _ASTTypeAliasDeclaration {
    struct _ASTDeclaration base;
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
    UInt64 sizeValue;
};

enum _ASTBuiltinTypeKind {
    ASTBuiltinTypeKindError,
    ASTBuiltinTypeKindVoid,
    ASTBuiltinTypeKindBool,
    ASTBuiltinTypeKindInt8,
    ASTBuiltinTypeKindInt16,
    ASTBuiltinTypeKindInt32,
    ASTBuiltinTypeKindInt64,
    ASTBuiltinTypeKindInt,
    ASTBuiltinTypeKindUInt8,
    ASTBuiltinTypeKindUInt16,
    ASTBuiltinTypeKindUInt32,
    ASTBuiltinTypeKindUInt64,
    ASTBuiltinTypeKindUInt,
    ASTBuiltinTypeKindFloat32,
    ASTBuiltinTypeKindFloat64,
    ASTBuiltinTypeKindFloat,

    AST_BUILTIN_TYPE_KIND_COUNT,
};
typedef enum _ASTBuiltinTypeKind ASTBuiltinTypeKind;

struct _ASTBuiltinType {
    struct _ASTNode base;

    ASTBuiltinTypeKind kind;
};

struct _ASTEnumerationType {
    struct _ASTNode base;

    ASTEnumerationDeclarationRef declaration;
};

struct _ASTFunctionType {
    struct _ASTNode base;

    ASTArrayRef parameterTypes;
    ASTTypeRef resultType;
    ASTFunctionDeclarationRef declaration;
};

struct _ASTStructureType {
    struct _ASTNode base;

    ASTStructureDeclarationRef declaration;
};

struct _ASTGenericType {
    struct _ASTNode base;

    ASTTypeRef baseType;
    ASTArrayRef arguments;
    ScopeID argumentScope;
};

JELLY_EXTERN_C_END

#endif
