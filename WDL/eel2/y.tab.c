
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 1

/* Substitute the variable and function names.  */
#define yyparse         nseelparse
#define yylex           nseellex
#define yyerror         nseelerror
#define yylval          nseellval
#define yychar          nseelchar
#define yydebug         nseeldebug
#define yynerrs         nseelnerrs
#define yylloc          nseellloc

/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 13 "eel2.y"

#ifdef _WIN32
#include <windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "y.tab.h"
#include "ns-eel-int.h"
  
#define scanner context->scanner
#define YY_(x) ("")



/* Line 189 of yacc.c  */
#line 99 "y.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     VALUE = 258,
     IDENTIFIER = 259,
     FUNCTION1 = 260,
     FUNCTION2 = 261,
     FUNCTION3 = 262,
     FUNCTIONX = 263,
     TOKEN_SHL = 264,
     TOKEN_SHR = 265,
     TOKEN_LTE = 266,
     TOKEN_GTE = 267,
     TOKEN_EQ = 268,
     TOKEN_EQ_EXACT = 269,
     TOKEN_NE = 270,
     TOKEN_NE_EXACT = 271,
     TOKEN_LOGICAL_AND = 272,
     TOKEN_LOGICAL_OR = 273,
     TOKEN_ADD_OP = 274,
     TOKEN_SUB_OP = 275,
     TOKEN_MOD_OP = 276,
     TOKEN_OR_OP = 277,
     TOKEN_AND_OP = 278,
     TOKEN_XOR_OP = 279,
     TOKEN_DIV_OP = 280,
     TOKEN_MUL_OP = 281,
     TOKEN_POW_OP = 282
   };
#endif
/* Tokens.  */
#define VALUE 258
#define IDENTIFIER 259
#define FUNCTION1 260
#define FUNCTION2 261
#define FUNCTION3 262
#define FUNCTIONX 263
#define TOKEN_SHL 264
#define TOKEN_SHR 265
#define TOKEN_LTE 266
#define TOKEN_GTE 267
#define TOKEN_EQ 268
#define TOKEN_EQ_EXACT 269
#define TOKEN_NE 270
#define TOKEN_NE_EXACT 271
#define TOKEN_LOGICAL_AND 272
#define TOKEN_LOGICAL_OR 273
#define TOKEN_ADD_OP 274
#define TOKEN_SUB_OP 275
#define TOKEN_MOD_OP 276
#define TOKEN_OR_OP 277
#define TOKEN_AND_OP 278
#define TOKEN_XOR_OP 279
#define TOKEN_DIV_OP 280
#define TOKEN_MUL_OP 281
#define TOKEN_POW_OP 282




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 208 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  69
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   162

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  49
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  18
/* YYNRULES -- Number of rules.  */
#define YYNRULES  67
/* YYNRULES -- Number of states.  */
#define YYNSTATES  133

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   282

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    36,     2,     2,     2,    38,    41,     2,
      29,    30,    40,    34,    28,    35,     2,    39,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    47,    48,
      44,    33,    45,    46,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    31,     2,    32,    37,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    42,     2,    43,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     5,     9,    11,    13,    17,    22,    26,
      33,    42,    51,    53,    57,    62,    64,    68,    72,    76,
      80,    84,    88,    92,    96,   100,   104,   106,   109,   112,
     115,   117,   121,   123,   127,   129,   133,   135,   139,   141,
     145,   147,   151,   153,   157,   161,   165,   169,   173,   175,
     179,   183,   187,   191,   195,   199,   203,   207,   209,   213,
     217,   219,   225,   230,   234,   236,   240,   243
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      66,     0,    -1,    65,    -1,    65,    28,    50,    -1,     3,
      -1,     4,    -1,    29,    65,    30,    -1,     5,    29,    65,
      30,    -1,     5,    29,    30,    -1,     6,    29,    65,    28,
      65,    30,    -1,     7,    29,    65,    28,    65,    28,    65,
      30,    -1,     8,    29,    65,    28,    65,    28,    50,    30,
      -1,    51,    -1,    52,    31,    32,    -1,    52,    31,    65,
      32,    -1,    52,    -1,    52,    33,    64,    -1,    52,    19,
      64,    -1,    52,    20,    64,    -1,    52,    21,    64,    -1,
      52,    22,    64,    -1,    52,    23,    64,    -1,    52,    24,
      64,    -1,    52,    25,    64,    -1,    52,    26,    64,    -1,
      52,    27,    64,    -1,    53,    -1,    34,    54,    -1,    35,
      54,    -1,    36,    54,    -1,    54,    -1,    55,    37,    54,
      -1,    55,    -1,    56,    38,    55,    -1,    56,    -1,    57,
      39,    56,    -1,    57,    -1,    58,    40,    57,    -1,    58,
      -1,    59,    35,    58,    -1,    59,    -1,    60,    34,    59,
      -1,    60,    -1,    61,    41,    60,    -1,    61,    42,    60,
      -1,    61,    43,    60,    -1,    61,     9,    60,    -1,    61,
      10,    60,    -1,    61,    -1,    62,    44,    61,    -1,    62,
      45,    61,    -1,    62,    11,    61,    -1,    62,    12,    61,
      -1,    62,    13,    61,    -1,    62,    14,    61,    -1,    62,
      15,    61,    -1,    62,    16,    61,    -1,    62,    -1,    63,
      17,    62,    -1,    63,    18,    62,    -1,    63,    -1,    63,
      46,    64,    47,    64,    -1,    63,    46,    47,    64,    -1,
      63,    46,    64,    -1,    64,    -1,    65,    48,    64,    -1,
      65,    48,    -1,    65,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    39,    39,    40,    47,    48,    49,    53,    57,    61,
      65,    69,    76,    77,    81,    88,    89,    93,    97,   101,
     105,   109,   113,   117,   121,   125,   132,   133,   137,   141,
     148,   149,   156,   157,   164,   165,   173,   174,   182,   183,
     190,   191,   198,   199,   203,   207,   211,   215,   222,   223,
     227,   231,   235,   239,   243,   247,   251,   258,   259,   263,
     270,   271,   275,   279,   287,   288,   292,   300
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "VALUE", "IDENTIFIER", "FUNCTION1",
  "FUNCTION2", "FUNCTION3", "FUNCTIONX", "TOKEN_SHL", "TOKEN_SHR",
  "TOKEN_LTE", "TOKEN_GTE", "TOKEN_EQ", "TOKEN_EQ_EXACT", "TOKEN_NE",
  "TOKEN_NE_EXACT", "TOKEN_LOGICAL_AND", "TOKEN_LOGICAL_OR",
  "TOKEN_ADD_OP", "TOKEN_SUB_OP", "TOKEN_MOD_OP", "TOKEN_OR_OP",
  "TOKEN_AND_OP", "TOKEN_XOR_OP", "TOKEN_DIV_OP", "TOKEN_MUL_OP",
  "TOKEN_POW_OP", "','", "'('", "')'", "'['", "']'", "'='", "'+'", "'-'",
  "'!'", "'^'", "'%'", "'/'", "'*'", "'&'", "'|'", "'~'", "'<'", "'>'",
  "'?'", "':'", "';'", "$accept", "more_params", "value_thing",
  "memory_access", "assignment", "unary_expr", "pow_expr", "mod_expr",
  "div_expr", "mul_expr", "sub_expr", "add_expr", "andor_expr", "cmp_expr",
  "logical_and_or_expr", "if_else_expr", "expression", "program", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,    44,    40,
      41,    91,    93,    61,    43,    45,    33,    94,    37,    47,
      42,    38,   124,   126,    60,    62,    63,    58,    59
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    49,    50,    50,    51,    51,    51,    51,    51,    51,
      51,    51,    52,    52,    52,    53,    53,    53,    53,    53,
      53,    53,    53,    53,    53,    53,    54,    54,    54,    54,
      55,    55,    56,    56,    57,    57,    58,    58,    59,    59,
      60,    60,    61,    61,    61,    61,    61,    61,    62,    62,
      62,    62,    62,    62,    62,    62,    62,    63,    63,    63,
      64,    64,    64,    64,    65,    65,    65,    66
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     3,     1,     1,     3,     4,     3,     6,
       8,     8,     1,     3,     4,     1,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     2,     2,     2,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     3,     3,     3,     3,     1,     3,
       3,     3,     3,     3,     3,     3,     3,     1,     3,     3,
       1,     5,     4,     3,     1,     3,     2,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     4,     5,     0,     0,     0,     0,     0,     0,     0,
       0,    12,    15,    26,    30,    32,    34,    36,    38,    40,
      42,    48,    57,    60,    64,    67,     0,     0,     0,     0,
       0,     0,    27,    28,    29,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    66,     1,
       8,     0,     0,     0,     0,     6,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    13,     0,    16,    31,    33,
      35,    37,    39,    41,    46,    47,    43,    44,    45,    51,
      52,    53,    54,    55,    56,    49,    50,    58,    59,     0,
      63,    65,     7,     0,     0,     0,    14,    62,     0,     0,
       0,     0,    61,     9,     0,     0,     0,     0,     2,    10,
      11,     0,     3
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,   127,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,   128,    26
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -28
static const yytype_int8 yypact[] =
{
     103,   -28,   -28,   -25,   -12,    39,    58,   103,   103,   103,
     103,   -28,   121,   -28,   -28,     6,    52,    53,    49,    60,
      59,    28,    20,    42,   -28,    68,    97,    50,   103,   103,
     103,    48,   -28,   -28,   -28,   103,   103,   103,   103,   103,
     103,   103,   103,   103,    69,   103,   103,   103,   103,   103,
     103,   103,   103,   103,   103,   103,   103,   103,   103,   103,
     103,   103,   103,   103,   103,   103,   103,    16,   103,   -28,
     -28,    64,   -23,   -22,    14,   -28,   -28,   -28,   -28,   -28,
     -28,   -28,   -28,   -28,   -28,   -28,    51,   -28,   -28,     6,
      52,    53,    49,    60,    59,    59,    59,    59,    59,    28,
      28,    28,    28,    28,    28,    28,    28,    20,    20,   103,
      74,   -28,   -28,   103,   103,   103,   -28,   -28,   103,    70,
      18,    19,   -28,   -28,   103,   103,    72,    87,    33,   -28,
     -28,   103,   -28
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -28,    -9,   -28,   -28,   -28,    -7,    76,    71,    81,    83,
      80,    73,    98,   -17,   -28,   -27,     0,   -28
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      25,    32,    33,    34,    27,   113,   114,    31,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    28,    87,     1,
       2,     3,     4,     5,     6,    68,    68,    71,    72,    73,
      74,    57,    58,    59,    60,    61,    62,    52,    53,    88,
     110,   111,   115,    46,    86,     7,   124,   125,   107,   108,
       8,     9,    10,     1,     2,     3,     4,     5,     6,    65,
      66,   131,    68,   109,    63,    64,    68,    68,    29,    54,
      55,    56,     1,     2,     3,     4,     5,     6,    75,     7,
      70,    68,   117,   116,     8,     9,    10,    30,    67,    49,
      47,   122,    48,    51,   112,    50,    68,    69,     7,    68,
     123,    85,   129,     8,     9,    10,     1,     2,     3,     4,
       5,     6,    68,   119,   120,   121,    68,   130,    68,    90,
      68,   118,   132,    89,   126,    94,    95,    96,    97,    98,
      91,    93,     7,    92,     0,     0,     0,     8,     9,    10,
      35,    36,    37,    38,    39,    40,    41,    42,    43,     0,
       0,     0,    44,     0,    45,    99,   100,   101,   102,   103,
     104,   105,   106
};

static const yytype_int16 yycheck[] =
{
       0,     8,     9,    10,    29,    28,    28,     7,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    29,    45,     3,
       4,     5,     6,     7,     8,    48,    48,    27,    28,    29,
      30,    11,    12,    13,    14,    15,    16,     9,    10,    46,
      67,    68,    28,    37,    44,    29,    28,    28,    65,    66,
      34,    35,    36,     3,     4,     5,     6,     7,     8,    17,
      18,    28,    48,    47,    44,    45,    48,    48,    29,    41,
      42,    43,     3,     4,     5,     6,     7,     8,    30,    29,
      30,    48,   109,    32,    34,    35,    36,    29,    46,    40,
      38,   118,    39,    34,    30,    35,    48,     0,    29,    48,
      30,    32,    30,    34,    35,    36,     3,     4,     5,     6,
       7,     8,    48,   113,   114,   115,    48,    30,    48,    48,
      48,    47,   131,    47,   124,    52,    53,    54,    55,    56,
      49,    51,    29,    50,    -1,    -1,    -1,    34,    35,    36,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    -1,
      -1,    -1,    31,    -1,    33,    57,    58,    59,    60,    61,
      62,    63,    64
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,    29,    34,    35,
      36,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    29,    29,    29,
      29,    65,    54,    54,    54,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    31,    33,    37,    38,    39,    40,
      35,    34,     9,    10,    41,    42,    43,    11,    12,    13,
      14,    15,    16,    44,    45,    17,    18,    46,    48,     0,
      30,    65,    65,    65,    65,    30,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    32,    65,    64,    54,    55,
      56,    57,    58,    59,    60,    60,    60,    60,    60,    61,
      61,    61,    61,    61,    61,    61,    61,    62,    62,    47,
      64,    64,    30,    28,    28,    28,    32,    64,    47,    65,
      65,    65,    64,    30,    28,    28,    65,    50,    65,    30,
      30,    28,    50
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (&yylloc, context, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, &yylloc, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, &yylloc, scanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location, context); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, compileContext* context)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, context)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    compileContext* context;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
  YYUSE (context);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, compileContext* context)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, context)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    compileContext* context;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, context);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, compileContext* context)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, context)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    compileContext* context;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , context);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule, context); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, compileContext* context)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, context)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    compileContext* context;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (context);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {
      case 3: /* "VALUE" */

/* Line 1000 of yacc.c  */
#line 8 "eel2.y"
	{
 #define yydestruct(a,b,c,d,e) 0
};

/* Line 1000 of yacc.c  */
#line 1245 "y.tab.c"
	break;

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (compileContext* context);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (compileContext* context)
#else
int
yyparse (context)
    compileContext* context;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Location data for the lookahead symbol.  */
YYLTYPE yylloc;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.
       `yyls': related to locations.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[2];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;

#if YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 1;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);

	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
	YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:

/* Line 1455 of yacc.c  */
#line 41 "eel2.y"
    {
	  (yyval) = nseel_createMoreParametersOpcode(context,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
	}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 50 "eel2.y"
    {
	  (yyval) = (yyvsp[(2) - (3)]);
	}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 54 "eel2.y"
    {
  	  (yyval) = nseel_setCompiledFunctionCallParameters((yyvsp[(1) - (4)]), (yyvsp[(3) - (4)]), 0, 0);
	}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 58 "eel2.y"
    {
  	  (yyval) = nseel_setCompiledFunctionCallParameters((yyvsp[(1) - (3)]), nseel_createCompiledValue(context,0.0), 0, 0);
	}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 62 "eel2.y"
    {
  	  (yyval) = nseel_setCompiledFunctionCallParameters((yyvsp[(1) - (6)]), (yyvsp[(3) - (6)]), (yyvsp[(5) - (6)]), 0);
	}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 66 "eel2.y"
    {
  	  (yyval) = nseel_setCompiledFunctionCallParameters((yyvsp[(1) - (8)]), (yyvsp[(3) - (8)]), (yyvsp[(5) - (8)]), (yyvsp[(7) - (8)]));
	}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 70 "eel2.y"
    {
  	  (yyval) = nseel_setCompiledFunctionCallParameters((yyvsp[(1) - (8)]), (yyvsp[(3) - (8)]), (yyvsp[(5) - (8)]), (yyvsp[(7) - (8)]));
	}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 78 "eel2.y"
    {
	  (yyval) = nseel_createMemoryAccess(context,(yyvsp[(1) - (3)]),0);
        }
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 82 "eel2.y"
    {
	  (yyval) = nseel_createMemoryAccess(context,(yyvsp[(1) - (4)]),(yyvsp[(3) - (4)]));
        }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 90 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_ASSIGN,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 94 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_ADD_OP,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 98 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_SUB_OP,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 102 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_MOD_OP,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 106 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_OR_OP,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 110 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_AND_OP,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 114 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_XOR_OP,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 118 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_DIV_OP,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 122 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_MUL_OP,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 126 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_POW_OP,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 134 "eel2.y"
    {
	  (yyval) = (yyvsp[(2) - (2)]);
	}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 138 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_UMINUS,1,(yyvsp[(2) - (2)]),0);
	}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 142 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_NOT,1,(yyvsp[(2) - (2)]),0);
	}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 150 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_POW,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 158 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_MOD,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 166 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_DIVIDE,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
	}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 175 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_MULTIPLY,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
	}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 184 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_SUB,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
	}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 192 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_ADD,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
	}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 200 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_AND,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
	}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 204 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_OR,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
	}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 208 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_XOR,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
	}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 212 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_SHL,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 216 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_SHR,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 224 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_LT,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 228 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_GT,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 232 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_LTE,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 236 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_GTE,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 240 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_EQ,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 244 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_EQ_EXACT,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 248 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_NE,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 252 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_NE_EXACT,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 260 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_LOGICAL_AND,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 264 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_LOGICAL_OR,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
        }
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 272 "eel2.y"
    {
	  (yyval) = nseel_createIfElse(context, (yyvsp[(1) - (5)]), (yyvsp[(3) - (5)]), (yyvsp[(5) - (5)]));
        }
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 276 "eel2.y"
    {
	  (yyval) = nseel_createIfElse(context, (yyvsp[(1) - (4)]), 0, (yyvsp[(4) - (4)]));
        }
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 280 "eel2.y"
    {
	  (yyval) = nseel_createIfElse(context, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), 0);
        }
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 289 "eel2.y"
    {
	  (yyval) = nseel_createSimpleCompiledFunction(context,FN_JOIN_STATEMENTS,2,(yyvsp[(1) - (3)]),(yyvsp[(3) - (3)]));
	}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 293 "eel2.y"
    {
	  (yyval) = (yyvsp[(1) - (2)]);
	}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 301 "eel2.y"
    { 
                int a = (yylsp[(1) - (1)]).first_line;
                context->result = (yyvsp[(1) - (1)]);
	}
    break;



/* Line 1455 of yacc.c  */
#line 2018 "y.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, context, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (&yylloc, context, yymsg);
	  }
	else
	  {
	    yyerror (&yylloc, context, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc, context);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp, context);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, context, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc, context);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp, context);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 308 "eel2.y"


