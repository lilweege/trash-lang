#pragma once

// Language Grammar
// TODO: bitwise operators, pointers

// Program          =  { Procedure }
// Procedure        =  "proc" Identifier "(" [ VarDefn { "," VarDefn } ] ")" [ "->" Type ] Block
// Block            =  "{" { Statement } "}"

// Statement        =  BlockStatement | LineStatement
// BlockStmt        =  IfStmt | ForStmt
// LineStmt         =  Definition | Assignment | Expression

// IfStmt           =  "if" "(" Expression ")" ( Block | Statement ) [ ElseStmt ]
// ElseStmt         =  "else" "(" Expression ")" ( Block | Statement )
// ForSmtm          =  "for" "(" LineStmt ";" LineStmt ";" LineStmt ")" ( Block | Statement )

// Expression       =  LogicalTerm { "||" LogicalTerm }
// LogicalTerm      =  LogicalFactor { "&&" LogicalFactor }
// LogicalFactor    =  Comparison { ( "==" | "!=" ) Comparison }
// Comparison       =  Value { ( ">=" | "<=" | ">" | "<" ) Value }
// Value            =  Term { ( "+" | "-" ) Term }
// Term             =  Factor { ( "*" | "/" | "%" ) Factor }
// Factor           =  IntegerLiteral | FloatLiteral | StringLiteral | CharacterLiteral | ProcedureCall | LValue | "(" Expression ")" | ( "-" Factor ) | ( "!" Factor )
// ProcedureCall    =  Identifier "(" [ Expression { "," Expression } ] ")"

// VarDefn          =  ( "let" | "mut" ) Type Identifier
// Type             =  Primitive [ Subscript ]
// LValue           =  Identifier [ Subscript ]
// Subscript        =  "[" Expression "]"
// Primitive        =  "u8" | "i64" | "f64"

// Technically not EBNF...
// LineComment      =  ? ...
// IntegerLiteral   =  [0-9]+
// FloatLiteral     =  IntegerLiteral . IntegerLiteral
// StringLiteral    =  " { AsciiOrEscapeChar } "
// CharacterLiteral =  ' AsciiOrEscapeChar '

