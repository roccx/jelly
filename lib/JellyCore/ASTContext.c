#include "JellyCore/ASTContext.h"
#include "JellyCore/ASTFunctions.h"
#include "JellyCore/ASTMangling.h"
#include "JellyCore/ASTNodes.h"
#include "JellyCore/SymbolTable.h"
#include "JellyCore/TempAllocator.h"

// TODO: Add unified identifier storage and remove temp allocator!
struct _ASTContext {
    AllocatorRef allocator;
    AllocatorRef tempAllocator;
    SymbolTableRef symbolTable;
    BucketArrayRef nodes[AST_TAG_COUNT];
    ASTModuleDeclarationRef module;

    ASTModuleDeclarationRef builtinModule;
    ASTBuiltinTypeRef builtinTypes[AST_BUILTIN_TYPE_KIND_COUNT];
    ASTStructureTypeRef stringType;
};

ASTNodeRef _ASTContextCreateNode(ASTContextRef context, ASTTag tag, SourceRange location, ScopeID scope);
ASTBuiltinTypeRef _ASTContextCreateBuiltinType(ASTContextRef context, SourceRange location, ScopeID scope, ASTBuiltinTypeKind builtinKind);
void _ASTContextInitBuiltinTypes(ASTContextRef context);
void _ASTContextInitBuiltinFunctions(ASTContextRef context);

ASTBuiltinTypeRef _ASTContextGetBuiltinTypeByName(ASTContextRef context, const Char *name);

ASTContextRef ASTContextCreate(AllocatorRef allocator, StringRef moduleName) {
    ASTContextRef context                        = AllocatorAllocate(allocator, sizeof(struct _ASTContext));
    context->allocator                           = allocator;
    context->tempAllocator                       = TempAllocatorCreate(allocator);
    context->symbolTable                         = SymbolTableCreate(allocator);
    context->nodes[ASTTagSourceUnit]             = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTSourceUnit), 8);
    context->nodes[ASTTagLinkedList]             = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTLinkedList), 8);
    context->nodes[ASTTagArray]                  = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTArray), 8);
    context->nodes[ASTTagLoadDirective]          = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTLoadDirective), 8);
    context->nodes[ASTTagLinkDirective]          = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTLinkDirective), 8);
    context->nodes[ASTTagImportDirective]        = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTImportDirective), 8);
    context->nodes[ASTTagIncludeDirective]       = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTIncludeDirective), 8);
    context->nodes[ASTTagBlock]                  = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTBlock), 8);
    context->nodes[ASTTagIfStatement]            = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTIfStatement), 8);
    context->nodes[ASTTagLoopStatement]          = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTLoopStatement), 8);
    context->nodes[ASTTagCaseStatement]          = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTCaseStatement), 8);
    context->nodes[ASTTagSwitchStatement]        = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTSwitchStatement), 8);
    context->nodes[ASTTagControlStatement]       = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTControlStatement), 8);
    context->nodes[ASTTagReferenceExpression]    = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTReferenceExpression), 8);
    context->nodes[ASTTagDereferenceExpression]  = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTDereferenceExpression), 8);
    context->nodes[ASTTagUnaryExpression]        = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTUnaryExpression), 8);
    context->nodes[ASTTagBinaryExpression]       = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTBinaryExpression), 8);
    context->nodes[ASTTagIdentifierExpression]   = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTIdentifierExpression), 8);
    context->nodes[ASTTagMemberAccessExpression] = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTMemberAccessExpression), 8);
    context->nodes[ASTTagAssignmentExpression]   = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTAssignmentExpression), 8);
    context->nodes[ASTTagCallExpression]         = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTCallExpression), 8);
    context->nodes[ASTTagConstantExpression]     = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTConstantExpression), 8);
    context->nodes[ASTTagSizeOfExpression]       = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTSizeOfExpression), 8);
    context->nodes[ASTTagSubscriptExpression]    = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTSubscriptExpression), 8);
    context->nodes[ASTTagTypeOperationExpression] = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTTypeOperationExpression),
                                                                           8);
    context->nodes[ASTTagTypeExpression]          = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTTypeExpression), 8);
    context->nodes[ASTTagModuleDeclaration]       = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTModuleDeclaration), 8);
    context->nodes[ASTTagEnumerationDeclaration] = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTEnumerationDeclaration), 8);
    context->nodes[ASTTagFunctionDeclaration]    = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTFunctionDeclaration), 8);
    context->nodes[ASTTagForeignFunctionDeclaration]   = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTFunctionDeclaration),
                                                                              8);
    context->nodes[ASTTagIntrinsicFunctionDeclaration] = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTFunctionDeclaration),
                                                                                8);
    context->nodes[ASTTagGenericFunctionDeclaration]   = BucketArrayCreateEmpty(context->allocator,
                                                                              sizeof(struct _ASTGenericFunctionDeclaration), 8);
    context->nodes[ASTTagStructureDeclaration] = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTStructureDeclaration), 8);
    context->nodes[ASTTagGenericStructureDeclaration] = BucketArrayCreateEmpty(context->allocator,
                                                                               sizeof(struct _ASTGenericStructureDeclaration), 8);
    context->nodes[ASTTagInitializerDeclaration] = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTInitializerDeclaration), 8);
    context->nodes[ASTTagValueDeclaration]       = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTValueDeclaration), 8);
    context->nodes[ASTTagTypeAliasDeclaration]   = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTTypeAliasDeclaration), 8);
    context->nodes[ASTTagOpaqueType]             = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTOpaqueType), 8);
    context->nodes[ASTTagPointerType]            = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTPointerType), 8);
    context->nodes[ASTTagArrayType]              = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTArrayType), 8);
    context->nodes[ASTTagBuiltinType]            = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTBuiltinType), 8);
    context->nodes[ASTTagEnumerationType]        = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTEnumerationType), 8);
    context->nodes[ASTTagFunctionType]           = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTFunctionType), 8);
    context->nodes[ASTTagStructureType]          = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTStructureType), 8);
    context->nodes[ASTTagGenericType]            = BucketArrayCreateEmpty(context->allocator, sizeof(struct _ASTGenericType), 8);
    context->module = ASTContextCreateModuleDeclaration(context, SourceRangeNull(), NULL, ASTModuleKindExecutable, moduleName, NULL, NULL);
    SymbolTableSetScopeUserdata(context->symbolTable, kScopeGlobal, context->module);
    _ASTContextInitBuiltinTypes(context);
    _ASTContextInitBuiltinFunctions(context);
    return context;
}

void ASTContextDestroy(ASTContextRef context) {
    for (Index index = 0; index < AST_TAG_COUNT; index++) {
        BucketArrayDestroy(context->nodes[index]);
    }

    SymbolTableDestroy(context->symbolTable);
    AllocatorDestroy(context->tempAllocator);
    AllocatorDeallocate(context->allocator, context);
}

AllocatorRef ASTContextGetTempAllocator(ASTContextRef context) {
    return context->tempAllocator;
}

SymbolTableRef ASTContextGetSymbolTable(ASTContextRef context) {
    return context->symbolTable;
}

ASTModuleDeclarationRef ASTContextGetModule(ASTContextRef context) {
    return context->module;
}

BucketArrayRef ASTContextGetAllNodes(ASTContextRef context, ASTTag tag) {
    return context->nodes[tag];
}

void ASTModuleAddSourceUnit(ASTContextRef context, ASTModuleDeclarationRef module, ASTSourceUnitRef sourceUnit) {
    ASTArrayAppendElement(module->sourceUnits, sourceUnit);
}

ASTSourceUnitRef ASTContextCreateSourceUnit(ASTContextRef context, SourceRange location, ScopeID scope, StringRef filePath,
                                            ArrayRef declarations) {
    assert(filePath);

    ASTSourceUnitRef node = (ASTSourceUnitRef)_ASTContextCreateNode(context, ASTTagSourceUnit, location, scope);
    node->filePath        = StringCreateCopy(context->tempAllocator, filePath);
    node->declarations    = ASTContextCreateArray(context, location, scope);
    if (declarations) {
        ASTArrayAppendArray(node->declarations, declarations);
    }
    return node;
}

ASTLinkedListRef ASTContextCreateLinkedList(ASTContextRef context, SourceRange location, ScopeID scope) {
    ASTLinkedListRef list = (ASTLinkedListRef)_ASTContextCreateNode(context, ASTTagLinkedList, location, scope);
    list->node            = NULL;
    list->next            = NULL;
    return list;
}

ASTArrayRef ASTContextCreateArray(ASTContextRef context, SourceRange location, ScopeID scope) {
    ASTArrayRef array   = (ASTArrayRef)_ASTContextCreateNode(context, ASTTagArray, location, scope);
    array->context      = context;
    array->elementCount = 0;
    array->list         = NULL;
    return array;
}

ASTLoadDirectiveRef ASTContextCreateLoadDirective(ASTContextRef context, SourceRange location, ScopeID scope,
                                                  ASTConstantExpressionRef filePath) {
    assert(filePath && filePath->kind == ASTConstantKindString);

    ASTLoadDirectiveRef node = (ASTLoadDirectiveRef)_ASTContextCreateNode(context, ASTTagLoadDirective, location, scope);
    node->filePath           = filePath;
    return node;
}

ASTLinkDirectiveRef ASTContextCreateLinkDirective(ASTContextRef context, SourceRange location, ScopeID scope, Bool isFramework,
                                                  StringRef library) {
    assert(library);

    ASTLinkDirectiveRef node = (ASTLinkDirectiveRef)_ASTContextCreateNode(context, ASTTagLinkDirective, location, scope);
    node->isFramework        = isFramework;
    node->library            = StringCreateCopy(context->tempAllocator, library);
    return node;
}

ASTImportDirectiveRef ASTContextCreateImportDirective(ASTContextRef context, SourceRange location, ScopeID scope, StringRef modulePath) {
    assert(modulePath);

    ASTImportDirectiveRef node = (ASTImportDirectiveRef)_ASTContextCreateNode(context, ASTTagImportDirective, location, scope);
    node->modulePath           = StringCreateCopy(context->tempAllocator, modulePath);
    return node;
}

ASTIncludeDirectiveRef ASTContextCreateIncludeDirective(ASTContextRef context, SourceRange location, ScopeID scope, StringRef headerPath) {
    assert(headerPath);

    ASTIncludeDirectiveRef node = (ASTIncludeDirectiveRef)_ASTContextCreateNode(context, ASTTagIncludeDirective, location, scope);
    node->headerPath            = StringCreateCopy(context->tempAllocator, headerPath);
    return node;
}

ASTBlockRef ASTContextCreateBlock(ASTContextRef context, SourceRange location, ScopeID scope, ArrayRef statements) {
    ASTBlockRef node = (ASTBlockRef)_ASTContextCreateNode(context, ASTTagBlock, location, scope);
    node->statements = ASTContextCreateArray(context, location, scope);
    if (statements) {
        ASTArrayAppendArray(node->statements, statements);
    }
    return node;
}

ASTIfStatementRef ASTContextCreateIfStatement(ASTContextRef context, SourceRange location, ScopeID scope, ASTExpressionRef condition,
                                              ASTBlockRef thenBlock, ASTBlockRef elseBlock) {
    assert(condition && thenBlock && elseBlock);

    ASTIfStatementRef node = (ASTIfStatementRef)_ASTContextCreateNode(context, ASTTagIfStatement, location, scope);
    node->condition        = condition;
    node->thenBlock        = thenBlock;
    node->elseBlock        = elseBlock;
    return node;
}

ASTLoopStatementRef ASTContextCreateLoopStatement(ASTContextRef context, SourceRange location, ScopeID scope, ASTLoopKind kind,
                                                  ASTExpressionRef condition, ASTBlockRef loopBlock) {
    assert(condition && loopBlock);

    ASTLoopStatementRef node = (ASTLoopStatementRef)_ASTContextCreateNode(context, ASTTagLoopStatement, location, scope);
    node->kind               = kind;
    node->condition          = condition;
    node->loopBlock          = loopBlock;
    node->irEntry            = NULL;
    node->irExit             = NULL;
    return node;
}

ASTCaseStatementRef ASTContextCreateCaseStatement(ASTContextRef context, SourceRange location, ScopeID scope, ASTCaseKind kind,
                                                  ASTExpressionRef condition, ASTBlockRef body) {
    assert((kind == ASTCaseKindElse || condition) && body);

    ASTCaseStatementRef node = (ASTCaseStatementRef)_ASTContextCreateNode(context, ASTTagCaseStatement, location, scope);
    node->kind               = kind;
    node->condition          = condition;
    node->body               = body;
    node->enclosingSwitch    = NULL;
    node->comparator         = NULL;
    node->irNext             = NULL;
    return node;
}

ASTSwitchStatementRef ASTContextCreateSwitchStatement(ASTContextRef context, SourceRange location, ScopeID scope, ASTExpressionRef argument,
                                                      ArrayRef cases) {
    assert(argument);

    ASTSwitchStatementRef node = (ASTSwitchStatementRef)_ASTContextCreateNode(context, ASTTagSwitchStatement, location, scope);
    node->argument             = argument;
    node->cases                = ASTContextCreateArray(context, location, scope);
    node->irExit               = NULL;
    if (cases) {
        ASTArrayAppendArray(node->cases, cases);
    }
    return node;
}

ASTControlStatementRef ASTContextCreateControlStatement(ASTContextRef context, SourceRange location, ScopeID scope, ASTControlKind kind,
                                                        ASTExpressionRef result) {
    ASTControlStatementRef node = (ASTControlStatementRef)_ASTContextCreateNode(context, ASTTagControlStatement, location, scope);
    node->kind                  = kind;
    node->result                = result;
    node->enclosingNode         = NULL;
    return node;
}

ASTReferenceExpressionRef ASTContextCreateReferenceExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                              ASTExpressionRef argument) {
    assert(argument);

    ASTReferenceExpressionRef node = (ASTReferenceExpressionRef)_ASTContextCreateNode(context, ASTTagReferenceExpression, location, scope);
    node->argument                 = argument;
    node->base.type                = NULL;
    node->base.expectedType        = NULL;
    return node;
}

ASTDereferenceExpressionRef ASTContextCreateDereferenceExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                  ASTExpressionRef argument) {
    assert(argument);

    ASTDereferenceExpressionRef node = (ASTDereferenceExpressionRef)_ASTContextCreateNode(context, ASTTagDereferenceExpression, location,
                                                                                          scope);
    node->argument                   = argument;
    node->base.type                  = NULL;
    node->base.expectedType          = NULL;
    return node;
}

ASTUnaryExpressionRef ASTContextCreateUnaryExpression(ASTContextRef context, SourceRange location, ScopeID scope, ASTUnaryOperator op,
                                                      ASTExpressionRef arguments[1]) {
    assert(arguments[0]);

    ASTUnaryExpressionRef node = (ASTUnaryExpressionRef)_ASTContextCreateNode(context, ASTTagUnaryExpression, location, scope);
    node->op                   = op;
    node->arguments[0]         = arguments[0];
    node->opFunction           = NULL;
    node->base.type            = NULL;
    node->base.expectedType    = NULL;
    return node;
}

ASTBinaryExpressionRef ASTContextCreateBinaryExpression(ASTContextRef context, SourceRange location, ScopeID scope, ASTBinaryOperator op,
                                                        ASTExpressionRef arguments[2]) {
    assert(arguments[0] && arguments[1]);

    ASTBinaryExpressionRef node = (ASTBinaryExpressionRef)_ASTContextCreateNode(context, ASTTagBinaryExpression, location, scope);
    node->op                    = op;
    node->arguments[0]          = arguments[0];
    node->arguments[1]          = arguments[1];
    node->opFunction            = NULL;
    node->base.type             = NULL;
    node->base.expectedType     = NULL;
    return node;
}

ASTIdentifierExpressionRef ASTContextCreateIdentifierExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                StringRef name) {
    assert(name);

    ASTIdentifierExpressionRef node = (ASTIdentifierExpressionRef)_ASTContextCreateNode(context, ASTTagIdentifierExpression, location,
                                                                                        scope);
    node->name                      = StringCreateCopy(context->tempAllocator, name);
    node->base.type                 = NULL;
    node->base.expectedType         = NULL;
    node->candidateDeclarations     = ASTContextCreateArray(context, location, scope);
    node->resolvedDeclaration       = NULL;
    return node;
}

ASTMemberAccessExpressionRef ASTContextCreateMemberAccessExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                    ASTExpressionRef argument, StringRef memberName) {
    assert(argument && memberName);

    ASTMemberAccessExpressionRef node = (ASTMemberAccessExpressionRef)_ASTContextCreateNode(context, ASTTagMemberAccessExpression, location,
                                                                                            scope);
    node->argument                    = argument;
    node->memberName                  = StringCreateCopy(context->tempAllocator, memberName);
    node->memberIndex                 = -1;
    node->pointerDepth                = 0;
    node->base.type                   = NULL;
    node->base.expectedType           = NULL;
    return node;
}

ASTAssignmentExpressionRef ASTContextCreateAssignmentExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                ASTBinaryOperator op, ASTExpressionRef variable,
                                                                ASTExpressionRef expression) {
    assert(variable && expression);

    ASTAssignmentExpressionRef node = (ASTAssignmentExpressionRef)_ASTContextCreateNode(context, ASTTagAssignmentExpression, location,
                                                                                        scope);
    node->op                        = op;
    node->variable                  = variable;
    node->expression                = expression;
    return node;
}

ASTCallExpressionRef ASTContextCreateCallExpression(ASTContextRef context, SourceRange location, ScopeID scope, ASTExpressionRef callee,
                                                    ArrayRef arguments) {
    assert(callee);

    ASTCallExpressionRef node = (ASTCallExpressionRef)_ASTContextCreateNode(context, ASTTagCallExpression, location, scope);
    node->fixity              = ASTFixityNone;
    node->callee              = callee;
    node->arguments           = ASTContextCreateArray(context, location, scope);
    node->op.unary            = ASTUnaryOperatorUnknown;
    node->base.type           = NULL;
    node->base.expectedType   = NULL;
    if (arguments) {
        ASTArrayAppendArray(node->arguments, arguments);
    }
    return node;
}

ASTCallExpressionRef ASTContextCreateUnaryCallExpression(ASTContextRef context, SourceRange location, ScopeID scope, ASTUnaryOperator op,
                                                         ASTExpressionRef arguments[1]) {
    assert(op != ASTUnaryOperatorUnknown);

    StringRef name            = ASTGetPrefixOperatorName(context->tempAllocator, op);
    ASTCallExpressionRef node = (ASTCallExpressionRef)_ASTContextCreateNode(context, ASTTagCallExpression, location, scope);
    node->fixity              = ASTFixityPrefix;
    node->callee              = (ASTExpressionRef)ASTContextCreateIdentifierExpression(context, location, scope, name);
    node->arguments           = ASTContextCreateArray(context, location, scope);
    node->op.unary            = op;
    node->base.type           = NULL;
    node->base.expectedType   = NULL;
    ASTArrayAppendElement(node->arguments, arguments[0]);
    return node;
}

ASTCallExpressionRef ASTContextCreateBinaryCallExpression(ASTContextRef context, SourceRange location, ScopeID scope, ASTBinaryOperator op,
                                                          ASTExpressionRef arguments[2]) {
    assert(op != ASTUnaryOperatorUnknown);

    StringRef name            = ASTGetInfixOperatorName(context->tempAllocator, op);
    ASTCallExpressionRef node = (ASTCallExpressionRef)_ASTContextCreateNode(context, ASTTagCallExpression, location, scope);
    node->fixity              = ASTFixityInfix;
    node->callee              = (ASTExpressionRef)ASTContextCreateIdentifierExpression(context, location, scope, name);
    node->arguments           = ASTContextCreateArray(context, location, scope);
    node->op.binary           = op;
    node->base.type           = NULL;
    node->base.expectedType   = NULL;
    ASTArrayAppendElement(node->arguments, arguments[0]);
    ASTArrayAppendElement(node->arguments, arguments[1]);
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantNilExpression(ASTContextRef context, SourceRange location, ScopeID scope) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location, scope);
    node->kind                    = ASTConstantKindNil;
    node->minimumBitWidth         = -1;
    node->base.type               = NULL;
    node->base.expectedType       = NULL;
    node->base.base.flags |= ASTFlagsIsConstantEvaluable;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantBoolExpression(ASTContextRef context, SourceRange location, ScopeID scope, Bool value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location, scope);
    node->kind                    = ASTConstantKindBool;
    node->minimumBitWidth         = 1;
    node->boolValue               = value;
    node->base.type               = NULL;
    node->base.expectedType       = NULL;
    node->base.base.flags |= ASTFlagsIsConstantEvaluable;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantIntExpression(ASTContextRef context, SourceRange location, ScopeID scope, UInt64 value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location, scope);
    node->kind                    = ASTConstantKindInt;
    node->intValue                = value;
    node->base.type               = NULL;
    node->base.expectedType       = NULL;
    node->base.base.flags |= ASTFlagsIsConstantEvaluable;

    node->minimumBitWidth = 0;
    UInt64 intValue       = value;
    while (intValue > 0) {
        node->minimumBitWidth += 1;
        intValue >>= 1;
    }
    node->minimumBitWidth = MAX(node->minimumBitWidth, 1);

    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantFloatExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                 Float64 value) {
    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location, scope);
    node->kind                    = ASTConstantKindFloat;
    node->minimumBitWidth         = -1;
    node->floatValue              = value;
    node->base.type               = NULL;
    node->base.expectedType       = NULL;
    node->base.base.flags |= ASTFlagsIsConstantEvaluable;
    return node;
}

ASTConstantExpressionRef ASTContextCreateConstantStringExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                  StringRef value) {
    assert(value);

    ASTConstantExpressionRef node = (ASTConstantExpressionRef)_ASTContextCreateNode(context, ASTTagConstantExpression, location, scope);
    node->kind                    = ASTConstantKindString;
    node->stringValue             = StringCreateCopy(context->tempAllocator, value);
    node->base.type               = NULL;
    node->base.expectedType       = NULL;
    node->base.base.flags |= ASTFlagsIsConstantEvaluable;
    return node;
}

ASTSizeOfExpressionRef ASTContextCreateSizeOfExpression(ASTContextRef context, SourceRange location, ScopeID scope, ASTTypeRef sizeType) {
    ASTSizeOfExpressionRef node = (ASTSizeOfExpressionRef)_ASTContextCreateNode(context, ASTTagSizeOfExpression, location, scope);
    node->sizeType              = sizeType;
    node->base.type             = NULL;
    node->base.expectedType     = NULL;
    node->base.base.flags |= ASTFlagsIsConstantEvaluable;
    return node;
}

ASTSubscriptExpressionRef ASTContextCreateSubscriptExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                              ASTExpressionRef expression, ArrayRef arguments) {
    ASTSubscriptExpressionRef node = (ASTSubscriptExpressionRef)_ASTContextCreateNode(context, ASTTagSubscriptExpression, location, scope);
    node->expression               = expression;
    node->arguments                = ASTContextCreateArray(context, location, scope);
    if (arguments) {
        ASTArrayAppendArray(node->arguments, arguments);
    }
    return node;
}

ASTTypeOperationExpressionRef ASTContextCreateTypeOperationExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                      ASTTypeOperation op, ASTExpressionRef expression,
                                                                      ASTTypeRef expressionType) {
    ASTTypeOperationExpressionRef node = (ASTTypeOperationExpressionRef)_ASTContextCreateNode(context, ASTTagTypeOperationExpression,
                                                                                              location, scope);
    node->op                           = op;
    node->expression                   = expression;
    node->argumentType                 = expressionType;
    node->base.type                    = NULL;
    node->base.expectedType            = NULL;
    return node;
}

ASTTypeExpressionRef ASTContextCreateTypeExpression(ASTContextRef context, SourceRange location, ScopeID scope, ASTTypeRef referencedType) {
    ASTTypeExpressionRef node = (ASTTypeExpressionRef)_ASTContextCreateNode(context, ASTTagTypeExpression, location, scope);
    node->referencedType      = referencedType;
    node->base.type           = NULL;
    node->base.expectedType   = NULL;
    return node;
}

ASTModuleDeclarationRef ASTContextCreateModuleDeclaration(ASTContextRef context, SourceRange location, ScopeID scope, ASTModuleKind kind,
                                                          StringRef name, ArrayRef sourceUnits, ArrayRef importedModules) {
    ASTModuleDeclarationRef node = (ASTModuleDeclarationRef)_ASTContextCreateNode(context, ASTTagModuleDeclaration, location, scope);
    node->base.name              = StringCreateCopy(context->tempAllocator, name);
    node->base.mangledName       = NULL;
    node->base.type              = NULL;
    node->kind                   = kind;
    node->innerScope             = kScopeGlobal;
    node->sourceUnits            = ASTContextCreateArray(context, location, scope);
    node->importedModules        = ASTContextCreateArray(context, location, scope);
    node->linkDirectives         = ASTContextCreateArray(context, location, scope);
    node->entryPointName         = StringCreate(context->tempAllocator, "main");
    node->entryPoint             = NULL;
    if (sourceUnits) {
        ASTArrayAppendArray(node->sourceUnits, sourceUnits);
    }
    if (importedModules) {
        ASTArrayAppendArray(node->importedModules, importedModules);
    }
    return node;
}

ASTEnumerationDeclarationRef ASTContextCreateEnumerationDeclaration(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                    StringRef name, ArrayRef elements) {
    assert(name);

    ASTEnumerationDeclarationRef node = (ASTEnumerationDeclarationRef)_ASTContextCreateNode(context, ASTTagEnumerationDeclaration, location,
                                                                                            scope);
    node->base.name                   = StringCreateCopy(context->tempAllocator, name);
    node->base.mangledName            = NULL;
    node->elements                    = ASTContextCreateArray(context, location, scope);
    node->innerScope                  = kScopeNull;
    if (elements) {
        ASTArrayAppendArray(node->elements, elements);
    }
    node->base.type = (ASTTypeRef)ASTContextCreateEnumerationType(context, location, scope, node);
    return node;
}

ASTFunctionDeclarationRef ASTContextCreateFunctionDeclaration(ASTContextRef context, SourceRange location, ScopeID scope, ASTFixity fixity,
                                                              StringRef name, ArrayRef parameters, ASTTypeRef returnType,
                                                              ASTBlockRef body) {
    assert(name && returnType && body);

    ASTFunctionDeclarationRef node = (ASTFunctionDeclarationRef)_ASTContextCreateNode(context, ASTTagFunctionDeclaration, location, scope);
    node->base.name                = StringCreateCopy(context->tempAllocator, name);
    node->base.mangledName         = NULL;
    node->fixity                   = ASTFixityNone;
    node->parameters               = ASTContextCreateArray(context, location, scope);
    node->returnType               = returnType;
    node->body                     = body;
    node->innerScope               = kScopeNull;
    if (parameters) {
        ASTArrayAppendArray(node->parameters, parameters);
    }
    node->base.type     = (ASTTypeRef)ASTContextCreateFunctionTypeForDeclaration(context, location, scope, node);
    node->foreignName   = NULL;
    node->intrinsicName = NULL;
    return node;
}

ASTFunctionDeclarationRef ASTContextCreateForeignFunctionDeclaration(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                     ASTFixity fixity, StringRef name, ArrayRef parameters,
                                                                     ASTTypeRef returnType, StringRef foreignName) {
    assert(name && returnType && foreignName);

    ASTFunctionDeclarationRef node = (ASTFunctionDeclarationRef)_ASTContextCreateNode(context, ASTTagForeignFunctionDeclaration, location,
                                                                                      scope);
    node->base.name                = StringCreateCopy(context->tempAllocator, name);
    node->base.mangledName         = NULL;
    node->fixity                   = fixity;
    node->parameters               = ASTContextCreateArray(context, location, scope);
    node->returnType               = returnType;
    node->body                     = NULL;
    if (parameters) {
        ASTArrayAppendArray(node->parameters, parameters);
    }
    node->base.type     = (ASTTypeRef)ASTContextCreateFunctionTypeForDeclaration(context, location, scope, node);
    node->foreignName   = StringCreateCopy(context->tempAllocator, foreignName);
    node->intrinsicName = NULL;
    return node;
}

ASTFunctionDeclarationRef ASTContextCreateIntrinsicFunctionDeclaration(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                       ASTFixity fixity, StringRef name, ArrayRef parameters,
                                                                       ASTTypeRef returnType, StringRef intrinsicName) {
    assert(name && returnType && intrinsicName);

    ASTFunctionDeclarationRef node = (ASTFunctionDeclarationRef)_ASTContextCreateNode(context, ASTTagIntrinsicFunctionDeclaration, location,
                                                                                      scope);
    node->base.name                = StringCreateCopy(context->tempAllocator, name);
    node->base.mangledName         = NULL;
    node->fixity                   = fixity;
    node->parameters               = ASTContextCreateArray(context, location, scope);
    node->returnType               = returnType;
    node->body                     = NULL;
    if (parameters) {
        ASTArrayAppendArray(node->parameters, parameters);
    }
    node->base.type     = (ASTTypeRef)ASTContextCreateFunctionTypeForDeclaration(context, location, scope, node);
    node->foreignName   = NULL;
    node->intrinsicName = StringCreateCopy(context->tempAllocator, intrinsicName);
    return node;
}

ASTGenericFunctionDeclarationRef ASTContextCreateGenericFunctionDeclaration(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                            ASTFixity fixity, StringRef name, ArrayRef genericTypes,
                                                                            ArrayRef parameters, ASTTypeRef returnType, ASTBlockRef body) {
    assert(name && returnType && body);

    ASTGenericFunctionDeclarationRef node = (ASTGenericFunctionDeclarationRef)_ASTContextCreateNode(
        context, ASTTagGenericFunctionDeclaration, location, scope);
    node->base.name        = StringCreateCopy(context->tempAllocator, name);
    node->base.mangledName = NULL;
    node->base.type        = NULL;
    node->fixity           = ASTFixityNone;
    node->genericTypes     = ASTContextCreateArray(context, location, scope);
    if (genericTypes) {
        ASTArrayAppendArray(node->genericTypes, genericTypes);
    }
    node->parameters = ASTContextCreateArray(context, location, scope);
    if (parameters) {
        ASTArrayAppendArray(node->parameters, parameters);
    }
    node->returnType = returnType;
    node->body       = body;
    node->innerScope = kScopeNull;
    return node;
}

ASTStructureDeclarationRef ASTContextCreateStructureDeclaration(ASTContextRef context, SourceRange location, ScopeID scope, StringRef name,
                                                                ArrayRef values, ArrayRef initializers) {
    assert(name);

    ASTStructureDeclarationRef node = (ASTStructureDeclarationRef)_ASTContextCreateNode(context, ASTTagStructureDeclaration, location,
                                                                                        scope);
    node->base.name                 = StringCreateCopy(context->tempAllocator, name);
    node->base.mangledName          = NULL;
    node->values                    = ASTContextCreateArray(context, location, scope);
    node->initializers              = ASTContextCreateArray(context, location, scope);
    node->innerScope                = kScopeNull;
    if (values) {
        ASTArrayAppendArray(node->values, values);
    }

    if (initializers) {
        ASTArrayAppendArray(node->initializers, initializers);
    }

    node->base.type = (ASTTypeRef)ASTContextCreateStructureType(context, location, scope, node);
    return node;
}

ASTGenericStructureDeclarationRef ASTContextCreateGenericStructureDeclaration(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                              StringRef name, ArrayRef genericParameters, ArrayRef values,
                                                                              ArrayRef initializers) {
    assert(name);

    ASTGenericStructureDeclarationRef node = (ASTGenericStructureDeclarationRef)_ASTContextCreateNode(
        context, ASTTagGenericStructureDeclaration, location, scope);
    node->base.name         = StringCreateCopy(context->tempAllocator, name);
    node->base.mangledName  = NULL;
    node->base.type         = NULL;
    node->genericParameters = ASTContextCreateArray(context, location, scope);
    node->values            = ASTContextCreateArray(context, location, scope);
    node->initializers      = ASTContextCreateArray(context, location, scope);
    node->innerScope        = kScopeNull;
    if (genericParameters) {
        ASTArrayAppendArray(node->genericParameters, genericParameters);
    }

    if (values) {
        ASTArrayAppendArray(node->values, values);
    }

    if (initializers) {
        ASTArrayAppendArray(node->initializers, initializers);
    }

    return node;
}

ASTInitializerDeclarationRef ASTContextCreateInitializerDeclaration(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                    ArrayRef parameters, ASTBlockRef body) {
    ASTInitializerDeclarationRef node = (ASTInitializerDeclarationRef)_ASTContextCreateNode(context, ASTTagInitializerDeclaration, location,
                                                                                            scope);
    node->base.name                   = StringCreate(context->tempAllocator, "init");
    node->base.mangledName            = NULL;
    node->parameters                  = ASTContextCreateArray(context, location, scope);
    node->body                        = body;
    node->innerScope                  = kScopeNull;
    node->structure                   = NULL;
    node->implicitSelf                = NULL;
    if (parameters) {
        ASTArrayAppendArray(node->parameters, parameters);
    }
    node->base.type = NULL;
    return node;
}

ASTValueDeclarationRef ASTContextCreateValueDeclaration(ASTContextRef context, SourceRange location, ScopeID scope, ASTValueKind kind,
                                                        StringRef name, ASTTypeRef type, ASTExpressionRef initializer) {
    assert(name && type);
    assert((kind == ASTValueKindParameter && !initializer) || (kind == ASTValueKindVariable || kind == ASTValueKindEnumerationElement));

    ASTValueDeclarationRef node = (ASTValueDeclarationRef)_ASTContextCreateNode(context, ASTTagValueDeclaration, location, scope);
    node->base.name             = StringCreateCopy(context->tempAllocator, name);
    node->base.mangledName      = NULL;
    node->kind                  = kind;
    node->base.type             = type;
    node->initializer           = initializer;
    return node;
}

ASTTypeAliasDeclarationRef ASTContextCreateTypeAliasDeclaration(ASTContextRef context, SourceRange location, ScopeID scope, StringRef name,
                                                                ASTTypeRef type) {
    ASTTypeAliasDeclarationRef node = (ASTTypeAliasDeclarationRef)_ASTContextCreateNode(context, ASTTagTypeAliasDeclaration, location,
                                                                                        scope);
    node->base.name                 = StringCreateCopy(context->tempAllocator, name);
    node->base.mangledName          = NULL;
    node->base.type                 = type;
    return node;
}

ASTOpaqueTypeRef ASTContextCreateOpaqueType(ASTContextRef context, SourceRange location, ScopeID scope, StringRef name) {
    assert(name);

    ASTOpaqueTypeRef node = (ASTOpaqueTypeRef)_ASTContextCreateNode(context, ASTTagOpaqueType, location, scope);
    node->name            = StringCreateCopy(context->tempAllocator, name);
    node->declaration     = NULL;
    return node;
}

ASTPointerTypeRef ASTContextCreatePointerType(ASTContextRef context, SourceRange location, ScopeID scope, ASTTypeRef pointeeType) {
    assert(pointeeType);

    ASTPointerTypeRef node = (ASTPointerTypeRef)_ASTContextCreateNode(context, ASTTagPointerType, location, scope);
    node->pointeeType      = pointeeType;
    return node;
}

ASTArrayTypeRef ASTContextCreateArrayType(ASTContextRef context, SourceRange location, ScopeID scope, ASTTypeRef elementType,
                                          ASTExpressionRef size) {
    assert(elementType);

    ASTArrayTypeRef node = (ASTArrayTypeRef)_ASTContextCreateNode(context, ASTTagArrayType, location, scope);
    node->elementType    = elementType;
    node->size           = size;
    node->sizeValue      = 0;
    return node;
}

ASTEnumerationTypeRef ASTContextCreateEnumerationType(ASTContextRef context, SourceRange location, ScopeID scope,
                                                      ASTEnumerationDeclarationRef declaration) {
    ASTEnumerationTypeRef node = (ASTEnumerationTypeRef)_ASTContextCreateNode(context, ASTTagEnumerationType, location, scope);
    node->declaration          = declaration;
    return node;
}

ASTFunctionTypeRef ASTContextCreateFunctionTypeForDeclaration(ASTContextRef context, SourceRange location, ScopeID scope,
                                                              ASTFunctionDeclarationRef declaration) {
    ASTFunctionTypeRef node = (ASTFunctionTypeRef)_ASTContextCreateNode(context, ASTTagFunctionType, location, scope);
    node->declaration       = declaration;
    node->parameterTypes    = ASTContextCreateArray(context, location, scope);
    node->resultType        = declaration->returnType;

    ASTArrayIteratorRef iterator = ASTArrayGetIterator(declaration->parameters);
    while (iterator) {
        ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayIteratorGetElement(iterator);
        ASTArrayAppendElement(node->parameterTypes, parameter->base.type);

        iterator = ASTArrayIteratorNext(iterator);
    }

    return node;
}

ASTFunctionTypeRef ASTContextCreateFunctionType(ASTContextRef context, SourceRange location, ScopeID scope, ArrayRef parameterTypes,
                                                ASTTypeRef resultType) {
    assert(resultType);

    ASTFunctionTypeRef node = (ASTFunctionTypeRef)_ASTContextCreateNode(context, ASTTagFunctionType, location, scope);
    node->declaration       = NULL;
    node->parameterTypes    = ASTContextCreateArray(context, location, scope);
    node->resultType        = resultType;
    if (parameterTypes) {
        ASTArrayAppendArray(node->parameterTypes, parameterTypes);
    }
    return node;
}

ASTStructureTypeRef ASTContextCreateStructureType(ASTContextRef context, SourceRange location, ScopeID scope,
                                                  ASTStructureDeclarationRef declaration) {
    ASTStructureTypeRef node = (ASTStructureTypeRef)_ASTContextCreateNode(context, ASTTagStructureType, location, scope);
    node->declaration        = declaration;
    return node;
}

ASTGenericTypeRef ASTContextCreateGenericType(ASTContextRef context, SourceRange location, ScopeID scope, ASTTypeRef baseType,
                                              ArrayRef arguments) {
    ASTGenericTypeRef node = (ASTGenericTypeRef)_ASTContextCreateNode(context, ASTTagGenericType, location, scope);
    node->baseType         = baseType;
    node->arguments        = ASTContextCreateArray(context, location, scope);
    node->argumentScope    = kScopeNull;
    if (arguments) {
        ASTArrayAppendArray(node->arguments, arguments);
    }
    return node;
}

ASTBuiltinTypeRef ASTContextGetBuiltinType(ASTContextRef context, ASTBuiltinTypeKind kind) {
    return context->builtinTypes[kind];
}

ASTStructureTypeRef ASTContextGetStringType(ASTContextRef context) {
    return context->stringType;
}

ASTNodeRef _ASTContextCreateNode(ASTContextRef context, ASTTag tag, SourceRange location, ScopeID scope) {
    ASTNodeRef node  = BucketArrayAppendUninitializedElement(context->nodes[tag]);
    node->tag        = tag;
    node->flags      = ASTFlagsNone;
    node->location   = location;
    node->scope      = scope;
    node->substitute = NULL;
    node->primary    = NULL;
    node->irValue    = NULL;
    node->irType     = NULL;
    return node;
}

ASTBuiltinTypeRef _ASTContextCreateBuiltinType(ASTContextRef context, SourceRange location, ScopeID scope, ASTBuiltinTypeKind kind) {
    ASTBuiltinTypeRef node = (ASTBuiltinTypeRef)_ASTContextCreateNode(context, ASTTagBuiltinType, location, scope);
    node->kind             = kind;
    return node;
}

// TODO: Move builtin types to a builtin module and implicitly import the module to the main module
void _ASTContextInitBuiltinTypes(ASTContextRef context) {
    const Char *builtinTypeNames[AST_BUILTIN_TYPE_KIND_COUNT] = {
        "<error>", "Void",   "Bool",   "Int8",   "Int16", "Int32",   "Int64",   "Int",
        "UInt8",   "UInt16", "UInt32", "UInt64", "UInt",  "Float32", "Float64", "Float",
    };

    StringRef name                                 = StringCreate(context->allocator, builtinTypeNames[ASTBuiltinTypeKindError]);
    context->builtinTypes[ASTBuiltinTypeKindError] = _ASTContextCreateBuiltinType(context, SourceRangeNull(), kScopeGlobal,
                                                                                  ASTBuiltinTypeKindError);
    StringDestroy(name);

    // NOTE: Iteration begins after ASTBuiltinTypeKindError which is 0 to skip addition of <error> type to the scope.
    for (Index index = ASTBuiltinTypeKindError + 1; index < AST_BUILTIN_TYPE_KIND_COUNT; index++) {
        name                         = StringCreate(context->allocator, builtinTypeNames[index]);
        context->builtinTypes[index] = _ASTContextCreateBuiltinType(context, SourceRangeNull(), kScopeGlobal, (ASTBuiltinTypeKind)index);
        // TODO: May replace structure declaration with some builtin declaration?
        ASTStructureDeclarationRef structure = ASTContextCreateStructureDeclaration(context, SourceRangeNull(), kScopeGlobal, name, NULL,
                                                                                    NULL);
        structure->innerScope                = SymbolTableInsertScope(context->symbolTable, ScopeKindStructure, kScopeGlobal, NULL);
        structure->base.type                 = (ASTTypeRef)context->builtinTypes[index];
        SymbolID symbol                      = SymbolTableInsertSymbol(context->symbolTable, kScopeGlobal, structure->base.name);
        SymbolTableSetSymbolDefinition(context->symbolTable, symbol, structure);
        StringDestroy(name);
    }

    StringRef stringName        = StringCreate(context->allocator, "String");
    ScopeID stringScope         = SymbolTableInsertScope(context->symbolTable, ScopeKindStructure, kScopeGlobal, NULL);
    ASTTypeRef stringBufferType = (ASTTypeRef)ASTContextCreatePointerType(
        context, SourceRangeNull(), stringScope, (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindUInt8));
    StringRef stringBufferName          = StringCreate(context->allocator, "buffer");
    ASTValueDeclarationRef stringBuffer = ASTContextCreateValueDeclaration(context, SourceRangeNull(), stringScope, ASTValueKindVariable,
                                                                           stringBufferName, stringBufferType, NULL);
    ASTTypeRef stringCountType          = (ASTTypeRef)ASTContextGetBuiltinType(context, ASTBuiltinTypeKindInt);
    StringRef stringCountName           = StringCreate(context->allocator, "count");
    ASTValueDeclarationRef stringCount  = ASTContextCreateValueDeclaration(context, SourceRangeNull(), stringScope, ASTValueKindVariable,
                                                                          stringCountName, stringCountType, NULL);
    ArrayRef stringValues               = ArrayCreateEmpty(context->allocator, sizeof(ASTValueDeclarationRef), 2);
    ArrayAppendElement(stringValues, &stringBuffer);
    ArrayAppendElement(stringValues, &stringCount);
    ASTStructureDeclarationRef stringDeclaration = ASTContextCreateStructureDeclaration(context, SourceRangeNull(), kScopeGlobal,
                                                                                        stringName, stringValues, NULL);
    PerformNameManglingForDeclaration(context, (ASTDeclarationRef)stringDeclaration);
    context->stringType = ASTContextCreateStructureType(context, SourceRangeNull(), kScopeGlobal, stringDeclaration);
    StringDestroy(stringCountName);
    StringDestroy(stringBufferName);
    StringDestroy(stringName);
    ArrayDestroy(stringValues);
}

void _ASTContextInitBuiltinFunctions(ASTContextRef context){
// TODO: All builtin definitions should pass the name resolution phase!
//       Currently we assume that there will be no name collisions here, which is a possible source for bugs and errors...
#define UNARY_OPERATOR(SYMBOL, ARGUMENT_TYPE, RESULT_TYPE, INTRINSIC)                                                                      \
    {                                                                                                                                      \
        ArrayRef parameters              = ArrayCreateEmpty(context->allocator, sizeof(ASTValueDeclarationRef), 1);                        \
        ScopeID scope                    = SymbolTableInsertScope(context->symbolTable, ScopeKindFunction, kScopeGlobal, NULL);            \
        StringRef parameterName          = StringCreate(context->allocator, "value");                                                      \
        ASTTypeRef parameterType         = (ASTTypeRef)_ASTContextGetBuiltinTypeByName(context, ARGUMENT_TYPE);                            \
        ASTValueDeclarationRef parameter = ASTContextCreateValueDeclaration(context, SourceRangeNull(), scope, ASTValueKindParameter,      \
                                                                            parameterName, parameterType, NULL);                           \
        ArrayAppendElement(parameters, &parameter);                                                                                        \
        ASTTypeRef resultType                 = (ASTTypeRef)_ASTContextGetBuiltinTypeByName(context, RESULT_TYPE);                         \
        StringRef name                        = StringCreate(context->allocator, SYMBOL);                                                  \
        StringRef intrinsic                   = StringCreate(context->allocator, INTRINSIC);                                               \
        ASTFunctionDeclarationRef declaration = ASTContextCreateIntrinsicFunctionDeclaration(                                              \
            context, SourceRangeNull(), kScopeGlobal, ASTFixityPrefix, name, parameters, resultType, intrinsic);                           \
        SymbolTableSetScopeUserdata(context->symbolTable, scope, declaration);                                                             \
        assert(declaration->base.name);                                                                                                    \
        SymbolID symbol  = SymbolTableInsertOrGetSymbolGroup(context->symbolTable, kScopeGlobal, declaration->base.name);                  \
        Index entryIndex = SymbolTableInsertSymbolGroupEntry(context->symbolTable, symbol);                                                \
        SymbolTableSetSymbolGroupDefinition(context->symbolTable, symbol, entryIndex, declaration);                                        \
        StringDestroy(intrinsic);                                                                                                          \
        StringDestroy(name);                                                                                                               \
        StringDestroy(parameterName);                                                                                                      \
        ArrayDestroy(parameters);                                                                                                          \
    }

#define BINARY_OPERATOR(SYMBOL, ARGUMENT_TYPE1, ARGUMENT_TYPE2, RESULT_TYPE, INTRINSIC)                                                    \
    {                                                                                                                                      \
        ArrayRef parameters                 = ArrayCreateEmpty(context->allocator, sizeof(ASTValueDeclarationRef), 2);                     \
        ScopeID scope                       = SymbolTableInsertScope(context->symbolTable, ScopeKindFunction, kScopeGlobal, NULL);         \
        StringRef lhsParameterName          = StringCreate(context->allocator, "lhs");                                                     \
        ASTTypeRef lhsParameterType         = (ASTTypeRef)_ASTContextGetBuiltinTypeByName(context, ARGUMENT_TYPE1);                        \
        ASTValueDeclarationRef lhsParameter = ASTContextCreateValueDeclaration(context, SourceRangeNull(), scope, ASTValueKindParameter,   \
                                                                               lhsParameterName, lhsParameterType, NULL);                  \
        StringRef rhsParameterName          = StringCreate(context->allocator, "rhs");                                                     \
        ASTTypeRef rhsParameterType         = (ASTTypeRef)_ASTContextGetBuiltinTypeByName(context, ARGUMENT_TYPE2);                        \
        ASTValueDeclarationRef rhsParameter = ASTContextCreateValueDeclaration(context, SourceRangeNull(), scope, ASTValueKindParameter,   \
                                                                               rhsParameterName, rhsParameterType, NULL);                  \
        ArrayAppendElement(parameters, &lhsParameter);                                                                                     \
        ArrayAppendElement(parameters, &rhsParameter);                                                                                     \
        ASTTypeRef resultType                 = (ASTTypeRef)_ASTContextGetBuiltinTypeByName(context, RESULT_TYPE);                         \
        StringRef intrinsic                   = StringCreate(context->allocator, INTRINSIC);                                               \
        StringRef name                        = StringCreate(context->allocator, SYMBOL);                                                  \
        ASTFunctionDeclarationRef declaration = ASTContextCreateIntrinsicFunctionDeclaration(                                              \
            context, SourceRangeNull(), kScopeGlobal, ASTFixityInfix, name, parameters, resultType, intrinsic);                            \
        SymbolTableSetScopeUserdata(context->symbolTable, scope, declaration);                                                             \
        assert(declaration->base.name);                                                                                                    \
        SymbolID symbol  = SymbolTableInsertOrGetSymbolGroup(context->symbolTable, kScopeGlobal, declaration->base.name);                  \
        Index entryIndex = SymbolTableInsertSymbolGroupEntry(context->symbolTable, symbol);                                                \
        SymbolTableSetSymbolGroupDefinition(context->symbolTable, symbol, entryIndex, declaration);                                        \
        StringDestroy(name);                                                                                                               \
        StringDestroy(intrinsic);                                                                                                          \
        StringDestroy(rhsParameterName);                                                                                                   \
        StringDestroy(lhsParameterName);                                                                                                   \
        ArrayDestroy(parameters);                                                                                                          \
    }

#include "JellyCore/RuntimeSupportDefinitions.h"
}

ASTBuiltinTypeRef _ASTContextGetBuiltinTypeByName(ASTContextRef context, const Char *name) {
    const Char *builtinTypeNames[AST_BUILTIN_TYPE_KIND_COUNT - 1] = {
        "Void",   "Bool",   "Int8",   "Int16", "Int32",   "Int64",   "Int",   "UInt8",
        "UInt16", "UInt32", "UInt64", "UInt",  "Float32", "Float64", "Float",
    };

    for (Index index = 0; index < AST_BUILTIN_TYPE_KIND_COUNT - 1; index++) {
        if (strcmp(builtinTypeNames[index], name) == 0) {
            return ASTContextGetBuiltinType(context, (ASTBuiltinTypeKind)(index + 1));
        }
    }

    return NULL;
}
