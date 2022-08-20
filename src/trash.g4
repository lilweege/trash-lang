grammar trash;

// TODO: bitwise operators, pointers, struct

program
    : { prcedure }
    ;

procedure
    : 'proc'
    ;

body
    : block
    | statement
    ;

block
    : '{' { statement } '}'
    ;

statement
    : ifStatement
    | forStatement
    |   ( variableDefnAsgn
        | assignment
        | expression
        | returnStatement
        | breakStatement
        | continueStatement
        ) ';'
    ;

returnStatement
    : 'return' expression?
    ;

breakStatement
    : 'break'
    ;

continueStatement
    : 'continue'
    ;

ifStatement
    : 'if' '(' expression ')' body ('else' body)?
    ;

forStatement
    : 'for' '(' variableDefnAsgn? ';' expression? ';' assignment? ')' body
    ;

expression
    : logicalTerm ( '||' logicalTerm )?
    ;

logicalTerm
    : logicalFactor ( '&&' logicalFactor )?
    ;

logicalFactor
    : comparison ( ( '==' | '!=' ) comparison )?
    ;

comparison
    : sum ( ( '>=' | '<=' | '>' | '<' ) sum )?
    ;

sum
    : term ( ( '+' | '-' ) term )?
    ;

term
    : factor ( ( '*' | '/' | '%' ) factor )?
    ;

factor
    : IntegerLiteral
    | FloatLiteral
    | StringLiteral
    | CharacterLiteral
    | procedureCall
    | lValue
    | '(' expression ')'
    | '-' factor
    | '!' factor
    ;

procedureCall
    : identifier '(' ( expression { ',' expression } )? ')'
    ;

assignment
    : lValue '=' expression
    ;

variableDefnAsgn
    : variableDefn ( '=' expression )?
    ;

variableDefn
    : ( 'let' | 'mut' ) type identifier
    ;

type
    : primitive subscript?
    ;

lValue
    : identifier subscript?
    ;

subscript
    : '[' expression ']'
    ;

primitive
    : 'u8' | 'i64' | 'f64'
    ;












StringLiteral
    : '"' AsciiOrEscapeChar '"'
    ;

fragment AsciiOrEscapeChar : 'a';
CharacterLiteral
    : 'a'
    ;
identifier : 'a';



IntegerLiteral : [0-9]+;

FloatLiteral
    : IntegerLiteral '.' IntegerLiteral
    ;
LineComment
    : '?' ~[\r\n]* -> channel(HIDDEN)
    ;
