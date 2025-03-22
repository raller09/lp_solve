/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* All symbols defined below should begin with lpt_yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum lpt_yytokentype {
     VAR = 258,
     CONS = 259,
     INTCONS = 260,
     VARIABLECOLON = 261,
     INF = 262,
     FRE = 263,
     SEC_INT = 264,
     SEC_SEC = 265,
     SEC_SOS = 266,
     SOSTYPE = 267,
     TOK_SIGN = 268,
     RE_OPEQ = 269,
     RE_OPLE = 270,
     RE_OPGE = 271,
     MINIMISE = 272,
     MAXIMISE = 273,
     SUBJECTTO = 274,
     BOUNDS = 275,
     END = 276,
     UNDEFINED = 277
   };
#endif
/* Tokens.  */
#define VAR 258
#define CONS 259
#define INTCONS 260
#define VARIABLECOLON 261
#define INF 262
#define FRE 263
#define SEC_INT 264
#define SEC_SEC 265
#define SEC_SOS 266
#define SOSTYPE 267
#define TOK_SIGN 268
#define RE_OPEQ 269
#define RE_OPLE 270
#define RE_OPGE 271
#define MINIMISE 272
#define MAXIMISE 273
#define SUBJECTTO 274
#define BOUNDS 275
#define END 276
#define UNDEFINED 277




/* Copy the first part of user declarations.  */


#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define scanner lpt_yyscanner
#define PARM lpt_yyget_extra(lpt_yyscanner)
#define YYSTYPE int
#define YY_EXTRA_TYPE parse_parm *
#define YY_FATAL_ERROR(msg) lex_fatal_error(PARM, lpt_yyscanner, msg)
#undef YY_INPUT
#define lpt_yyerror read_error

#include "lpkit.h"
#include "yacc_read.h"

typedef struct parse_vars_s
{
  char HadVar, HadConstraint, Had_lineair_sum, HadSign, OP, Sign, isign, isign0, make_neg, objconst;
  char Within_gen_decl;  /* TRUE when we are within an gen declaration */
  char Within_bin_decl;  /* TRUE when we are within an bin declaration */
  char Within_sec_decl;  /* TRUE when we are within a sec declaration */
  char Within_sos_decl;  /* TRUE when we are within a sos declaration */
  short SOStype;         /* SOS type */
  int SOSNr;
  int SOSweight;         /* SOS weight */
  int weight;
  char *Last_var;
  REAL f, f0, f1, f2;
} parse_vars;

#ifdef FORTIFY
# include "lp_fortify.h"
#endif

/* let's please C++ users */
#ifdef __cplusplus
extern "C" {
#endif

#if defined MSDOS || defined __MSDOS__ || defined WINDOWS || defined _WINDOWS || defined WIN32 || defined _WIN32
#define YY_NO_UNISTD_H

static int isatty(int f)
{
  return(FALSE);
}

#if !defined _STDLIB_H
# define _STDLIB_H
#endif
#endif

#ifdef __cplusplus
};
#endif

#include "lp_rlpt.inc"

#undef lpt_yylval



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

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define lpt_yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */


#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 lpt_yytype_uint8;
#else
typedef unsigned char lpt_yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 lpt_yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char lpt_yytype_int8;
#else
typedef short int lpt_yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 lpt_yytype_uint16;
#else
typedef unsigned short int lpt_yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 lpt_yytype_int16;
#else
typedef short int lpt_yytype_int16;
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
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined lpt_yyoverflow || YYERROR_VERBOSE

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
#endif /* ! defined lpt_yyoverflow || YYERROR_VERBOSE */


#if (! defined lpt_yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union lpt_yyalloc
{
  lpt_yytype_int16 lpt_yyss;
  YYSTYPE lpt_yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union lpt_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (lpt_yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

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
	  YYSIZE_T lpt_yyi;				\
	  for (lpt_yyi = 0; lpt_yyi < (Count); lpt_yyi++)	\
	    (To)[lpt_yyi] = (From)[lpt_yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T lpt_yynewbytes;						\
	YYCOPY (&lpt_yyptr->Stack, Stack, lpt_yysize);				\
	Stack = &lpt_yyptr->Stack;						\
	lpt_yynewbytes = lpt_yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	lpt_yyptr += lpt_yynewbytes / sizeof (*lpt_yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   149

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  23
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  74
/* YYNRULES -- Number of rules.  */
#define YYNRULES  110
/* YYNRULES -- Number of states.  */
#define YYNSTATES  151

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   277

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? lpt_yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const lpt_yytype_uint8 lpt_yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21,    22
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const lpt_yytype_uint16 lpt_yyprhs[] =
{
       0,     0,     3,     4,     5,    12,    13,    16,    19,    22,
      24,    25,    29,    31,    33,    35,    37,    40,    41,    42,
      47,    49,    52,    54,    56,    57,    61,    63,    65,    67,
      70,    72,    73,    77,    78,    84,    86,    88,    90,    93,
      94,    99,   104,   106,   108,   110,   113,   115,   117,   120,
     122,   124,   126,   129,   130,   135,   136,   137,   138,   139,
     140,   152,   153,   158,   159,   160,   166,   168,   169,   174,
     176,   178,   180,   182,   184,   186,   190,   192,   195,   197,
     200,   202,   204,   205,   209,   211,   213,   215,   218,   219,
     223,   225,   227,   229,   232,   233,   237,   239,   241,   243,
     246,   249,   251,   253,   255,   258,   259,   263,   265,   267,
     269
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const lpt_yytype_int8 lpt_yyrhs[] =
{
      25,     0,    -1,    -1,    -1,    26,    27,    39,    54,    74,
      96,    -1,    -1,    28,    29,    -1,    18,    30,    -1,    17,
      30,    -1,    32,    -1,    -1,     6,    31,    32,    -1,    33,
      -1,    24,    -1,    34,    -1,    35,    -1,    34,    35,    -1,
      -1,    -1,    36,    72,    37,    38,    -1,    70,    -1,    95,
      73,    -1,    24,    -1,    40,    -1,    -1,    19,    41,    42,
      -1,    24,    -1,    43,    -1,    44,    -1,    43,    44,    -1,
      46,    -1,    -1,     6,    45,    46,    -1,    -1,    48,    52,
      47,    53,    71,    -1,    24,    -1,    49,    -1,    50,    -1,
      49,    50,    -1,    -1,    72,    95,    51,    73,    -1,    72,
      70,    95,    73,    -1,    14,    -1,    15,    -1,    16,    -1,
      72,    70,    -1,     7,    -1,    24,    -1,    20,    55,    -1,
      24,    -1,    56,    -1,    57,    -1,    56,    57,    -1,    -1,
      95,    58,    73,    64,    -1,    -1,    -1,    -1,    -1,    -1,
      53,    59,    52,    60,    95,    61,    73,    62,    71,    63,
      68,    -1,    -1,    52,    65,    53,    71,    -1,    -1,    -1,
       8,    66,    71,    67,    71,    -1,    24,    -1,    -1,    52,
      69,    53,    71,    -1,     5,    -1,     4,    -1,    24,    -1,
      24,    -1,    13,    -1,    24,    -1,    80,    84,    88,    -1,
      77,    -1,    75,    77,    -1,    78,    -1,    76,    78,    -1,
      95,    -1,    95,    -1,    -1,     6,    79,     5,    -1,    24,
      -1,    81,    -1,    82,    -1,    81,    82,    -1,    -1,     9,
      83,    75,    -1,    24,    -1,    85,    -1,    86,    -1,    85,
      86,    -1,    -1,    10,    87,    75,    -1,    24,    -1,    89,
      -1,    90,    -1,    89,    90,    -1,    11,    91,    -1,    24,
      -1,    92,    -1,    93,    -1,    92,    93,    -1,    -1,    12,
      94,    76,    -1,     3,    -1,     8,    -1,    24,    -1,    21,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const lpt_yytype_uint16 lpt_yyrline[] =
{
       0,    92,    92,    96,    96,   129,   129,   140,   145,   152,
     154,   153,   165,   185,   186,   189,   190,   195,   202,   195,
     213,   225,   261,   262,   266,   265,   281,   282,   285,   286,
     290,   292,   291,   305,   303,   329,   336,   339,   340,   346,
     344,   353,   359,   359,   359,   362,   364,   392,   393,   397,
     398,   401,   402,   407,   406,   417,   425,   434,   442,   450,
     416,   471,   470,   494,   505,   493,   533,   535,   534,   559,
     559,   562,   576,   583,   593,   614,   619,   620,   624,   625,
     629,   638,   649,   648,   666,   667,   670,   671,   676,   675,
     686,   687,   690,   691,   696,   695,   707,   708,   711,   712,
     716,   721,   722,   726,   727,   733,   732,   773,   773,   778,
     779
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const lpt_yytname[] =
{
  "$end", "error", "$undefined", "VAR", "CONS", "INTCONS",
  "VARIABLECOLON", "INF", "FRE", "SEC_INT", "SEC_SEC", "SEC_SOS",
  "SOSTYPE", "TOK_SIGN", "RE_OPEQ", "RE_OPLE", "RE_OPGE", "MINIMISE",
  "MAXIMISE", "SUBJECTTO", "BOUNDS", "END", "UNDEFINED", "$accept",
  "EMPTY", "inputfile", "@1", "objective_function", "@2",
  "objective_function1", "of", "@3", "real_of", "of_lineair_sum",
  "of_lineair_sum1", "of_lineair_term", "@4", "@5", "of_lineair_term1",
  "constraints", "constraints1", "@6", "constraints2", "constraints3",
  "constraint", "@7", "real_constraint", "@8", "x_lineair_sum",
  "lineair_sum", "lineair_term", "@9", "RE_OP", "cons_term", "bounds",
  "x_bounds", "x_bounds1", "bound", "@10", "@11", "@12", "@13", "@14",
  "@15", "bound2", "@16", "@17", "@18", "optionalbound", "@19", "REALCONS",
  "RHS_STORE", "x_SIGN", "VAR_STORE", "int_sec_sos_declarations",
  "VARIABLES", "SOSVARIABLES", "ONEVARIABLE", "ONESOSVARIABLE", "@20",
  "x_int_declarations", "int_declarations", "int_declaration", "@21",
  "x_sec_declarations", "sec_declarations", "sec_declaration", "@22",
  "x_sos_declarations", "sos_declarations", "sos_declaration",
  "x_single_sos_declarations", "single_sos_declarations",
  "single_sos_declaration", "@23", "VARIABLE", "end", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const lpt_yytype_uint16 lpt_yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const lpt_yytype_uint8 lpt_yyr1[] =
{
       0,    23,    24,    26,    25,    28,    27,    29,    29,    30,
      31,    30,    32,    33,    33,    34,    34,    36,    37,    35,
      38,    38,    39,    39,    41,    40,    42,    42,    43,    43,
      44,    45,    44,    47,    46,    48,    48,    49,    49,    51,
      50,    50,    52,    52,    52,    53,    53,    54,    54,    55,
      55,    56,    56,    58,    57,    59,    60,    61,    62,    63,
      57,    65,    64,    66,    67,    64,    68,    69,    68,    70,
      70,    71,    72,    72,    73,    74,    75,    75,    76,    76,
      77,    78,    79,    78,    80,    80,    81,    81,    83,    82,
      84,    84,    85,    85,    87,    86,    88,    88,    89,    89,
      90,    91,    91,    92,    92,    94,    93,    95,    95,    96,
      96
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const lpt_yytype_uint8 lpt_yyr2[] =
{
       0,     2,     0,     0,     6,     0,     2,     2,     2,     1,
       0,     3,     1,     1,     1,     1,     2,     0,     0,     4,
       1,     2,     1,     1,     0,     3,     1,     1,     1,     2,
       1,     0,     3,     0,     5,     1,     1,     1,     2,     0,
       4,     4,     1,     1,     1,     2,     1,     1,     2,     1,
       1,     1,     2,     0,     4,     0,     0,     0,     0,     0,
      11,     0,     4,     0,     0,     5,     1,     0,     4,     1,
       1,     1,     1,     1,     1,     3,     1,     2,     1,     2,
       1,     1,     0,     3,     1,     1,     1,     2,     0,     3,
       1,     1,     1,     2,     0,     3,     1,     1,     1,     2,
       2,     1,     1,     1,     2,     0,     3,     1,     1,     1,
       1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const lpt_yytype_uint8 lpt_yydefact[] =
{
       3,     0,     5,     1,     2,     0,    24,    22,     2,    23,
       2,     2,     6,     2,     2,    47,     2,    10,    13,     8,
       9,    12,    14,    15,     2,     7,    31,    73,    26,    25,
       2,    28,    30,     0,     2,    37,     0,   107,    46,   108,
      49,    55,    48,    50,    51,     0,    53,    88,    84,     2,
       2,    85,    86,     2,    16,    72,    18,     2,    72,    29,
      42,    43,    44,    33,    38,    70,    69,     0,    39,     0,
      52,    45,     2,     0,   110,   109,     4,    94,    90,     2,
      91,    92,    87,    11,     0,    32,     2,     2,     2,    56,
      74,     0,    89,    76,    80,     0,     2,    96,    75,    97,
      98,    93,    19,    20,     2,     2,    41,    40,     0,    63,
      61,    54,    77,    95,   105,   101,   100,   102,   103,    99,
      21,    71,    34,    57,     2,     2,     0,   104,     2,    64,
       2,    82,   106,    78,    81,    58,     2,    62,     0,    79,
       2,    65,    83,    59,     2,    66,    67,    60,     2,     2,
      68
};

/* YYDEFGOTO[NTERM-NUM].  */
static const lpt_yytype_int16 lpt_yydefgoto[] =
{
      -1,    55,     1,     2,     4,     5,    12,    19,    53,    20,
      21,    22,    23,    24,    84,   102,     8,     9,    13,    29,
      30,    31,    57,    32,    86,    33,    34,    35,    88,    63,
      41,    16,    42,    43,    44,    72,    69,   108,   128,   140,
     144,   111,   125,   124,   136,   147,   148,    67,   122,    45,
      91,    49,    92,   132,    93,   133,   138,    50,    51,    52,
      73,    79,    80,    81,    95,    98,    99,   100,   116,   117,
     118,   126,    94,    76
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -109
static const lpt_yytype_int8 lpt_yypact[] =
{
    -109,    17,  -109,  -109,    16,     3,  -109,  -109,    19,  -109,
      83,    83,  -109,     5,   102,  -109,    34,  -109,  -109,  -109,
    -109,  -109,    77,  -109,    38,  -109,  -109,  -109,    58,  -109,
      27,  -109,  -109,   119,   114,  -109,   113,  -109,  -109,  -109,
      55,  -109,  -109,    99,  -109,    74,  -109,  -109,  -109,    46,
      59,    34,  -109,    77,  -109,  -109,  -109,    38,   123,  -109,
    -109,  -109,  -109,  -109,  -109,  -109,  -109,    90,  -109,   119,
    -109,  -109,  -109,    90,  -109,  -109,  -109,  -109,  -109,    65,
      59,  -109,  -109,  -109,   113,  -109,    57,  -109,  -109,  -109,
    -109,    42,    90,  -109,  -109,    90,    96,  -109,  -109,    65,
    -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,    90,  -109,
    -109,  -109,  -109,    90,  -109,  -109,  -109,    96,  -109,  -109,
    -109,  -109,  -109,  -109,  -109,    57,    91,  -109,  -109,  -109,
    -109,  -109,    91,  -109,  -109,  -109,  -109,  -109,   106,  -109,
    -109,  -109,  -109,  -109,   119,  -109,  -109,  -109,    57,  -109,
    -109
};

/* YYPGOTO[NTERM-NUM].  */
static const lpt_yytype_int8 lpt_yypgoto[] =
{
    -109,    -4,  -109,  -109,  -109,  -109,  -109,   103,  -109,    69,
    -109,  -109,   101,  -109,  -109,  -109,  -109,  -109,  -109,  -109,
    -109,    95,  -109,    84,  -109,  -109,  -109,    97,  -109,   -67,
     -83,  -109,  -109,  -109,   100,  -109,  -109,  -109,  -109,  -109,
    -109,  -109,  -109,  -109,  -109,  -109,  -109,   -40,  -108,    -5,
     -73,  -109,    47,  -109,   -79,    12,  -109,  -109,  -109,    98,
    -109,  -109,  -109,    66,  -109,  -109,  -109,    48,  -109,  -109,
      31,  -109,   -13,  -109
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -73
static const lpt_yytype_int16 lpt_yytable[] =
{
       7,    46,    89,   105,    15,    71,    18,    18,    36,    28,
      40,    26,    48,   112,   106,   107,   129,     3,    27,    56,
      10,    11,   137,    68,   110,    36,    58,   -27,   141,    36,
      46,   120,   143,    26,   112,     6,   -27,   -27,   -27,    14,
      27,   150,   130,    47,   103,    75,    78,   -27,   -27,    18,
     109,    27,    36,    58,    87,   135,    60,    61,    62,   -72,
     -72,   -72,   -72,   -72,    38,   149,   -72,    74,    90,    77,
      27,   104,   -35,   -35,   -35,    97,    96,   146,    65,    66,
     -17,   -17,   -17,    90,    90,   -17,   -17,   -17,   -17,    17,
     -17,   -17,   115,    37,    37,   123,   -17,   131,    39,    39,
      90,   121,    37,    -2,    -2,    37,    38,    39,   114,    38,
      39,   142,    27,   134,    25,    27,    37,    65,    66,   134,
     121,    39,    83,    54,    90,    59,   121,    27,   -36,   -36,
     -36,    64,   121,    60,    61,    62,   121,   -35,   -35,   -35,
     145,    85,   113,    70,   139,   121,   101,   119,   127,    82
};

static const lpt_yytype_uint8 lpt_yycheck[] =
{
       4,    14,    69,    86,     8,    45,    10,    11,    13,    13,
      14,     6,    16,    92,    87,    88,   124,     0,    13,    24,
      17,    18,   130,    36,    91,    30,    30,     0,   136,    34,
      43,   104,   140,     6,   113,    19,     9,    10,    11,    20,
      13,   149,   125,     9,    84,    49,    50,    20,    21,    53,
       8,    13,    57,    57,    67,   128,    14,    15,    16,     4,
       5,     3,     4,     5,     7,   148,     8,    21,    72,    10,
      13,    84,    14,    15,    16,    79,    11,   144,     4,     5,
       3,     4,     5,    87,    88,     8,     3,     4,     5,     6,
      13,     8,    96,     3,     3,   108,    13,     6,     8,     8,
     104,   105,     3,     4,     5,     3,     7,     8,    12,     7,
       8,     5,    13,   126,    11,    13,     3,     4,     5,   132,
     124,     8,    53,    22,   128,    30,   130,    13,    14,    15,
      16,    34,   136,    14,    15,    16,   140,    14,    15,    16,
     144,    57,    95,    43,   132,   149,    80,    99,   117,    51
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const lpt_yytype_uint8 lpt_yystos[] =
{
       0,    25,    26,     0,    27,    28,    19,    24,    39,    40,
      17,    18,    29,    41,    20,    24,    54,     6,    24,    30,
      32,    33,    34,    35,    36,    30,     6,    13,    24,    42,
      43,    44,    46,    48,    49,    50,    72,     3,     7,     8,
      24,    53,    55,    56,    57,    72,    95,     9,    24,    74,
      80,    81,    82,    31,    35,    24,    72,    45,    24,    44,
      14,    15,    16,    52,    50,     4,     5,    70,    95,    59,
      57,    70,    58,    83,    21,    24,    96,    10,    24,    84,
      85,    86,    82,    32,    37,    46,    47,    95,    51,    52,
      24,    73,    75,    77,    95,    87,    11,    24,    88,    89,
      90,    86,    38,    70,    95,    53,    73,    73,    60,     8,
      52,    64,    77,    75,    12,    24,    91,    92,    93,    90,
      73,    24,    71,    95,    66,    65,    94,    93,    61,    71,
      53,     6,    76,    78,    95,    73,    67,    71,    79,    78,
      62,    71,     5,    71,    63,    24,    52,    68,    69,    53,
      71
};

#define lpt_yyerrok		(lpt_yyerrstatus = 0)
#define lpt_yyclearin	(lpt_yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto lpt_yyacceptlab
#define YYABORT		goto lpt_yyabortlab
#define YYERROR		goto lpt_yyerrorlab


/* Like YYERROR except do call lpt_yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto lpt_yyerrlab

#define YYRECOVERING()  (!!lpt_yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (lpt_yychar == YYEMPTY && lpt_yylen == 1)				\
    {								\
      lpt_yychar = (Token);						\
      lpt_yylval = (Value);						\
      lpt_yytoken = YYTRANSLATE (lpt_yychar);				\
      YYPOPSTACK (1);						\
      goto lpt_yybackup;						\
    }								\
  else								\
    {								\
      lpt_yyerror (parm, scanner, YY_("syntax error: cannot back up")); \
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


/* YYLEX -- calling `lpt_yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX lpt_yylex (&lpt_yylval, YYLEX_PARAM)
#else
# define YYLEX lpt_yylex (&lpt_yylval, scanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (lpt_yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (lpt_yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      lpt_yy_symbol_print (stderr,						  \
		  Type, Value, parm, scanner); \
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
lpt_yy_symbol_value_print (FILE *lpt_yyoutput, int lpt_yytype, YYSTYPE const * const lpt_yyvaluep, parse_parm *parm, void *scanner)
#else
static void
lpt_yy_symbol_value_print (lpt_yyoutput, lpt_yytype, lpt_yyvaluep, parm, scanner)
    FILE *lpt_yyoutput;
    int lpt_yytype;
    YYSTYPE const * const lpt_yyvaluep;
    parse_parm *parm;
    void *scanner;
#endif
{
  if (!lpt_yyvaluep)
    return;
  YYUSE (parm);
  YYUSE (scanner);
# ifdef YYPRINT
  if (lpt_yytype < YYNTOKENS)
    YYPRINT (lpt_yyoutput, lpt_yytoknum[lpt_yytype], *lpt_yyvaluep);
# else
  YYUSE (lpt_yyoutput);
# endif
  switch (lpt_yytype)
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
lpt_yy_symbol_print (FILE *lpt_yyoutput, int lpt_yytype, YYSTYPE const * const lpt_yyvaluep, parse_parm *parm, void *scanner)
#else
static void
lpt_yy_symbol_print (lpt_yyoutput, lpt_yytype, lpt_yyvaluep, parm, scanner)
    FILE *lpt_yyoutput;
    int lpt_yytype;
    YYSTYPE const * const lpt_yyvaluep;
    parse_parm *parm;
    void *scanner;
#endif
{
  if (lpt_yytype < YYNTOKENS)
    YYFPRINTF (lpt_yyoutput, "token %s (", lpt_yytname[lpt_yytype]);
  else
    YYFPRINTF (lpt_yyoutput, "nterm %s (", lpt_yytname[lpt_yytype]);

  lpt_yy_symbol_value_print (lpt_yyoutput, lpt_yytype, lpt_yyvaluep, parm, scanner);
  YYFPRINTF (lpt_yyoutput, ")");
}

/*------------------------------------------------------------------.
| lpt_yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
lpt_yy_stack_print (lpt_yytype_int16 *bottom, lpt_yytype_int16 *top)
#else
static void
lpt_yy_stack_print (bottom, top)
    lpt_yytype_int16 *bottom;
    lpt_yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (lpt_yydebug)							\
    lpt_yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
lpt_yy_reduce_print (YYSTYPE *lpt_yyvsp, int lpt_yyrule, parse_parm *parm, void *scanner)
#else
static void
lpt_yy_reduce_print (lpt_yyvsp, lpt_yyrule, parm, scanner)
    YYSTYPE *lpt_yyvsp;
    int lpt_yyrule;
    parse_parm *parm;
    void *scanner;
#endif
{
  int lpt_yynrhs = lpt_yyr2[lpt_yyrule];
  int lpt_yyi;
  unsigned long int lpt_yylno = lpt_yyrline[lpt_yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     lpt_yyrule - 1, lpt_yylno);
  /* The symbols being reduced.  */
  for (lpt_yyi = 0; lpt_yyi < lpt_yynrhs; lpt_yyi++)
    {
      fprintf (stderr, "   $%d = ", lpt_yyi + 1);
      lpt_yy_symbol_print (stderr, lpt_yyrhs[lpt_yyprhs[lpt_yyrule] + lpt_yyi],
		       &(lpt_yyvsp[(lpt_yyi + 1) - (lpt_yynrhs)])
		       		       , parm, scanner);
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (lpt_yydebug)				\
    lpt_yy_reduce_print (lpt_yyvsp, Rule, parm, scanner); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int lpt_yydebug;
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

# ifndef lpt_yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define lpt_yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
lpt_yystrlen (const char *lpt_yystr)
#else
static YYSIZE_T
lpt_yystrlen (lpt_yystr)
    const char *lpt_yystr;
#endif
{
  YYSIZE_T lpt_yylen;
  for (lpt_yylen = 0; lpt_yystr[lpt_yylen]; lpt_yylen++)
    continue;
  return lpt_yylen;
}
#  endif
# endif

# ifndef lpt_yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define lpt_yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
lpt_yystpcpy (char *lpt_yydest, const char *lpt_yysrc)
#else
static char *
lpt_yystpcpy (lpt_yydest, lpt_yysrc)
    char *lpt_yydest;
    const char *lpt_yysrc;
#endif
{
  char *lpt_yyd = lpt_yydest;
  const char *lpt_yys = lpt_yysrc;

  while ((*lpt_yyd++ = *lpt_yys++) != '\0')
    continue;

  return lpt_yyd - 1;
}
#  endif
# endif

# ifndef lpt_yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for lpt_yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from lpt_yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
lpt_yytnamerr (char *lpt_yyres, const char *lpt_yystr)
{
  if (*lpt_yystr == '"')
    {
      YYSIZE_T lpt_yyn = 0;
      char const *lpt_yyp = lpt_yystr;

      for (;;)
	switch (*++lpt_yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++lpt_yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (lpt_yyres)
	      lpt_yyres[lpt_yyn] = *lpt_yyp;
	    lpt_yyn++;
	    break;

	  case '"':
	    if (lpt_yyres)
	      lpt_yyres[lpt_yyn] = '\0';
	    return lpt_yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! lpt_yyres)
    return lpt_yystrlen (lpt_yystr);

  return lpt_yystpcpy (lpt_yyres, lpt_yystr) - lpt_yyres;
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
lpt_yysyntax_error (char *lpt_yyresult, int lpt_yystate, int lpt_yychar)
{
  int lpt_yyn = lpt_yypact[lpt_yystate];

  if (! (YYPACT_NINF < lpt_yyn && lpt_yyn <= YYLAST))
    return 0;
  else
    {
      int lpt_yytype = YYTRANSLATE (lpt_yychar);
      YYSIZE_T lpt_yysize0 = lpt_yytnamerr (0, lpt_yytname[lpt_yytype]);
      YYSIZE_T lpt_yysize = lpt_yysize0;
      YYSIZE_T lpt_yysize1;
      int lpt_yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *lpt_yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int lpt_yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *lpt_yyfmt;
      char const *lpt_yyf;
      static char const lpt_yyunexpected[] = "syntax error, unexpected %s";
      static char const lpt_yyexpecting[] = ", expecting %s";
      static char const lpt_yyor[] = " or %s";
      char lpt_yyformat[sizeof lpt_yyunexpected
		    + sizeof lpt_yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof lpt_yyor - 1))];
      char const *lpt_yyprefix = lpt_yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int lpt_yyxbegin = lpt_yyn < 0 ? -lpt_yyn : 0;

      /* Stay within bounds of both lpt_yycheck and lpt_yytname.  */
      int lpt_yychecklim = YYLAST - lpt_yyn + 1;
      int lpt_yyxend = lpt_yychecklim < YYNTOKENS ? lpt_yychecklim : YYNTOKENS;
      int lpt_yycount = 1;

      lpt_yyarg[0] = lpt_yytname[lpt_yytype];
      lpt_yyfmt = lpt_yystpcpy (lpt_yyformat, lpt_yyunexpected);

      for (lpt_yyx = lpt_yyxbegin; lpt_yyx < lpt_yyxend; ++lpt_yyx)
	if (lpt_yycheck[lpt_yyx + lpt_yyn] == lpt_yyx && lpt_yyx != YYTERROR)
	  {
	    if (lpt_yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		lpt_yycount = 1;
		lpt_yysize = lpt_yysize0;
		lpt_yyformat[sizeof lpt_yyunexpected - 1] = '\0';
		break;
	      }
	    lpt_yyarg[lpt_yycount++] = lpt_yytname[lpt_yyx];
	    lpt_yysize1 = lpt_yysize + lpt_yytnamerr (0, lpt_yytname[lpt_yyx]);
	    lpt_yysize_overflow |= (lpt_yysize1 < lpt_yysize);
	    lpt_yysize = lpt_yysize1;
	    lpt_yyfmt = lpt_yystpcpy (lpt_yyfmt, lpt_yyprefix);
	    lpt_yyprefix = lpt_yyor;
	  }

      lpt_yyf = YY_(lpt_yyformat);
      lpt_yysize1 = lpt_yysize + lpt_yystrlen (lpt_yyf);
      lpt_yysize_overflow |= (lpt_yysize1 < lpt_yysize);
      lpt_yysize = lpt_yysize1;

      if (lpt_yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (lpt_yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *lpt_yyp = lpt_yyresult;
	  int lpt_yyi = 0;
	  while ((*lpt_yyp = *lpt_yyf) != '\0')
	    {
	      if (*lpt_yyp == '%' && lpt_yyf[1] == 's' && lpt_yyi < lpt_yycount)
		{
		  lpt_yyp += lpt_yytnamerr (lpt_yyp, lpt_yyarg[lpt_yyi++]);
		  lpt_yyf += 2;
		}
	      else
		{
		  lpt_yyp++;
		  lpt_yyf++;
		}
	    }
	}
      return lpt_yysize;
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
lpt_yydestruct (const char *lpt_yymsg, int lpt_yytype, YYSTYPE *lpt_yyvaluep, parse_parm *parm, void *scanner)
#else
static void
lpt_yydestruct (lpt_yymsg, lpt_yytype, lpt_yyvaluep, parm, scanner)
    const char *lpt_yymsg;
    int lpt_yytype;
    YYSTYPE *lpt_yyvaluep;
    parse_parm *parm;
    void *scanner;
#endif
{
  YYUSE (lpt_yyvaluep);
  YYUSE (parm);
  YYUSE (scanner);

  if (!lpt_yymsg)
    lpt_yymsg = "Deleting";
  YY_SYMBOL_PRINT (lpt_yymsg, lpt_yytype, lpt_yyvaluep, lpt_yylocationp);

  switch (lpt_yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int lpt_yyparse (void *YYPARSE_PARAM);
#else
int lpt_yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int lpt_yyparse (parse_parm *parm, void *scanner);
#else
int lpt_yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| lpt_yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
lpt_yyparse (void *YYPARSE_PARAM)
#else
int
lpt_yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
lpt_yyparse (parse_parm *parm, void *scanner)
#else
int
lpt_yyparse (parm, scanner)
    parse_parm *parm;
    void *scanner;
#endif
#endif
{
  /* The look-ahead symbol.  */
int lpt_yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE lpt_yylval;

/* Number of syntax errors so far.  */
int lpt_yynerrs;

  int lpt_yystate;
  int lpt_yyn;
  int lpt_yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int lpt_yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int lpt_yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char lpt_yymsgbuf[128];
  char *lpt_yymsg = lpt_yymsgbuf;
  YYSIZE_T lpt_yymsg_alloc = sizeof lpt_yymsgbuf;
#endif

  /* Three stacks and their tools:
     `lpt_yyss': related to states,
     `lpt_yyvs': related to semantic values,
     `lpt_yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow lpt_yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  lpt_yytype_int16 lpt_yyssa[YYINITDEPTH];
  lpt_yytype_int16 *lpt_yyss = lpt_yyssa;
  lpt_yytype_int16 *lpt_yyssp;

  /* The semantic value stack.  */
  YYSTYPE lpt_yyvsa[YYINITDEPTH];
  YYSTYPE *lpt_yyvs = lpt_yyvsa;
  YYSTYPE *lpt_yyvsp;



#define YYPOPSTACK(N)   (lpt_yyvsp -= (N), lpt_yyssp -= (N))

  YYSIZE_T lpt_yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE lpt_yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int lpt_yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  lpt_yystate = 0;
  lpt_yyerrstatus = 0;
  lpt_yynerrs = 0;
  lpt_yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  lpt_yyssp = lpt_yyss;
  lpt_yyvsp = lpt_yyvs;

  goto lpt_yysetstate;

/*------------------------------------------------------------.
| lpt_yynewstate -- Push a new state, which is found in lpt_yystate.  |
`------------------------------------------------------------*/
 lpt_yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  lpt_yyssp++;

 lpt_yysetstate:
  *lpt_yyssp = lpt_yystate;

  if (lpt_yyss + lpt_yystacksize - 1 <= lpt_yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T lpt_yysize = lpt_yyssp - lpt_yyss + 1;

#ifdef lpt_yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *lpt_yyvs1 = lpt_yyvs;
	lpt_yytype_int16 *lpt_yyss1 = lpt_yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if lpt_yyoverflow is a macro.  */
	lpt_yyoverflow (YY_("memory exhausted"),
		    &lpt_yyss1, lpt_yysize * sizeof (*lpt_yyssp),
		    &lpt_yyvs1, lpt_yysize * sizeof (*lpt_yyvsp),

		    &lpt_yystacksize);

	lpt_yyss = lpt_yyss1;
	lpt_yyvs = lpt_yyvs1;
      }
#else /* no lpt_yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto lpt_yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= lpt_yystacksize)
	goto lpt_yyexhaustedlab;
      lpt_yystacksize *= 2;
      if (YYMAXDEPTH < lpt_yystacksize)
	lpt_yystacksize = YYMAXDEPTH;

      {
	lpt_yytype_int16 *lpt_yyss1 = lpt_yyss;
	union lpt_yyalloc *lpt_yyptr =
	  (union lpt_yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (lpt_yystacksize));
	if (! lpt_yyptr)
	  goto lpt_yyexhaustedlab;
	YYSTACK_RELOCATE (lpt_yyss);
	YYSTACK_RELOCATE (lpt_yyvs);

#  undef YYSTACK_RELOCATE
	if (lpt_yyss1 != lpt_yyssa)
	  YYSTACK_FREE (lpt_yyss1);
      }
# endif
#endif /* no lpt_yyoverflow */

      lpt_yyssp = lpt_yyss + lpt_yysize - 1;
      lpt_yyvsp = lpt_yyvs + lpt_yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) lpt_yystacksize));

      if (lpt_yyss + lpt_yystacksize - 1 <= lpt_yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", lpt_yystate));

  goto lpt_yybackup;

/*-----------.
| lpt_yybackup.  |
`-----------*/
lpt_yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  lpt_yyn = lpt_yypact[lpt_yystate];
  if (lpt_yyn == YYPACT_NINF)
    goto lpt_yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (lpt_yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      lpt_yychar = YYLEX;
    }

  if (lpt_yychar <= YYEOF)
    {
      lpt_yychar = lpt_yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      lpt_yytoken = YYTRANSLATE (lpt_yychar);
      YY_SYMBOL_PRINT ("Next token is", lpt_yytoken, &lpt_yylval, &lpt_yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  lpt_yyn += lpt_yytoken;
  if (lpt_yyn < 0 || YYLAST < lpt_yyn || lpt_yycheck[lpt_yyn] != lpt_yytoken)
    goto lpt_yydefault;
  lpt_yyn = lpt_yytable[lpt_yyn];
  if (lpt_yyn <= 0)
    {
      if (lpt_yyn == 0 || lpt_yyn == YYTABLE_NINF)
	goto lpt_yyerrlab;
      lpt_yyn = -lpt_yyn;
      goto lpt_yyreduce;
    }

  if (lpt_yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (lpt_yyerrstatus)
    lpt_yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", lpt_yytoken, &lpt_yylval, &lpt_yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (lpt_yychar != YYEOF)
    lpt_yychar = YYEMPTY;

  lpt_yystate = lpt_yyn;
  *++lpt_yyvsp = lpt_yylval;

  goto lpt_yynewstate;


/*-----------------------------------------------------------.
| lpt_yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
lpt_yydefault:
  lpt_yyn = lpt_yydefact[lpt_yystate];
  if (lpt_yyn == 0)
    goto lpt_yyerrlab;
  goto lpt_yyreduce;


/*-----------------------------.
| lpt_yyreduce -- Do a reduction.  |
`-----------------------------*/
lpt_yyreduce:
  /* lpt_yyn is the number of a rule to reduce with.  */
  lpt_yylen = lpt_yyr2[lpt_yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  lpt_yyval = lpt_yyvsp[1-lpt_yylen];


  YY_REDUCE_PRINT (lpt_yyn);
  switch (lpt_yyn)
    {
        case 3:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->isign = 0;
  pv->make_neg = 0;
  pv->Sign = 0;
  pv->HadConstraint = FALSE;
  pv->HadVar = FALSE;
}
    break;

  case 5:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->f = 1;
  pv->f1 = pv->f2 = 0;
}
    break;

  case 7:

    {
  set_obj_dir(PARM, TRUE);
}
    break;

  case 8:

    {
  set_obj_dir(PARM, FALSE);
}
    break;

  case 10:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if(!add_constraint_name(pp, pv->Last_var))
    YYABORT;
  /* pv->HadConstraint = TRUE; */
}
    break;

  case 12:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if((!pv->objconst) && (pv->f1 + pv->f2 != 0)) {
    lpt_yyerror(pp, pp->scanner, "constant in objective not supported");
    YYABORT;
  }
  if(!rhs_store(pp, -(pv->f1 + pv->f2), pv->HadConstraint, pv->HadVar, pv->Had_lineair_sum))
    YYABORT;

  add_row(pp);
  /* pv->HadConstraint = FALSE; */
  pv->HadVar = FALSE;
  pv->isign = 0;
  pv->make_neg = 0;
}
    break;

  case 17:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->HadSign = FALSE;
}
    break;

  case 18:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if(pv->HadSign) {
    pv->f1 += pv->f2;
    pv->f = 1;
  }
}
    break;

  case 20:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if (    (pv->isign || pv->make_neg)
      && !(pv->isign && pv->make_neg)) /* but not both! */
    pv->f = -pv->f;
  pv->f2 = pv->f;
  pv->isign = 0;
}
    break;

  case 21:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->f2 = 0;
  pv->f = 1;
}
    break;

  case 24:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->HadConstraint = TRUE;
}
    break;

  case 25:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->HadConstraint = FALSE;
}
    break;

  case 31:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if(!add_constraint_name(pp, pv->Last_var))
    YYABORT;
  /* pv->HadConstraint = TRUE; */
}
    break;

  case 33:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if(!store_re_op(pp, pv->OP, pv->HadConstraint, pv->HadVar, pv->Had_lineair_sum))
    YYABORT;
  pv->make_neg = 1;
}
    break;

  case 34:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->Had_lineair_sum = TRUE;
  add_row(pp);
  /* pv->HadConstraint = FALSE; */
  pv->HadVar = FALSE;
  pv->isign = 0;
  pv->make_neg = 0;
  null_tmp_store(pp, TRUE);
}
    break;

  case 35:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->HadConstraint = pv->HadVar = TRUE;
}
    break;

  case 39:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->f = 1.0;
}
    break;

  case 46:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->isign = pv->Sign;
}
    break;

  case 53:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->f = 1.0;
  pv->isign = 0;
}
    break;

  case 55:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->f0 = pv->f;
  pv->isign0 = pv->isign;
}
    break;

  case 56:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if(!store_re_op(pp, pv->OP, pv->HadConstraint, pv->HadVar, pv->Had_lineair_sum))
    YYABORT;
  pv->make_neg = 0;
}
    break;

  case 57:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->isign = 0;
  pv->f = -1.0;
}
    break;

  case 58:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->isign = pv->isign0;
  pv->f = pv->f0;
}
    break;

  case 59:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if(!store_bounds(pp, TRUE))
    YYABORT;
}
    break;

  case 60:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  /* pv->HadConstraint = FALSE; */
  pv->HadVar = FALSE;
  pv->isign = 0;
  pv->make_neg = 0;
  null_tmp_store(pp, TRUE);
}
    break;

  case 61:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if(!store_re_op(pp, pv->OP, pv->HadConstraint, pv->HadVar, pv->Had_lineair_sum))
    YYABORT;
  pv->make_neg = 1;
}
    break;

  case 62:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if(!store_bounds(pp, TRUE))
    YYABORT;
  /* pv->HadConstraint = FALSE; */
  pv->HadVar = FALSE;
  pv->isign = 0;
  pv->make_neg = 0;
  null_tmp_store(pp, TRUE);
}
    break;

  case 63:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if(!store_re_op(pp, '>', pv->HadConstraint, pv->HadVar, pv->Had_lineair_sum))
    YYABORT;
  pv->make_neg = 1;
  pv->isign = 0;
  pv->f = -DEF_INFINITE;
}
    break;

  case 64:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if(!store_bounds(pp, FALSE))
    YYABORT;

  if(!store_re_op(pp, '<', pv->HadConstraint, pv->HadVar, pv->Had_lineair_sum))
    YYABORT;
  pv->f = DEF_INFINITE;
  pv->isign = 0;
}
    break;

  case 65:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if(!store_bounds(pp, FALSE))
    YYABORT;
  /* pv->HadConstraint = FALSE; */
  pv->HadVar = FALSE;
  pv->isign = 0;
  pv->make_neg = 0;
  null_tmp_store(pp, TRUE);
}
    break;

  case 67:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if(!store_re_op(pp, (char) ((pv->OP == '<') ? '>' : (pv->OP == '>') ? '<' : pv->OP), (int) pv->HadConstraint, (int) pv->HadVar, (int) pv->Had_lineair_sum))
    YYABORT;
  pv->make_neg = 0;
  pv->isign = 0;
}
    break;

  case 68:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if(!store_bounds(pp, TRUE))
    YYABORT;
}
    break;

  case 71:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if (    (pv->isign || !pv->make_neg)
      && !(pv->isign && !pv->make_neg)) /* but not both! */
    pv->f = -pv->f;
  if(!rhs_store(pp, pv->f, pv->HadConstraint, pv->HadVar, pv->Had_lineair_sum))
    YYABORT;
  pv->isign = 0;
}
    break;

  case 72:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->isign = 0;
}
    break;

  case 73:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->isign = pv->Sign;
  pv->HadSign = TRUE;
}
    break;

  case 74:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  if (    (pv->isign || pv->make_neg)
      && !(pv->isign && pv->make_neg)) /* but not both! */
    pv->f = -pv->f;
  if(!var_store(pp, pv->Last_var, pv->f, pv->HadConstraint, pv->HadVar, pv->Had_lineair_sum)) {
    lpt_yyerror(pp, pp->scanner, "var_store failed");
    YYABORT;
  }
  /* pv->HadConstraint |= pv->HadVar; */
  pv->HadVar = TRUE;
  pv->isign = 0;
}
    break;

  case 80:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  storevarandweight(pp, pv->Last_var);
}
    break;

  case 81:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->SOSNr++;
  pv->weight = pv->SOSNr;
  storevarandweight(pp, pv->Last_var);
  set_sos_weight(pp, pv->weight, 2);
}
    break;

  case 82:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  storevarandweight(pp, pv->Last_var);
}
    break;

  case 83:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  pv->weight = (int) (pv->f + .1);
  set_sos_weight(pp, pv->weight, 2);
}
    break;

  case 88:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  check_int_sec_sos_free_decl(pp, pv->Within_gen_decl ? 1 : pv->Within_bin_decl ? 2 : 0, 0, 0, 0);
}
    break;

  case 94:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  check_int_sec_sos_free_decl(pp, 0, 1, 0, 0);
}
    break;

  case 105:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;
  char buf[16], *ptr, *var;

  check_int_sec_sos_free_decl(pp, 0, 0, 1, 0);
  pv->SOSweight++;
  for (ptr = pv->Last_var; (*ptr) && (*ptr != ':'); ptr++);
  if (ptr[1] == ':') {
    sprintf(buf, "SOS%d", pv->SOSweight);
    var = buf;
    ptr = pv->Last_var;
  }
  else {
    var = ptr + 1;
    while ((ptr > pv->Last_var) && (isspace(ptr[-1]))) ptr--;
    *ptr = 0;
    ptr = var;
    var = pv->Last_var;
    while (isspace(*ptr)) ptr++;
  }
  storevarandweight(pp, var);
  pv->SOStype = ptr[1] - '0';
  set_sos_type(pp, pv->SOStype);
  check_int_sec_sos_free_decl(pp, 0, 0, 2, 0);
  pv->weight = 0;
  pv->SOSNr = 0;
}
    break;

  case 106:

    {
  parse_parm *pp = PARM;
  parse_vars *pv = (parse_vars *) pp->parse_vars;

  set_sos_weight(pp, pv->SOSweight, 1);
}
    break;


/* Line 1267 of yacc.c.  */

      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", lpt_yyr1[lpt_yyn], &lpt_yyval, &lpt_yyloc);

  YYPOPSTACK (lpt_yylen);
  lpt_yylen = 0;
  YY_STACK_PRINT (lpt_yyss, lpt_yyssp);

  *++lpt_yyvsp = lpt_yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  lpt_yyn = lpt_yyr1[lpt_yyn];

  lpt_yystate = lpt_yypgoto[lpt_yyn - YYNTOKENS] + *lpt_yyssp;
  if (0 <= lpt_yystate && lpt_yystate <= YYLAST && lpt_yycheck[lpt_yystate] == *lpt_yyssp)
    lpt_yystate = lpt_yytable[lpt_yystate];
  else
    lpt_yystate = lpt_yydefgoto[lpt_yyn - YYNTOKENS];

  goto lpt_yynewstate;


/*------------------------------------.
| lpt_yyerrlab -- here on detecting error |
`------------------------------------*/
lpt_yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!lpt_yyerrstatus)
    {
      ++lpt_yynerrs;
#if ! YYERROR_VERBOSE
      lpt_yyerror (parm, scanner, YY_("syntax error"));
#else
      {
	YYSIZE_T lpt_yysize = lpt_yysyntax_error (0, lpt_yystate, lpt_yychar);
	if (lpt_yymsg_alloc < lpt_yysize && lpt_yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T lpt_yyalloc = 2 * lpt_yysize;
	    if (! (lpt_yysize <= lpt_yyalloc && lpt_yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      lpt_yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (lpt_yymsg != lpt_yymsgbuf)
	      YYSTACK_FREE (lpt_yymsg);
	    lpt_yymsg = (char *) YYSTACK_ALLOC (lpt_yyalloc);
	    if (lpt_yymsg)
	      lpt_yymsg_alloc = lpt_yyalloc;
	    else
	      {
		lpt_yymsg = lpt_yymsgbuf;
		lpt_yymsg_alloc = sizeof lpt_yymsgbuf;
	      }
	  }

	if (0 < lpt_yysize && lpt_yysize <= lpt_yymsg_alloc)
	  {
	    (void) lpt_yysyntax_error (lpt_yymsg, lpt_yystate, lpt_yychar);
	    lpt_yyerror (parm, scanner, lpt_yymsg);
	  }
	else
	  {
	    lpt_yyerror (parm, scanner, YY_("syntax error"));
	    if (lpt_yysize != 0)
	      goto lpt_yyexhaustedlab;
	  }
      }
#endif
    }



  if (lpt_yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (lpt_yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (lpt_yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  lpt_yydestruct ("Error: discarding",
		      lpt_yytoken, &lpt_yylval, parm, scanner);
	  lpt_yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto lpt_yyerrlab1;


/*---------------------------------------------------.
| lpt_yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
lpt_yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label lpt_yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto lpt_yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (lpt_yylen);
  lpt_yylen = 0;
  YY_STACK_PRINT (lpt_yyss, lpt_yyssp);
  lpt_yystate = *lpt_yyssp;
  goto lpt_yyerrlab1;


/*-------------------------------------------------------------.
| lpt_yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
lpt_yyerrlab1:
  lpt_yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      lpt_yyn = lpt_yypact[lpt_yystate];
      if (lpt_yyn != YYPACT_NINF)
	{
	  lpt_yyn += YYTERROR;
	  if (0 <= lpt_yyn && lpt_yyn <= YYLAST && lpt_yycheck[lpt_yyn] == YYTERROR)
	    {
	      lpt_yyn = lpt_yytable[lpt_yyn];
	      if (0 < lpt_yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (lpt_yyssp == lpt_yyss)
	YYABORT;


      lpt_yydestruct ("Error: popping",
		  lpt_yystos[lpt_yystate], lpt_yyvsp, parm, scanner);
      YYPOPSTACK (1);
      lpt_yystate = *lpt_yyssp;
      YY_STACK_PRINT (lpt_yyss, lpt_yyssp);
    }

  if (lpt_yyn == YYFINAL)
    YYACCEPT;

  *++lpt_yyvsp = lpt_yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", lpt_yystos[lpt_yyn], lpt_yyvsp, lpt_yylsp);

  lpt_yystate = lpt_yyn;
  goto lpt_yynewstate;


/*-------------------------------------.
| lpt_yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
lpt_yyacceptlab:
  lpt_yyresult = 0;
  goto lpt_yyreturn;

/*-----------------------------------.
| lpt_yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
lpt_yyabortlab:
  lpt_yyresult = 1;
  goto lpt_yyreturn;

#ifndef lpt_yyoverflow
/*-------------------------------------------------.
| lpt_yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
lpt_yyexhaustedlab:
  lpt_yyerror (parm, scanner, YY_("memory exhausted"));
  lpt_yyresult = 2;
  /* Fall through.  */
#endif

lpt_yyreturn:
  if (lpt_yychar != YYEOF && lpt_yychar != YYEMPTY)
     lpt_yydestruct ("Cleanup: discarding lookahead",
		 lpt_yytoken, &lpt_yylval, parm, scanner);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (lpt_yylen);
  YY_STACK_PRINT (lpt_yyss, lpt_yyssp);
  while (lpt_yyssp != lpt_yyss)
    {
      lpt_yydestruct ("Cleanup: popping",
		  lpt_yystos[*lpt_yyssp], lpt_yyvsp, parm, scanner);
      YYPOPSTACK (1);
    }
#ifndef lpt_yyoverflow
  if (lpt_yyss != lpt_yyssa)
    YYSTACK_FREE (lpt_yyss);
#endif
#if YYERROR_VERBOSE
  if (lpt_yymsg != lpt_yymsgbuf)
    YYSTACK_FREE (lpt_yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (lpt_yyresult);
}





static void lpt_yy_delete_allocated_memory(parse_parm *pp)
{
  parse_vars *pv = (parse_vars *) pp->parse_vars;
  /* free memory allocated by flex. Otherwise some memory is not freed.
     This is a bit tricky. There is not much documentation about this, but a lot of
     reports of memory that keeps allocated */

  /* If you get errors on this function call, just comment it. This will only result
     in some memory that is not being freed. */

# if defined YY_CURRENT_BUFFER
    /* flex defines the macro YY_CURRENT_BUFFER, so you should only get here if lp_rlp.h is
       generated by flex */
    /* lex doesn't define this macro and thus should not come here, but lex doesn't has
       this memory leak also ...*/

#  if 0
    /* older versions of flex */
    lpt_yy_delete_buffer(YY_CURRENT_BUFFER); /* comment this line if you have problems with it */
    lpt_yy_init = 1; /* make sure that the next time memory is allocated again */
    lpt_yy_start = 0;
#  else
    /* As of version 2.5.9 Flex  */
    lpt_yylex_destroy(pp->scanner); /* comment this line if you have problems with it */
#  endif
# endif

  FREE(pv->Last_var);
}

static int parse(parse_parm *pp)
{
  return(lpt_yyparse(pp, pp->scanner));
}

lprec *read_lptex(lprec *lp, FILE *filename, int verbose, char *lp_name, char objconst0)
{
  parse_vars *pv;
  lprec *lp1 = NULL;

  CALLOC(pv, 1, parse_vars);
  if (pv != NULL) {
    parse_parm pp;

    memset(&pp, 0, sizeof(pp));
    pp.parse_vars = (void *) pv;

    lpt_yylex_init(&pp.scanner);
    lpt_yyset_extra(&pp, pp.scanner);

    lpt_yyset_in((FILE *) filename, pp.scanner);
    lpt_yyset_out(NULL, pp.scanner);
    pv->objconst = objconst0;
    lp1 = yacc_read(lp, verbose, lp_name, parse, &pp, lpt_yy_delete_allocated_memory);
    FREE(pv);
  }
  return(lp1);
}

lprec * __WINAPI read_lpt(FILE *filename, int verbose, char *lp_name)
{
  return(read_lptex(NULL, filename, verbose, lp_name, FALSE));
}

lprec *read_LPTex(lprec *lp, char *filename, int verbose, char *lp_name, char objconst)
{
  FILE *fpin;

  if((fpin = fopen(filename, "r")) != NULL) {
    lp = read_lptex(lp, fpin, verbose, lp_name, objconst);
    fclose(fpin);
  }
  else
    lp = NULL;
  return(lp);
}

lprec * __WINAPI read_LPT(char *filename, int verbose, char *lp_name)
{
  return(read_LPTex(NULL, filename, verbose, lp_name, FALSE));
}

