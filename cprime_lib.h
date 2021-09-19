
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <wchar.h>



struct SymbolMapItem
{
    struct SymbolMapItem* pNext;
    unsigned int HashValue;
    char* Key;
    struct TypePointer* pValue;
};

struct SymbolMap
{
    struct SymbolMapItem** pHashTable;
    int nHashTableSize;
    int  nCount;
    struct SymbolMap* pPrevious;
};


#define SYMBOLMAP_INIT { 0, 100, 0 , 0}

int SymbolMap_SetAt(struct SymbolMap* pMap,
                    const char* Key,
                    struct TypePointer* newValue);


struct TypePointer* SymbolMap_Find(struct SymbolMap* pMap, const char* Key);

bool SymbolMap_RemoveKey(struct SymbolMap* pMap, const char* Key, struct TypePointer** ppValue);

void SymbolMap_Init(struct SymbolMap* p);

void SymbolMap_Destroy(struct SymbolMap* p);

struct SymbolMapItem* SymbolMap_FindBucket(struct SymbolMap* pMap, const char* Key);

void SymbolMap_Swap(struct SymbolMap* pA, struct SymbolMap* pB);

void SymbolMap_Print(struct SymbolMap* pMap);
bool SymbolMap_IsTypeName(struct SymbolMap* pMap, const char* identifierName);

struct SymbolMapItem* SymbolMap_GetAssocAt(struct SymbolMap* pMap,
        const char* Key,
        unsigned int* nHashBucket,
        unsigned int* HashValue);

struct StructUnionSpecifier* SymbolMap_FindCompleteStructUnionSpecifier(struct SymbolMap* pMap, const char* structTagName);

struct TypeSpecifier* SymbolMap_FindTypedefSpecifierTarget(struct SymbolMap* pMap, const char* typedefName);

struct Declaration* SymbolMap_FindTypedefDeclarationTarget(struct SymbolMap* pMap, const char* typedefName);

struct Declaration* SymbolMap_FindFunction(struct SymbolMap* pMap, const char* funcName);

struct EnumSpecifier* SymbolMap_FindCompleteEnumSpecifier(struct SymbolMap* pMap, const char* enumName);

struct Declarator;
struct DeclarationSpecifiers* SymbolMap_FindTypedefTarget(struct SymbolMap* pMap, const char* typedefName, struct Declarator* declarator);

struct DeclarationSpecifiers* SymbolMap_FindTypedefFirstTarget(struct SymbolMap* pMap, const char* typedefName, struct Declarator* declarator);


#define LOCALSTRBUILDER_NCHARS 7

struct LocalStrBuilder
{
    char* c_str;
    int size;
    int capacity;
    char chars[LOCALSTRBUILDER_NCHARS + 1];

};




enum TokenType
{
    TK_NONE,
    TK_BOF,
    TK_EOF,
    TK_ENDMARK,
    TK_LINE_COMMENT,
    TK_COMMENT,
    TK_OPEN_COMMENT,
    TK_CLOSE_COMMENT,
    TK_STRING_LITERAL,
    TK_IDENTIFIER,
    TK_SPACES,
    TK_DECIMAL_INTEGER,
    TK_HEX_INTEGER,
    TK_OCTAL_INTEGER,
    TK_FLOAT_NUMBER,
    TK_MACROPLACEHOLDER,
    TK_BREAKLINE,
    TK_BACKSLASHBREAKLINE,
    CHAR1, // '/' ,
    CHARACTER_TABULATION,// = '\t';
    TK_PREPROCESSOR,
    TK_ERROR, //reservado para erro
    //
    TK_EXCLAMATION_MARK,// = '!';
    TK_QUOTATION_MARK,// = '\"';
    TK_NUMBER_SIGN,// = '#';

    TK_DOLLAR_SIGN,// = '$';
    TK_PERCENT_SIGN,// = '%';
    TK_AMPERSAND,// = '&';
    TK_APOSTROPHE,// = '\'';
    TK_LEFT_PARENTHESIS,// = '(';
    TK_RIGHT_PARENTHESIS,// = ')';
    TK_ASTERISK,// = '*';
    TK_PLUS_SIGN,// = '+';
    TK_COMMA,// = ',';
    TK_HYPHEN_MINUS,// = '-';
    TK_HYPHEN_MINUS_NEG,// = '-'; //nao retorna no basic char mas eh usado para saber se eh - unario
    TK_FULL_STOP,// = '.';
    TK_SOLIDUS,// = '/';

    TK_COLON,// = ':';
    TK_SEMICOLON,// = ';';
    TK_LESS_THAN_SIGN,// = '<';
    TK_EQUALS_SIGN,// = '=';
    TK_GREATER_THAN_SIGN,// = '>';
    TK_QUESTION_MARK,// = '\?';
    TK_COMMERCIAL_AT,// = '@';

    TK_LEFT_SQUARE_BRACKET,// = '[';
    REVERSE_SOLIDUS,// = '\\';
    TK_RIGHT_SQUARE_BRACKET,// = ']';
    TK_CIRCUMFLEX_ACCENT,// = '^';
    TK_LOW_LINE,// = '_';
    TK_GRAVE_ACCENT,// = '`';

    TK_LEFT_CURLY_BRACKET,// = '{';
    TK_VERTICAL_LINE,// = '|';
    TK_RIGHT_CURLY_BRACKET,// = '}';
    TK_TILDE, // ~

    TK_ARROW,
    TK_PLUSPLUS,
    TK_MINUSMINUS,
    TK_LESSLESS,
    TK_GREATERGREATER,
    TK_LESSEQUAL,
    TK_GREATEREQUAL,
    TK_EQUALEQUAL,
    TK_NOTEQUAL,
    TK_ANDAND,
    TK_OROR,
    TK_MULTIEQUAL,
    TK_DIVEQUAL,
    TK_PERCENT_EQUAL,
    TK_PLUSEQUAL,
    TK_MINUS_EQUAL,
    TK_ANDEQUAL,
    TK_CARETEQUAL,
    TK_OREQUAL,
    TK_NUMBERNUMBER,
    TK_LESSCOLON,
    TK_COLONGREATER,
    TK_LESSPERCENT,
    TK_PERCENTGREATER,
    TK_PERCENTCOLON,
    TK_DOTDOTDOT,
    TK_GREATERGREATEREQUAL,
    TK_LESSLESSEQUAL,
    TK_PERCENTCOLONPERCENTCOLON,

    TK_CHAR_LITERAL,

    TK_AUTO,

    TK_BREAK,
    TK_CASE,
    TK_CHAR,
    TK_CONST,
    TK_CONTINUE,
    TK_DEFAULT,
    TK_DO,
    TK_DOUBLE,
    TK_ELSE,
    TK_ENUM,
    TK_EXTERN,
    TK_FLOAT,
    TK_FOR,
    TK_GOTO,
    TK_IF,
    TK_TRY,
    TK_DEFER,
    TK_INT,
    TK_LONG,
    ////////////////
    //Microsoft - specific
    TK__INT8,
    TK__INT16,
    TK__INT32,
    TK__INT64,
    TK__WCHAR_T,
    //TK___DECLSPEC,
    ////////////////
    TK_REGISTER,
    TK_RETURN,
    TK_THROW, /*extension*/
    TK_SHORT,
    TK_SIGNED,
    TK_SIZEOF,
    TK_STATIC,
    TK_STRUCT,
    TK_SWITCH,
    TK_TYPEDEF,
    TK_UNION,
    TK_UNSIGNED,
    TK_VOID,
    TK_VOLATILE,
    TK_WHILE,
    TK_CATCH, /*ex*/
    TK__THREAD_LOCAL,
    TK__BOOL,
    TK__COMPLEX,
    TK__ATOMIC,
    TK_RESTRICT,
    TK__STATIC_ASSERT,
    TK_INLINE,
    TK__INLINE,//ms
    TK__FORCEINLINE, //ms
    TK__NORETURN,
    TK__ALIGNAS,
    TK__GENERIC,
    TK__IMAGINARY,
    TK__ALINGOF,

    ///
    TK__ASM, //visual c++
    TK__PRAGMA, //visual c++
    TK__C99PRAGMA, 

    //enum Tokens para linhas do pre processador
    TK_PRE_INCLUDE,
    TK_PRE_PRAGMA,
    TK_PRE_IF,
    TK_PRE_ELIF,
    TK_PRE_IFNDEF,
    TK_PRE_IFDEF,
    TK_PRE_ENDIF,
    TK_PRE_ELSE,
    TK_PRE_ERROR,
    TK_PRE_WARNING, /*warning*/
    TK_PRE_LINE,
    TK_PRE_UNDEF,
    TK_PRE_DEFINE,
    //fim tokens preprocessador
    TK_MACRO_CALL,
    TK_MACRO_EOF,
    TK_FILE_EOF,

    /*novas*/

    TK_NEW
};

const char* TokenToString(enum TokenType tk);
const char* TokenToNameString(enum TokenType tk);


struct Token
{
    struct LocalStrBuilder lexeme;
    enum TokenType token;
    int Line /*= -1*/;
    int FileIndex /*= -1*/; /*indice do arquivo incluido*/
    bool bActive /*= 1*/;  /*ativo quando nao esta dentro de #if 0 etc*/
    struct Token* pNext;
};
#define TOKEN_INIT {LOCALSTRBUILDER_INIT, TK_NONE,-1,-1,1,0}


struct TokenList
{
    struct Token* pHead, * pTail;
};
#define TOKENLIST_INIT {0}



struct FileInfo
{
    char* FullPath;
    char* IncludePath;
    int FileIndex;
    bool PragmaOnce;
};


struct FileArray
{
    struct FileInfo** pItems;
    int Size;
    int Capacity;
};



struct PPTokenArray
{
    struct PPToken* *   pItems;
    int Size;
    int Capacity;
};



struct TokenArrayMapItem
{
    struct TokenArrayMapItem* pNext;
    unsigned int HashValue;
    char*  Key;
    struct PPTokenArray* pValue;
};

struct TokenArrayMap
{
    struct TokenArrayMapItem** pHashTable;
    unsigned int nHashTableSize;
    int  nCount;
};



struct PPTokenSet
{
    struct PPToken**  pItems;
    int Size;
    int Capacity;
};


/*
preprocessing-token:
  header-name
  identifier
  pp-number
  character-constant
  string-literal
  punctuator
  each non-white-space character that cannot be one of the above
*/
enum PPTokenType
{
    PPTokenType_Identifier,
    PPTokenType_Number,
    PPTokenType_CharConstant,
    PPTokenType_StringLiteral,
    PPTokenType_Punctuator,
    PPTokenType_Spaces,
    PPTokenType_Other,
};

struct PPToken
{
    enum PPTokenType Token;
    char* Lexeme;
    struct PPTokenSet HiddenSet;
};



struct StrBuilder
{
    char* c_str;
    int size;
    int capacity;
};


struct Macro
{
    char* Name;
    bool bIsFunction;
    struct PPTokenArray ReplacementList;
    struct PPTokenArray FormalArguments;
    int FileIndex;
};

#define MACRO_INIT {0}

struct MacroMapItem
{
    struct MacroMapItem* pNext;
    unsigned int HashValue;
    char* Key;
    struct Macro* pValue;
};

struct MacroMap
{
    struct MacroMapItem** pHashTable;
    unsigned int nHashTableSize;
    int  nCount;
};


/*
AST data structures
*/

enum Type
{
    Null_ID,
    Declaration_ID,

    StaticAssertDeclaration_ID,
    EofDeclaration_ID,
    InitDeclarator_ID,

    SingleTypeSpecifier_ID,
    EnumSpecifier_ID,
    StructUnionSpecifier_ID,
    StorageSpecifier_ID,
    AtomicTypeSpecifier_ID,
    TemplateTypeSpecifier_ID,
    StructDeclaration_ID,
    AlignmentSpecifier_ID,
    TypeQualifier_ID,
    FunctionSpecifier_ID,
    CompoundStatement_ID,
    ExpressionStatement_ID,
    SwitchStatement_ID,
    LabeledStatement_ID,
    ForStatement_ID,
    JumpStatement_ID,
    AsmStatement_ID,
    WhileStatement_ID,
    DoStatement_ID,
    TryBlockStatement_ID,
    
    IfStatement_ID,
    TypeName_ID,
    Enumerator_ID,

    InitializerListType_ID,
    DeferStatement_ID,
    PrimaryExpression_ID,
    UnaryExpressionOperator_ID,
    CastExpressionType_ID,
    PrimaryExpressionValue_ID,
    PrimaryExpressionLiteral_ID,
    PostfixExpression_ID,
    BinaryExpression_ID,
    TernaryExpression_ID,
    PrimaryExpressionLambda_ID,
    Parameter_ID
};

struct TypePointer
{
    enum Type Type;
};


struct FilePos
{
    int FileIndex;
    int Line;
};

#define FILEPOS_INIT {0,0}


struct StaticAssertDeclaration
{
    /*
    static_assert-declaration:
      _Static_assert ( constant-expression , char-literal ) ;
    */
    enum Type Type;

    struct Expression* pConstantExpression;
    char* Text;
    struct TokenList ClueList0;
    struct TokenList ClueList1;
    struct TokenList ClueList2;
    struct TokenList ClueList3;
    struct TokenList ClueList4;
    struct TokenList ClueList5;

};

#define STATICASSERTDECLARATION_INIT {StaticAssertDeclaration_ID}


struct EofDeclaration
{
    /*
    Marca fim de um arquivo
    */
    enum Type Type;
    struct TokenList ClueList0;
};
#define EOFDECLARATION_INIT {EofDeclaration_ID}



struct AnyDeclaration
{
    /*
      StaticAssertDeclaration
      Declaration
      GroupDeclaration
      EofDeclaration
    */
    enum Type Type;
};


struct BlockItemList
{
    /*
    block-item-list:
      block-item
      block-item-list block-item
    */
    struct BlockItem** data;
    int size;
    int capacity;
};

#define BLOCKITEMLIST_INIT {0}


struct CompoundStatement
{
    enum Type Type;
    struct BlockItemList BlockItemList;
    struct TokenList ClueList0;
    struct TokenList ClueList1;    
};

#define COMPOUNDSTATEMENT_INIT {CompoundStatement_ID}

struct TypeQualifier
{
    /*
    type-qualifier:
      const
      restrict
      volatile
      _Atomic
    */

    /*
      auto
    */

    enum Type Type;
    enum TokenType Token;
    struct TokenList ClueList0;
};

#define TYPEQUALIFIER_INIT {TypeQualifier_ID}



struct TypeQualifierList
{
    /*
    type-qualifier-list:
      type-qualifier
      type-qualifier-list type-qualifier
    */

    struct TypeQualifier* Data[4];
    int Size;
};



struct ExpressionStatement
{
    enum Type Type;
    struct Expression* pExpression;
    struct TokenList ClueList0;
};

#define EXPRESSIONSTATEMENT_INIT {ExpressionStatement_ID}


struct JumpStatement
{
    /*
    jump-statement:
       goto identifier ;
       continue ;
       break ;
       return expression_opt ;
    */
    enum Type Type;
    enum TokenType token;
    char* Identifier;
    struct Expression* pExpression;
    struct TokenList ClueList0;
    struct TokenList ClueList1;
    struct TokenList ClueList2;
};

struct DeferStatement
{
    /*
    jump-statement:
       defer statement;       
    */
    enum Type Type;
    struct Statement* pStatement;
    struct TokenList ClueList0;      
};
#define DEFERSTATEMENT_INIT  {DeferStatement_ID}
void DeferStatement_Delete(struct DeferStatement* p);



struct AsmStatement
{
    enum Type Type;
    struct TokenList ClueList;
};


struct ForStatement
{
    /*
    iteration-statement:
     for ( expressionopt ; expressionopt ; expressionopt ) statement
     for ( declaration expressionopt ; expressionopt ) statement
    */

    enum Type Type;
    struct AnyDeclaration* pInitDeclarationOpt;
    struct Expression* pExpression1;
    struct Expression* pExpression2;
    struct Expression* pExpression3;
    struct Statement* pStatement;
    struct TokenList ClueList0;
    struct TokenList ClueList1;
    struct TokenList ClueList2;
    struct TokenList ClueList3;
    struct TokenList ClueList4;
};

#define FORSTATEMENT_INIT  {ForStatement_ID}
void ForStatement_Delete(struct ForStatement* p);

struct WhileStatement
{
    /*
    iteration-statement:
       while ( expression ) statement
    */
    enum Type Type;
    struct Expression* pExpression;
    struct Statement* pStatement;
    struct TokenList ClueList0;
    struct TokenList ClueList1;
    struct TokenList ClueList2;
};
#define WHILESTATEMENT_INIT {WhileStatement_ID}
void WhileStatement_Delete(struct WhileStatement* p);

struct DoStatement
{
    /*
    iteration-statement:
       do statement while ( expression ) ;
    */

    enum Type Type;
    struct Expression* pExpression;
    struct Statement* pStatement;
    struct TokenList ClueList0; //do
    struct TokenList ClueList1; //while
    struct TokenList ClueList2; //(
    struct TokenList ClueList3; //)
    struct TokenList ClueList4; // ;

};
#define DOSTATEMENT_INIT {DoStatement_ID}
void DoStatement_Delete(struct DoStatement* p);

struct TryBlockStatement
{
    /*
    try-block:
       try compound-statement catch ( parameter ) compound-statement
    */

    enum Type Type;
    
    struct CompoundStatement* pCompoundStatement;
    struct CompoundStatement* pCompoundCatchStatement;
    struct TokenList ClueListTry;    
    struct TokenList ClueListCatch;
    struct TokenList ClueListLeftPar;
    struct TokenList ClueListRightPar;
    

};
#define TRYBLOCKSTATEMENT_INIT {TryBlockStatement_ID}
void TryBlockStatement_Delete(struct TryBlockStatement* p);

struct LabeledStatement
{
    enum Type Type;

    struct Statement* pStatementOpt;
    struct Expression* pExpression;
    char* Identifier;
    enum TokenType token;
    struct TokenList ClueList0;
    struct TokenList ClueList1;
};
#define LABELEDSTATEMENT_INIT {LabeledStatement_ID}
void LabeledStatement_Delete(struct LabeledStatement* p);


struct SwitchStatement
{
    /*
    selection-statement:
     switch ( expression ) statement
    */
    enum Type Type;
    struct Expression* pConditionExpression;
    struct Statement* pExpression;
    struct TokenList ClueList0;
    struct TokenList ClueList1;
    struct TokenList ClueList2;
};
#define SWITCHSTATEMENT_INIT {SwitchStatement_ID}
void SwitchStatement_Delete(struct SwitchStatement* p);


struct IfStatement
{
    /*
    selection-statement:
      if ( condition-expression ) statement
      if ( condition-expression ) statement else statement
    */

    /*
    selection-statement:
     if ( declaration condition-expression) statement
     if ( declaration condition-expression) statement else statement
     if ( declaration condition-expression ; expression) statement
    */

    /*
    selection-statement:
     try (condition-expression) ;
     try (declaration or expression ; condition-expression; defer optional) ;     
    */

    enum Type Type;

    struct AnyDeclaration* pInitDeclarationOpt;
    struct Expression* pDeferExpression;
    struct Expression* pConditionExpression;
    struct Expression* pInitialExpression;

    struct Statement* pStatement;
    struct Statement* pElseStatement;
    struct TokenList ClueList0; //if or try
    struct TokenList ClueList1; //(
    struct TokenList ClueList2; //;
    struct TokenList ClueList3; //;
    struct TokenList ClueList4; //)
    struct TokenList ClueList5; //)
    struct TokenList ClueList6; //; 
    struct TokenList ClueListThrow; //; 
};

#define IFSTATEMENT_INIT  {IfStatement_ID}
void IfStatement_Delete(struct IfStatement* p);


struct Statement
{
    /*
    TCompoundStatement
    TExpressionStatement
    TLabeledStatement
    TJumpStatement
    TIfStatement
    TDoStatement
    TForStatement
    TAsmStatement
    TWhileStatement
    TSwitchStatement
    */
    enum Type Type;
};
void Statement_Delete(struct Statement* p);

struct /*<Declaration | Statement>*/ BlockItem
{
    /*block-item:
       declaration
       statement
    */
    enum Type Type;
};

void BlockItem_Delete(struct BlockItem* p);


struct Pointer
{
    /*
    pointer:
      * type-qualifier-list_opt
      * type-qualifier-list_opt pointer
    */
    struct TypeQualifierList Qualifier;
    struct Pointer* pNext;
    struct TokenList ClueList0;
};
#define POINTER_INIT {0}

void Pointer_Delete(struct Pointer* p);
void Pointer_CopyFrom(struct Pointer* dest, struct Pointer* src);


struct PointerList
{
    /*
    pointer:
      * type-qualifier-listopt
      * type-qualifier-listopt pointer
    */
    struct Pointer* pHead, * pTail;
};

#define POINTERLIST_INIT {0}

void PointerList_PopFront(struct PointerList* pList);
void PointerList_PushBack(struct PointerList* pList, struct Pointer* pItem);

void PointerList_CopyTo(struct PointerList* from, struct PointerList* to);
void PointerList_Destroy(struct PointerList* p);
bool PointerList_IsPointer(struct PointerList* pPointerlist);

bool PointerList_IsAutoPointer(struct PointerList* pPointerlist);
bool PointerList_IsAutoPointerToAutoPointer(struct PointerList* pPointerlist);
bool PointerList_IsPointerToObject(struct PointerList* pPointerlist);
bool PointerList_IsAutoPointerToObject(struct PointerList* pPointerlist);
bool PointerList_IsAutoPointerToPointer(struct PointerList* pPointerlist);
bool PointerList_IsPointer(struct PointerList* pPointerlist);
void PointerList_Swap(struct PointerList* a, struct PointerList* b);
bool PointerList_IsAutoPointerSizeToObject(struct PointerList* pPointerlist);
bool PointerList_IsPointerN(struct PointerList* pPointerlist, int n);


struct FunctionSpecifier
{
    /*
    function-specifier:
      inline
      _Noreturn
    */
    enum Type Type;
    enum TokenType Token;
    struct TokenList ClueList0;
};

#define FUNCTIONSPECIFIER_INIT {FunctionSpecifier_ID}

struct FunctionSpecifier* FunctionSpecifier_Clone(struct FunctionSpecifier* p);
void FunctionSpecifier_Delete(struct FunctionSpecifier* p);
bool FunctionSpecifier_Compare(struct FunctionSpecifier* p1, struct FunctionSpecifier* p2);

struct StorageSpecifier
{
    /*
    storage-class-specifier:
      typedef
      extern
      static
      _Thread_local
      auto
      register
    */
    enum Type Type;
    enum TokenType Token;
    struct TokenList ClueList0;
};
#define STORAGESPECIFIER_INIT {StorageSpecifier_ID}

struct StorageSpecifier* StorageSpecifier_Clone(struct StorageSpecifier* p);
void StorageSpecifier_Delete(struct StorageSpecifier* p);
bool StorageSpecifier_Compare(struct StorageSpecifier* p1, struct StorageSpecifier* p2);

struct AlignmentSpecifier
{
    /*
    alignment-specifier:
      _Alignas ( type-name )
      _Alignas ( constant-expression )
    */
    enum Type Type;
    char* TypeName;
};
#define ALIGNMENTSPECIFIER_INIT {AlignmentSpecifier_ID}
void AlignmentSpecifier_Delete(struct AlignmentSpecifier* p);
struct AlignmentSpecifier* AlignmentSpecifier_Clone(struct AlignmentSpecifier* p);


struct Enumerator
{
    /*
    enumerator:
      enumeration-constant
      enumeration-constant = constant-expression
    */
    struct Enumerator* pNext;
    char* Name;
    struct Expression* pConstantExpression;
    struct TokenList ClueList0;
    struct TokenList ClueList1; // =
    struct TokenList ClueList2; // ,
    bool bHasComma;
};
#define ENUMERATOR_INIT {0}
void Enumerator_Delete(struct Enumerator* p);
struct Enumerator* Enumerator_Clone(struct Enumerator* p);


struct EnumeratorList
{
    /*
    enumerator-list:
      enumerator
      enumerator-list, enumerator
    */
    struct Enumerator* pHead, * pTail;
};
#define ENUMERATORLIST_INIT {0}
void EnumeratorList_Destroy(struct EnumeratorList* p);

struct EnumSpecifier
{
    /*
    enum-specifier:
      enum identifieropt { enumerator-list }
      enum identifieropt { enumerator-list , }
      enum identifier
    */

    enum Type Type;
    char* Tag;
    struct EnumeratorList EnumeratorList;
    struct TokenList ClueList0;
    struct TokenList ClueList1;
    struct TokenList ClueList2;
    struct TokenList ClueList3;
};
#define ENUMSPECIFIER_INIT {EnumSpecifier_ID}

struct EnumSpecifier* EnumSpecifier_Clone(struct EnumSpecifier* p);
void EnumSpecifier_Delete(struct EnumSpecifier* p);
bool EnumSpecifier_IsSameTag(struct EnumSpecifier* p1, struct EnumSpecifier* p2);

struct SingleTypeSpecifier
{
    /*
    void
    char
    short
    int
    long
    float
    double
    signed
    unsigned
    _Bool
    _Complex
    typedef-name
    */

    /*
    * typedef-name < type-name , type-name >
    * struct-tag   < type-name , type-name >
    */
    enum Type Type;
    enum TokenType Token2;
    char* TypedefName;
    struct TokenList ClueList0;
};
#define SINGLETYPESPECIFIER_INIT {SingleTypeSpecifier_ID}

void SingleTypeSpecifier_Delete(struct SingleTypeSpecifier* p);
struct SingleTypeSpecifier* SingleTypeSpecifier_Clone(struct SingleTypeSpecifier* p);
const char* SingleTypeSpecifier_GetTypedefName(struct SingleTypeSpecifier* p);
bool SingleTypeSpecifier_Compare(struct SingleTypeSpecifier* p1, struct SingleTypeSpecifier* p2);


/*
declaration-specifiers:
storage-class-specifier declaration-specifiersopt
type-specifier declaration-specifiersopt
type-qualifier declaration-specifiersopt
function-specifier declaration-specifiersopt
alignment-specifier declaration-specifiersopt
*/

struct DeclarationSpecifier
{
    /*
     StorageSpecifier
     TypeSpecifier
     TypeQualifier
     FunctionSpecifier
     AlignmentSpecifier
    */
    enum Type Type;
};


struct SpecifierQualifier
{
    /* 
     TypeSpecifier 
     TypeQualifier
    */
    enum Type Type;
};


struct SpecifierQualifierList
{
    /*
    specifier-qualifier-list:
      type-specifier specifier-qualifier-listopt
      type-qualifier specifier-qualifier-listopt
    */
    struct SpecifierQualifier** pData;
    int Size;
    int Capacity;
};
#define SPECIFIERQUALIFIERLIST_INIT {0}

void SpecifierQualifierList_CopyTo(struct SpecifierQualifierList* from, struct SpecifierQualifierList* to);
void SpecifierQualifierList_Destroy(struct SpecifierQualifierList* pDeclarationSpecifiers);
void SpecifierQualifierList_PushBack(struct SpecifierQualifierList* p, struct SpecifierQualifier* pItem);

const char* SpecifierQualifierList_GetTypedefName(struct SpecifierQualifierList* p);
struct DeclarationSpecifier* SpecifierQualifierList_GetMainSpecifier(struct SpecifierQualifierList* p);
bool SpecifierQualifierList_IsTypedefQualifier(struct SpecifierQualifierList* p);
bool SpecifierQualifierList_Compare(struct SpecifierQualifierList* p1, struct SpecifierQualifierList* p2);

bool SpecifierQualifierList_CanAdd(struct SpecifierQualifierList* p, enum TokenType token);
bool SpecifierQualifierList_IsBool(struct SpecifierQualifierList* p);
bool SpecifierQualifierList_IsChar(struct SpecifierQualifierList* p);
bool SpecifierQualifierList_IsAnyInteger(struct SpecifierQualifierList* p);
bool SpecifierQualifierList_IsAnyFloat(struct SpecifierQualifierList* p);


struct DeclarationSpecifiers
{
    /*
    declaration-specifiers:
      storage-class-specifier declaration-specifiersopt
      type-specifier declaration-specifiersopt
      type-qualifier declaration-specifiersopt
      function-specifier declaration-specifiersopt
      alignment-specifier declaration-specifiersopt
    */
    struct DeclarationSpecifier** pData;
    int Size;
    int Capacity;
};

#define DECLARATIONSPECIFIERS_INIT {0}

void DeclarationSpecifiers_CopyTo(struct DeclarationSpecifiers* from, struct DeclarationSpecifiers* to);

void DeclarationSpecifiers_CopyTo_SpecifierQualifierList(struct DeclarationSpecifiers* from, struct SpecifierQualifierList* to);

void DeclarationSpecifiers_Destroy(struct DeclarationSpecifiers* pDeclarationSpecifiers);

void DeclarationSpecifiers_PushBack(struct DeclarationSpecifiers* p, struct DeclarationSpecifier* pItem);

struct DeclarationSpecifier* DeclarationSpecifiers_GetMainSpecifier(struct DeclarationSpecifiers* p, enum Type type);

const char* DeclarationSpecifiers_GetTypedefName(struct DeclarationSpecifiers* pDeclarationSpecifiers);

bool DeclarationSpecifiers_CanAddSpeficier(struct DeclarationSpecifiers* pDeclarationSpecifiers, enum TokenType token, const char* lexeme);

struct StructUnionSpecifier;

struct StructUnionSpecifier* TDeclarationSpecifiers_GetCompleteStructUnionSpecifier(struct SymbolMap* pSymbolMap, struct DeclarationSpecifiers* pDeclarationSpecifiers);


struct ParameterList
{
    /*
    parameter-list:
      parameter-declaration
      parameter-list , parameter-declaration
    */

    struct Parameter* pHead, * pTail;
};
#define PARAMETERLIST_INIT {0}

void ParameterList_Destroy(struct ParameterList* p);

const char* Parameter_GetName(struct Parameter* p);
bool Parameter_IsDirectPointer(struct Parameter* p);

struct ParameterTypeList
{
    /*
    parameter-type-list:
      parameter-list
      parameter-list , ...
    */
    struct ParameterList ParameterList;
    struct TokenList ClueList0; //,
    struct TokenList ClueList1; //...
    bool bVariadicArgs;
    bool bIsVoid;
};

#define PARAMETERTYPELIST_INIT {0}

void ParameterTypeList_Destroy(struct ParameterTypeList* p);
void ParameterTypeList_Delete(struct ParameterTypeList* p);
void ParameterTypeList_CopyTo(struct ParameterTypeList* from, struct ParameterTypeList* to);

const char* ParameterTypeList_GetFirstParameterName(struct ParameterTypeList* p);
const char* ParameterTypeList_GetSecondParameterName(struct ParameterTypeList* p);
struct Parameter* ParameterTypeList_FindParameterByName(struct ParameterTypeList* p, const char* name);
struct Parameter* ParameterTypeList_GetParameterByIndex(struct ParameterTypeList* p, int index);
void ParameterTypeList_GetArgsString(struct ParameterTypeList* p, struct StrBuilder* sb);
bool ParameterTypeList_HasNamedArgs(struct ParameterTypeList* p);

struct Designator
{
    /*
    designator:
      [ constant-expression ]
      0                     1
      . identifier
      0
    */
    char* Name; /*se tem name quer dizer que eh .identifer*/
    struct Expression* pExpression;
    struct Designator* pNext;
    struct TokenList ClueList0; //. ou [
    struct TokenList ClueList1; //]
};
#define DESIGNATOR_INIT {0}
void Designator_Delete(struct Designator* p);


struct Designation
{
    /*
    designator-list:
      designator
      designator-list designator
    */
    struct Designator* pHead, * pTail;

    /*
    designation:
    designator-list =
                    0
    */

    struct TokenList ClueList0;
};
#define DESIGNATORLIST_INIT {0}

void Designation_Destroy(struct Designation* p);
void Designation_PushBack(struct Designation* p, struct Designator* pItem);

struct Initializer;

void Initializer_Delete(struct Initializer* p);

struct InitializerListItem
{
    /*
    initializer-list:
      designationopt initializer
      initializer-list , designationopt initializer
    */
    struct Designation  DesignationOpt;
    struct Initializer* pInitializer;
    struct InitializerListItem* pNext;
    struct TokenList ClueList;
    bool bDefault;
};

#define INITIALIZERLISTITEM_INIT  {0}
void InitializerListItem_Delete(struct InitializerListItem* p);


struct InitializerList
{
    /*
    initializer-list:
      designation_opt initializer
      initializer-list , designation_opt initializer
    */

    struct InitializerListItem* pHead, * pTail;
};
#define INITIALIZERLIST_INIT {0}
void InitializerList_Destroy(struct InitializerList* p);


struct InitializerListType
{
    /*
       { initializer-list  }
       { initializer-list , }
    */

    enum Type Type;
    struct InitializerList InitializerList;
    struct TokenList ClueList0;
    struct TokenList ClueList1;
    struct TokenList ClueList2;
};

#define INITIALIZERLISTTYPE_INIT {InitializerListType_ID}
void InitializerListType_Delete(struct InitializerListType* p);


struct /*<TInitializerListType | TExpression>*/ Initializer
{
    /*
    initializer:
      assignment-expression
      { initializer-list }
      { initializer-list ,}
    */
    enum Type Type;
};

struct Declarator
{
    /*
    declarator:
       pointeropt direct-declarator
    */

    /*
    abstract-declarator:
       pointer
       pointeropt direct-abstract-declarator
    */

    struct PointerList PointerList;
    struct DirectDeclarator* pDirectDeclarator;
};
#define TDECLARATOR_INIT {0}
void Declarator_CopyTo(struct Declarator* from, struct Declarator* to);
void Declarator_CopyToAbstractDeclarator(struct Declarator* from, struct Declarator* to);
struct Declarator* Declarator_Clone(struct Declarator* p);
void Declarator_Delete(struct Declarator* p);
void Declarator_Destroy(struct Declarator* p);
const char* Declarator_GetName(struct Declarator* p);
void Declarator_Swap(struct Declarator* a, struct Declarator* b);
bool Declarator_IsAutoArray(struct Declarator* pDeclarator);

enum DirectDeclaratorType
{
    TDirectDeclaratorTypeNone,
    TDirectDeclaratorTypeIdentifier,
    TDirectDeclaratorTypeDeclarator,
    TDirectDeclaratorTypeFunction,
    TDirectDeclaratorTypeArray,
    TDirectDeclaratorTypeAutoArray,
};

struct DirectDeclarator
{
    /*
    direct-declarator:
      identifier
      ( declarator )
      direct-declarator [ type-qualifier-listopt assignment-expressionopt ]
      direct-declarator [ static type-qualifier-listopt assignment-expression ]
      direct-declarator [ type-qualifier-list static assignment-expression ]
      direct-declarator [ type-qualifier-listopt * ]
      direct-declarator ( parameter-type-list )
      direct-declarator ( identifier-listopt )
    */

    /*
    *   direct-declarator ( parameter-type-list ) overload_opt
    */

    char* Identifier;
    struct Declarator* pDeclarator;
    struct DirectDeclarator* pDirectDeclarator;
    struct FilePos Position;
    struct ParameterTypeList  Parameters;
    struct Expression* pExpression;
    enum DirectDeclaratorType DeclaratorType; //para diferenciar pois null nao basta []
    struct TokenList ClueList0;
    struct TokenList ClueList1;
    struct TokenList ClueList2;
    struct TokenList ClueList3;
    struct TokenList ClueList4;
    bool bOverload;
    char* NameMangling;
};

#define TDIRECTDECLARATOR_INIT {0}

struct DirectDeclarator* DirectDeclarator_Clone(struct DirectDeclarator* p);
void DirectDeclarator_Delete(struct DirectDeclarator* p);

struct InitDeclarator
{
    /*
    init-declarator:
      declarator
      declarator = initializer
    */
    struct Declarator* pDeclarator;
    struct Initializer* pInitializer;
    struct InitDeclarator* pNext;
    struct TokenList ClueList0;
    struct TokenList ClueList1; //defval
};
#define INITDECLARATOR_INIT {0}

typedef struct InitDeclarator StructDeclarator;


void InitDeclarator_Delete(struct InitDeclarator* p);


struct InitDeclaratorList
{
    /*
    init-declarator-list:
      init-declarator
      init-declarator-list , init-declarator
    */
    struct InitDeclarator* pHead, * pTail;
};
#define INITDECLARATORLIST_INIT {0}

void InitDeclaratorList_Destroy(struct InitDeclaratorList* p);

const char* Declarator_GetName(struct Declarator* p);
const char* InitDeclarator_FindName(struct InitDeclarator* p);
bool HasAutoArray(struct InitDeclaratorList* p);

struct StructDeclaratorList
{
    /*
    init-declarator-list:
      init-declarator
      init-declarator-list , init-declarator
    */
    struct InitDeclarator* pHead, * pTail;
};
#define STRUCTDECLARATORLIST_INIT {0}
void StructDeclaratorList_Destroy(struct StructDeclaratorList* p);

void StructDeclaratorList_Add(struct StructDeclaratorList* p, struct InitDeclarator* pItem);


struct StructDeclaration
{
    /*
    struct-declaration:
      specifier-qualifier-list struct-declarator-listopt ;
      static_assert-declaration
    */

    enum Type Type;
    struct SpecifierQualifierList SpecifierQualifierList;
    struct StructDeclaratorList DeclaratorList;
    struct TokenList ClueList1;
};
#define STRUCTDECLARATION_INIT {StructDeclaration_ID}


void StructDeclaration_Delete(struct StructDeclaration* p);


/*
struct-declaration:
specifier-qualifier-list struct-declarator-listopt ;
static_assert-declaration
*/
struct /*<TStructDeclaration | TStaticAssertDeclaration>*/ AnyStructDeclaration
{
    enum Type Type;
};


void AnyStructDeclaration_Delete(struct AnyStructDeclaration* p);



struct StructDeclarationList
{
    /*
    struct-declaration-list:
      struct-declaration
      struct-declaration-list struct-declaration
    */
    struct AnyStructDeclaration** pItems;
    int Size;
    int Capacity;
};
#define STRUCTDECLARATIONLIST_INIT {0}
void StructDeclarationList_Destroy(struct StructDeclarationList* p);
void StructDeclarationList_PushBack(struct StructDeclarationList* p, struct AnyStructDeclaration* pItem);


struct UnionSetItem
{
    struct UnionSetItem* pNext;
    enum TokenType Token;
    enum TokenType TokenFollow;
    char* Name;
    struct TokenList ClueList0;
    struct TokenList ClueList1;
    struct TokenList ClueList2;
};
#define TUNIONSETITEM_INIT {0}
void UnionSetItem_Delete(struct UnionSetItem*);
struct UnionSetItem* UnionSetItem_Clone(struct UnionSetItem*);

struct UnionSet
{
    struct UnionSetItem* pHead, * pTail;
    struct TokenList ClueList0; // <
    struct TokenList ClueList1; // >
};
#define TUNIONSET_INIT {0}

void TUnionSet_CopyTo(struct UnionSet* from, struct UnionSet* to);
void TUnionSet_Destroy(struct UnionSet* p);
void TUnionSet_PushBack(struct UnionSet* p, struct UnionSetItem* pItem);

struct StructUnionSpecifier
{
    /*
    struct-or-union-specifier:
      struct-or-union identifier_opt { struct-declaration-list }
      struct-or-union identifier
    */

    /*
    struct-or-union-specifier:
      struct-or-union union-set
      struct-or-union union-set identifier
    */

    enum Type Type;
    struct StructDeclarationList StructDeclarationList;
    char* Tag;

    enum TokenType Token;


    struct UnionSet UnionSet;
    struct TokenList ClueList0; //struct-or-union
    struct TokenList ClueList1; //identifieropt
    struct TokenList ClueList2; //{
    struct TokenList ClueList3; //}
};

#define STRUCTUNIONSPECIFIER_INIT  {StructUnionSpecifier_ID}

struct StructUnionSpecifier* StructUnionSpecifier_Clone(struct StructUnionSpecifier* p);
void StructUnionSpecifier_Delete(struct StructUnionSpecifier* p);
void GetOrGenerateStructTagName(struct StructUnionSpecifier* p, char* out, int size);


struct TypeSpecifier
{
    /* 
      SingleTypeSpecifier 
      AtomicTypeSpecifier 
      EnumSpecifier 
      StructUnionSpecifier
    */
    enum Type Type;
};

bool TypeSpecifier_Compare(struct TypeSpecifier* p1, struct TypeSpecifier* p2);





struct Declaration
{
    /*
    declaration:
       declaration-specifiers init-declarator-listopt ;
       static_assert-declaration
    */


    enum Type Type;
    struct DeclarationSpecifiers Specifiers;
    struct InitDeclaratorList InitDeclaratorList;

    //se for funcao
    struct CompoundStatement* pCompoundStatementOpt;

    int FileIndex;
    int Line;

    struct TokenList ClueList0; //default FunctionTag
    struct TokenList ClueList00; //:
    struct TokenList ClueList001; //identifier
    struct TokenList ClueList1;
};

#define DECLARATION_INIT {Declaration_ID}


void Declaration_Delete(struct Declaration* p);

struct DeclarationSpecifiers* Declaration_GetArgTypeSpecifier(struct Declaration* p, int index);

const char* Declaration_GetArgName(struct Declaration* p, int index);
int Declaration_GetNumberFuncArgs(struct Declaration* p);

bool Declaration_Is_StructOrUnionDeclaration(struct Declaration* p);

struct CompoundStatement* Declaration_Is_FunctionDefinition(struct Declaration* p);

struct Declarator* Declaration_FindDeclarator(struct Declaration* p, const char* name);
const char* Declaration_GetFunctionName(struct Declaration* p);




bool AnyDeclaration_IsTypedef(struct AnyDeclaration* pDeclaration);
bool AnyDeclaration_Is_StructOrUnionDeclaration(struct AnyDeclaration* pDeclaration);
int AnyDeclaration_GetFileIndex(struct AnyDeclaration* pDeclaration);


struct Parameter
{
    enum Type Type;
    /*
    parameter-declaration:
      declaration-specifiers declarator
      declaration-specifiers abstract-declaratoropt
    */

    struct Parameter* pNext;
    struct DeclarationSpecifiers Specifiers;
    struct Declarator Declarator;
    struct TokenList ClueList0; //, do parametro
    bool bHasComma;
};
#define PARAMETER_INIT {Parameter_ID}

struct Parameter* Parameter_Clone(struct Parameter* p);
void Parameter_Delete(struct Parameter* p);

const char* Parameter_GetName(struct Parameter* p);
const char* Parameter_GetTypedefName(struct Parameter* p);
bool Parameter_IsAutoArray(struct Parameter* pParameter);

struct Declarations
{
    struct AnyDeclaration** pItems;
    int Size;
    int Capacity;
};
#define TDECLARATIONS_INIT {0}
void Declarations_Destroy(struct Declarations* p);
void Declarations_PushBack(struct Declarations* p, struct AnyDeclaration* pItem);


struct TypeName
{
    /*
    type-name:
      specifier-qualifier-list abstract-declaratoropt
    */
    enum Type Type;
    struct SpecifierQualifierList SpecifierQualifierList;
    struct Declarator Declarator;
};

#define TYPENAME_INIT { TypeName_ID}
void TypeName_CopyTo(struct TypeName* from, struct TypeName* to);
void TypeName_Destroy(struct TypeName* p);
void TypeName_Delete(struct TypeName* p);
void TypeName_Swap(struct TypeName* a, struct TypeName* b);


struct AtomicTypeSpecifier
{
    /*
    atomic-type-specifier:
      _Atomic ( type-name )
    */
    enum Type Type;
    struct TypeName TypeName;
    struct TokenList ClueList0;
    struct TokenList ClueList1;
    struct TokenList ClueList2;
};

#define ATOMICTYPESPECIFIER_INIT {AtomicTypeSpecifier_ID}
struct AtomicTypeSpecifier* AtomicTypeSpecifier_Clone(struct AtomicTypeSpecifier* p);
void AtomicTypeSpecifier_Delete(struct AtomicTypeSpecifier* p);
bool AtomicTypeSpecifier_Compare(struct AtomicTypeSpecifier* p1, struct AtomicTypeSpecifier* p2);


bool EvaluateConstantExpression(struct Expression* p, int* pResult);

struct ParameterTypeList* TDeclaration_GetFunctionArguments(struct Declaration* p);

struct Declaration;
struct SyntaxTree;

struct Declaration* SyntaxTree_FindFunctionDeclaration(struct SyntaxTree* p, const char* name);

bool DeclarationSpecifiers_IsTypedef(struct DeclarationSpecifiers* pDeclarationSpecifiers);



struct PrimaryExpressionValue
{
    /*
    (6.5.1) primary-expression:
      identifier
      constant
      char-literal
      ( expression )
      generic-selection
    */

    enum Type Type;
    enum TokenType token;
    char* lexeme;
    struct Expression* pExpressionOpt; //( expression )
    struct Declaration* pDeclaration; //se for identificador aponta para a declaracao dele
    struct Parameter* pParameter; //se for identificador pode apontar para parametro funcao

    struct TokenList ClueList0;
    struct TokenList ClueList1;
    struct TypeName TypeName;
};

#define PRIMARYEXPRESSIONVALUE_INIT {PrimaryExpressionValue_ID}
void PrimaryExpressionValue_Destroy(struct PrimaryExpressionValue* p);
void PrimaryExpressionValue_Delete(struct PrimaryExpressionValue* p);

struct PrimaryExpressionLiteralItem
{
    struct PrimaryExpressionLiteralItem* pNext;
    char* lexeme;
    struct TokenList ClueList0;
};
#define PRIMARYEXPRESSIONLITERALITEM_INIT {0}

void PrimaryExpressionLiteralItem_Delete(struct PrimaryExpressionLiteralItem* p);

struct PrimaryExpressionLiteralItemList
{
    struct PrimaryExpressionLiteralItem* pHead, * pTail;
};
#define PRIMARYEXPRESSIONLITERALITEMLIST_INIT {0}

void PrimaryExpressionLiteralItemList_Destroy(struct PrimaryExpressionLiteralItemList* p);
void PrimaryExpressionLiteralItemList_Add(struct PrimaryExpressionLiteralItemList* p, struct PrimaryExpressionLiteralItem* pItem);


struct PrimaryExpressionLiteral
{
    enum Type Type;
    struct PrimaryExpressionLiteralItemList List;
    struct TypeName TypeName;
};

#define PRIMARYEXPRESSIONLITERAL_INIT {PrimaryExpressionLiteral_ID}
void PrimaryExpressionLiteral_Delete(struct PrimaryExpressionLiteral* p);


struct BinaryExpression
{
    enum Type Type;
    enum TokenType token;
    struct Expression* pExpressionLeft;
    struct Expression* pExpressionRight;
    struct FilePos Position;
    struct TokenList ClueList0;
    struct TypeName TypeName;
};
#define BINARYEXPRESSION_INIT {BinaryExpression_ID}
void BinaryExpression_Delete(struct BinaryExpression* p);

struct TernaryExpression
{
    enum Type Type;
    enum TokenType token;
    struct Expression* pExpressionLeft;
    struct Expression* pExpressionMiddle;
    struct Expression* pExpressionRight;
    struct TokenList ClueList0;
    struct TokenList ClueList1;
    struct TypeName TypeName;
};
#define TERNARYEXPRESSION_INIT {TernaryExpression_ID}
void TernaryExpression_Delete(struct TernaryExpression* p);

struct ArgumentExpression
{
    struct Expression* pExpression;
    bool bHasComma;
    struct TokenList ClueList0;
};
#define ARGUMENTEXPRESSION_INIT {0}
void ArgumentExpression_Delete(struct ArgumentExpression* p);

struct ArgumentExpressionList
{
    struct ArgumentExpression** data;
    int size;
    int capacity;
};

#define ARGUMENTEXPRESSIONLIST_INIT {0}
void ArgumentExpressionList_Destroy(struct ArgumentExpressionList* pArgumentExpressionList);
void ArgumentExpressionList_PushBack(struct ArgumentExpressionList* pArgumentExpressionList, struct ArgumentExpression* pItem);

struct PostfixExpression
{
    /*
    postfix-expression:
       primary-expression
       postfix-expression [ expression ]
       postfix-expression ( argument-expression-listopt )
       postfix-expression . identifier
       postfix-expression -> identifier
       postfix-expression ++
       postfix-expression --
       ( type-name ) { initializer-list }
       0           1 2                  3

       ( type-name ) { initializer-list , }
       0           1 2                  3 4
    */
    enum Type Type;
    enum TokenType token;
    char* lexeme;
    struct Expression* pExpressionLeft;
    struct Expression* pExpressionRight;
    struct ArgumentExpressionList ArgumentExpressionList;

    struct PostfixExpression* pNext;
    struct InitializerList InitializerList;
    char* Identifier;
    struct TypeName* pTypeName;

    struct TokenList ClueList0;
    struct TokenList ClueList1;
    struct TokenList ClueList2;
    struct TokenList ClueList3;
    struct TokenList ClueList4;

    struct TypeName TypeName; //(juntar com outro?)
};
#define POSTFIXEXPRESSIONCORE_INIT {PostfixExpression_ID}
void PostfixExpression_Delete(struct PostfixExpression* p);


struct CastExpressionType
{
    enum Type Type;
    struct Expression* pExpression;
    struct TypeName TypeName;
    struct TokenList ClueList0;
    struct TokenList ClueList1;
};
#define CASTEXPRESSIONTYPE_INIT {CastExpressionType_ID}
void CastExpressionType_Delete(struct CastExpressionType* p);


struct UnaryExpressionOperator
{
    enum Type Type;
    enum TokenType token;
    struct Expression* pExpressionRight;
    struct TypeName TypeName;
    struct TokenList ClueList0;
    struct TokenList ClueList1; //sizeof (
    struct TokenList ClueList2; //sizeof ( )
};

#define UNARYEXPRESSIONOPERATOR_INIT {UnaryExpressionOperator_ID}
void UnaryExpressionOperator_Delete(struct UnaryExpressionOperator* p);

struct Expression
{
    /*
      PrimaryExpressionLiteral 
      PrimaryExpressionValue 
      BinaryExpression 
      UnaryExpressionOperator 
      PostfixExpression 
      CastExpressionType 
      TernaryExpression 
      PrimaryExpressionLambda
    */

    enum Type Type;
};

struct TypeName* Expression_GetTypeName(struct Expression* p);
void Expression_Delete(struct Expression* p);

struct PrimaryExpressionLambda
{
    enum Type Type;

    struct CompoundStatement* pCompoundStatement;
    //struct ParameterTypeList* pParameterTypeListOpt;
    struct TokenList ClueList0; //[
    struct TokenList ClueList1; //]
    struct TokenList ClueList2; //(
    struct TokenList ClueList3; //)
    struct TypeName TypeName;
};
#define PRIMARYEXPRESSIONLAMBDA_INIT {PrimaryExpressionLambda_ID}
void PrimaryExpressionLambda_Delete(struct PrimaryExpressionLambda* p);


struct SyntaxTree
{
    //declaracoes
    struct Declarations Declarations;

    //arquivos na qual declaracao tem indice
    struct FileArray Files;

    struct SymbolMap GlobalScope;

    //Defines
    struct MacroMap Defines;
};

#define SYNTAXTREE_INIT {0}

void SyntaxTree_Destroy(struct SyntaxTree* p);

struct Declaration* SyntaxTree_GetFinalTypeDeclaration(struct SyntaxTree* p, const char* typeName);

struct Declaration* SyntaxTree_FindDeclaration(struct SyntaxTree* p, const char* name);

void DeclarationSpecifiers_PrintNameMangling(struct DeclarationSpecifiers* pDeclarationSpecifiers, struct StrBuilder* fp);

void ParameterList_PrintNameMangling(struct ParameterList* p, struct StrBuilder* fp);

void Parameter_PrintNameMangling(struct Parameter* p, struct StrBuilder* fp);

void GetTypeName(struct Expression* pExpression, struct StrBuilder* str);



enum CompilerTarget
{
    CompilerTarget_C99,
    CompilerTarget_Preprocessed,
    CompilerTarget_CXX
};

struct StrArray
{
    const char** data;
    int size;
    int capacity;
};

#define STRARRAY_INIT { 0 }
int StrArray_Push(struct StrArray* p, const char* textSource);

struct CompilerOptions
{
    char output[200];
    char outputDir[200];

    bool bExpandMacros;
    bool bIncludeComments /*=1*/;

    enum CompilerTarget Target;

    //imprime na forma para declarar um tipo, remove o resto
    bool bCannonical;

    struct StrArray IncludeDir;
    struct StrArray SourceFiles;
    struct StrArray Defines; /*name=1*/
};

#define OUTPUTOPTIONS_INIT {false, true, CompilerTarget_C99,  false}
void OutputOptions_Destroy(struct CompilerOptions* options);


int Compile(struct CompilerOptions* options);


char* CompileText(int type, int bNoImplicitTag, const char* input);

void PrintPreprocessedToFile(const char* fileIn, struct CompilerOptions* options);

void PrintPreprocessedToConsole(const char* fileIn, const char* configFileName);



