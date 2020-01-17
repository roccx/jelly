#include "JellyCore/ASTDumper.h"

// TODO: Add runtime informations like memory address and also other members like types
//       for now this implementation will be helpful to run tests for the Parser...

struct _ASTDumper {
    AllocatorRef allocator;
    FILE *target;
    Index indentation;
};

static inline void _ASTDumperPrintIndentation(ASTDumperRef dumper);
static inline void _ASTDumperPrintCString(ASTDumperRef dumper, const Char *string);
static inline void _ASTDumperPrintProperty(ASTDumperRef dumper, const Char *key, const Char *value);
static inline void _ASTDumperPrintTag(ASTDumperRef dumper, ASTNodeRef node);
static inline void _ASTDumperPrintBuiltinTypeKind(ASTDumperRef dumper, ASTBuiltinTypeKind kind);
static inline void _ASTDumperPrintPrefixOperator(ASTDumperRef dumper, ASTUnaryOperator op);
static inline void _ASTDumperPrintInfixOperator(ASTDumperRef dumper, ASTBinaryOperator op);
static inline void _ASTDumperDumpChild(ASTDumperRef dumper, ASTNodeRef child);
static inline void _ASTDumperDumpChildrenArray(ASTDumperRef dumper, ASTArrayRef array);

ASTDumperRef ASTDumperCreate(AllocatorRef allocator, FILE *target) {
    ASTDumperRef dumper = (ASTDumperRef)AllocatorAllocate(allocator, sizeof(struct _ASTDumper));
    dumper->allocator   = allocator;
    dumper->target      = target;
    dumper->indentation = 0;
    return dumper;
}

void ASTDumperDestroy(ASTDumperRef dumper) {
    AllocatorDeallocate(dumper->allocator, dumper);
}

void ASTDumperDump(ASTDumperRef dumper, ASTNodeRef node) {
    _ASTDumperPrintIndentation(dumper);
    _ASTDumperPrintTag(dumper, node);
    _ASTDumperPrintCString(dumper, "\n");

    switch (node->tag) {
    case ASTTagSourceUnit: {
        ASTSourceUnitRef unit = (ASTSourceUnitRef)node;
        _ASTDumperPrintProperty(dumper, "filePath", StringGetCharacters(unit->filePath));
        _ASTDumperDumpChildrenArray(dumper, unit->declarations);
        return;
    }

    case ASTTagLoadDirective: {
        ASTLoadDirectiveRef load = (ASTLoadDirectiveRef)node;
        _ASTDumperPrintProperty(dumper, "sourceFilePath", StringGetCharacters(load->filePath->stringValue));
        return;
    }

    case ASTTagImportDirective: {
        ASTImportDirectiveRef import = (ASTImportDirectiveRef)node;
        _ASTDumperPrintProperty(dumper, "modulePath", StringGetCharacters(import->modulePath));
        return;
    }

    case ASTTagIncludeDirective: {
        ASTIncludeDirectiveRef include = (ASTIncludeDirectiveRef)node;
        _ASTDumperPrintProperty(dumper, "headerPath", StringGetCharacters(include->headerPath));
        return;
    }

    case ASTTagBlock: {
        ASTBlockRef block = (ASTBlockRef)node;
        _ASTDumperDumpChildrenArray(dumper, block->statements);
        return;
    }

    case ASTTagIfStatement: {
        ASTIfStatementRef statement = (ASTIfStatementRef)node;
        _ASTDumperDumpChild(dumper, (ASTNodeRef)statement->condition);
        _ASTDumperDumpChild(dumper, (ASTNodeRef)statement->thenBlock);
        _ASTDumperDumpChild(dumper, (ASTNodeRef)statement->elseBlock);
        return;
    }

    case ASTTagLoopStatement: {
        ASTLoopStatementRef loop = (ASTLoopStatementRef)node;
        _ASTDumperDumpChild(dumper, (ASTNodeRef)loop->condition);
        _ASTDumperDumpChild(dumper, (ASTNodeRef)loop->loopBlock);
        return;
    }

    case ASTTagCaseStatement: {
        ASTCaseStatementRef statement = (ASTCaseStatementRef)node;
        if (statement->kind == ASTCaseKindConditional) {
            _ASTDumperDumpChild(dumper, (ASTNodeRef)statement->condition);
        }

        _ASTDumperDumpChild(dumper, (ASTNodeRef)statement->body);
        return;
    }

    case ASTTagSwitchStatement: {
        ASTSwitchStatementRef statement = (ASTSwitchStatementRef)node;
        _ASTDumperDumpChild(dumper, (ASTNodeRef)statement->argument);
        _ASTDumperDumpChildrenArray(dumper, statement->cases);
        return;
    }

    case ASTTagControlStatement: {
        ASTControlStatementRef control = (ASTControlStatementRef)node;
        if (control->kind == ASTControlKindReturn && control->result) {
            _ASTDumperDumpChild(dumper, (ASTNodeRef)control->result);
        }
        return;
    }

    case ASTTagReferenceExpression: {
        ASTReferenceExpressionRef expression = (ASTReferenceExpressionRef)node;
        _ASTDumperDumpChild(dumper, (ASTNodeRef)expression->argument);
        return;
    }

    case ASTTagDereferenceExpression: {
        ASTDereferenceExpressionRef expression = (ASTDereferenceExpressionRef)node;
        _ASTDumperDumpChild(dumper, (ASTNodeRef)expression->argument);
        return;
    }

    case ASTTagUnaryExpression: {
        ASTUnaryExpressionRef unary = (ASTUnaryExpressionRef)node;

        dumper->indentation += 1;
        _ASTDumperPrintIndentation(dumper);
        _ASTDumperPrintCString(dumper, "@operator = '");
        _ASTDumperPrintPrefixOperator(dumper, unary->op);
        _ASTDumperPrintCString(dumper, "'\n");
        dumper->indentation -= 1;

        _ASTDumperDumpChild(dumper, (ASTNodeRef)unary->arguments[0]);
        return;
    }

    case ASTTagBinaryExpression: {
        ASTBinaryExpressionRef binary = (ASTBinaryExpressionRef)node;

        dumper->indentation += 1;
        _ASTDumperPrintIndentation(dumper);
        _ASTDumperPrintCString(dumper, "@operator = '");
        _ASTDumperPrintInfixOperator(dumper, binary->op);
        _ASTDumperPrintCString(dumper, "'\n");
        dumper->indentation -= 1;

        _ASTDumperDumpChild(dumper, (ASTNodeRef)binary->arguments[0]);
        _ASTDumperDumpChild(dumper, (ASTNodeRef)binary->arguments[1]);
        return;
    }

    case ASTTagIdentifierExpression: {
        ASTIdentifierExpressionRef expression = (ASTIdentifierExpressionRef)node;
        _ASTDumperPrintProperty(dumper, "identifier", StringGetCharacters(expression->name));
        return;
    }

    case ASTTagMemberAccessExpression: {
        ASTMemberAccessExpressionRef expression = (ASTMemberAccessExpressionRef)node;
        _ASTDumperPrintProperty(dumper, "memberName", StringGetCharacters(expression->memberName));
        _ASTDumperDumpChild(dumper, (ASTNodeRef)expression->argument);
        return;
    }

    case ASTTagAssignmentExpression: {
        ASTAssignmentExpressionRef assignment = (ASTAssignmentExpressionRef)node;

        dumper->indentation += 1;
        _ASTDumperPrintIndentation(dumper);
        _ASTDumperPrintCString(dumper, "@operator = '");
        _ASTDumperPrintInfixOperator(dumper, assignment->op);
        _ASTDumperPrintCString(dumper, "'\n");
        dumper->indentation -= 1;

        _ASTDumperDumpChild(dumper, (ASTNodeRef)assignment->variable);
        _ASTDumperDumpChild(dumper, (ASTNodeRef)assignment->expression);
        return;
    }

    case ASTTagCallExpression: {
        ASTCallExpressionRef call = (ASTCallExpressionRef)node;
        _ASTDumperDumpChild(dumper, (ASTNodeRef)call->callee);
        _ASTDumperDumpChildrenArray(dumper, call->arguments);
        return;
    }

    case ASTTagConstantExpression: {
        ASTConstantExpressionRef constant = (ASTConstantExpressionRef)node;
        if (constant->kind == ASTConstantKindNil) {
        } else if (constant->kind == ASTConstantKindBool) {
            if (constant->boolValue) {
                _ASTDumperPrintProperty(dumper, "value", "true");
            } else {
                _ASTDumperPrintProperty(dumper, "value", "false");
            }
        } else if (constant->kind == ASTConstantKindInt) {
            dumper->indentation += 1;
            _ASTDumperPrintIndentation(dumper);
            _ASTDumperPrintCString(dumper, "@value = '");
            Char buffer[20];
            snprintf(&buffer[0], 20, "%lld", constant->intValue);
            _ASTDumperPrintCString(dumper, &buffer[0]);
            _ASTDumperPrintCString(dumper, "'\n");
            dumper->indentation -= 1;
        } else if (constant->kind == ASTConstantKindFloat) {
            dumper->indentation += 1;
            _ASTDumperPrintIndentation(dumper);
            _ASTDumperPrintCString(dumper, "@value = '");
            Char buffer[1024];
            snprintf(&buffer[0], 1024, "%f", constant->floatValue);
            _ASTDumperPrintCString(dumper, &buffer[0]);
            _ASTDumperPrintCString(dumper, "'\n");
            dumper->indentation -= 1;
        } else if (constant->kind == ASTConstantKindString) {
            _ASTDumperPrintProperty(dumper, "value", StringGetCharacters(constant->stringValue));
        }
        return;
    }

    case ASTTagSubscriptExpression: {
        ASTSubscriptExpressionRef expression = (ASTSubscriptExpressionRef)node;
        _ASTDumperDumpChild(dumper, (ASTNodeRef)expression->expression);
        _ASTDumperDumpChildrenArray(dumper, expression->arguments);
        return;
    }

    case ASTTagTypeExpression: {
        ASTTypeExpressionRef expression = (ASTTypeExpressionRef)node;
        _ASTDumperDumpChild(dumper, (ASTNodeRef)expression->referencedType);
        return;
    }

    case ASTTagModuleDeclaration: {
        ASTModuleDeclarationRef module = (ASTModuleDeclarationRef)node;
        _ASTDumperDumpChildrenArray(dumper, module->importedModules);
        _ASTDumperDumpChildrenArray(dumper, module->sourceUnits);
        return;
    }

    case ASTTagEnumerationDeclaration: {
        ASTEnumerationDeclarationRef enumeration = (ASTEnumerationDeclarationRef)node;
        _ASTDumperPrintProperty(dumper, "name", StringGetCharacters(enumeration->base.name));
        _ASTDumperDumpChildrenArray(dumper, enumeration->elements);
        return;
    }

    case ASTTagForeignFunctionDeclaration:
    case ASTTagFunctionDeclaration: {
        ASTFunctionDeclarationRef func = (ASTFunctionDeclarationRef)node;
        _ASTDumperPrintProperty(dumper, "name", StringGetCharacters(func->base.name));
        _ASTDumperDumpChildrenArray(dumper, func->parameters);
        _ASTDumperDumpChild(dumper, func->returnType);

        if (func->base.base.tag == ASTTagForeignFunctionDeclaration) {
            _ASTDumperPrintProperty(dumper, "foreignName", StringGetCharacters(func->foreignName));
        }

        if (func->body) {
            _ASTDumperDumpChild(dumper, (ASTNodeRef)func->body);
        }
        return;
    }

    case ASTTagInitializerDeclaration: {
        ASTInitializerDeclarationRef init = (ASTInitializerDeclarationRef)node;
        _ASTDumperDumpChildrenArray(dumper, init->parameters);
        _ASTDumperDumpChild(dumper, (ASTNodeRef)init->body);
        return;
    }

    case ASTTagStructureDeclaration: {
        ASTStructureDeclarationRef structure = (ASTStructureDeclarationRef)node;
        _ASTDumperPrintProperty(dumper, "name", StringGetCharacters(structure->base.name));
        _ASTDumperDumpChildrenArray(dumper, structure->values);
        _ASTDumperDumpChildrenArray(dumper, structure->initializers);
        return;
    }

    case ASTTagValueDeclaration: {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)node;
        _ASTDumperPrintProperty(dumper, "name", StringGetCharacters(value->base.name));
        _ASTDumperDumpChild(dumper, value->base.type);

        if (value->initializer) {
            _ASTDumperDumpChild(dumper, (ASTNodeRef)value->initializer);
        }

        return;
    }

    case ASTTagTypeAliasDeclaration: {
        ASTTypeAliasDeclarationRef alias = (ASTTypeAliasDeclarationRef)node;
        _ASTDumperPrintProperty(dumper, "name", StringGetCharacters(alias->base.name));
        _ASTDumperDumpChild(dumper, alias->base.type);
        return;
    }

    case ASTTagOpaqueType: {
        ASTOpaqueTypeRef type = (ASTOpaqueTypeRef)node;
        _ASTDumperPrintProperty(dumper, "name", StringGetCharacters(type->name));
        return;
    }

    case ASTTagPointerType: {
        ASTPointerTypeRef type = (ASTPointerTypeRef)node;
        _ASTDumperDumpChild(dumper, type->pointeeType);
        return;
    }

    case ASTTagArrayType: {
        ASTArrayTypeRef type = (ASTArrayTypeRef)node;
        _ASTDumperDumpChild(dumper, type->elementType);

        if (type->size) {
            _ASTDumperDumpChild(dumper, (ASTNodeRef)type->size);
        }
        return;
    }

    case ASTTagBuiltinType: {
        ASTBuiltinTypeRef type = (ASTBuiltinTypeRef)node;

        dumper->indentation += 1;
        _ASTDumperPrintIndentation(dumper);
        _ASTDumperPrintCString(dumper, "@name = '");
        _ASTDumperPrintBuiltinTypeKind(dumper, type->kind);
        _ASTDumperPrintCString(dumper, "'\n");
        dumper->indentation -= 1;
        return;
    }

    case ASTTagFunctionType: {
        ASTFunctionTypeRef type = (ASTFunctionTypeRef)node;
        _ASTDumperDumpChildrenArray(dumper, type->parameterTypes);
        _ASTDumperDumpChild(dumper, type->resultType);
        return;
    }

    case ASTTagGenericType: {
        ASTGenericTypeRef type = (ASTGenericTypeRef)node;
        _ASTDumperDumpChild(dumper, type->baseType);
        _ASTDumperDumpChildrenArray(dumper, type->arguments);
        return;
    }

    default:
        JELLY_UNREACHABLE("Invalid tag given for ASTNode in ASTDumper!");
    }
}

static inline void _ASTDumperPrintIndentation(ASTDumperRef dumper) {
    for (Index i = 0; i < dumper->indentation; i++) {
        fprintf(dumper->target, "%s", "  ");
    }
}

static inline void _ASTDumperPrintCString(ASTDumperRef dumper, const Char *string) {
    fprintf(dumper->target, "%s", string);
}

static inline void _ASTDumperPrintProperty(ASTDumperRef dumper, const Char *key, const Char *value) {
    dumper->indentation += 1;
    _ASTDumperPrintIndentation(dumper);
    _ASTDumperPrintCString(dumper, "@");
    _ASTDumperPrintCString(dumper, key);
    _ASTDumperPrintCString(dumper, " = '");
    _ASTDumperPrintCString(dumper, value);
    _ASTDumperPrintCString(dumper, "'\n");
    dumper->indentation -= 1;
}

static inline void _ASTDumperPrintTag(ASTDumperRef dumper, ASTNodeRef node) {
    switch (node->tag) {
    case ASTTagSourceUnit:
        return _ASTDumperPrintCString(dumper, "SourceUnit");

    case ASTTagLoadDirective:
        return _ASTDumperPrintCString(dumper, "LoadDirective");

    case ASTTagImportDirective:
        return _ASTDumperPrintCString(dumper, "ImportDirective");

    case ASTTagIncludeDirective:
        return _ASTDumperPrintCString(dumper, "IncludeDirective");

    case ASTTagBlock:
        return _ASTDumperPrintCString(dumper, "Block");

    case ASTTagIfStatement:
        return _ASTDumperPrintCString(dumper, "IfStatement");

    case ASTTagLoopStatement: {
        ASTLoopStatementRef loop = (ASTLoopStatementRef)node;
        if (loop->kind == ASTLoopKindDo) {
            return _ASTDumperPrintCString(dumper, "DoStatement");
        } else if (loop->kind == ASTLoopKindWhile) {
            return _ASTDumperPrintCString(dumper, "WhileStatement");
        }
        break;
    }

    case ASTTagCaseStatement: {
        ASTCaseStatementRef statement = (ASTCaseStatementRef)node;
        if (statement->kind == ASTCaseKindConditional) {
            return _ASTDumperPrintCString(dumper, "ConditionalCaseStatement");
        } else if (statement->kind == ASTCaseKindElse) {
            return _ASTDumperPrintCString(dumper, "ElseCaseStatement");
        }
        break;
    }

    case ASTTagSwitchStatement:
        return _ASTDumperPrintCString(dumper, "SwitchStatement");

    case ASTTagControlStatement: {
        ASTControlStatementRef control = (ASTControlStatementRef)node;
        if (control->kind == ASTControlKindBreak) {
            return _ASTDumperPrintCString(dumper, "BreakStatement");
        } else if (control->kind == ASTControlKindReturn) {
            return _ASTDumperPrintCString(dumper, "ReturnStatement");
        } else if (control->kind == ASTControlKindContinue) {
            return _ASTDumperPrintCString(dumper, "ContinueStatement");
        } else if (control->kind == ASTControlKindFallthrough) {
            return _ASTDumperPrintCString(dumper, "FallthroughStatement");
        }
        break;
    }

    case ASTTagReferenceExpression:
        return _ASTDumperPrintCString(dumper, "ReferenceExpression");

    case ASTTagDereferenceExpression:
        return _ASTDumperPrintCString(dumper, "DereferenceExpression");

    case ASTTagUnaryExpression:
        return _ASTDumperPrintCString(dumper, "UnaryExpression");

    case ASTTagBinaryExpression:
        return _ASTDumperPrintCString(dumper, "BinaryExpression");

    case ASTTagIdentifierExpression:
        return _ASTDumperPrintCString(dumper, "IdentifierExpression");

    case ASTTagMemberAccessExpression:
        return _ASTDumperPrintCString(dumper, "MemberAccessExpression");

    case ASTTagAssignmentExpression:
        return _ASTDumperPrintCString(dumper, "AssignmentExpression");

    case ASTTagCallExpression:
        return _ASTDumperPrintCString(dumper, "CallExpression");

    case ASTTagConstantExpression: {
        ASTConstantExpressionRef constant = (ASTConstantExpressionRef)node;
        if (constant->kind == ASTConstantKindNil) {
            return _ASTDumperPrintCString(dumper, "NilLiteral");
        } else if (constant->kind == ASTConstantKindBool) {
            return _ASTDumperPrintCString(dumper, "BoolLiteral");
        } else if (constant->kind == ASTConstantKindInt) {
            return _ASTDumperPrintCString(dumper, "IntLiteral");
        } else if (constant->kind == ASTConstantKindFloat) {
            return _ASTDumperPrintCString(dumper, "FloatLiteral");
        } else if (constant->kind == ASTConstantKindString) {
            return _ASTDumperPrintCString(dumper, "StringLiteral");
        }
        break;
    }

    case ASTTagSubscriptExpression:
        return _ASTDumperPrintCString(dumper, "SubscriptExpression");

    case ASTTagTypeExpression:
        return _ASTDumperPrintCString(dumper, "TypeExpression");

    case ASTTagModuleDeclaration:
        return _ASTDumperPrintCString(dumper, "ModuleDeclaration");

    case ASTTagEnumerationDeclaration:
        return _ASTDumperPrintCString(dumper, "EnumerationDeclaration");

    case ASTTagForeignFunctionDeclaration:
        return _ASTDumperPrintCString(dumper, "ForeignFunctionDeclaration");

    case ASTTagFunctionDeclaration:
        return _ASTDumperPrintCString(dumper, "FunctionDeclaration");

    case ASTTagInitializerDeclaration:
        return _ASTDumperPrintCString(dumper, "InitializerDeclaration");

    case ASTTagStructureDeclaration:
        return _ASTDumperPrintCString(dumper, "StructureDeclaration");

    case ASTTagValueDeclaration: {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)node;
        if (value->kind == ASTValueKindVariable) {
            return _ASTDumperPrintCString(dumper, "VariableDeclaration");
        } else if (value->kind == ASTValueKindParameter) {
            return _ASTDumperPrintCString(dumper, "ParameterDeclaration");
        } else if (value->kind == ASTValueKindEnumerationElement) {
            return _ASTDumperPrintCString(dumper, "EnumerationElementDeclaration");
        }
        break;
    }

    case ASTTagTypeAliasDeclaration:
        return _ASTDumperPrintCString(dumper, "TypeAliasDeclaration");

    case ASTTagOpaqueType:
        return _ASTDumperPrintCString(dumper, "OpaqueType");

    case ASTTagPointerType:
        return _ASTDumperPrintCString(dumper, "PointerType");

    case ASTTagArrayType:
        return _ASTDumperPrintCString(dumper, "ArrayType");

    case ASTTagBuiltinType:
        return _ASTDumperPrintCString(dumper, "BuiltinType");

    case ASTTagFunctionType:
        return _ASTDumperPrintCString(dumper, "FunctionType");

    case ASTTagGenericType:
        return _ASTDumperPrintCString(dumper, "GenericType");

    default:
        break;
    }

    JELLY_UNREACHABLE("Invalid tag given for ASTNode in ASTDumper!");
}

static inline void _ASTDumperPrintBuiltinTypeKind(ASTDumperRef dumper, ASTBuiltinTypeKind kind) {
    switch (kind) {
    case ASTBuiltinTypeKindError:
        return _ASTDumperPrintCString(dumper, "<error>");
    case ASTBuiltinTypeKindVoid:
        return _ASTDumperPrintCString(dumper, "Void");
    case ASTBuiltinTypeKindBool:
        return _ASTDumperPrintCString(dumper, "Bool");
    case ASTBuiltinTypeKindInt8:
        return _ASTDumperPrintCString(dumper, "Int8");
    case ASTBuiltinTypeKindInt16:
        return _ASTDumperPrintCString(dumper, "Int16");
    case ASTBuiltinTypeKindInt32:
        return _ASTDumperPrintCString(dumper, "Int32");
    case ASTBuiltinTypeKindInt64:
        return _ASTDumperPrintCString(dumper, "Int64");
    case ASTBuiltinTypeKindInt:
        return _ASTDumperPrintCString(dumper, "Int");
    case ASTBuiltinTypeKindUInt8:
        return _ASTDumperPrintCString(dumper, "UInt8");
    case ASTBuiltinTypeKindUInt16:
        return _ASTDumperPrintCString(dumper, "UInt16");
    case ASTBuiltinTypeKindUInt32:
        return _ASTDumperPrintCString(dumper, "UInt32");
    case ASTBuiltinTypeKindUInt64:
        return _ASTDumperPrintCString(dumper, "UInt64");
    case ASTBuiltinTypeKindUInt:
        return _ASTDumperPrintCString(dumper, "UInt");
    case ASTBuiltinTypeKindFloat32:
        return _ASTDumperPrintCString(dumper, "Float32");
    case ASTBuiltinTypeKindFloat64:
        return _ASTDumperPrintCString(dumper, "Float64");
    case ASTBuiltinTypeKindFloat:
        return _ASTDumperPrintCString(dumper, "Float");

    default:
        break;
    }

    JELLY_UNREACHABLE("Unknown kind given for ASTBuiltinTypeKind in ASTDumper!");
}

static inline void _ASTDumperPrintPrefixOperator(ASTDumperRef dumper, ASTUnaryOperator op) {
    switch (op) {
    case ASTUnaryOperatorUnknown:
        return _ASTDumperPrintCString(dumper, "<unknown>");
    case ASTUnaryOperatorLogicalNot:
        return _ASTDumperPrintCString(dumper, "!");
    case ASTUnaryOperatorBitwiseNot:
        return _ASTDumperPrintCString(dumper, "~");
    case ASTUnaryOperatorUnaryPlus:
        return _ASTDumperPrintCString(dumper, "+");
    case ASTUnaryOperatorUnaryMinus:
        return _ASTDumperPrintCString(dumper, "-");

    default:
        break;
    }

    JELLY_UNREACHABLE("Unknown value given for ASTUnaryOperator in ASTDumper!");
}

static inline void _ASTDumperPrintInfixOperator(ASTDumperRef dumper, ASTBinaryOperator op) {
    switch (op) {
    case ASTBinaryOperatorUnknown:
        return _ASTDumperPrintCString(dumper, "<unknown>");
    case ASTBinaryOperatorBitwiseLeftShift:
        return _ASTDumperPrintCString(dumper, "<<");
    case ASTBinaryOperatorBitwiseRightShift:
        return _ASTDumperPrintCString(dumper, ">>");
    case ASTBinaryOperatorMultiply:
        return _ASTDumperPrintCString(dumper, "*");
    case ASTBinaryOperatorDivide:
        return _ASTDumperPrintCString(dumper, "/");
    case ASTBinaryOperatorReminder:
        return _ASTDumperPrintCString(dumper, "%");
    case ASTBinaryOperatorBitwiseAnd:
        return _ASTDumperPrintCString(dumper, "&");
    case ASTBinaryOperatorAdd:
        return _ASTDumperPrintCString(dumper, "+");
    case ASTBinaryOperatorSubtract:
        return _ASTDumperPrintCString(dumper, "-");
    case ASTBinaryOperatorBitwiseOr:
        return _ASTDumperPrintCString(dumper, "|");
    case ASTBinaryOperatorBitwiseXor:
        return _ASTDumperPrintCString(dumper, "^");
    case ASTBinaryOperatorTypeCheck:
        return _ASTDumperPrintCString(dumper, "is");
    case ASTBinaryOperatorTypeCast:
        return _ASTDumperPrintCString(dumper, "as");
    case ASTBinaryOperatorLessThan:
        return _ASTDumperPrintCString(dumper, "<");
    case ASTBinaryOperatorLessThanEqual:
        return _ASTDumperPrintCString(dumper, "<=");
    case ASTBinaryOperatorGreaterThan:
        return _ASTDumperPrintCString(dumper, ">");
    case ASTBinaryOperatorGreaterThanEqual:
        return _ASTDumperPrintCString(dumper, ">=");
    case ASTBinaryOperatorEqual:
        return _ASTDumperPrintCString(dumper, "==");
    case ASTBinaryOperatorNotEqual:
        return _ASTDumperPrintCString(dumper, "!=");
    case ASTBinaryOperatorLogicalAnd:
        return _ASTDumperPrintCString(dumper, "&&");
    case ASTBinaryOperatorLogicalOr:
        return _ASTDumperPrintCString(dumper, "||");
    case ASTBinaryOperatorAssign:
        return _ASTDumperPrintCString(dumper, "=");
    case ASTBinaryOperatorMultiplyAssign:
        return _ASTDumperPrintCString(dumper, "*=");
    case ASTBinaryOperatorDivideAssign:
        return _ASTDumperPrintCString(dumper, "/=");
    case ASTBinaryOperatorReminderAssign:
        return _ASTDumperPrintCString(dumper, "%=");
    case ASTBinaryOperatorAddAssign:
        return _ASTDumperPrintCString(dumper, "+=");
    case ASTBinaryOperatorSubtractAssign:
        return _ASTDumperPrintCString(dumper, "-=");
    case ASTBinaryOperatorBitwiseLeftShiftAssign:
        return _ASTDumperPrintCString(dumper, "<<=");
    case ASTBinaryOperatorBitwiseRightShiftAssign:
        return _ASTDumperPrintCString(dumper, ">>=");
    case ASTBinaryOperatorBitwiseAndAssign:
        return _ASTDumperPrintCString(dumper, "&=");
    case ASTBinaryOperatorBitwiseOrAssign:
        return _ASTDumperPrintCString(dumper, "|=");
    case ASTBinaryOperatorBitwiseXorAssign:
        return _ASTDumperPrintCString(dumper, "^=");

    default:
        break;
    }
}

static inline void _ASTDumperDumpChild(ASTDumperRef dumper, ASTNodeRef child) {
    dumper->indentation += 1;
    ASTDumperDump(dumper, child);
    dumper->indentation -= 1;
}

static inline void _ASTDumperDumpChildrenArray(ASTDumperRef dumper, ASTArrayRef array) {
    for (Index index = 0; index < ASTArrayGetElementCount(array); index++) {
        _ASTDumperDumpChild(dumper, (ASTNodeRef)ASTArrayGetElementAtIndex(array, index));
    }
}
