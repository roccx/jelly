# ![jelly-logo](jelly-logo.png)

This project was created in my spare time based on my interest in compilers and on their functionality.

The long term aim is to acquire knowledge about compiler programming and semantic analysis.

Jelly could eventually reach a feature complete release in the future however there are no active plans to finish this project 
as long as the grammar and the feature set is not finalized the project will stay *Work in Progress*.

## Introduction

*Not specified yet*

## Supported Platforms

Currently only macOS 10.14 is tested and compiling and running tests. 

Linux and Windows support is planned after releasing a feature complete alpha version.

## Mac OS Requirements

Tested with macOS 10.14+

- CMake 3.11.1
- LLVM (tested with version 8.0)
- googletest (submodule)
- gcovr 4.1

## Summary of the grammar

*Reading the grammar*
```
- Production  = ':='
- Alternation = '|'
- Option      = '[]'
- Repetition  = '{}'
```

For further details see sample snippets in test directory.

```
identifier      := identifier-head { identifier-tail }
identifier-head := "a" ... "z" | "A" ... "Z" | "_"
identifier-tail := identifier-head | "0" ... "9"

directive         := load-directive | link-directive | import-directive | include-directive
load-directive    := "#load" string-literal
link-directive    := "#link" string-literal
import-directive  := "#import" string-literal
include-directive := "#include" string-literal

block := '{' { statement } '}'

if-statement := "if" expression { "," expression } block [ "else" ( if-statement | block ) ]

loop-statement  := while-statement | do-statement
while-statement := "while" expression { "," expression } block
do-statement    := "do" block "while" expression

case-statement             := conditional-case-statement | else-case-statement
conditional-case-statement := "case" expression ":" statement { line-break statement }
else-case-statement        := "else" ":" statement { line-break statement }

switch-statement := "switch" expression "{" [ case-statement { line-break case-statement } ] "}"

control-statement      := break-statement | continue-statement | fallthrough-statement | return-statement
break-statement        := "break"
continue-statement     := "continue"
fallthrough-statement  := "fallthrough"
return-statement       := "return" [ expression ]

statement := variable-declaration | control-statement | loop-statement | if-statement | switch-statement | expression

atom-expression       := group-expression | literal-expression | identifier-expression | sizeof-expression
group-expression      := "(" expression ")"
literal-expression    := literal
identifier-expression := identifier
sizeof-expression      := "sizeof" "(" type ")"

primary-expression     := unary-expression | atom-expression | reference-expression | dereference-expression
reference-expression   := '&' expression
dereference-expression := '*' expression

unary-expression := prefix-operator expression
prefix-operator  := '!' | '~' | '+' | '-'

expression        := binary-expression | primary-expression
binary-expression := primary-expression infix-operator expression

call-expression := expression "(" [ expression { "," expression } ] ")"

constant-expression := nil-literal | bool-literal | numeric-literal | string-literal
nil-literal         := "nil"
bool-literal        := "true" | "false"

module-declaration := "module" identifier "{" [ { load-directive | link-directive } ] "}"

enum-declaration := "enum" identifier "{" [ enum-element { line-break enum-element } ] "}"

func-declaration := "func" identifier [ "<" opaque-type { "," opaque-type } ">" ] "(" [ parameter-declaration { "," parameter-declaration } ] ")" "->" type-identifier block

foreign-func-declaration := "prefix" "func" identifier "(" [ parameter { "," parameter } ] ")" "->" type-identifier

prefix-func-declaration := "prefix" "func" unary-operator [ "<" opaque-type { "," opaque-type } ">" ] "(" [ parameter { "," parameter } ] ")" "->" type-identifier block

infix-func-declaration := "infix" "func" binary-operator [ "<" opaque-type { "," opaque-type } ">" ] "(" [ parameter { "," parameter } ] ")" "->" type-identifier block

struct-declaration := "struct" identifier "{" { value-declaration | initializer-declaration } "}"

initializer-declaration := "init" "(" [ parameter-declaration { "," parameter-declaration } ] ")" block

variable-declaration := "var" identifier ":" type-identifier [ "=" expression ]

type-alias := "typealias" identifier = type

enumeration-element-declaration := "case" identifier [ "=" expression ]

parameter-declaration := identifier ":" type-identifier

type                     := builtin-type | opaque-type | pointer-type | array-type | function-pointer-type | generic-type
builtin-type             := "Void" | "Bool" |
                            "Int8" | "Int16" | "Int32" | "Int64" | "Int" |
                            "UInt8" | "UInt16" | "UInt32" | "UInt64" | "UInt" |
                            "Float32" | "Float64" | "Float"
opaque-type              := identifier
pointer-type             := type "*"
array-type               := type "[" [ expression ] "]"
function-pointer-type    := "(" [ type { "," type } ] ")" "->" type
generic-type             := type "<" expression { "," expression } ">"

top-level-node := directive | enum-declaration | func-declaration | struct-declaration | variable-declaration
top-level-interface-node := load-directive | link-directive | enum-declaration | foreing-func-declaration | struct-declaration | variable-declaration | type-alias
```
