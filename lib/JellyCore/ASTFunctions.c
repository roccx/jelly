#include "JellyCore/ASTFunctions.h"

static inline Bool _ASTOpaqueTypeIsEqual(ASTOpaqueTypeRef opaque, ASTTypeRef other);

ASTOperatorPrecedence ASTGetBinaryOperatorPrecedence(ASTBinaryOperator binary) {
    switch (binary) {
    case ASTBinaryOperatorBitwiseLeftShift:
    case ASTBinaryOperatorBitwiseRightShift:
        return 900;

    case ASTBinaryOperatorMultiply:
    case ASTBinaryOperatorDivide:
    case ASTBinaryOperatorReminder:
    case ASTBinaryOperatorBitwiseAnd:
        return 800;

    case ASTBinaryOperatorAdd:
    case ASTBinaryOperatorSubtract:
    case ASTBinaryOperatorBitwiseOr:
    case ASTBinaryOperatorBitwiseXor:
        return 700;

    case ASTBinaryOperatorTypeCheck:
    case ASTBinaryOperatorTypeCast:
    case ASTBinaryOperatorTypeBitcast:
        return 500;

    case ASTBinaryOperatorLessThan:
    case ASTBinaryOperatorLessThanEqual:
    case ASTBinaryOperatorGreaterThan:
    case ASTBinaryOperatorGreaterThanEqual:
    case ASTBinaryOperatorEqual:
    case ASTBinaryOperatorNotEqual:
        return 400;

    case ASTBinaryOperatorLogicalAnd:
        return 300;

    case ASTBinaryOperatorLogicalOr:
        return 200;

    case ASTBinaryOperatorAssign:
    case ASTBinaryOperatorMultiplyAssign:
    case ASTBinaryOperatorDivideAssign:
    case ASTBinaryOperatorReminderAssign:
    case ASTBinaryOperatorAddAssign:
    case ASTBinaryOperatorSubtractAssign:
    case ASTBinaryOperatorBitwiseLeftShiftAssign:
    case ASTBinaryOperatorBitwiseRightShiftAssign:
    case ASTBinaryOperatorBitwiseAndAssign:
    case ASTBinaryOperatorBitwiseOrAssign:
    case ASTBinaryOperatorBitwiseXorAssign:
        return 100;

    default:
        return 250;
    }
}

ASTOperatorPrecedence ASTGetPostfixOperatorPrecedence(ASTPostfixOperator op) {
    switch (op) {
    case ASTPostfixOperatorUnknown:
        return 250;

    default:
        return 1000;
    }
}

ASTOperatorPrecedence ASTGetOperatorPrecedenceBefore(ASTOperatorPrecedence precedence) {
    if (precedence > 900) {
        return 900;
    } else if (precedence > 800) {
        return 800;
    } else if (precedence > 700) {
        return 700;
    } else if (precedence > 500) {
        return 500;
    } else if (precedence > 400) {
        return 400;
    } else if (precedence > 300) {
        return 300;
    } else if (precedence > 250) {
        return 250;
    } else if (precedence > 200) {
        return 200;
    } else if (precedence > 100) {
        return 100;
    } else {
        return 0;
    }
}

ASTOperatorAssociativity ASTGetBinaryOperatorAssociativity(ASTBinaryOperator binary) {
    switch (binary) {
    case ASTBinaryOperatorBitwiseLeftShift:
    case ASTBinaryOperatorBitwiseRightShift:
        return ASTOperatorAssociativityNone;

    case ASTBinaryOperatorMultiply:
    case ASTBinaryOperatorDivide:
    case ASTBinaryOperatorReminder:
    case ASTBinaryOperatorBitwiseAnd:
    case ASTBinaryOperatorAdd:
    case ASTBinaryOperatorSubtract:
    case ASTBinaryOperatorBitwiseOr:
    case ASTBinaryOperatorBitwiseXor:
    case ASTBinaryOperatorTypeCheck:
    case ASTBinaryOperatorTypeCast:
    case ASTBinaryOperatorTypeBitcast:
        return ASTOperatorAssociativityLeft;

    case ASTBinaryOperatorLessThan:
    case ASTBinaryOperatorLessThanEqual:
    case ASTBinaryOperatorGreaterThan:
    case ASTBinaryOperatorGreaterThanEqual:
    case ASTBinaryOperatorEqual:
    case ASTBinaryOperatorNotEqual:
        return ASTOperatorAssociativityNone;

    case ASTBinaryOperatorLogicalAnd:
    case ASTBinaryOperatorLogicalOr:
        return ASTOperatorAssociativityLeft;

    case ASTBinaryOperatorAssign:
    case ASTBinaryOperatorMultiplyAssign:
    case ASTBinaryOperatorDivideAssign:
    case ASTBinaryOperatorReminderAssign:
    case ASTBinaryOperatorAddAssign:
    case ASTBinaryOperatorSubtractAssign:
    case ASTBinaryOperatorBitwiseLeftShiftAssign:
    case ASTBinaryOperatorBitwiseRightShiftAssign:
    case ASTBinaryOperatorBitwiseAndAssign:
    case ASTBinaryOperatorBitwiseOrAssign:
    case ASTBinaryOperatorBitwiseXorAssign:
        return ASTOperatorAssociativityRight;

    default:
        return ASTOperatorAssociativityNone;
    }
}

Bool ASTBinaryOperatorIsAssignment(ASTBinaryOperator binary) {
    switch (binary) {
    case ASTBinaryOperatorAssign:
    case ASTBinaryOperatorMultiplyAssign:
    case ASTBinaryOperatorDivideAssign:
    case ASTBinaryOperatorReminderAssign:
    case ASTBinaryOperatorAddAssign:
    case ASTBinaryOperatorSubtractAssign:
    case ASTBinaryOperatorBitwiseLeftShiftAssign:
    case ASTBinaryOperatorBitwiseRightShiftAssign:
    case ASTBinaryOperatorBitwiseAndAssign:
    case ASTBinaryOperatorBitwiseOrAssign:
    case ASTBinaryOperatorBitwiseXorAssign:
        return true;

    default:
        return false;
    }
}

StringRef ASTGetPrefixOperatorName(AllocatorRef allocator, ASTUnaryOperator op) {
    switch (op) {
    case ASTUnaryOperatorLogicalNot:
        return StringCreate(allocator, "!");

    case ASTUnaryOperatorBitwiseNot:
        return StringCreate(allocator, "~");

    case ASTUnaryOperatorUnaryPlus:
        return StringCreate(allocator, "+");

    case ASTUnaryOperatorUnaryMinus:
        return StringCreate(allocator, "-");

    default:
        JELLY_UNREACHABLE("Unknown value given for prefix operator!");
        break;
    }
}

StringRef ASTGetInfixOperatorName(AllocatorRef allocator, ASTBinaryOperator op) {
    switch (op) {
    case ASTBinaryOperatorBitwiseLeftShift:
        return StringCreate(allocator, "<<");
    case ASTBinaryOperatorBitwiseRightShift:
        return StringCreate(allocator, ">>");
    case ASTBinaryOperatorMultiply:
        return StringCreate(allocator, "*");
    case ASTBinaryOperatorDivide:
        return StringCreate(allocator, "/");
    case ASTBinaryOperatorReminder:
        return StringCreate(allocator, "%");
    case ASTBinaryOperatorBitwiseAnd:
        return StringCreate(allocator, "&");
    case ASTBinaryOperatorAdd:
        return StringCreate(allocator, "+");
    case ASTBinaryOperatorSubtract:
        return StringCreate(allocator, "-");
    case ASTBinaryOperatorBitwiseOr:
        return StringCreate(allocator, "|");
    case ASTBinaryOperatorBitwiseXor:
        return StringCreate(allocator, "^");
    case ASTBinaryOperatorTypeCheck:
        return StringCreate(allocator, "is");
    case ASTBinaryOperatorTypeCast:
        return StringCreate(allocator, "as");
    case ASTBinaryOperatorLessThan:
        return StringCreate(allocator, "<");
    case ASTBinaryOperatorLessThanEqual:
        return StringCreate(allocator, "<=");
    case ASTBinaryOperatorGreaterThan:
        return StringCreate(allocator, ">");
    case ASTBinaryOperatorGreaterThanEqual:
        return StringCreate(allocator, ">=");
    case ASTBinaryOperatorEqual:
        return StringCreate(allocator, "==");
    case ASTBinaryOperatorNotEqual:
        return StringCreate(allocator, "!=");
    case ASTBinaryOperatorLogicalAnd:
        return StringCreate(allocator, "&&");
    case ASTBinaryOperatorLogicalOr:
        return StringCreate(allocator, "||");
    case ASTBinaryOperatorAssign:
        return StringCreate(allocator, "=");
    case ASTBinaryOperatorMultiplyAssign:
        return StringCreate(allocator, "*=");
    case ASTBinaryOperatorDivideAssign:
        return StringCreate(allocator, "/=");
    case ASTBinaryOperatorReminderAssign:
        return StringCreate(allocator, "%=");
    case ASTBinaryOperatorAddAssign:
        return StringCreate(allocator, "+=");
    case ASTBinaryOperatorSubtractAssign:
        return StringCreate(allocator, "-=");
    case ASTBinaryOperatorBitwiseLeftShiftAssign:
        return StringCreate(allocator, "<<=");
    case ASTBinaryOperatorBitwiseRightShiftAssign:
        return StringCreate(allocator, ">>=");
    case ASTBinaryOperatorBitwiseAndAssign:
        return StringCreate(allocator, "&=");
    case ASTBinaryOperatorBitwiseOrAssign:
        return StringCreate(allocator, "|=");
    case ASTBinaryOperatorBitwiseXorAssign:
        return StringCreate(allocator, "^=");

    default:
        JELLY_UNREACHABLE("Unknown value given for infix operator!");
        break;
    }
}

Bool ASTExpressionIsEqual(ASTExpressionRef lhs, ASTExpressionRef rhs) {
    // TODO: Implementation missing!
    return false;
}

Bool ASTTypeIsEqual(ASTTypeRef lhs, ASTTypeRef rhs) {
    if (lhs->tag == ASTTagPointerType && rhs->tag == ASTTagPointerType) {
        ASTPointerTypeRef lhsPointer = (ASTPointerTypeRef)lhs;
        ASTPointerTypeRef rhsPointer = (ASTPointerTypeRef)rhs;

        return ASTTypeIsEqual(lhsPointer->pointeeType, rhsPointer->pointeeType);
    }

    if (lhs->tag == ASTTagArrayType && rhs->tag == ASTTagArrayType) {
        ASTArrayTypeRef lhsArray = (ASTArrayTypeRef)lhs;
        ASTArrayTypeRef rhsArray = (ASTArrayTypeRef)rhs;

        // TODO: Move the size check to the semantic analysis for better error messages!
        if ((lhsArray->size && !rhsArray->size) || (!lhsArray->size && rhsArray->size)) {
            return false;
        }

        if (lhsArray->size && lhsArray->size->base.tag == ASTTagConstantExpression && rhsArray->size &&
            rhsArray->size->base.tag == ASTTagConstantExpression) {
            ASTConstantExpressionRef lhsSize = (ASTConstantExpressionRef)lhsArray->size;
            ASTConstantExpressionRef rhsSize = (ASTConstantExpressionRef)rhsArray->size;

            if (lhsSize->kind == ASTConstantKindInt && rhsSize->kind == ASTConstantKindInt && lhsSize->intValue != rhsSize->intValue) {
                return false;
            }
        }

        return ASTTypeIsEqual(lhsArray->elementType, rhsArray->elementType);
    }

    if (lhs->tag == ASTTagBuiltinType && rhs->tag == ASTTagBuiltinType) {
        ASTBuiltinTypeRef lhsBuiltin = (ASTBuiltinTypeRef)lhs;
        ASTBuiltinTypeRef rhsBuiltin = (ASTBuiltinTypeRef)rhs;

        if ((lhsBuiltin->kind == ASTBuiltinTypeKindInt || lhsBuiltin->kind == ASTBuiltinTypeKindInt64) &&
            (rhsBuiltin->kind == ASTBuiltinTypeKindInt || rhsBuiltin->kind == ASTBuiltinTypeKindInt64)) {
            return true;
        }

        if ((lhsBuiltin->kind == ASTBuiltinTypeKindUInt || lhsBuiltin->kind == ASTBuiltinTypeKindUInt64) &&
            (rhsBuiltin->kind == ASTBuiltinTypeKindUInt || rhsBuiltin->kind == ASTBuiltinTypeKindUInt64)) {
            return true;
        }

        return lhsBuiltin->kind == rhsBuiltin->kind;
    }

    if (lhs->tag == ASTTagOpaqueType) {
        return _ASTOpaqueTypeIsEqual((ASTOpaqueTypeRef)lhs, rhs);
    }

    if (rhs->tag == ASTTagOpaqueType) {
        return _ASTOpaqueTypeIsEqual((ASTOpaqueTypeRef)rhs, lhs);
    }

    if (lhs->tag == ASTTagEnumerationType && rhs->tag == ASTTagEnumerationType) {
        ASTEnumerationTypeRef lhsEnumeration = (ASTEnumerationTypeRef)lhs;
        ASTEnumerationTypeRef rhsEnumeration = (ASTEnumerationTypeRef)rhs;

        assert(lhsEnumeration->declaration);
        assert(rhsEnumeration->declaration);

        return lhsEnumeration->declaration == rhsEnumeration->declaration;
    }

    if (lhs->tag == ASTTagFunctionType && rhs->tag == ASTTagFunctionType) {
        ASTFunctionTypeRef lhsFunction = (ASTFunctionTypeRef)lhs;
        ASTFunctionTypeRef rhsFunction = (ASTFunctionTypeRef)rhs;

        if (ASTArrayGetElementCount(lhsFunction->parameterTypes) != ASTArrayGetElementCount(rhsFunction->parameterTypes)) {
            return false;
        }

        ASTArrayIteratorRef lhsIterator = ASTArrayGetIterator(lhsFunction->parameterTypes);
        ASTArrayIteratorRef rhsIterator = ASTArrayGetIterator(rhsFunction->parameterTypes);
        while (lhsIterator && rhsIterator) {
            ASTTypeRef lhsType = (ASTTypeRef)ASTArrayIteratorGetElement(lhsIterator);
            ASTTypeRef rhsType = (ASTTypeRef)ASTArrayIteratorGetElement(rhsIterator);
            if (!ASTTypeIsEqual(lhsType, rhsType)) {
                return false;
            }

            lhsIterator = ASTArrayIteratorNext(lhsIterator);
            rhsIterator = ASTArrayIteratorNext(rhsIterator);
        }

        return ASTTypeIsEqual(lhsFunction->resultType, rhsFunction->resultType);
    }

    if (lhs->tag == ASTTagStructureType && rhs->tag == ASTTagStructureType) {
        ASTStructureTypeRef lhsStructure = (ASTStructureTypeRef)lhs;
        ASTStructureTypeRef rhsStructure = (ASTStructureTypeRef)rhs;

        assert(lhsStructure->declaration);
        assert(rhsStructure->declaration);

        return lhsStructure->declaration == rhsStructure->declaration;
    }

    if (lhs->tag == ASTTagGenericType && rhs->tag == ASTTagGenericType) {
        ASTGenericTypeRef lhsType = (ASTGenericTypeRef)lhs;
        ASTGenericTypeRef rhsType = (ASTGenericTypeRef)rhs;
        if (!ASTTypeIsEqual(lhsType->baseType, rhsType->baseType)) {
            return false;
        }

        ASTArrayIteratorRef lhsIterator = ASTArrayGetIterator(lhsType->arguments);
        ASTArrayIteratorRef rhsIterator = ASTArrayGetIterator(rhsType->arguments);

        while (lhsIterator && rhsIterator) {
            ASTExpressionRef lhsExpression = (ASTExpressionRef)ASTArrayIteratorGetElement(lhsIterator);
            ASTExpressionRef rhsExpression = (ASTExpressionRef)ASTArrayIteratorGetElement(rhsIterator);

            if (!ASTExpressionIsEqual(lhsExpression, rhsExpression)) {
                return false;
            }

            lhsIterator = ASTArrayIteratorNext(lhsIterator);
            rhsIterator = ASTArrayIteratorNext(rhsIterator);
        }
    }

    return false;
}

static inline Bool _ASTOpaqueTypeIsEqual(ASTOpaqueTypeRef opaque, ASTTypeRef other) {
    assert(opaque->declaration);

    if (opaque->declaration->base.tag == ASTTagTypeAliasDeclaration) {
        assert((ASTTypeRef)opaque != opaque->declaration->type);
        return ASTTypeIsEqual(opaque->declaration->type, other);
    }

    switch (other->tag) {
    case ASTTagOpaqueType: {
        ASTOpaqueTypeRef otherOpaque = (ASTOpaqueTypeRef)other;
        assert(otherOpaque->declaration);
        return opaque->declaration == otherOpaque->declaration;
    }

    case ASTTagEnumerationType: {
        ASTEnumerationTypeRef enumeration = (ASTEnumerationTypeRef)other;
        assert(enumeration->declaration);
        return opaque->declaration == (ASTDeclarationRef)enumeration->declaration;
    }

    case ASTTagFunctionType: {
        ASTFunctionTypeRef function = (ASTFunctionTypeRef)other;
        if (function->declaration) {
            return opaque->declaration == (ASTDeclarationRef)function->declaration;
        }

        return false;
    }

    case ASTTagStructureType: {
        ASTStructureTypeRef structure = (ASTStructureTypeRef)other;
        assert(structure->declaration);
        return opaque->declaration == (ASTDeclarationRef)structure->declaration;
    }

    default:
        return false;
    }
}

Bool ASTTypeIsError(ASTTypeRef type) {
    assert(type);

    if (type->tag == ASTTagBuiltinType) {
        ASTBuiltinTypeRef builtin = (ASTBuiltinTypeRef)type;
        return builtin->kind == ASTBuiltinTypeKindError;
    }

    return false;
}

Bool ASTTypeIsInteger(ASTTypeRef type) {
    if (type->tag != ASTTagBuiltinType) {
        return false;
    }

    ASTBuiltinTypeRef builtin = (ASTBuiltinTypeRef)type;
    switch (builtin->kind) {
    case ASTBuiltinTypeKindInt8:
    case ASTBuiltinTypeKindInt16:
    case ASTBuiltinTypeKindInt32:
    case ASTBuiltinTypeKindInt64:
    case ASTBuiltinTypeKindInt:
    case ASTBuiltinTypeKindUInt8:
    case ASTBuiltinTypeKindUInt16:
    case ASTBuiltinTypeKindUInt32:
    case ASTBuiltinTypeKindUInt64:
    case ASTBuiltinTypeKindUInt:
        return true;

    default:
        return false;
    }
}

Int ASTIntegerTypeGetBitwidth(ASTTypeRef type) {
    if (type->tag != ASTTagBuiltinType) {
        return -1;
    }

    ASTBuiltinTypeRef builtin = (ASTBuiltinTypeRef)type;
    switch (builtin->kind) {
    case ASTBuiltinTypeKindInt8:
    case ASTBuiltinTypeKindUInt8:
        return 8;

    case ASTBuiltinTypeKindInt16:
    case ASTBuiltinTypeKindUInt16:
        return 16;

    case ASTBuiltinTypeKindInt32:
    case ASTBuiltinTypeKindUInt32:
        return 32;

    case ASTBuiltinTypeKindInt64:
    case ASTBuiltinTypeKindInt:
    case ASTBuiltinTypeKindUInt64:
    case ASTBuiltinTypeKindUInt:
        return 64;

    default:
        return -1;
    }
}

Int ASTIntegerTypeIsSigned(ASTTypeRef type) {
    if (type->tag != ASTTagBuiltinType) {
        return -1;
    }

    ASTBuiltinTypeRef builtin = (ASTBuiltinTypeRef)type;
    switch (builtin->kind) {
    case ASTBuiltinTypeKindInt8:
    case ASTBuiltinTypeKindInt16:
    case ASTBuiltinTypeKindInt32:
    case ASTBuiltinTypeKindInt64:
    case ASTBuiltinTypeKindInt:
        return true;

    case ASTBuiltinTypeKindUInt8:
    case ASTBuiltinTypeKindUInt16:
    case ASTBuiltinTypeKindUInt32:
    case ASTBuiltinTypeKindUInt64:
    case ASTBuiltinTypeKindUInt:
        return false;

    default:
        return false;
    }
}

Bool ASTTypeIsVoid(ASTTypeRef type) {
    if (type->tag != ASTTagBuiltinType) {
        return false;
    }

    ASTBuiltinTypeRef builtin = (ASTBuiltinTypeRef)type;
    return builtin->kind == ASTBuiltinTypeKindVoid;
}

Bool ASTTypeIsFloatingPoint(ASTTypeRef type) {
    if (type->tag != ASTTagBuiltinType) {
        return false;
    }

    ASTBuiltinTypeRef builtin = (ASTBuiltinTypeRef)type;
    switch (builtin->kind) {
    case ASTBuiltinTypeKindFloat32:
    case ASTBuiltinTypeKindFloat64:
    case ASTBuiltinTypeKindFloat:
        return true;

    default:
        return false;
    }
}

Int ASTFloatingPointTypeGetBitwidth(ASTTypeRef type) {
    if (type->tag != ASTTagBuiltinType) {
        return -1;
    }

    ASTBuiltinTypeRef builtin = (ASTBuiltinTypeRef)type;
    switch (builtin->kind) {
    case ASTBuiltinTypeKindFloat32:
        return 32;

    case ASTBuiltinTypeKindFloat64:
    case ASTBuiltinTypeKindFloat:
        return 64;

    default:
        return -1;
    }
}

Bool ASTTypeIsLosslessConvertible(ASTTypeRef type, ASTTypeRef targetType) {
    if (!ASTTypeIsInteger(type) || !ASTTypeIsInteger(targetType)) {
        return false;
    }

    Int lhsBitwidth = ASTIntegerTypeGetBitwidth(type);
    Bool lhsSigned  = ASTIntegerTypeIsSigned(type);
    Int rhsBitwidth = ASTIntegerTypeGetBitwidth(targetType);
    Bool rhsSigned  = ASTIntegerTypeIsSigned(targetType);

    if (!lhsSigned) {
        if (rhsSigned) {
            return lhsBitwidth < rhsBitwidth;
        }

        if (!rhsSigned) {
            return lhsBitwidth <= rhsBitwidth;
        }
    } else if (rhsSigned) {
        return lhsBitwidth <= rhsBitwidth;
    }

    return false;
}

Bool ASTTypeIsImplicitlyConvertible(ASTTypeRef type, ASTTypeRef targetType) {
    if (ASTTypeIsLosslessConvertible(type, targetType)) {
        return true;
    }

    if (ASTTypeIsInteger(type) && ASTTypeIsFloatingPoint(targetType)) {
        return true;
    }

    if (ASTTypeIsFloatingPoint(type) && ASTTypeIsFloatingPoint(targetType)) {
        return true;
    }

    return false;
}
