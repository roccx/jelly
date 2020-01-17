#ifndef __JELLY_ASTFUNCTIONS__
#define __JELLY_ASTFUNCTIONS__

#include <JellyCore/ASTContext.h>
#include <JellyCore/ASTNodes.h>

JELLY_EXTERN_C_BEGIN

ASTOperatorPrecedence ASTGetBinaryOperatorPrecedence(ASTBinaryOperator binary);

ASTOperatorPrecedence ASTGetPostfixOperatorPrecedence(ASTPostfixOperator op);

ASTOperatorPrecedence ASTGetOperatorPrecedenceBefore(ASTOperatorPrecedence precedence);

ASTOperatorAssociativity ASTGetBinaryOperatorAssociativity(ASTBinaryOperator binary);

Bool ASTBinaryOperatorIsAssignment(ASTBinaryOperator binary);

StringRef ASTGetPrefixOperatorName(AllocatorRef allocator, ASTUnaryOperator op);

StringRef ASTGetInfixOperatorName(AllocatorRef allocator, ASTBinaryOperator op);

Bool ASTExpressionIsEqual(ASTExpressionRef lhs, ASTExpressionRef rhs);

Bool ASTTypeIsEqual(ASTTypeRef lhs, ASTTypeRef rhs);

Bool ASTTypeIsError(ASTTypeRef type);

Bool ASTTypeIsInteger(ASTTypeRef type);

Int ASTIntegerTypeGetBitwidth(ASTTypeRef type);
Int ASTIntegerTypeIsSigned(ASTTypeRef type);

Bool ASTTypeIsVoid(ASTTypeRef type);

Bool ASTTypeIsFloatingPoint(ASTTypeRef type);

Int ASTFloatingPointTypeGetBitwidth(ASTTypeRef type);

Bool ASTTypeIsLosslessConvertible(ASTTypeRef type, ASTTypeRef targetType);
Bool ASTTypeIsImplicitlyConvertible(ASTTypeRef type, ASTTypeRef targetType);

JELLY_EXTERN_C_END

#endif
