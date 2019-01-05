//
// MIT License
//
// Copyright (c) 2018 Murat Yilmaz
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

#include "Core/AST.h"
#include "Core/ASTContext.h"

void* ASTNode::operator new (size_t size, ASTContext* context) {
    auto memory = context->nodeAllocator.Allocate(size, 8);
    context->nodes.push_back(reinterpret_cast<ASTNode*>(memory));
    return memory;
}

bool ASTNode::isDecl() const {
    return
    kind == AST_LOAD || 
    kind == AST_PARAMETER ||
    kind == AST_FUNC ||
    kind == AST_VAR ||
    kind == AST_LET ||
    kind == AST_STRUCT ||
    kind == AST_ENUM_ELEMENT ||
    kind == AST_ENUM;
}

ASTBlock* ASTNode::getParentBlock() {
    auto current = parent;
    while (current) {
        if (current->kind == AST_BLOCK) {
            return reinterpret_cast<ASTBlock*>(current);
        }

        current = current->parent;
    }

    return nullptr;
}
