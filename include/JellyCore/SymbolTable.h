#ifndef __JELLY_SYMBOLTABLE__
#define __JELLY_SYMBOLTABLE__

#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>
#include <JellyCore/String.h>

JELLY_EXTERN_C_BEGIN

enum _ScopeKind {
    ScopeKindGlobal      = 1 << 0,
    ScopeKindBranch      = 1 << 1,
    ScopeKindLoop        = 1 << 2,
    ScopeKindCase        = 1 << 4,
    ScopeKindSwitch      = 1 << 5,
    ScopeKindEnumeration = 1 << 6,
    ScopeKindFunction    = 1 << 7,
    ScopeKindInitializer = 1 << 8,
    ScopeKindStructure   = 1 << 9,
    ScopeKindGenericType = 1 << 10,
};
typedef enum _ScopeKind ScopeKind;

typedef Int64 ScopeID;

static ScopeID kScopeGlobal = 0;
static ScopeID kScopeNull   = -1;

typedef Int64 SymbolID;

static SymbolID kSymbolNull = -1;

typedef struct _Symbol *SymbolRef;

typedef struct _SymbolTable *SymbolTableRef;

SymbolTableRef SymbolTableCreate(AllocatorRef allocator);

void SymbolTableDestroy(SymbolTableRef table);

ScopeID SymbolTableInsertScope(SymbolTableRef table, ScopeKind kind, ScopeID parent, const Char *location);

ScopeID SymbolTableGetScopeParent(SymbolTableRef table, ScopeID id);

ScopeID SymbolTableGetScopeOrParentOfKinds(SymbolTableRef table, ScopeID id, ScopeKind kinds);

ScopeID SymbolTableGetEnclosingScopeParent(SymbolTableRef table, ScopeID id);

ScopeID SymbolTableGetScopeOrEnclosingParentOfKinds(SymbolTableRef table, ScopeID id, ScopeKind kinds);

ScopeKind SymbolTableGetScopeKind(SymbolTableRef table, ScopeID id);

void *SymbolTableGetScopeUserdata(SymbolTableRef table, ScopeID id);

void SymbolTableSetScopeUserdata(SymbolTableRef table, ScopeID id, void *userdata);

void SymbolTableGetScopeSymbols(SymbolTableRef table, ScopeID id, SymbolID **symbols, Index *count);

Bool SymbolTableIsSymbolGroup(SymbolTableRef table, SymbolID id);

SymbolID SymbolTableInsertSymbol(SymbolTableRef table, ScopeID id, StringRef name);

SymbolID SymbolTableInsertOrGetSymbol(SymbolTableRef table, ScopeID id, StringRef name);

SymbolID SymbolTableLookupSymbol(SymbolTableRef table, ScopeID id, StringRef name);

SymbolID SymbolTableLookupSymbolInHierarchy(SymbolTableRef table, ScopeID id, StringRef name);

void *SymbolTableGetSymbolDefinition(SymbolTableRef table, SymbolID id);

void SymbolTableSetSymbolDefinition(SymbolTableRef table, SymbolID id, void *definition);

void *SymbolTableGetSymbolType(SymbolTableRef table, SymbolID id);

void SymbolTableSetSymbolType(SymbolTableRef table, SymbolID id, void *type);

SymbolID SymbolTableInsertSymbolGroup(SymbolTableRef table, ScopeID id, StringRef name);

SymbolID SymbolTableInsertOrGetSymbolGroup(SymbolTableRef table, ScopeID id, StringRef name);

Index SymbolTableGetSymbolGroupEntryCount(SymbolTableRef table, SymbolID id);

Index SymbolTableInsertSymbolGroupEntry(SymbolTableRef table, SymbolID id);

void *SymbolTableGetSymbolGroupDefinition(SymbolTableRef table, SymbolID id, Index index);

void SymbolTableSetSymbolGroupDefinition(SymbolTableRef table, SymbolID id, Index index, void *definition);

void *SymbolTableGetSymbolGroupType(SymbolTableRef table, SymbolID id, Index index);

void SymbolTableSetSymbolGroupType(SymbolTableRef table, SymbolID id, Index index, void *type);

JELLY_EXTERN_C_END

#endif
