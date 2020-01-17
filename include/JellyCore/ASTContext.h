#ifndef __JELLY_ASTCONTEXT__
#define __JELLY_ASTCONTEXT__

#include <JellyCore/ASTNodes.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>
#include <JellyCore/BucketArray.h>

JELLY_EXTERN_C_BEGIN

typedef struct _ASTContext *ASTContextRef;

ASTContextRef ASTContextCreate(AllocatorRef allocator, StringRef moduleName);

void ASTContextDestroy(ASTContextRef context);

AllocatorRef ASTContextGetTempAllocator(ASTContextRef context);

SymbolTableRef ASTContextGetSymbolTable(ASTContextRef context);

ASTModuleDeclarationRef ASTContextGetModule(ASTContextRef context);

BucketArrayRef ASTContextGetAllNodes(ASTContextRef context, ASTTag tag);

void ASTModuleAddSourceUnit(ASTContextRef context, ASTModuleDeclarationRef module, ASTSourceUnitRef sourceUnit);

ASTSourceUnitRef ASTContextCreateSourceUnit(ASTContextRef context, SourceRange location, ScopeID scope, StringRef filePath,
                                            ArrayRef declarations);

ASTLinkedListRef ASTContextCreateLinkedList(ASTContextRef context, SourceRange location, ScopeID scope);

ASTArrayRef ASTContextCreateArray(ASTContextRef context, SourceRange location, ScopeID scope);

ASTLoadDirectiveRef ASTContextCreateLoadDirective(ASTContextRef context, SourceRange location, ScopeID scope,
                                                  ASTConstantExpressionRef filePath);

ASTLinkDirectiveRef ASTContextCreateLinkDirective(ASTContextRef context, SourceRange location, ScopeID scope, Bool isFramework,
                                                  StringRef library);

ASTImportDirectiveRef ASTContextCreateImportDirective(ASTContextRef context, SourceRange location, ScopeID scope, StringRef modulePath);

ASTIncludeDirectiveRef ASTContextCreateIncludeDirective(ASTContextRef context, SourceRange location, ScopeID scope, StringRef headerPath);

ASTBlockRef ASTContextCreateBlock(ASTContextRef context, SourceRange location, ScopeID scope, ArrayRef statements);

ASTIfStatementRef ASTContextCreateIfStatement(ASTContextRef context, SourceRange location, ScopeID scope, ASTExpressionRef condition,
                                              ASTBlockRef thenBlock, ASTBlockRef elseBlock);
ASTLoopStatementRef ASTContextCreateLoopStatement(ASTContextRef context, SourceRange location, ScopeID scope, ASTLoopKind kind,
                                                  ASTExpressionRef condition, ASTBlockRef loopBlock);
ASTCaseStatementRef ASTContextCreateCaseStatement(ASTContextRef context, SourceRange location, ScopeID scope, ASTCaseKind kind,
                                                  ASTExpressionRef condition, ASTBlockRef body);
ASTSwitchStatementRef ASTContextCreateSwitchStatement(ASTContextRef context, SourceRange location, ScopeID scope, ASTExpressionRef argument,
                                                      ArrayRef cases);
ASTControlStatementRef ASTContextCreateControlStatement(ASTContextRef context, SourceRange location, ScopeID scope, ASTControlKind kind,
                                                        ASTExpressionRef result);
ASTReferenceExpressionRef ASTContextCreateReferenceExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                              ASTExpressionRef argument);
ASTDereferenceExpressionRef ASTContextCreateDereferenceExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                  ASTExpressionRef argument);
ASTUnaryExpressionRef ASTContextCreateUnaryExpression(ASTContextRef context, SourceRange location, ScopeID scope, ASTUnaryOperator op,
                                                      ASTExpressionRef arguments[1]);
ASTBinaryExpressionRef ASTContextCreateBinaryExpression(ASTContextRef context, SourceRange location, ScopeID scope, ASTBinaryOperator op,
                                                        ASTExpressionRef arguments[2]);
ASTIdentifierExpressionRef ASTContextCreateIdentifierExpression(ASTContextRef context, SourceRange location, ScopeID scope, StringRef name);
ASTMemberAccessExpressionRef ASTContextCreateMemberAccessExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                    ASTExpressionRef argument, StringRef memberName);
ASTAssignmentExpressionRef ASTContextCreateAssignmentExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                ASTBinaryOperator op, ASTExpressionRef variable,
                                                                ASTExpressionRef expression);
ASTCallExpressionRef ASTContextCreateCallExpression(ASTContextRef context, SourceRange location, ScopeID scope, ASTExpressionRef callee,
                                                    ArrayRef arguments);
ASTCallExpressionRef ASTContextCreateUnaryCallExpression(ASTContextRef context, SourceRange location, ScopeID scope, ASTUnaryOperator op,
                                                         ASTExpressionRef arguments[1]);
ASTCallExpressionRef ASTContextCreateBinaryCallExpression(ASTContextRef context, SourceRange location, ScopeID scope, ASTBinaryOperator op,
                                                          ASTExpressionRef arguments[2]);
ASTConstantExpressionRef ASTContextCreateConstantNilExpression(ASTContextRef context, SourceRange location, ScopeID scope);
ASTConstantExpressionRef ASTContextCreateConstantBoolExpression(ASTContextRef context, SourceRange location, ScopeID scope, Bool value);
ASTConstantExpressionRef ASTContextCreateConstantIntExpression(ASTContextRef context, SourceRange location, ScopeID scope, UInt64 value);
ASTConstantExpressionRef ASTContextCreateConstantFloatExpression(ASTContextRef context, SourceRange location, ScopeID scope, Float64 value);
ASTConstantExpressionRef ASTContextCreateConstantStringExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                  StringRef value);

ASTSizeOfExpressionRef ASTContextCreateSizeOfExpression(ASTContextRef context, SourceRange location, ScopeID scope, ASTTypeRef sizeType);

ASTSubscriptExpressionRef ASTContextCreateSubscriptExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                              ASTExpressionRef expression, ArrayRef arguments);

ASTTypeOperationExpressionRef ASTContextCreateTypeOperationExpression(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                      ASTTypeOperation op, ASTExpressionRef expression,
                                                                      ASTTypeRef expressionType);

ASTTypeExpressionRef ASTContextCreateTypeExpression(ASTContextRef context, SourceRange location, ScopeID scope, ASTTypeRef referencedType);

ASTModuleDeclarationRef ASTContextCreateModuleDeclaration(ASTContextRef context, SourceRange location, ScopeID scope, ASTModuleKind kind,
                                                          StringRef moduleName, ArrayRef sourceUnits, ArrayRef importedModules);

ASTEnumerationDeclarationRef ASTContextCreateEnumerationDeclaration(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                    StringRef name, ArrayRef elements);

ASTFunctionDeclarationRef ASTContextCreateFunctionDeclaration(ASTContextRef context, SourceRange location, ScopeID scope, ASTFixity fixity,
                                                              StringRef name, ArrayRef parameters, ASTTypeRef returnType, ASTBlockRef body);

ASTFunctionDeclarationRef ASTContextCreateForeignFunctionDeclaration(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                     ASTFixity fixity, StringRef name, ArrayRef parameters,
                                                                     ASTTypeRef returnType, StringRef foreignName);

ASTFunctionDeclarationRef ASTContextCreateIntrinsicFunctionDeclaration(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                       ASTFixity fixity, StringRef name, ArrayRef parameters,
                                                                       ASTTypeRef returnType, StringRef intrinsicName);

ASTStructureDeclarationRef ASTContextCreateStructureDeclaration(ASTContextRef context, SourceRange location, ScopeID scope, StringRef name,
                                                                ArrayRef values, ArrayRef initializers);

ASTInitializerDeclarationRef ASTContextCreateInitializerDeclaration(ASTContextRef context, SourceRange location, ScopeID scope,
                                                                    ArrayRef parameters, ASTBlockRef body);

ASTValueDeclarationRef ASTContextCreateValueDeclaration(ASTContextRef context, SourceRange location, ScopeID scope, ASTValueKind kind,
                                                        StringRef name, ASTTypeRef type, ASTExpressionRef initializer);

ASTTypeAliasDeclarationRef ASTContextCreateTypeAliasDeclaration(ASTContextRef context, SourceRange location, ScopeID scope, StringRef name,
                                                                ASTTypeRef type);

ASTOpaqueTypeRef ASTContextCreateOpaqueType(ASTContextRef context, SourceRange location, ScopeID scope, StringRef name);

ASTPointerTypeRef ASTContextCreatePointerType(ASTContextRef context, SourceRange location, ScopeID scope, ASTTypeRef pointeeType);

ASTArrayTypeRef ASTContextCreateArrayType(ASTContextRef context, SourceRange location, ScopeID scope, ASTTypeRef elementType,
                                          ASTExpressionRef size);

ASTEnumerationTypeRef ASTContextCreateEnumerationType(ASTContextRef context, SourceRange location, ScopeID scope,
                                                      ASTEnumerationDeclarationRef declaration);

ASTFunctionTypeRef ASTContextCreateFunctionTypeForDeclaration(ASTContextRef context, SourceRange location, ScopeID scope,
                                                              ASTFunctionDeclarationRef declaration);

ASTFunctionTypeRef ASTContextCreateFunctionType(ASTContextRef context, SourceRange location, ScopeID scope, ArrayRef parameterTypes,
                                                ASTTypeRef resultType);

ASTStructureTypeRef ASTContextCreateStructureType(ASTContextRef context, SourceRange location, ScopeID scope,
                                                  ASTStructureDeclarationRef declaration);

ASTGenericTypeRef ASTContextCreateGenericType(ASTContextRef context, SourceRange location, ScopeID scope, ASTTypeRef baseType,
                                              ArrayRef arguments);

ASTBuiltinTypeRef ASTContextGetBuiltinType(ASTContextRef context, ASTBuiltinTypeKind kind);

ASTStructureTypeRef ASTContextGetStringType(ASTContextRef context);

JELLY_EXTERN_C_END

#endif
