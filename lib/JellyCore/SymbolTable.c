#include "JellyCore/Array.h"
#include "JellyCore/Dictionary.h"
#include "JellyCore/SymbolTable.h"

const Index kDefaultSymbolDictionaryCapacity = 64;
const Index kDefaultSymbolArrayCapacity      = 8;

struct _SymbolEntry {
    void *definition;
    void *type;
};
typedef struct _SymbolEntry SymbolEntry;

struct _Symbol {
    SymbolID id;
    Bool isGroup;
    union {
        SymbolEntry entry;
        ArrayRef entries;
    };
};

struct _Scope {
    ScopeKind kind;
    ScopeID id;
    ScopeID parent;
    const Char *location;
    DictionaryRef symbols;
    void *userdata;
};
typedef struct _Scope *ScopeRef;

struct _SymbolTable {
    AllocatorRef allocator;
    ScopeID nextScopeID;
    SymbolID nextSymbolID;
    ArrayRef scopes;
    ArrayRef symbols;
};

SymbolTableRef SymbolTableCreate(AllocatorRef allocator) {
    SymbolTableRef table = (SymbolTableRef)AllocatorAllocate(allocator, sizeof(struct _SymbolTable));
    table->allocator     = allocator;
    table->nextScopeID   = kScopeGlobal + 1;
    table->scopes        = ArrayCreateEmpty(table->allocator, sizeof(struct _Scope), 8);
    table->nextSymbolID  = 0;
    table->symbols       = ArrayCreateEmpty(table->allocator, sizeof(struct _Symbol), kDefaultSymbolArrayCapacity);

    ScopeRef globalScope  = ArrayAppendUninitializedElement(table->scopes);
    globalScope->kind     = ScopeKindGlobal;
    globalScope->id       = kScopeGlobal;
    globalScope->parent   = kScopeNull;
    globalScope->location = NULL;
    globalScope->symbols  = CStringDictionaryCreate(table->allocator, kDefaultSymbolDictionaryCapacity);
    globalScope->userdata = NULL;

    return table;
}

void SymbolTableDestroy(SymbolTableRef table) {
    for (Index index = 0; index < ArrayGetElementCount(table->symbols); index++) {
        SymbolRef symbol = (SymbolRef)ArrayGetElementAtIndex(table->symbols, index);
        if (symbol->isGroup) {
            ArrayDestroy(symbol->entries);
        }
    }

    for (Index index = 0; index < ArrayGetElementCount(table->scopes); index++) {
        ScopeRef scope = (ScopeRef)ArrayGetElementAtIndex(table->scopes, index);
        DictionaryDestroy(scope->symbols);
    }

    ArrayDestroy(table->scopes);
    ArrayDestroy(table->symbols);
    AllocatorDeallocate(table->allocator, table);
}

ScopeID SymbolTableInsertScope(SymbolTableRef table, ScopeKind kind, ScopeID parent, const Char *location) {
    ScopeRef scope  = ArrayAppendUninitializedElement(table->scopes);
    scope->kind     = kind;
    scope->id       = table->nextScopeID;
    scope->parent   = parent;
    scope->location = location;
    scope->symbols  = CStringDictionaryCreate(table->allocator, kDefaultSymbolDictionaryCapacity);
    scope->userdata = NULL;
    table->nextScopeID += 1;
    return scope->id;
}

ScopeID SymbolTableGetScopeParent(SymbolTableRef table, ScopeID id) {
    if (id == kScopeNull) {
        return kScopeNull;
    }

    ScopeRef scope = (ScopeRef)ArrayGetElementAtIndex(table->scopes, id);
    return scope->parent;
}

ScopeID SymbolTableGetScopeOrParentOfKinds(SymbolTableRef table, ScopeID id, ScopeKind kinds) {
    if (id == kScopeNull) {
        return kScopeNull;
    }

    assert(0 <= id && id < ArrayGetElementCount(table->scopes));
    ScopeRef scope = (ScopeRef)ArrayGetElementAtIndex(table->scopes, id);
    if ((scope->kind & kinds) > 0) {
        return id;
    }

    return SymbolTableGetScopeOrParentOfKinds(table, scope->parent, kinds);
}

ScopeID SymbolTableGetEnclosingScopeParent(SymbolTableRef table, ScopeID id) {
    assert(0 <= id && id < ArrayGetElementCount(table->scopes));

    ScopeRef scope  = (ScopeRef)ArrayGetElementAtIndex(table->scopes, id);
    ScopeRef parent = NULL;
    if (scope->parent != kScopeNull) {
        parent = (ScopeRef)ArrayGetElementAtIndex(table->scopes, scope->parent);
    }

    while (parent) {
        switch (scope->kind) {
        case ScopeKindGlobal:
        case ScopeKindBranch:
        case ScopeKindLoop:
        case ScopeKindCase:
        case ScopeKindSwitch:
            return parent->id;

        case ScopeKindEnumeration:
        case ScopeKindFunction:
        case ScopeKindStructure:
        case ScopeKindGenericType:
            if (parent->kind == ScopeKindGlobal) {
                return parent->id;
            }
            break;

        case ScopeKindInitializer:
            if (parent->kind == ScopeKindGlobal || parent->kind == ScopeKindStructure) {
                return parent->id;
            }
            break;
        }

        if (parent->parent != kScopeNull) {
            parent = (ScopeRef)ArrayGetElementAtIndex(table->scopes, parent->parent);
        } else {
            parent = NULL;
        }
    }

    return scope->parent;
}

ScopeID SymbolTableGetScopeOrEnclosingParentOfKinds(SymbolTableRef table, ScopeID id, ScopeKind kinds) {
    assert(0 <= id && id < ArrayGetElementCount(table->scopes));

    ScopeID parentID = id;
    while (parentID != kScopeNull) {
        ScopeRef parent = (ScopeRef)ArrayGetElementAtIndex(table->scopes, parentID);
        if ((parent->kind & kinds) > 0) {
            return parentID;
        }

        parentID = SymbolTableGetEnclosingScopeParent(table, parentID);
    }

    return kScopeNull;
}

ScopeKind SymbolTableGetScopeKind(SymbolTableRef table, ScopeID id) {
    assert(0 <= id && id < ArrayGetElementCount(table->scopes));

    ScopeRef scope = (ScopeRef)ArrayGetElementAtIndex(table->scopes, id);
    return scope->kind;
}

void *SymbolTableGetScopeUserdata(SymbolTableRef table, ScopeID id) {
    assert(0 <= id && id < ArrayGetElementCount(table->scopes));

    ScopeRef scope = (ScopeRef)ArrayGetElementAtIndex(table->scopes, id);
    return scope->userdata;
}

void SymbolTableSetScopeUserdata(SymbolTableRef table, ScopeID id, void *userdata) {
    assert(0 <= id && id < ArrayGetElementCount(table->scopes));

    ScopeRef scope  = (ScopeRef)ArrayGetElementAtIndex(table->scopes, id);
    scope->userdata = userdata;
}

void SymbolTableGetScopeSymbols(SymbolTableRef table, ScopeID id, SymbolID **symbols, Index *count) {
    assert(0 <= id && id < ArrayGetElementCount(table->scopes));

    ScopeRef scope = (ScopeRef)ArrayGetElementAtIndex(table->scopes, id);

    void *buffer;
    Index length;
    DictionaryGetValueBuffer(scope->symbols, &buffer, &length);

    *symbols = (SymbolID *)buffer;
    *count   = length / sizeof(SymbolID);
}

Bool SymbolTableIsSymbolGroup(SymbolTableRef table, SymbolID id) {
    assert(0 <= id && id < ArrayGetElementCount(table->symbols));

    SymbolRef symbol = (SymbolRef)ArrayGetElementAtIndex(table->symbols, id);
    return symbol->isGroup;
}

SymbolID SymbolTableInsertSymbol(SymbolTableRef table, ScopeID id, StringRef name) {
    assert(SymbolTableLookupSymbol(table, id, name) == kSymbolNull);

    SymbolRef symbol = ArrayAppendUninitializedElement(table->symbols);
    memset(symbol, 0, sizeof(struct _Symbol));
    symbol->id = table->nextSymbolID;
    table->nextSymbolID += 1;

    assert(0 <= id && id < ArrayGetElementCount(table->scopes));
    ScopeRef scope = ArrayGetElementAtIndex(table->scopes, id);
    DictionaryInsert(scope->symbols, StringGetCharacters(name), &symbol->id, sizeof(SymbolID));

    return symbol->id;
}

SymbolID SymbolTableInsertOrGetSymbol(SymbolTableRef table, ScopeID id, StringRef name) {
    SymbolID symbolID = SymbolTableLookupSymbol(table, id, name);
    if (symbolID != kSymbolNull) {
        return symbolID;
    }

    return SymbolTableInsertSymbol(table, id, name);
}

SymbolID SymbolTableLookupSymbol(SymbolTableRef table, ScopeID id, StringRef name) {
    assert(0 <= id && id < ArrayGetElementCount(table->scopes));
    ScopeRef scope = ArrayGetElementAtIndex(table->scopes, id);

    const void *ref = DictionaryLookup(scope->symbols, StringGetCharacters(name));
    if (ref == NULL) {
        return kSymbolNull;
    }

    return *((SymbolID *)ref);
}

SymbolID SymbolTableLookupSymbolInHierarchy(SymbolTableRef table, ScopeID id, StringRef name) {
    assert(0 <= id && id < ArrayGetElementCount(table->scopes));

    SymbolID nextID = id;
    while (nextID != kScopeNull) {
        SymbolID symbol = SymbolTableLookupSymbol(table, nextID, name);
        if (symbol != kSymbolNull) {
            return symbol;
        } else {
            nextID = SymbolTableGetEnclosingScopeParent(table, nextID);
        }
    }

    return kSymbolNull;
}

void *SymbolTableGetSymbolDefinition(SymbolTableRef table, SymbolID id) {
    assert(0 <= id && id < ArrayGetElementCount(table->symbols));

    SymbolRef symbol = (SymbolRef)ArrayGetElementAtIndex(table->symbols, id);
    assert(!symbol->isGroup);
    return symbol->entry.definition;
}

void SymbolTableSetSymbolDefinition(SymbolTableRef table, SymbolID id, void *definition) {
    assert(0 <= id && id < ArrayGetElementCount(table->symbols));

    SymbolRef symbol = (SymbolRef)ArrayGetElementAtIndex(table->symbols, id);
    assert(!symbol->isGroup);
    symbol->entry.definition = definition;
}

void *SymbolTableGetSymbolType(SymbolTableRef table, SymbolID id) {
    assert(0 <= id && id < ArrayGetElementCount(table->symbols));

    SymbolRef symbol = (SymbolRef)ArrayGetElementAtIndex(table->symbols, id);
    assert(!symbol->isGroup);
    return symbol->entry.type;
}

void SymbolTableSetSymbolType(SymbolTableRef table, SymbolID id, void *type) {
    assert(0 <= id && id < ArrayGetElementCount(table->symbols));

    SymbolRef symbol = (SymbolRef)ArrayGetElementAtIndex(table->symbols, id);
    assert(!symbol->isGroup);
    symbol->entry.type = type;
}

SymbolID SymbolTableInsertSymbolGroup(SymbolTableRef table, ScopeID id, StringRef name) {
    SymbolID symbolID = SymbolTableInsertSymbol(table, id, name);
    assert(symbolID != kSymbolNull);

    SymbolRef symbol = (SymbolRef)ArrayGetElementAtIndex(table->symbols, symbolID);
    symbol->isGroup  = true;
    symbol->entries  = ArrayCreateEmpty(table->allocator, sizeof(SymbolEntry), 8);
    return symbol->id;
}

SymbolID SymbolTableInsertOrGetSymbolGroup(SymbolTableRef table, ScopeID id, StringRef name) {
    SymbolID symbolID = SymbolTableLookupSymbol(table, id, name);
    if (symbolID != kSymbolNull) {
        SymbolRef symbol = ArrayGetElementAtIndex(table->symbols, symbolID);
        assert(symbol->isGroup);
        return symbol->id;
    }

    return SymbolTableInsertSymbolGroup(table, id, name);
}

Index SymbolTableGetSymbolGroupEntryCount(SymbolTableRef table, SymbolID id) {
    assert(0 <= id && id < ArrayGetElementCount(table->symbols));

    SymbolRef symbol = (SymbolRef)ArrayGetElementAtIndex(table->symbols, id);
    assert(symbol->isGroup);
    return ArrayGetElementCount(symbol->entries);
}

Index SymbolTableInsertSymbolGroupEntry(SymbolTableRef table, SymbolID id) {
    assert(0 <= id && id < ArrayGetElementCount(table->symbols));

    SymbolRef symbol = (SymbolRef)ArrayGetElementAtIndex(table->symbols, id);
    assert(symbol->isGroup);
    SymbolEntry *entry = ArrayAppendUninitializedElement(symbol->entries);
    memset(entry, 0, sizeof(SymbolEntry));
    return ArrayGetElementCount(symbol->entries) - 1;
}

void *SymbolTableGetSymbolGroupDefinition(SymbolTableRef table, SymbolID id, Index index) {
    assert(0 <= id && id < ArrayGetElementCount(table->symbols));

    SymbolRef symbol = (SymbolRef)ArrayGetElementAtIndex(table->symbols, id);
    assert(symbol->isGroup);
    assert(0 <= index && index < ArrayGetElementCount(symbol->entries));
    SymbolEntry *entry = (SymbolEntry *)ArrayGetElementAtIndex(symbol->entries, index);
    return entry->definition;
}

void SymbolTableSetSymbolGroupDefinition(SymbolTableRef table, SymbolID id, Index index, void *definition) {
    assert(0 <= id && id < ArrayGetElementCount(table->symbols));

    SymbolRef symbol = (SymbolRef)ArrayGetElementAtIndex(table->symbols, id);
    assert(symbol->isGroup);
    assert(0 <= index && index < ArrayGetElementCount(symbol->entries));
    SymbolEntry *entry = (SymbolEntry *)ArrayGetElementAtIndex(symbol->entries, index);
    entry->definition  = definition;
}

void *SymbolTableGetSymbolGroupType(SymbolTableRef table, SymbolID id, Index index) {
    assert(0 <= id && id < ArrayGetElementCount(table->symbols));

    SymbolRef symbol = (SymbolRef)ArrayGetElementAtIndex(table->symbols, id);
    assert(symbol->isGroup);
    assert(0 <= index && index < ArrayGetElementCount(symbol->entries));
    SymbolEntry *entry = (SymbolEntry *)ArrayGetElementAtIndex(symbol->entries, index);
    return entry->type;
}

void SymbolTableSetSymbolGroupType(SymbolTableRef table, SymbolID id, Index index, void *type) {
    assert(0 <= id && id < ArrayGetElementCount(table->symbols));

    SymbolRef symbol = (SymbolRef)ArrayGetElementAtIndex(table->symbols, id);
    assert(symbol->isGroup);
    assert(0 <= index && index < ArrayGetElementCount(symbol->entries));
    SymbolEntry *entry = (SymbolEntry *)ArrayGetElementAtIndex(symbol->entries, index);
    entry->type        = type;
}
