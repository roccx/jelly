#include "JellyCore/ASTFunctions.h"
#include "JellyCore/ASTSubstitution.h"

static inline void _ASTApplySubstitution(ASTContextRef context, ASTNodeRef node);

void ASTPerformSubstitution(ASTContextRef context, ASTTag tag, ASTTransform transform) {
    BucketArrayRef nodes = ASTContextGetAllNodes(context, tag);
    for (Index index = 0; index < BucketArrayGetElementCount(nodes); index++) {
        ASTNodeRef node = (ASTNodeRef)BucketArrayGetElementAtIndex(nodes, index);
        if (node->substitute) {
            continue;
        }

        ASTNodeRef substitute = transform(context, node);
        if (substitute) {
            substitute->primary = node;
            node->substitute    = substitute;
        }
    }
}

void ASTApplySubstitution(ASTContextRef context, ASTModuleDeclarationRef module) {
    _ASTApplySubstitution(context, (ASTNodeRef)module->sourceUnits);
}

ASTNodeRef ASTUnaryExpressionUnification(ASTContextRef context, ASTNodeRef node) {
    assert(node->tag == ASTTagUnaryExpression);

    ASTUnaryExpressionRef expression = (ASTUnaryExpressionRef)node;
    return (ASTNodeRef)ASTContextCreateUnaryCallExpression(context, node->location, node->scope, expression->op, expression->arguments);
}

ASTNodeRef ASTBinaryExpressionUnification(ASTContextRef context, ASTNodeRef node) {
    assert(node->tag == ASTTagBinaryExpression);

    ASTBinaryExpressionRef expression = (ASTBinaryExpressionRef)node;
    return (ASTNodeRef)ASTContextCreateBinaryCallExpression(context, node->location, node->scope, expression->op, expression->arguments);
}

#define _ASTApplySubstitutionInplace(__CONTEXT__, __NODE__, __TYPE__)                                                                      \
    while (((ASTNodeRef)__NODE__)->substitute) {                                                                                           \
        __NODE__ = (__TYPE__)((ASTNodeRef)__NODE__)->substitute;                                                                           \
    }                                                                                                                                      \
    _ASTApplySubstitution(__CONTEXT__, (ASTNodeRef)__NODE__);

static inline void _ASTApplySubstitution(ASTContextRef context, ASTNodeRef node) {
    if (node->tag == ASTTagSourceUnit) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)node;
        _ASTApplySubstitutionInplace(context, sourceUnit->declarations, ASTArrayRef);
        return;
    }

    if (node->tag == ASTTagArray) {
        ASTArrayRef array            = (ASTArrayRef)node;
        ASTArrayIteratorRef iterator = ASTArrayGetIterator(array);
        while (iterator) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayIteratorGetElement(iterator);
            _ASTApplySubstitutionInplace(context, child, ASTNodeRef);
            ASTArrayIteratorSetElement(iterator, child);
            iterator = ASTArrayIteratorNext(iterator);
        }
        return;
    }

    if (node->tag == ASTTagLoadDirective) {
        ASTLoadDirectiveRef load = (ASTLoadDirectiveRef)node;
        _ASTApplySubstitutionInplace(context, load->filePath, ASTConstantExpressionRef);
        return;
    }

    if (node->tag == ASTTagImportDirective) {
        return;
    }

    if (node->tag == ASTTagIncludeDirective) {
        return;
    }

    if (node->tag == ASTTagBlock) {
        ASTBlockRef block = (ASTBlockRef)node;
        _ASTApplySubstitutionInplace(context, block->statements, ASTArrayRef);
        return;
    }

    if (node->tag == ASTTagIfStatement) {
        ASTIfStatementRef statement = (ASTIfStatementRef)node;
        _ASTApplySubstitutionInplace(context, statement->condition, ASTExpressionRef);
        _ASTApplySubstitutionInplace(context, statement->thenBlock, ASTBlockRef);
        _ASTApplySubstitutionInplace(context, statement->elseBlock, ASTBlockRef);
        return;
    }

    if (node->tag == ASTTagLoopStatement) {
        ASTLoopStatementRef statement = (ASTLoopStatementRef)node;
        _ASTApplySubstitutionInplace(context, statement->condition, ASTExpressionRef);
        _ASTApplySubstitutionInplace(context, statement->loopBlock, ASTBlockRef);
        return;
    }

    if (node->tag == ASTTagCaseStatement) {
        ASTCaseStatementRef statement = (ASTCaseStatementRef)node;
        if (statement->kind == ASTCaseKindConditional) {
            _ASTApplySubstitutionInplace(context, statement->condition, ASTExpressionRef);
        }

        _ASTApplySubstitutionInplace(context, statement->body, ASTBlockRef);
        return;
    }

    if (node->tag == ASTTagSwitchStatement) {
        ASTSwitchStatementRef statement = (ASTSwitchStatementRef)node;
        _ASTApplySubstitutionInplace(context, statement->argument, ASTExpressionRef);
        _ASTApplySubstitutionInplace(context, statement->cases, ASTArrayRef);
        return;
    }

    if (node->tag == ASTTagControlStatement) {
        ASTControlStatementRef statement = (ASTControlStatementRef)node;
        if (statement->result) {
            _ASTApplySubstitutionInplace(context, statement->result, ASTExpressionRef);
        }
        return;
    }

    if (node->tag == ASTTagReferenceExpression) {
        ASTReferenceExpressionRef expression = (ASTReferenceExpressionRef)node;
        _ASTApplySubstitutionInplace(context, expression->argument, ASTExpressionRef);
        return;
    }

    if (node->tag == ASTTagDereferenceExpression) {
        ASTDereferenceExpressionRef expression = (ASTDereferenceExpressionRef)node;
        _ASTApplySubstitutionInplace(context, expression->argument, ASTExpressionRef);
        return;
    }

    if (node->tag == ASTTagUnaryExpression) {
        ASTUnaryExpressionRef expression = (ASTUnaryExpressionRef)node;
        _ASTApplySubstitutionInplace(context, expression->arguments[0], ASTExpressionRef);
        return;
    }

    if (node->tag == ASTTagBinaryExpression) {
        ASTBinaryExpressionRef expression = (ASTBinaryExpressionRef)node;
        _ASTApplySubstitutionInplace(context, expression->arguments[0], ASTExpressionRef);
        _ASTApplySubstitutionInplace(context, expression->arguments[1], ASTExpressionRef);
        return;
    }

    if (node->tag == ASTTagIdentifierExpression) {
        return;
    }

    if (node->tag == ASTTagMemberAccessExpression) {
        ASTMemberAccessExpressionRef expression = (ASTMemberAccessExpressionRef)node;
        _ASTApplySubstitutionInplace(context, expression->argument, ASTExpressionRef);
        return;
    }

    if (node->tag == ASTTagAssignmentExpression) {
        ASTAssignmentExpressionRef expression = (ASTAssignmentExpressionRef)node;
        _ASTApplySubstitutionInplace(context, expression->variable, ASTExpressionRef);
        _ASTApplySubstitutionInplace(context, expression->expression, ASTExpressionRef);
        return;
    }

    if (node->tag == ASTTagCallExpression) {
        ASTCallExpressionRef expression = (ASTCallExpressionRef)node;
        _ASTApplySubstitutionInplace(context, expression->callee, ASTExpressionRef);
        _ASTApplySubstitutionInplace(context, expression->arguments, ASTArrayRef);
        return;
    }

    if (node->tag == ASTTagConstantExpression) {
        return;
    }

    if (node->tag == ASTTagSizeOfExpression) {
        ASTSizeOfExpressionRef expression = (ASTSizeOfExpressionRef)node;
        _ASTApplySubstitutionInplace(context, expression->sizeType, ASTTypeRef);
        return;
    }

    if (node->tag == ASTTagSubscriptExpression) {
        ASTSubscriptExpressionRef expression = (ASTSubscriptExpressionRef)node;
        _ASTApplySubstitutionInplace(context, expression->expression, ASTExpressionRef);
        _ASTApplySubstitutionInplace(context, expression->arguments, ASTArrayRef);
        return;
    }

    if (node->tag == ASTTagTypeOperationExpression) {
        ASTTypeOperationExpressionRef expression = (ASTTypeOperationExpressionRef)node;
        _ASTApplySubstitutionInplace(context, expression->expression, ASTExpressionRef);
        _ASTApplySubstitutionInplace(context, expression->argumentType, ASTTypeRef);
        return;
    }

    if (node->tag == ASTTagEnumerationDeclaration) {
        ASTEnumerationDeclarationRef declaration = (ASTEnumerationDeclarationRef)node;
        _ASTApplySubstitutionInplace(context, declaration->elements, ASTArrayRef);
        return;
    }

    if (node->tag == ASTTagFunctionDeclaration || node->tag == ASTTagForeignFunctionDeclaration ||
        node->tag == ASTTagIntrinsicFunctionDeclaration) {
        ASTFunctionDeclarationRef declaration = (ASTFunctionDeclarationRef)node;
        _ASTApplySubstitutionInplace(context, declaration->parameters, ASTArrayRef);
        _ASTApplySubstitutionInplace(context, declaration->returnType, ASTTypeRef);
        if (declaration->body) {
            _ASTApplySubstitutionInplace(context, declaration->body, ASTBlockRef);
        }
        return;
    }

    if (node->tag == ASTTagStructureDeclaration) {
        ASTStructureDeclarationRef declaration = (ASTStructureDeclarationRef)node;
        _ASTApplySubstitutionInplace(context, declaration->values, ASTArrayRef);
        return;
    }

    if (node->tag == ASTTagValueDeclaration) {
        ASTValueDeclarationRef declaration = (ASTValueDeclarationRef)node;
        _ASTApplySubstitutionInplace(context, declaration->base.type, ASTTypeRef);
        if (declaration->initializer) {
            _ASTApplySubstitutionInplace(context, declaration->initializer, ASTExpressionRef);
        }
        return;
    }

    if (node->tag == ASTTagTypeAliasDeclaration) {
        ASTTypeAliasDeclarationRef declaration = (ASTTypeAliasDeclarationRef)node;
        _ASTApplySubstitutionInplace(context, declaration->base.type, ASTTypeRef);
        return;
    }

    if (node->tag == ASTTagOpaqueType) {
        return;
    }

    if (node->tag == ASTTagPointerType) {
        ASTPointerTypeRef type = (ASTPointerTypeRef)node;
        _ASTApplySubstitutionInplace(context, type->pointeeType, ASTTypeRef);
        return;
    }

    if (node->tag == ASTTagArrayType) {
        ASTArrayTypeRef type = (ASTArrayTypeRef)node;
        _ASTApplySubstitutionInplace(context, type->elementType, ASTTypeRef);
        if (type->size) {
            _ASTApplySubstitutionInplace(context, type->size, ASTExpressionRef);
        }
        return;
    }

    if (node->tag == ASTTagBuiltinType) {
        return;
    }

    if (node->tag == ASTTagEnumerationType) {
        return;
    }

    if (node->tag == ASTTagFunctionType) {
        ASTFunctionTypeRef type = (ASTFunctionTypeRef)node;
        _ASTApplySubstitutionInplace(context, type->parameterTypes, ASTArrayRef);
        _ASTApplySubstitutionInplace(context, type->resultType, ASTTypeRef);
        return;
    }

    if (node->tag == ASTTagStructureType) {
        return;
    }

    if (node->tag == ASTTagGenericType) {
        return;
    }

    JELLY_UNREACHABLE("Invalid tag given for substitution!");
}

#undef _ASTApplySubstitutionInplace
