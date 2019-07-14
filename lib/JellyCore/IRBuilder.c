#include "JellyCore/Allocator.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/IRBuilder.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Support.h>

struct _IRBuilder {
    AllocatorRef allocator;
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
};

static inline void _IRBuilderBuildTypes(IRBuilderRef builder, ASTModuleDeclarationRef module);
static inline void _IRBuilderBuildGlobalVariables(IRBuilderRef builder, ASTModuleDeclarationRef module);
static inline void _IRBuilderBuildGlobalVariable(IRBuilderRef builder, ASTValueDeclarationRef declaration);
static inline void _IRBuilderBuildFunctionDeclaration(IRBuilderRef builder, ASTFunctionDeclarationRef declaration);
static inline void _IRBuilderBuildLocalVariable(IRBuilderRef builder, LLVMValueRef function, ASTValueDeclarationRef declaration);
static inline void _IRBuilderBuildBlock(IRBuilderRef builder, LLVMValueRef function, ASTBlockRef block);
static inline void _IRBuilderBuildStatement(IRBuilderRef builder, LLVMValueRef function, ASTNodeRef node);
static inline void _IRBuilderBuildIfStatement(IRBuilderRef builder, LLVMValueRef function, ASTIfStatementRef statement);
static inline void _IRBuilderBuildLoopStatement(IRBuilderRef builder, LLVMValueRef function, ASTLoopStatementRef statement);
static inline void _IRBuilderBuildSwitchStatement(IRBuilderRef builder, LLVMValueRef function, ASTSwitchStatementRef statement);
static inline void _IRBuilderBuildControlStatement(IRBuilderRef builder, LLVMValueRef function, ASTControlStatementRef statement);
static inline void _IRBuilderBuildExpression(IRBuilderRef builder, LLVMValueRef function, ASTExpressionRef expression);
static inline LLVMValueRef _IRBuilderLoadExpression(IRBuilderRef builder, LLVMValueRef function, ASTExpressionRef expression);

static inline LLVMTypeRef _IRBuilderGetIRType(IRBuilderRef builder, ASTTypeRef type);

IRBuilderRef IRBuilderCreate(AllocatorRef allocator) {
    IRBuilderRef builder = (IRBuilderRef)AllocatorAllocate(allocator, sizeof(struct _IRBuilder));
    builder->allocator   = allocator;
    return builder;
}

void IRBuilderDestroy(IRBuilderRef builder) {
    AllocatorDeallocate(builder->allocator, builder);
}

void IRBuilderBuild(IRBuilderRef builder, ASTModuleDeclarationRef module) {
    assert(module->base.name);

    builder->context = LLVMGetGlobalContext();
    builder->module  = LLVMModuleCreateWithNameInContext(StringGetCharacters(module->base.name), builder->context);
    builder->builder = LLVMCreateBuilderInContext(builder->context);

    // It would at least be better to pre create all known types and only use the prebuild types, same applies for literal values
    if (ASTArrayGetElementCount(module->sourceUnits) > 0) {
        ASTSourceUnitRef initialSourceUnit = ASTArrayGetElementAtIndex(module->sourceUnits, 0);
        LLVMSetSourceFileName(builder->module, StringGetCharacters(initialSourceUnit->filePath),
                              StringGetLength(initialSourceUnit->filePath));
    }

    _IRBuilderBuildTypes(builder, module);
    _IRBuilderBuildGlobalVariables(builder, module);

    for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(module->sourceUnits); sourceUnitIndex++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, sourceUnitIndex);
        for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
            if (child->tag == ASTTagLoadDirective) {
                continue;
            }

            switch (child->tag) {
            case ASTTagLoadDirective:
                continue;

            case ASTTagEnumerationDeclaration:
                continue;

            case ASTTagFunctionDeclaration:
                _IRBuilderBuildFunctionDeclaration(builder, (ASTFunctionDeclarationRef)child);
                break;

            case ASTTagStructureDeclaration:
                continue;

            case ASTTagValueDeclaration:
                continue;

            default:
                JELLY_UNREACHABLE("Invalid tag given for top level node!");
                break;
            }
        }
    }

    Char *message;
    LLVMBool error = LLVMVerifyModule(builder->module, LLVMReturnStatusAction, &message);
    if (error && message) {
        ReportCriticalFormat("LLVM Error:\n%s\n", message);
        LLVMDisposeMessage(message);
    }

    // TODO: Add configuration option to IRBuilder to disable dumping and also allow dumping to a FILE instead of stdout
    LLVMDumpModule(builder->module);

    LLVMDisposeBuilder(builder->builder);
    LLVMDisposeModule(builder->module);
}

static inline void _IRBuilderBuildTypes(IRBuilderRef builder, ASTModuleDeclarationRef module) {
    // Build structure signatures
    for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(module->sourceUnits); sourceUnitIndex++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, sourceUnitIndex);
        for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
            if (child->tag == ASTTagStructureDeclaration) {
                ASTDeclarationRef declaration = (ASTDeclarationRef)child;
                declaration->base.irType      = LLVMStructCreateNamed(builder->context, StringGetCharacters(declaration->mangledName));
            }
        }
    }

    // Build function signatures, structure bodies, global variable types
    ArrayRef temporaryTypes = ArrayCreateEmpty(builder->allocator, sizeof(LLVMTypeRef), 8);
    for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(module->sourceUnits); sourceUnitIndex++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, sourceUnitIndex);
        for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
            ArrayRemoveAllElements(temporaryTypes, true);

            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
            if (child->tag == ASTTagFunctionDeclaration) {
                ASTFunctionDeclarationRef declaration = (ASTFunctionDeclarationRef)child;
                for (Index index = 0; index < ASTArrayGetElementCount(declaration->parameters); index++) {
                    ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->parameters, index);
                    LLVMTypeRef parameterType        = _IRBuilderGetIRType(builder, parameter->base.type);
                    assert(parameterType);
                    parameter->base.base.irType = parameterType;
                    ArrayAppendElement(temporaryTypes, &parameterType);
                }

                declaration->base.base.irType = LLVMFunctionType(_IRBuilderGetIRType(builder, declaration->returnType),
                                                                 (LLVMTypeRef *)ArrayGetMemoryPointer(temporaryTypes),
                                                                 ArrayGetElementCount(temporaryTypes), false);
            }

            if (child->tag == ASTTagStructureDeclaration) {
                ASTStructureDeclarationRef declaration = (ASTStructureDeclarationRef)child;
                LLVMTypeRef type                       = (LLVMTypeRef)declaration->base.base.irType;
                assert(type);

                for (Index index = 0; index < ASTArrayGetElementCount(declaration->values); index++) {
                    ASTValueDeclarationRef value = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->values, index);
                    LLVMTypeRef valueType        = _IRBuilderGetIRType(builder, value->base.type);
                    assert(valueType);
                    value->base.base.irType = valueType;
                    ArrayAppendElement(temporaryTypes, &valueType);
                }
                LLVMStructSetBody(type, (LLVMTypeRef *)ArrayGetMemoryPointer(temporaryTypes), ArrayGetElementCount(temporaryTypes), false);
            }

            if (child->tag == ASTTagValueDeclaration) {
                ASTValueDeclarationRef declaration = (ASTValueDeclarationRef)child;
                declaration->base.base.irType      = _IRBuilderGetIRType(builder, declaration->base.type);
            }
        }
    }
    ArrayDestroy(temporaryTypes);
}

static inline void _IRBuilderBuildGlobalVariables(IRBuilderRef builder, ASTModuleDeclarationRef module) {
    for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(module->sourceUnits); sourceUnitIndex++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, sourceUnitIndex);
        for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
            if (child->tag == ASTTagValueDeclaration) {
                _IRBuilderBuildGlobalVariable(builder, (ASTValueDeclarationRef)child);
            }
        }
    }
}

static inline void _IRBuilderBuildGlobalVariable(IRBuilderRef builder, ASTValueDeclarationRef declaration) {
    assert(!declaration->base.base.irValue);
    assert(declaration->base.base.irType);

    LLVMTypeRef type   = (LLVMTypeRef)declaration->base.base.irType;
    LLVMValueRef value = LLVMAddGlobal(builder->module, type, StringGetCharacters(declaration->base.mangledName));
    if (declaration->initializer) {
        // TODO: Emit initialization of value into program entry point, this will also require the creation of a global value initialization
        // dependency graphs which would track cyclic initializations in global scope and also be helpful for topological sorting of
        // initialization instructions
    }

    declaration->base.base.irValue = value;
    declaration->base.base.flags |= ASTFlagsIsValuePointer;
}

static inline void _IRBuilderBuildFunctionDeclaration(IRBuilderRef builder, ASTFunctionDeclarationRef declaration) {
    assert(declaration->base.base.irType);

    LLVMValueRef function = NULL;
    if (declaration->foreign && declaration->foreignName) {
        // TODO: Check if the foreignName has to be mangled according to the used calling convention...
        function = LLVMAddFunction(builder->module, StringGetCharacters(declaration->foreignName),
                                   (LLVMTypeRef)declaration->base.base.irType);
    } else {
        function = LLVMAddFunction(builder->module, StringGetCharacters(declaration->base.mangledName),
                                   (LLVMTypeRef)declaration->base.base.irType);
    }
    declaration->base.base.irValue = function;

    for (Index index = 0; index < ASTArrayGetElementCount(declaration->parameters); index++) {
        ASTValueDeclarationRef parameter = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->parameters, index);
        parameter->base.base.irValue     = LLVMGetParam(function, index);
    }

    if (!declaration->foreign) {
        LLVMBasicBlockRef entry = LLVMAppendBasicBlock(function, "entry");
        LLVMPositionBuilder(builder->builder, entry, NULL);

        for (Index index = 0; index < ASTArrayGetElementCount(declaration->body->statements); index++) {
            ASTNodeRef statement = (ASTNodeRef)ASTArrayGetElementAtIndex(declaration->body->statements, index);
            _IRBuilderBuildStatement(builder, function, statement);
        }

        if (!(declaration->body->base.flags & ASTFlagsStatementIsAlwaysReturning)) {
            LLVMBuildRetVoid(builder->builder);
        }
    } else {
        // We are using the C calling convention for all foreign function declarations for now because we do not allow to specify the
        // calling convention in the AST
        LLVMSetFunctionCallConv(function, LLVMCCallConv);
    }
}

static inline void _IRBuilderBuildLocalVariable(IRBuilderRef builder, LLVMValueRef function, ASTValueDeclarationRef declaration) {
    // For now we will always alloca every local variable because we are not tracking in earlier passes if the value is referenced inside
    // the scope performing an alloca by default will guarantee some grade of correctness and could still be optimized by llvm passes
    assert(!declaration->base.base.irValue);
    assert(!declaration->base.mangledName);

    declaration->base.base.irType = _IRBuilderGetIRType(builder, declaration->base.type);

    if (declaration->initializer) {
        _IRBuilderBuildExpression(builder, function, declaration->initializer);
    }

    LLVMTypeRef type   = (LLVMTypeRef)declaration->base.base.irType;
    LLVMValueRef value = LLVMBuildAlloca(builder->builder, type, StringGetCharacters(declaration->base.name));
    if (declaration->initializer) {
        LLVMBuildStore(builder->builder, (LLVMValueRef)declaration->initializer->base.irValue, value);
    }

    declaration->base.base.irValue = value;
    declaration->base.base.flags |= ASTFlagsIsValuePointer;
}

static inline void _IRBuilderBuildBlock(IRBuilderRef builder, LLVMValueRef function, ASTBlockRef block) {
    for (Index index = 0; index < ASTArrayGetElementCount(block->statements); index++) {
        ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(block->statements, index);
        _IRBuilderBuildStatement(builder, function, child);
        if (child->flags & ASTFlagsBlockHasTerminator) {
            break;
        }
    }
}

static inline void _IRBuilderBuildStatement(IRBuilderRef builder, LLVMValueRef function, ASTNodeRef node) {
    switch (node->tag) {
    case ASTTagIfStatement:
        return _IRBuilderBuildIfStatement(builder, function, (ASTIfStatementRef)node);

    case ASTTagLoopStatement:
        return _IRBuilderBuildLoopStatement(builder, function, (ASTLoopStatementRef)node);

    case ASTTagSwitchStatement:
        return _IRBuilderBuildSwitchStatement(builder, function, (ASTSwitchStatementRef)node);

    case ASTTagControlStatement:
        return _IRBuilderBuildControlStatement(builder, function, (ASTControlStatementRef)node);

    case ASTTagUnaryExpression:
    case ASTTagBinaryExpression:
    case ASTTagIdentifierExpression:
    case ASTTagMemberAccessExpression:
    case ASTTagAssignmentExpression:
    case ASTTagCallExpression:
    case ASTTagConstantExpression:
        return _IRBuilderBuildExpression(builder, function, (ASTExpressionRef)node);

    case ASTTagValueDeclaration:
        return _IRBuilderBuildLocalVariable(builder, function, (ASTValueDeclarationRef)node);

    default:
        JELLY_UNREACHABLE("Invalid tag given for ASTStatement!");
        break;
    }
}

static inline void _IRBuilderBuildIfStatement(IRBuilderRef builder, LLVMValueRef function, ASTIfStatementRef statement) {
    LLVMBasicBlockRef entryBB  = LLVMGetInsertBlock(builder->builder);
    LLVMBasicBlockRef branchBB = LLVMAppendBasicBlock(function, "if-branch");
    LLVMBasicBlockRef thenBB   = LLVMAppendBasicBlock(function, "if-then");
    LLVMBasicBlockRef elseBB   = LLVMAppendBasicBlock(function, "if-else");
    LLVMBasicBlockRef mergeBB  = LLVMAppendBasicBlock(function, "if-merge");

    LLVMPositionBuilder(builder->builder, entryBB, NULL);
    LLVMBuildBr(builder->builder, branchBB);

    LLVMPositionBuilder(builder->builder, branchBB, NULL);
    _IRBuilderBuildExpression(builder, function, statement->condition);
    LLVMBuildCondBr(builder->builder, _IRBuilderLoadExpression(builder, function, statement->condition), thenBB, elseBB);

    LLVMPositionBuilder(builder->builder, thenBB, NULL);
    _IRBuilderBuildBlock(builder, function, statement->thenBlock);
    if (!(statement->thenBlock->base.flags & ASTFlagsBlockHasTerminator)) {
        LLVMBuildBr(builder->builder, mergeBB);
    }

    LLVMPositionBuilder(builder->builder, elseBB, NULL);
    _IRBuilderBuildBlock(builder, function, statement->elseBlock);
    if (!(statement->elseBlock->base.flags & ASTFlagsBlockHasTerminator)) {
        LLVMBuildBr(builder->builder, mergeBB);
    }

    LLVMPositionBuilder(builder->builder, mergeBB, NULL);
}

static inline void _IRBuilderBuildLoopStatement(IRBuilderRef builder, LLVMValueRef function, ASTLoopStatementRef statement) {
    LLVMBasicBlockRef entryBB  = LLVMGetInsertBlock(builder->builder);
    LLVMBasicBlockRef branchBB = LLVMAppendBasicBlock(function, "loop-branch");
    LLVMBasicBlockRef bodyBB   = LLVMAppendBasicBlock(function, "loop-body");
    LLVMBasicBlockRef endBB    = LLVMAppendBasicBlock(function, "loop-end");

    statement->irEntry = bodyBB;
    statement->irExit  = endBB;

    LLVMBasicBlockRef startBB = NULL;
    switch (statement->kind) {
    case ASTLoopKindWhile: {
        startBB = branchBB;
        break;
    }

    case ASTLoopKindDo: {
        startBB = bodyBB;
        break;
    }

    default:
        JELLY_UNREACHABLE("Invalid kind given for ASTLoopStatement");
        break;
    }

    LLVMPositionBuilder(builder->builder, entryBB, NULL);
    LLVMBuildBr(builder->builder, startBB);

    LLVMPositionBuilder(builder->builder, branchBB, NULL);
    _IRBuilderBuildExpression(builder, function, statement->condition);
    LLVMBuildCondBr(builder->builder, _IRBuilderLoadExpression(builder, function, statement->condition), bodyBB, endBB);

    LLVMPositionBuilder(builder->builder, bodyBB, NULL);
    _IRBuilderBuildBlock(builder, function, statement->loopBlock);
    if (!(statement->loopBlock->base.flags & ASTFlagsBlockHasTerminator)) {
        LLVMBuildBr(builder->builder, branchBB);
    }

    LLVMPositionBuilder(builder->builder, endBB, NULL);
}

static inline void _IRBuilderBuildSwitchStatement(IRBuilderRef builder, LLVMValueRef function, ASTSwitchStatementRef statement) {
    LLVMBasicBlockRef insertBB = LLVMGetInsertBlock(builder->builder);
    LLVMBasicBlockRef branchBB = LLVMAppendBasicBlock(function, "switch-branch");
    LLVMBasicBlockRef elseBB   = LLVMAppendBasicBlock(function, "switch-else");
    LLVMBasicBlockRef endBB    = LLVMAppendBasicBlock(function, "switch-end");

    statement->irExit = endBB;

    Index caseCount = ASTArrayGetElementCount(statement->cases);
    assert(caseCount > 0);
    ASTCaseStatementRef lastCase = ASTArrayGetElementAtIndex(statement->cases, caseCount - 1);
    if (lastCase->kind == ASTCaseKindElse) {
        lastCase->base.irValue = elseBB;
        caseCount -= 1;
    }

    for (Index caseIndex = 0; caseIndex < caseCount; caseIndex++) {
        ASTCaseStatementRef child = ASTArrayGetElementAtIndex(statement->cases, caseIndex);
        assert(child->kind != ASTCaseKindElse);
        child->base.irValue = LLVMAppendBasicBlock(function, "switch-case");
    }

    if (lastCase->kind == ASTCaseKindElse) {
        LLVMPositionBuilder(builder->builder, elseBB, NULL);
        for (Index index = 0; index < ASTArrayGetElementCount(lastCase->body->statements); index++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(lastCase->body->statements, index);
            _IRBuilderBuildStatement(builder, function, child);
        }
    } else {
        LLVMPositionBuilder(builder->builder, elseBB, NULL);
        LLVMBuildBr(builder->builder, endBB);
    }

    LLVMPositionBuilder(builder->builder, insertBB, NULL);
    LLVMBuildBr(builder->builder, branchBB);
    LLVMPositionBuilder(builder->builder, branchBB, NULL);
    _IRBuilderBuildExpression(builder, function, statement->argument);

    for (Index caseIndex = 0; caseIndex < caseCount; caseIndex++) {
        ASTCaseStatementRef child = ASTArrayGetElementAtIndex(statement->cases, caseIndex);
        assert(child->kind != ASTCaseKindElse);
        _IRBuilderBuildExpression(builder, function, child->condition);
    }

    // TODO: LLVM Switch is only supported for integer types, if the condition is not an integer then this will not work so this should be
    // implemented in a general way by resolving a comparison operator for the argument type and each case expression type
    LLVMValueRef switchValue = LLVMBuildSwitch(builder->builder, _IRBuilderLoadExpression(builder, function, statement->argument), elseBB,
                                               caseCount);
    statement->base.irValue  = switchValue;
    for (Index caseIndex = 0; caseIndex < caseCount; caseIndex++) {
        ASTCaseStatementRef child = (ASTCaseStatementRef)ASTArrayGetElementAtIndex(statement->cases, caseIndex);
        assert(child->kind != ASTCaseKindElse);
        assert(child->base.irValue);

        if (caseIndex + 1 < ASTArrayGetElementCount(statement->cases)) {
            ASTCaseStatementRef next = (ASTCaseStatementRef)ASTArrayGetElementAtIndex(statement->cases, caseIndex + 1);
            assert(next->base.irValue);
            child->irNext = next->base.irValue;
        }

        LLVMValueRef condition   = (LLVMValueRef)_IRBuilderLoadExpression(builder, function, child->condition);
        LLVMBasicBlockRef caseBB = (LLVMBasicBlockRef)child->base.irValue;

        LLVMPositionBuilder(builder->builder, branchBB, NULL);
        LLVMAddCase(switchValue, condition, caseBB);

        LLVMPositionBuilder(builder->builder, caseBB, NULL);
        for (Index index = 0; index < ASTArrayGetElementCount(child->body->statements); index++) {
            ASTNodeRef caseChild = (ASTNodeRef)ASTArrayGetElementAtIndex(child->body->statements, index);
            _IRBuilderBuildStatement(builder, function, caseChild);
        }
    }

    LLVMPositionBuilder(builder->builder, endBB, NULL);
}

static inline void _IRBuilderBuildControlStatement(IRBuilderRef builder, LLVMValueRef function, ASTControlStatementRef statement) {
    switch (statement->kind) {
    case ASTControlKindBreak: {
        assert(statement->enclosingNode);
        if (statement->enclosingNode->tag == ASTTagSwitchStatement) {
            ASTSwitchStatementRef parent = (ASTSwitchStatementRef)statement->enclosingNode;
            assert(parent->irExit);
            LLVMBuildBr(builder->builder, (LLVMBasicBlockRef)parent->irExit);
            return;
        } else if (statement->enclosingNode->tag == ASTTagLoopStatement) {
            ASTLoopStatementRef parent = (ASTLoopStatementRef)statement->enclosingNode;
            assert(parent->irExit);
            LLVMBuildBr(builder->builder, (LLVMBasicBlockRef)parent->irExit);
            return;
        }
        break;
    }

    case ASTControlKindContinue: {
        assert(statement->enclosingNode);
        if (statement->enclosingNode->tag == ASTTagLoopStatement) {
            ASTLoopStatementRef loop = (ASTLoopStatementRef)statement->enclosingNode;
            assert(loop->irEntry);
            LLVMBuildBr(builder->builder, (LLVMBasicBlockRef)loop->irEntry);
            return;
        }
        break;
    }

    case ASTControlKindFallthrough: {
        assert(statement->enclosingNode);
        if (statement->enclosingNode->tag == ASTTagCaseStatement) {
            ASTCaseStatementRef caseStatement = (ASTCaseStatementRef)statement->enclosingNode;
            assert(caseStatement->irNext);
            LLVMBuildBr(builder->builder, (LLVMBasicBlockRef)caseStatement->irNext);
            return;
        }
        break;
    }

    case ASTControlKindReturn: {
        if (statement->result) {
            _IRBuilderBuildExpression(builder, function, statement->result);
            LLVMBuildRet(builder->builder, _IRBuilderLoadExpression(builder, function, statement->result));
            return;
        } else {
            LLVMBuildRetVoid(builder->builder);
            return;
        }
        break;
    }

    default:
        break;
    }

    ReportCritical("Internal compiler error!");
}

static inline void _IRBuilderBuildExpression(IRBuilderRef builder, LLVMValueRef function, ASTExpressionRef expression) {
    switch (expression->base.tag) {
    case ASTTagUnaryExpression: {
        ASTUnaryExpressionRef unary = (ASTUnaryExpressionRef)expression;
        assert(unary->opFunction);

        // Prefix functions are currently not added to the declarations of the module and are just contained inside the global scope, so we
        // will force the IR generation of the function here for now...
        if (!unary->opFunction->base.base.irType) {
            assert(ASTArrayGetElementCount(unary->opFunction->parameters) == 1);
            ASTValueDeclarationRef parameter    = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(unary->opFunction->parameters, 0);
            LLVMTypeRef parameterTypes[]        = {_IRBuilderGetIRType(builder, parameter->base.type)};
            unary->opFunction->base.base.irType = LLVMFunctionType(_IRBuilderGetIRType(builder, unary->opFunction->returnType),
                                                                   parameterTypes, 1, false);
        }

        if (!unary->opFunction->base.base.irValue) {
            _IRBuilderBuildFunctionDeclaration(builder, unary->opFunction);
        }
        assert(unary->opFunction->base.base.irValue);

        _IRBuilderBuildExpression(builder, function, unary->arguments[0]);
        LLVMValueRef arguments[] = {_IRBuilderLoadExpression(builder, function, unary->arguments[0])};
        LLVMValueRef opFunction  = (LLVMValueRef)unary->opFunction->base.base.irValue;
        unary->base.base.irValue = LLVMBuildCall(builder->builder, opFunction, arguments, 1, "");
        return;
    }

    case ASTTagBinaryExpression: {
        ASTBinaryExpressionRef binary = (ASTBinaryExpressionRef)expression;
        assert(binary->opFunction);

        // Infix functions are currently not added to the declarations of the module and are just contained inside the global scope, so we
        // will force the IR generation of the function here for now...
        if (!binary->opFunction->base.base.irType) {
            assert(ASTArrayGetElementCount(binary->opFunction->parameters) == 2);
            ASTValueDeclarationRef lhsParameter  = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(binary->opFunction->parameters, 0);
            ASTValueDeclarationRef rhsParameter  = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(binary->opFunction->parameters, 1);
            LLVMTypeRef parameterTypes[]         = {_IRBuilderGetIRType(builder, lhsParameter->base.type),
                                            _IRBuilderGetIRType(builder, rhsParameter->base.type)};
            binary->opFunction->base.base.irType = LLVMFunctionType(_IRBuilderGetIRType(builder, binary->opFunction->returnType),
                                                                    parameterTypes, 2, false);
        }

        if (!binary->opFunction->base.base.irValue) {
            _IRBuilderBuildFunctionDeclaration(builder, binary->opFunction);
        }
        assert(binary->opFunction->base.base.irValue);

        _IRBuilderBuildExpression(builder, function, binary->arguments[0]);
        _IRBuilderBuildExpression(builder, function, binary->arguments[1]);

        LLVMValueRef opFunction   = (LLVMValueRef)binary->opFunction->base.base.irValue;
        LLVMValueRef arguments[]  = {_IRBuilderLoadExpression(builder, function, binary->arguments[0]),
                                    _IRBuilderLoadExpression(builder, function, binary->arguments[1])};
        binary->base.base.irValue = LLVMBuildCall(builder->builder, opFunction, arguments, 2, "");
        return;
    }

    case ASTTagIdentifierExpression: {
        ASTIdentifierExpressionRef identifier = (ASTIdentifierExpressionRef)expression;
        assert(identifier->resolvedDeclaration && identifier->resolvedDeclaration->base.irValue);
        if (identifier->resolvedDeclaration->base.flags & ASTFlagsIsValuePointer) {
            identifier->base.base.flags |= ASTFlagsIsValuePointer;
        }

        identifier->base.base.irType  = identifier->resolvedDeclaration->base.irType;
        identifier->base.base.irValue = identifier->resolvedDeclaration->base.irValue;
        return;
    }

    case ASTTagMemberAccessExpression: {
        ASTMemberAccessExpressionRef memberAccess = (ASTMemberAccessExpressionRef)expression;
        assert(memberAccess->memberIndex >= 0);

        _IRBuilderBuildExpression(builder, function, memberAccess->argument);
        LLVMValueRef pointer = NULL;
        if (memberAccess->argument->base.flags & ASTFlagsIsValuePointer) {
            pointer = memberAccess->argument->base.irValue;
        } else {
            pointer = LLVMBuildAlloca(builder->builder, (LLVMTypeRef)memberAccess->argument->base.irType, "");
            LLVMBuildStore(builder->builder, _IRBuilderLoadExpression(builder, function, memberAccess->argument), pointer);
        }

        Index pointerDepth = memberAccess->pointerDepth;
        while (pointerDepth > 0) {
            pointer = LLVMBuildLoad(builder->builder, pointer, "");
            pointerDepth -= 1;
        }

        memberAccess->base.base.irType  = memberAccess->argument->base.irType;
        memberAccess->base.base.irValue = LLVMBuildStructGEP(builder->builder, pointer, memberAccess->memberIndex, "");
        memberAccess->base.base.flags |= ASTFlagsIsValuePointer;
        return;
    }

    case ASTTagAssignmentExpression: {
        ASTAssignmentExpressionRef assignment = (ASTAssignmentExpressionRef)expression;
        assert(assignment->op == ASTBinaryOperatorAssign && "Composite assignment operations are not supported yet!");
        _IRBuilderBuildExpression(builder, function, assignment->variable);
        _IRBuilderBuildExpression(builder, function, assignment->expression);
        assignment->base.base.irType  = LLVMVoidType();
        assignment->base.base.irValue = LLVMBuildStore(builder->builder,
                                                       _IRBuilderLoadExpression(builder, function, assignment->expression),
                                                       (LLVMValueRef)assignment->variable->base.irValue);
        return;
    }

    case ASTTagCallExpression: {
        ASTCallExpressionRef call = (ASTCallExpressionRef)expression;
        assert(call->callee->type->tag == ASTTagFunctionType);
        ASTFunctionTypeRef functionType               = (ASTFunctionTypeRef)call->callee->type;
        ASTFunctionDeclarationRef functionDeclaration = functionType->declaration;

        _IRBuilderBuildExpression(builder, function, call->callee);

        ArrayRef arguments = ArrayCreateEmpty(builder->allocator, sizeof(LLVMValueRef), ASTArrayGetElementCount(call->arguments));
        for (Index index = 0; index < ASTArrayGetElementCount(call->arguments); index++) {
            ASTExpressionRef argument = (ASTExpressionRef)ASTArrayGetElementAtIndex(call->arguments, index);
            _IRBuilderBuildExpression(builder, function, argument);
            ArrayAppendElement(arguments, &argument->base.irValue);
        }

        assert(functionDeclaration->returnType->irType);
        call->base.base.irType  = functionDeclaration->returnType->irType;
        call->base.base.irValue = (LLVMValueRef)LLVMBuildCall(builder->builder, (LLVMValueRef)call->callee->base.irValue,
                                                              (LLVMValueRef *)ArrayGetMemoryPointer(arguments),
                                                              ArrayGetElementCount(arguments), "");

        ArrayDestroy(arguments);
        return;
    }

    case ASTTagConstantExpression: {
        ASTConstantExpressionRef constant = (ASTConstantExpressionRef)expression;
        LLVMTypeRef type                  = _IRBuilderGetIRType(builder, constant->base.type);
        switch (constant->kind) {
        case ASTConstantKindNil:
            constant->base.base.irValue = LLVMConstNull(type);
            break;

        case ASTConstantKindBool:
            constant->base.base.irValue = LLVMConstInt(type, constant->boolValue ? 1 : 0, false);
            break;

        case ASTConstantKindInt:
            constant->base.base.irValue = LLVMConstInt(type, constant->intValue, false);
            break;

        case ASTConstantKindFloat:
            constant->base.base.irValue = LLVMConstReal(type, constant->floatValue);
            break;

        case ASTConstantKindString:
            ReportCritical("String literals are currently not supported!");
            break;

        default:
            JELLY_UNREACHABLE("Invalid kind given for ASTConstantExpression!");
            break;
        }
        return;
    }

    default:
        break;
    }
}

static inline LLVMValueRef _IRBuilderLoadExpression(IRBuilderRef builder, LLVMValueRef function, ASTExpressionRef expression) {
    assert(expression->base.irValue);

    if (expression->base.flags & ASTFlagsIsValuePointer) {
        return LLVMBuildLoad(builder->builder, (LLVMValueRef)expression->base.irValue, "");
    }

    return (LLVMValueRef)expression->base.irValue;
}

static inline LLVMTypeRef _IRBuilderGetIRType(IRBuilderRef builder, ASTTypeRef type) {
    assert(type && type->tag != ASTTagOpaqueType);

    if (type->irType) {
        return (LLVMTypeRef)type->irType;
    }

    LLVMTypeRef llvmType = NULL;
    switch (type->tag) {
    case ASTTagPointerType: {
        ASTPointerTypeRef pointerType = (ASTPointerTypeRef)type;
        llvmType                      = _IRBuilderGetIRType(builder, pointerType->pointeeType);
        llvmType                      = LLVMPointerType(llvmType, 0);
        break;
    }

    case ASTTagArrayType: {
        ReportCritical("Array type is currently not supported!");
        break;
    }

    case ASTTagBuiltinType: {
        ASTBuiltinTypeRef builtinType = (ASTBuiltinTypeRef)type;
        switch (builtinType->kind) {
        case ASTBuiltinTypeKindVoid:
            llvmType = LLVMVoidType();
            break;

        case ASTBuiltinTypeKindBool:
            llvmType = LLVMInt1Type();
            break;

        case ASTBuiltinTypeKindInt8:
        case ASTBuiltinTypeKindUInt8:
            llvmType = LLVMInt8Type();
            break;

        case ASTBuiltinTypeKindInt16:
        case ASTBuiltinTypeKindUInt16:
            llvmType = LLVMInt16Type();
            break;

        case ASTBuiltinTypeKindInt32:
        case ASTBuiltinTypeKindUInt32:
            llvmType = LLVMInt32Type();
            break;

        case ASTBuiltinTypeKindInt64:
        case ASTBuiltinTypeKindUInt64:
        case ASTBuiltinTypeKindInt:
        case ASTBuiltinTypeKindUInt:
            llvmType = LLVMInt64Type();
            break;

        case ASTBuiltinTypeKindFloat32:
            llvmType = LLVMFloatType();
            break;

        case ASTBuiltinTypeKindFloat64:
        case ASTBuiltinTypeKindFloat:
            llvmType = LLVMDoubleType();
            break;

        default:
            JELLY_UNREACHABLE("Invalid kind given for ASTBuiltinType!");
            break;
        }

        break;
    }

    case ASTTagEnumerationType: {
        // TODO: Replace enumeration type with the least required bit size int based on the represented values of the enumeration elements
        llvmType = LLVMInt64Type();
        break;
    }

    case ASTTagFunctionType: {
        ASTFunctionTypeRef functionType = (ASTFunctionTypeRef)type;
        assert(functionType->declaration->base.base.irType);
        llvmType = (LLVMTypeRef)functionType->declaration->base.base.irType;
        break;
    }

    case ASTTagStructureType: {
        ASTStructureTypeRef structureType = (ASTStructureTypeRef)type;
        assert(structureType->declaration->base.base.irType);
        llvmType = (LLVMTypeRef)structureType->declaration->base.base.irType;
        break;
    }

    default:
        JELLY_UNREACHABLE("Invalid tag given for ASTType!");
        break;
    }

    type->irType = llvmType;
    return llvmType;
}