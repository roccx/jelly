#include "JellyCore/Array.h"
#include "JellyCore/SymbolTable.h"

struct _Symbol {
    StringRef name;
    SourceRange location;
    ASTNodeRef node;
};
typedef struct _Symbol Symbol;

struct _Scope {
    ScopeKind kind;
    ScopeRef parent;
    SourceRange location;
    ArrayRef children;
    ArrayRef symbols;
};

struct _SymbolTable {
    AllocatorRef allocator;
    ArrayRef scopes;
    ScopeRef currentScope;
};

ScopeRef _SymbolTableCreateScope(SymbolTableRef symbolTable, ScopeKind kind, ScopeRef parent);

Index _ScopeGetVirtualEnd(ScopeRef scope, const Char *virtualEndOfScope);

Bool _ArrayIsSymbolLocationOrderedAscending(const void *lhs, const void *rhs);

SymbolTableRef SymbolTableCreate(AllocatorRef allocator) {
    SymbolTableRef symbolTable = AllocatorAllocate(allocator, sizeof(struct _SymbolTable));
    assert(symbolTable);
    symbolTable->allocator = allocator;
    // TODO: @Bug Reallocation of Array causes dangling pointers of Scope(s)...
    symbolTable->scopes = ArrayCreateEmpty(allocator, sizeof(struct _Scope), 1024);
    assert(symbolTable->scopes);
    symbolTable->currentScope = _SymbolTableCreateScope(symbolTable, ScopeKindGlobal, NULL);
    return symbolTable;
}

void SymbolTableDestroy(SymbolTableRef symbolTable) {
    for (Index index = 0; index < ArrayGetElementCount(symbolTable->scopes); index++) {
        ScopeRef scope = ArrayGetElementAtIndex(symbolTable->scopes, index);
        ArrayDestroy(scope->symbols);
    }

    ArrayDestroy(symbolTable->scopes);
    AllocatorDeallocate(symbolTable->allocator, symbolTable);
}

ScopeRef SymbolTableGetGlobalScope(SymbolTableRef symbolTable) {
    return (ScopeRef)ArrayGetElementAtIndex(symbolTable->scopes, 0);
}

ScopeRef SymbolTableGetCurrentScope(SymbolTableRef symbolTable) {
    return symbolTable->currentScope;
}

ScopeRef SymbolTablePushScope(SymbolTableRef symbolTable, ScopeKind scopeKind) {
    ScopeRef currentScope = _SymbolTableCreateScope(symbolTable, scopeKind, symbolTable->currentScope);
    assert(currentScope);
    symbolTable->currentScope = currentScope;
    return symbolTable->currentScope;
}

ScopeRef SymbolTablePopScope(SymbolTableRef symbolTable) {
    assert(symbolTable->currentScope->parent);
    symbolTable->currentScope = symbolTable->currentScope->parent;
    return symbolTable->currentScope;
}

ScopeKind ScopeGetKind(ScopeRef scope) {
    return scope->kind;
}

ScopeRef ScopeGetParent(ScopeRef scope) {
    return scope->parent;
}

Index ScopeGetChildCount(ScopeRef scope) {
    return ArrayGetElementCount(scope->children);
}

ScopeRef ScopeGetChildAtIndex(ScopeRef scope, Index index) {
    return *((ScopeRef *)ArrayGetElementAtIndex(scope->children, index));
}

SymbolRef ScopeInsertSymbol(ScopeRef scope, StringRef name, SourceRange location) {
    if (ScopeLookupSymbol(scope, name, NULL) != NULL) {
        return NULL;
    }

    Symbol symbol;
    symbol.name     = name;
    symbol.location = location;
    symbol.node     = NULL;

    Index index = ArrayGetSortedInsertionIndex(scope->symbols, &_ArrayIsSymbolLocationOrderedAscending, &symbol);
    ArrayInsertElementAtIndex(scope->symbols, index, &symbol);

    if (scope->location.start == NULL) {
        scope->location = location;
    } else {
        scope->location.start = MIN(scope->location.start, location.start);
        scope->location.end   = MAX(scope->location.end, location.end);
    }

    return (SymbolRef)ArrayGetElementAtIndex(scope->symbols, index);
}

SymbolRef ScopeLookupSymbol(ScopeRef scope, StringRef name, const Char *virtualEndOfScope) {
    Index end = _ScopeGetVirtualEnd(scope, virtualEndOfScope);
    for (Index index = 0; index < end; index++) {
        SymbolRef symbol = ArrayGetElementAtIndex(scope->symbols, index);
        if (StringIsEqual(symbol->name, name)) {
            return symbol;
        }
    }

    return NULL;
}

StringRef SymbolGetName(SymbolRef symbol) {
    return symbol->name;
}

ASTNodeRef SymbolGetNode(SymbolRef symbol) {
    return symbol->node;
}

void SymbolSetNode(SymbolRef symbol, ASTNodeRef node) {
    symbol->node = node;
}

ScopeRef _SymbolTableCreateScope(SymbolTableRef symbolTable, ScopeKind kind, ScopeRef parent) {
    ScopeRef scope  = ArrayAppendUninitializedElement(symbolTable->scopes);
    scope->kind     = kind;
    scope->parent   = parent;
    scope->location = SourceRangeMake(NULL, NULL);
    // TODO: @Bug Reallocation of Array causes dangling pointers of Scope(s)...
    scope->children = ArrayCreateEmpty(symbolTable->allocator, sizeof(ScopeRef), 1024);
    scope->symbols  = ArrayCreateEmpty(symbolTable->allocator, sizeof(struct _Symbol), 1024);

    if (parent) {
        ArrayAppendElement(parent->children, &scope);
    }

    return scope;
}

Index _ScopeGetVirtualEnd(ScopeRef scope, const Char *virtualEndOfScope) {
    if (scope->kind == ScopeKindGlobal || virtualEndOfScope == NULL) {
        return ArrayGetElementCount(scope->symbols);
    }

    Index count = ArrayGetElementCount(scope->symbols);
    if (count > 0) {
        for (Index index = count; index > 0; index--) {
            SymbolRef symbol = ArrayGetElementAtIndex(scope->symbols, index - 1);
            if (symbol->location.start < virtualEndOfScope) {
                return index;
            }
        }
    }

    return 0;
}

Bool _ArrayIsSymbolLocationOrderedAscending(const void *lhs, const void *rhs) {
    SymbolRef a = (SymbolRef)lhs;
    SymbolRef b = (SymbolRef)rhs;

    if (a->location.start == b->location.start) {
        return a->location.end < b->location.end;
    }

    return a->location.start < b->location.start;
}
