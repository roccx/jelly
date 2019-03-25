//
// MIT License
//
// Copyright (c) 2019 Murat Yilmaz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "AST/ParameterDeclaration.h"
#include "AST/TypeRef.h"
#include "AST/Visitor.h"

using namespace jelly::AST;

ParameterDeclaration::ParameterDeclaration(Identifier name, TypeRef* typeRef) :
NamedDeclaration(Kind::Parameter, name),
typeRef(nullptr) {
    setTypeRef(typeRef);
}

TypeRef* ParameterDeclaration::getTypeRef() const {
    return typeRef;
}

void ParameterDeclaration::setTypeRef(TypeRef* typeRef) {
    if (typeRef) {
        typeRef->setParent(this);
    }

    if (this->typeRef) {
        this->typeRef->setParent(nullptr);
    }

    this->typeRef = typeRef;
}

void ParameterDeclaration::accept(Visitor &visitor) {
    visitor.visitParameterDeclaration(this);

    if (getTypeRef()) {
        getTypeRef()->accept(visitor);
    }
}