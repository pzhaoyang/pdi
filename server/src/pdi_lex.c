/******************************************************************************
 *
 *  Jan 1, 2008, Provides a command line parser
 *
 *  Copyright (c) 2008-2014 Ericsson AB.
 *  All rights reserved.
 *
******************************************************************************/

typedef enum        /* states for automatic insertion of parens in yylex */ {
    FIRST_LEXEME,
    NORMAL,
    P_OPEN,
    P_OPEN_DONE,
    P_CLOSE,
    P_CLOSE_DONE
} AUTO_STATE;

static AUTO_STATE autoState;     /* state of auto parens mods */
static char *nextChar;           /* ptr to next input char in line */
static char tempStrings [PDI_MAX_LINE_LEN];/* storage for strings while parsing */
static char *nextTempString;     /* ptr to free space in tempStrings */
/* forward declaration */
static void lexNewLine (char *line);
static int yylex (void);
static char *addTempString (char *string);
static void lexError (char *string, char *errmsg);
static int getNum (char *string, char *fmtString, VALUE *pValue);
static int getFloat (char *string, VALUE *pValue);
static int getString (char *string, int nChars, VALUE *pValue);
static int getChar (char *string, int nChars, VALUE *pValue);
static int getId (char *string, VALUE *pValue);
static int typeCast (char *string);
static int strToChar (char *string, char *pChar);
static void lexInit (void);
static int lexScan (void);
static void lexRetract (void);
#define RETRACT     lexRetract (); string[--nChars] = EOS
int lexActions (int state, char *string, int nChars,BOOL* pContinue)
{
    *pContinue = FALSE;
    switch (state) {
    case 1:
        { RETRACT;} break;
    case 2:
        { RETRACT;  return(string[0]);} break;
    case 3:
        {       return(string[0]);} break;
    case 4:
        {       lexError(string, "invalid number"); return(LEX_ERROR);} break;
    case 5:
        {       lexError(string, "invalid string"); return(LEX_ERROR);} break;
    case 6:
        {       lexError(string, "invalid char");   return(LEX_ERROR);} break;
    case 7:
        { RETRACT;  return(getNum (string, "%o", &yylval));} break;
    case 8:
        { RETRACT;  return(getNum (&string[2], "%x", &yylval));} break;
    case 9:
        { RETRACT;  return(getNum (&string[1], "%x", &yylval));} break;
    case 10:
        { RETRACT;  return(getNum (string, "%d", &yylval));} break;
    case 11:
        { RETRACT;  return(getFloat (string, &yylval));} break;
    case 12:
        { RETRACT;  return(getId (string, &yylval));} break;
    case 13:
        {       return(getString (string, nChars, &yylval));} break;
    case 14:
        {       return(getChar (string, nChars, &yylval));} break;
    case 15:
        {       return(OR);} break;
    case 16:
        {       return(OP_AND);} break;
    case 17:
        {       return(EQ);} break;
    case 18:
        {       return(NE);} break;
    case 19:
        {       return(GE);} break;
    case 20:
        {       return(LE);} break;
    case 21:
        { RETRACT;  return(ROT_RIGHT);} break;
    case 22:
        { RETRACT;  return(ROT_LEFT);} break;
    case 23:
        {       return(OP_PTR);} break;
    case 24:
        {       return(INCR);} break;
    case 25:
        {       return(DECR);} break;
    case 26:
        {       return(ADDA);} break;
    case 27:
        {       return(SUBA);} break;
    case 28:
        {       return(MULA);} break;
    case 29:
        {       return(DIVA);} break;
    case 30:
        {       return(MODA);} break;
    case 31:
        {       return(SHLA);} break;
    case 32:
        {       return(SHRA);} break;
    case 33:
        {       return(ANDA);} break;
    case 34:
        {       return(ORA);} break;
    case 35:
        {       return(XORA);} break;
    case 36:
        {       return(NL);} break;
    case 37:
        {       return(ENDFILE);} break;
    }
    *pContinue = TRUE;
    return(0);
}

int lexNclasses = 27;

signed char lexClass [] =
{
    26,
    25,  0,  0,  0, 26,  0,  0,  0,  0,  1, 25,  0,  0, 25,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    1, 16, 10,  0, 12, 23, 14, 11,  0,  0, 22, 20,  0, 19,  8, 21,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  0,  0, 18, 15, 17,  0,
    0,  5,  5,  5,  5,  5,  5,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  6,  7,  7,  0,  9,  0, 24,  7,
    0,  5,  5,  5,  5,  5,  5,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  6,  7,  7,  0, 13,  0,  0,  0,
};

signed char lexStateTable [] =
{
    -3, 1, 2, 8, 8,11,11,11, 9,-3,12,14, 6,19,20,21,22,23,24,25,26,16,27,28,29,-36,-37,
    -1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -10,-10, 3, 3,-4,-4, 4,-4,10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,
    -7,-7, 3, 3,-4,-4,-4,-4,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,
    -4,-4, 5, 5, 5, 5,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,
    -8,-8, 5, 5, 5, 5,-4,-4,-4,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,
    -4,-4, 7, 7, 7, 7,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,
    -9,-9, 7, 7, 7, 7,-4,-4,-4,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,
    -10,-10, 8, 8, 8,-4,-4,-4,10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,
    -11,-11,10,10,10,-4,-4,-4,-4,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,
    -11,-11,10,10,10,-4,-4,-4,-4,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,
    -12,-12,11,11,11,11,11,11,-12,-12,-12,-12,-12,-12,-12,-12,-12,-12,-12,-12,-12,-12,-12,-12,-12,-12,-12,
    12,12,12,12,12,12,12,12,12,13,-13,12,12,12,12,12,12,12,12,12,12,12,12,12,12,-5,-5,
    12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,13,-5,-5,
    14,14,14,14,14,14,14,14,14,15,14,-14,14,14,14,14,14,14,14,14,14,14,14,14,14,-6,-6,
    14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,-6,-6,
    -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-29,-2,-2,-2,-2,-2,-2,17,-2,-2,-2,-2,
    17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,18,17,17,17,17,
    17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17, 0,17,17,17,17,17,
    -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-15,-2,-34,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,
    -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-16,-33,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,
    -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-17,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,
    -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-18,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,
    -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-19,-2,30,-2,-2,-2,-2,-2,-2,-2,-2,-2,
    -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-20,-2,-2,31,-2,-2,-2,-2,-2,-2,-2,-2,
    -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-27,-2,-23,-2,-25,-2,-2,-2,-2,-2,-2,-2,
    -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-26,-2,-2,-2,-2,-24,-2,-2,-2,-2,-2,-2,
    -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-28,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,
    -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-30,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,
    -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-35,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,
    -21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-32,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,-21,
    -22,-22,-22,-22,-22,-22,-22,-22,-22,-22,-22,-22,-22,-22,-22,-31,-22,-22,-22,-22,-22,-22,-22,-22,-22,-22,-22,
};
/******************************************************************************
* FUNCTION: lexNewLine - initialize for lexical scan of new line
*
* RETURNS: none
******************************************************************************/
static void lexNewLine( char *line )
{
    lexInit ();
    nextChar = line;
    nextTempString = tempStrings;
    autoState = FIRST_LEXEME;
}
/******************************************************************************
* FUNCTION: yylex - get next lexeme for yacc
*
* This routine is called by yacc to get the next input lexeme.
* In addition to simply calling lexScan to do the scan, this routine
* also handles the automatic insertion of parens around the arguements
* of a "top-level" routine call.  If the first lexeme scanned from a new
* line is a T_SYMBOL (text id) and the second lexeme is NOT a '(',
* then the second lexeme is withheld and a '(' returned instead.
* The withheld lexeme is returned next.  Scanning then proceeds normally
* until a NL (newline) lexeme is scanned.  The NL is withheld and a
* ')' is returned instead, with the NL being returned next.
*
* RETURNS: next lexeme.
******************************************************************************/
static int yylex (void)
{
    static int heldCode;
    FAST int code;
    switch (autoState) {
    case FIRST_LEXEME:      /* first lex scan of new line */
        code = lexScan ();
        autoState = (code == T_SYMBOL) ? P_OPEN : NORMAL;
        break;
    case NORMAL:            /* parens not required to be inserted */
        code = lexScan ();
        if (code == ';')
            autoState = FIRST_LEXEME;
        break;
    case P_OPEN:            /* looking for '(' */
        code = lexScan ();
        if (code == '(')
            autoState = NORMAL;
        else {
            heldCode = code;
            code = '(';
            autoState = P_OPEN_DONE;
        }
        break;
    case P_OPEN_DONE:       /* artificial '(' has been returned */
        if ((heldCode == NL) || (heldCode == ';')) {
            code = ')';
            autoState = P_CLOSE_DONE;
        } else {
            code = heldCode;
            autoState = P_CLOSE;
        }
        break;
    case P_CLOSE:           /* looking for NL or ';' */
        code = lexScan ();
        if ((code == NL) || (code == ';')) {
            heldCode = code;
            code = ')';
            autoState = P_CLOSE_DONE;
        }
        break;
    case P_CLOSE_DONE:      /* artificial ')' has been returned */
        code = heldCode;
        autoState = FIRST_LEXEME;
        break;
    default:
        printf ("yylex: invalid state %#x\n", autoState);
        code = 0;   /* invalid? */
        break;
    }
    return(code);
}
/******************************************************************************
* FUNCTION: addTempString - add string to temporary storage
*
* This routine adds the specified string to the during-parse temporary
* string storage.
*
* RETURNS: pointer to new string appended to temporary area.
******************************************************************************/
static char *addTempString( char *string )
{
    char *newString = nextTempString;
    while (*string != EOS)
        string += strToChar (string, nextTempString++);
    *(nextTempString++) = EOS;
    return(newString);
}
/******************************************************************************
* FUNCTION: lexError - report error in lex scan
*
* RETURNS: none
******************************************************************************/
static void lexError( char *string, char *errmsg )
{
    printf ("%s: %s\n", errmsg, string);
}
/******************************************************************************
* FUNCTION: getNum - interpret scanned string as integer
*
* RETURNS: NUMBER
******************************************************************************/
static int getNum( char *string, char *fmtString, VALUE *pValue )
{
    int val;
    pValue->side = RHS;
    pValue->type = T_INT;
    sscanf (string, fmtString, &val);
    pValue->value.rv = (intptr_t) val;
    return(NUMBER);
}
/******************************************************************************
* FUNCTION: getFloat - interpret scanned string as float
*
* RETURNS: FLOAT
******************************************************************************/
static int getFloat( char *string, VALUE *pValue )
{
#ifndef PDI_NO_FP
    pValue->side = RHS;
    pValue->type = T_DOUBLE;
    sscanf (string, "%lf", &pValue->value.dp);
#endif  /* PDI_NO_FP */
    return(FLOAT);
}
/******************************************************************************
* FUNCTION: getString - interpret scanned string as quoted string
*
* RETURNS: STRING
******************************************************************************/
static int getString( char *string, int nChars, VALUE *pValue )
{
    pValue->side = RHS;
    pValue->type = T_INT;
    string [nChars - 1] = EOS;
    pValue->value.rv = (intptr_t)addTempString (&string[1]);
    return(STRING);
}
/******************************************************************************
* FUNCTION: getChar - interpret scanned string as quoted character
*
* RETURNS: CHAR
******************************************************************************/
static int getChar( char *string, int nChars, VALUE *pValue )
{
    char ch;
    int n = strToChar (&string [1], &ch);
    if (nChars != (n + 2)) {
        lexError (string, "invalid char");
        return(LEX_ERROR);
    }
    pValue->side       = RHS;
    pValue->type       = T_BYTE;
    pValue->value.byte = ch;
    return(CHAR);
}

/******************************************************************************
* FUNCTION: getId - interpret scanned string as identifier or keyword
*
* RETURNS: TYPECAST, {T,D,U}_SYMBOL
******************************************************************************/
static int getId( char *string, FAST VALUE *pValue )
{
    char     tempString [PDI_MAX_LINE_LEN + 1];
    SYM_TYPE type;
    char     *value;
    int      t = typeCast (string);
    if (t != PDI_ERROR) {
        pValue->type = (TYPE)t;
        return(TYPECAST);
    }
    tempString[0] = '_';
    strncpy (&tempString[1], string, PDI_MAX_LINE_LEN);
    tempString [PDI_MAX_LINE_LEN] = EOS;
    if ((pdi_sym_find_by_name (pdi_sym_tbl_id, &tempString[1], &value, &type) == PDI_OK) ||
        (pdi_sym_find_by_name (pdi_sym_tbl_id, &tempString[0], &value, &type) == PDI_OK)) {
        pValue->value.lv = (intptr_t *) value;
        pValue->type     = T_INT;
        pValue->side     = LHS;
        if ((type & 0xe) == N_TEXT) /* only need to check three bits of type*/
            return(T_SYMBOL);
        else
            return(D_SYMBOL);
    }
    /* check for mangled C++ names; return unique match or nothing */

    if (pdi_cplus_match_mangled(pdi_sym_tbl_id, string, &type,
                          (int *) &pValue->value.lv)) {
        pValue->type = T_INT;
        pValue->side = LHS;
        if ((type & N_TYPE) == N_TEXT)
            return T_SYMBOL;
        else
            return D_SYMBOL;
    }
    /* identifier not found */

    pValue->side = RHS;
    pValue->type = T_UNKNOWN;
    pValue->value.rv = (intptr_t)addTempString (string);
    return(U_SYMBOL);
}

/******************************************************************************
* FUNCTION: typeCast - determine if string is a keyword type cast
*
* RETURNS: T_{BYTE,WORD,INT,FLOAT,DOUBLE}, or PDI_ERROR
******************************************************************************/
static int typeCast( FAST char *string )
{
    static char *typen [] =
#ifndef PDI_NO_FP
    {"char", "short", "int", "long", "float", "double"};
#else   /* PDI_NO_FP */
    {"char", "short", "int", "long"};
#endif  /* PDI_NO_FP */
    static TYPE  typet [] =
#ifndef PDI_NO_FP
    {T_BYTE, T_WORD, T_INT, T_INT, T_FLOAT, T_DOUBLE};
#else   /* PDI_NO_FP */
    {T_BYTE, T_WORD, T_INT, T_INT};
#endif  /* PDI_NO_FP */
    FAST uint32 ix;
    for (ix = 0; ix < NELEMENTS (typet); ix++) {
        if (strcmp (string, typen [ix]) == 0)
            return((int)typet [ix]);
    }
    return(PDI_ERROR);
}
/******************************************************************************
* FUNCTION: strToChar - get a possibly escaped character from a string
*
* RETURNS: number of characters parsed and character in <pChar>.
******************************************************************************/
static int strToChar( FAST char *string, char *pChar )
{
    FAST int nchars = 1;
    int num;
    FAST char ch;
    if (*string != '\\') {
        *pChar = *string;
        return(nchars);
    }
    string++;
    if ((*string >= '0') && (*string <= '7')) {
        sscanf (string, "%o", &num);
        ch = num % 0400;
        while ((*string >= '0') && (*string <= '7')) {
            ++string;
            ++nchars;
        }
    } else {
        nchars++;
        switch (*string) {
        case 'n':  ch = '\n'; break;
        case 't':  ch = '\t'; break;
        case 'b':  ch = '\b'; break;
        case 'r':  ch = '\r'; break;
        case 'f':  ch = '\f'; break;
        case '\\': ch = '\\'; break;
        case '\'': ch = '\''; break;
        case '"':  ch = '"'; break;
        case 'a':  ch = (char)0x07; break;
        case 'v':  ch = (char)0x0b; break;
        default:   ch = *string; break;
        }
    }
    *pChar = ch;
    return(nchars);
}
/* lexeme scan routines */
#define EMPTY       -2
static int retractChar;
static int lastChar;
/******************************************************************************
* FUNCTION: lexInit - initialize lex scan routines
*
* RETURNS: none
******************************************************************************/
static void lexInit (void)
{
    retractChar = EMPTY;
}
/******************************************************************************
* FUNCTION: lexScan - scan input for next lexeme
*
* RETURNS: next lexeme.
******************************************************************************/
static int lexScan (void)
{
    FAST int ch;
    FAST int state;
    int nChars;
    int code;
    BOOL scanContinue;
    char string [PDI_MAX_LINE_LEN + 1];
    do {
        /* get first character; use any retracted character first */
        if (retractChar != EMPTY) {
            ch = retractChar;
            retractChar = EMPTY;
        } else
            ch = *(nextChar++);
        /* consume characters until final state reached */
        state = 0;
        for (nChars = 0; nChars < PDI_MAX_LINE_LEN; nChars++) {
            /* consume character and make state transition */
            string [nChars] = ch;
            state = lexStateTable [state * lexNclasses + lexClass [ch + 1]];
            /* if final state reached, quit; otherwise get next character */
            if (state < 0) {
                nChars++;
                break;
            }
            ch = *(nextChar++);
        }
        /* final state reached */
        state = -state;
        string [nChars] = EOS;
        lastChar = ch;
        code = lexActions (state, string, nChars, &scanContinue);
    }
    while (scanContinue);
    return(code);
}
/******************************************************************************
* FUNCTION: lexRetract - retract last character consumed
*
* RETURNS: none
******************************************************************************/
static void lexRetract (void)
{
    retractChar = lastChar;
}
