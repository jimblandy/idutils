
/*  A Bison parser, made from ./iid.y with Bison version GNU Bison version 1.22
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	SET	258
#define	ID	259
#define	SHELL_QUERY	260
#define	SHELL_COMMAND	261
#define	LID	262
#define	AID	263
#define	BEGIN	264
#define	SETS	265
#define	SS	266
#define	FILES	267
#define	SHOW	268
#define	HELP	269
#define	OFF	270
#define	MATCH	271
#define	OR	272
#define	AND	273
#define	NOT	274

#line 1 "./iid.y"

/* iid.y -- interactive mkid query language
   Copyright (C) 1991 Tom Horsley
  
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include "strxtra.h"

#if HAVE_ALLOCA

#if HAVE_ALLOCA_H
#include <alloca.h>
#endif
#define TEMP_ALLOC(s) alloca(s)
#define TEMP_FREE(s)

#else /* not HAVE_ALLOCA */

#define TEMP_ALLOC(s) malloc(s)
#define TEMP_FREE(s)  free(s)

#endif /* not HAVE_ALLOCA */

#define HASH_SIZE 947                  /* size of hash table for file names */
#define INIT_FILES 8000                /* start with bits for this many */
#define INIT_SETSPACE 500              /* start with room for this many */
#define MAXCMD 1024                    /* input command buffer size */

#define MAX(a,b) (((a)<(b))?(b):(a))
#define MIN(a,b) (((a)>(b))?(b):(a))

#ifndef PAGER
#define PAGER "pg"
#endif

#define PROMPT "iid> "

/* set_type is the struct defining a set of file names
 * The file names are stored in a symbol table and assigned
 * unique numbers. The set is a bit set of file numbers.
 * One of these set structs is calloced for each new set
 * constructed, the size allocated depends on the max file
 * bit number. An array of pointers to sets are kept to
 * represent the complete set of sets.
 */

struct set_struct {
   char *                 set_desc ;   /* string describing the set */
   int                    set_num ;    /* the set number */
   int                    set_size ;   /* number of long words in set */
   unsigned long int      set_tail ;   /* set extended with these bits */
   unsigned long int      set_data[1] ;/* the actual set data (calloced) */
} ;
typedef struct set_struct set_type ;

/* id_type is one element of an id_list
 */

struct id_struct {
   struct id_struct *     next_id ;    /* Linked list of IDs */
   char                   id [ 1 ] ;   /* calloced data holding id string */
} ;
typedef struct id_struct id_type ;

/* id_list_type is used during parsing to build lists of
 * identifiers that will eventually represent arguments
 * to be passed to the database query programs.
 */

struct id_list_struct {
   int                    id_count ;   /* count of IDs in the list */
   id_type * *            end_ptr_ptr ;/* pointer to link word at end of list */
   id_type *              id_list ;    /* pointer to list of IDs */
} ;
typedef struct id_list_struct id_list_type ;

/* symtab_type is used to record file names in the symbol table.
 */
struct symtab_struct {
   struct symtab_struct * hash_link ;  /* list of files with same hash code */
   int                    mask_word ;  /* word in bit vector */
   unsigned long          mask_bit ;   /* bit in word */
   char                   name [ 1 ] ; /* the file name */
} ;
typedef struct symtab_struct symtab_type ;

/* LidCommand is the command to run for a Lid_group. It is set
 * to "lid -kmn" if explicitly preceeded by "lid", otherwise
 * it is the default command which is determined by an option.
 */
char const * LidCommand ;

/* DefaultCommand is the default command for a Lid_group. If
 * the -a option is given to iid, it is set to use 'aid'.
 */
char const * DefaultCommand = "lid -kmn" ;

/* FileList is a lexically ordered list of file symbol table
 * pointers. It is dynamically expanded when necessary.
 */
symtab_type * *    FileList = NULL ;

/* FileSpace is the number of long ints in TheFiles array.
 */
int                FileSpace = 0 ;

/* HashTable is the symbol table used to store file names. Each
 * new name installed is assigned the next consecutive file number.
 */
symtab_type *      HashTable [ HASH_SIZE ] ;

/* HelpSet is a dummy set containing only one bit set which corresponds
 * to the help file name. Simply a cheesy way to maximize sharing of
 * the code that runs the pager.
 */
set_type *         HelpSet ;

/* high_bit is a unsigned long with the most significant bit set.
 */
unsigned long      high_bit ;

/* ListSpace is the amount of space avail in the FileList.
 */
int                ListSpace = 0 ;

/* MaxCurFile - max word that has any bit currently set in the
 * TheFiles array.
 */
int                MaxCurFile = 0 ;

/* NextFileNum is the file number that will be assigned to the next
 * new file name seen when it is installed in the symtab.
 */
int                NextFileNum = 0 ;

/* NextMaskBit is the bit within the next mask word that will
 * correspond to the next file added to the symbol table.
 */
unsigned long      NextMaskBit ;

/* NextMaskWord is the next word number to be assigned to a file
 * bit mask entry.
 */
int                NextMaskWord = 0 ;

/* NextSetNum is the number that will be assigned to the next set
 * created. Starts at 0 because I am a C programmer.
 */
int                NextSetNum = 0 ;

/* The PAGER program to run on a SHOW command.
 */
char               Pager[MAXCMD] ;

/* Prompt - the string to use for a prompt.
 */
char               Prompt[MAXCMD] ;

/* SetSpace is the number of pointers available in TheSets. TheSets
 * is realloced when we run out of space.
 */
int                SetSpace = 0 ;

/* TheFiles is a bit set used to construct the initial set of files
 * generated while running one of the subprograms. It is copied to
 * the alloced set once we know how many bits are set.
 */
unsigned long *    TheFiles = NULL ;

/* TheSets is a dynamically allocated array of pointers pointing
 * the sets that have been allocated. It represents the set of
 * sets.
 */
set_type * *       TheSets = NULL ;

/* VerboseQuery controls the actions of the semantic routines during
 * the process of a query. If TRUE the sets are described as they
 * are constructed.
 */
int                VerboseQuery ;

int yyerror( char const * s ) ;
void ScanInit( char * line ) ;
int yylex( void ) ;
int ArgListSize( id_list_type * idlp ) ;
int SetListSize( set_type * sp ) ;
void FlushFiles( void ) ;
void fatal( char const * s ) ;
int CountBits( set_type * sp ) ;
void OneDescription( set_type * sp ) ;
void DescribeSets( void ) ;
id_list_type * SetList( id_list_type * idlp , set_type * sp ) ;
void PrintSet( set_type * sp ) ;
void FlushSets( void ) ;
id_list_type * InitList( void ) ;
id_list_type * ExtendList( id_list_type * idlp , id_type * idp ) ;
void InitIid( void ) ;
symtab_type * InstallFile( char const * fp ) ;
void RunPager( char * pp , set_type * sp ) ;
void AddSet( set_type * sp ) ;
set_type * RunProg( char const * pp , id_list_type * idlp ) ;
void SetDirectory( id_type * dir ) ;
set_type * SetIntersect( set_type * sp1 , set_type * sp2 ) ;
set_type * SetUnion( set_type * sp1 , set_type * sp2 ) ;
set_type * SetInverse( set_type * sp ) ;
void RunShell( char * pp , id_list_type * idlp ) ;


#line 230 "./iid.y"
typedef union {
   set_type *     setdef ;   
   id_type *      strdef ;
   id_list_type * listdef ;
} YYSTYPE;

#ifndef YYLTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YYLTYPE yyltype
#endif

#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		46
#define	YYFLAG		-32768
#define	YYNTBASE	22

#define YYTRANSLATE(x) ((unsigned)(x) <= 274 ? yytranslate[x] : 31)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    20,
    21,     2,     2,     2,     2,     2,     2,     2,     2,     2,
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
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     3,     6,     9,    12,    14,    16,    18,    21,    24,
    26,    28,    30,    34,    38,    41,    43,    45,    47,    50,
    54,    56,    59,    62,    64,    66,    69,    72,    74
};

static const short yyrhs[] = {     9,
     4,     0,    23,    25,     0,    24,    25,     0,    13,     3,
     0,    10,     0,    14,     0,    15,     0,     5,    29,     0,
     6,    29,     0,    11,     0,    12,     0,    26,     0,    25,
    18,    25,     0,    25,    17,    25,     0,    19,    25,     0,
     3,     0,    27,     0,    28,     0,    16,    30,     0,    20,
    25,    21,     0,     4,     0,     7,    30,     0,     8,    30,
     0,     4,     0,     3,     0,    29,     4,     0,    29,     3,
     0,     4,     0,    30,     4,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   256,   264,   265,   271,   277,   283,   289,   293,   300,   309,
   318,   327,   334,   343,   352,   363,   370,   379,   388,   396,
   404,   413,   422,   431,   439,   446,   452,   460,   468
};

static const char * const yytname[] = {   "$","error","$illegal.","SET","ID",
"SHELL_QUERY","SHELL_COMMAND","LID","AID","BEGIN","SETS","SS","FILES","SHOW",
"HELP","OFF","MATCH","OR","AND","NOT","'('","')'","Command","Set_query","File_query",
"Query","Primitive","Lid_group","Aid_group","Command_list","Id_list",""
};
#endif

static const short yyr1[] = {     0,
    22,    22,    22,    22,    22,    22,    22,    22,    22,    23,
    24,    25,    25,    25,    25,    26,    26,    26,    26,    26,
    27,    27,    28,    29,    29,    29,    29,    30,    30
};

static const short yyr2[] = {     0,
     2,     2,     2,     2,     1,     1,     1,     2,     2,     1,
     1,     1,     3,     3,     2,     1,     1,     1,     2,     3,
     1,     2,     2,     1,     1,     2,     2,     1,     2
};

static const short yydefact[] = {     0,
     0,     0,     0,     5,    10,    11,     0,     6,     7,     0,
     0,    25,    24,     8,     9,     1,     4,    16,    21,     0,
     0,     0,     0,     0,     2,    12,    17,    18,     3,    27,
    26,    28,    22,    23,    19,    15,     0,     0,     0,    29,
    20,    14,    13,     0,     0,     0
};

static const short yydefgoto[] = {    44,
    10,    11,    25,    26,    27,    28,    14,    33
};

static const short yypact[] = {    10,
     5,     5,    22,-32768,-32768,-32768,    28,-32768,-32768,    -2,
    -2,-32768,-32768,     7,     7,-32768,-32768,-32768,-32768,    30,
    30,    30,    -2,    -2,    12,-32768,-32768,-32768,    12,-32768,
-32768,-32768,    31,    31,    31,-32768,   -14,    -2,    -2,-32768,
-32768,    18,-32768,    37,    38,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,   -11,-32768,-32768,-32768,    39,    11
};


#define	YYLAST		41


static const short yytable[] = {    29,
    18,    19,    38,    39,    20,    21,    41,    12,    13,    30,
    31,    36,    37,    22,     1,     2,    23,    24,     3,     4,
     5,     6,     7,     8,     9,    16,    42,    43,    38,    39,
    17,    34,    35,    32,    40,    39,    45,    46,     0,     0,
    15
};

static const short yycheck[] = {    11,
     3,     4,    17,    18,     7,     8,    21,     3,     4,     3,
     4,    23,    24,    16,     5,     6,    19,    20,     9,    10,
    11,    12,    13,    14,    15,     4,    38,    39,    17,    18,
     3,    21,    22,     4,     4,    18,     0,     0,    -1,    -1,
     2
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Bob Corbett and Richard Stallman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYLEX
#ifndef YYPURE
#define YYLEX		yylex()
#else
#ifdef YYLSP_NEEDED
#define YYLEX		yylex(&yylval, &yylloc)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
int yydebug_reducing = 0;
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_bcopy(FROM,TO,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_bcopy (from, to, count)
     char *from;
     char *to;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_bcopy (char *from, char *to, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 185 "/usr/lib/bison.simple"
int
yyparse()
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
#ifndef __cplusplus
    fprintf(stderr, "Starting parse\n");
#else /* __cplusplus */
    clog << "Starting parse" << endl;
#endif /* __cplusplus */
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_bcopy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_bcopy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_bcopy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug >= 3)
#ifndef __cplusplus
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#else /* __cplusplus */
	clog << "Stack size increased to " << yystacksize << endl;
#endif /* __cplusplus */
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug >= 3)
#ifndef __cplusplus
    fprintf(stderr, "Entering state %d\n", yystate);
#else /* __cplusplus */
    clog << "Entering state " << yystate << endl;
#endif /* __cplusplus */
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug >= 3)
#ifndef __cplusplus
	fprintf(stderr, "Reading a token: ");
#else /* __cplusplus */
	clog << "Reading a token: ";
#endif /* __cplusplus */
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
#ifndef __cplusplus
	fprintf(stderr, "Now at end of input.\n");
#else /* __cplusplus */
	clog << "Now at end of input." << endl;
#endif /* __cplusplus */
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug >= 3)
	{
#ifndef __cplusplus
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
#else /* __cplusplus */
	  clog << "Next token is " << yychar << " (" << yytname[yychar1];
#endif /* __cplusplus */
#ifdef YYPRINT
#ifndef __cplusplus
	  YYPRINT (stderr, yychar, yylval);
#else /* __cplusplus */
	  YYPRINT (yychar, yylval);
#endif /* __cplusplus */
#endif
#ifndef __cplusplus
	  fprintf (stderr, ")\n");
#else /* __cplusplus */
	  clog << ')' << endl;
#endif /* __cplusplus */
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    {
      if (yydebug_reducing)
	{
#ifndef __cplusplus
	fprintf(stderr, "\nShift:");
#else /* __cplusplus */
	  clog << endl << "Shift:";
#endif /* __cplusplus */
	yydebug_reducing = 0;
      }
      if (yydebug >= 2)
#ifndef __cplusplus
	fprintf (stderr, "Shifting token %d: %s", yychar, yytname[yychar1]);
#else /* __cplusplus */
	clog << "Shifting token " << yychar << ": " << yytname[yychar1];
#endif /* __cplusplus */
      else
#ifndef __cplusplus
	fprintf (stderr, " %s", yytname[yychar1]);
#else /* __cplusplus */
	clog << ' ' << yytname[yychar1];
#endif /* __cplusplus */
#ifdef YYPRINT
#ifndef __cplusplus
      YYPRINT (stderr, yychar, yylval);
#else /* __cplusplus */
      YYPRINT (yychar, yylval);
#endif /* __cplusplus */
#endif
      if (yydebug >= 2)
#ifndef __cplusplus
	fputc ('\n', stderr);
#else /* __cplusplus */
	clog << endl;
#endif /* __cplusplus */
    }
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;
      if (!yydebug_reducing)
	{
#ifndef __cplusplus
	  fputc('\n', stderr);
#else /* __cplusplus */
	  clog << endl;
#endif /* __cplusplus */
	  yydebug_reducing = 1;
      }
      if (yydebug >= 2)
#ifndef __cplusplus
	fprintf (stderr, "Reducing via rule %d (line %d): ", yyn, yyrline[yyn]);
#else /* __cplusplus */
	clog << "Reducing via rule " << yyn << " (line " << yyrline[yyn] << " ): ";
#endif /* __cplusplus */
      else
#ifndef __cplusplus
	fprintf (stderr, YYFILE ":%d: ", yyrline[yyn]);
#else /* __cplusplus */
	clog << YYFILE ":" << yyrline[yyn] << ": ";
#endif /* __cplusplus */

      /* Print the symbols being reduced, and their result.  */
#ifdef __cplusplus
      clog << yytname[yyr1[yyn]] << " <-";
#endif /* __cplusplus */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
#ifndef __cplusplus
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, "-> %s\n", yytname[yyr1[yyn]]);
#else /* __cplusplus */
	clog << ' ' << yytname[yyrhs[i]];
      clog << endl;
#endif /* __cplusplus */
    }
#endif


  switch (yyn) {

case 1:
#line 258 "./iid.y"
{
         /* cd to the directory specified as argument, flush sets */

         SetDirectory(yyvsp[0]. strdef ) ;
         FlushSets() ;
      ;
    break;}
case 3:
#line 266 "./iid.y"
{
         /* print the list of files resulting from Query */

         PrintSet(yyvsp[0]. setdef ) ;
      ;
    break;}
case 4:
#line 272 "./iid.y"
{
         /* run PAGER on the list of files in SET */

         RunPager(Pager, yyvsp[0]. setdef ) ;
      ;
    break;}
case 5:
#line 278 "./iid.y"
{
         /* describe sets created so far */

         DescribeSets() ;
      ;
    break;}
case 6:
#line 284 "./iid.y"
{
         /* run PAGER on the help file */

         RunPager(Pager, HelpSet) ;
      ;
    break;}
case 7:
#line 290 "./iid.y"
{
         exit(0) ;
      ;
    break;}
case 8:
#line 294 "./iid.y"
{
         /* run the shell command and eat the results as a file set */

         OneDescription(RunProg(yyvsp[-1]. strdef ->id, yyvsp[0]. listdef )) ;
         free(yyvsp[-1]. strdef ) ;
      ;
    break;}
case 9:
#line 301 "./iid.y"
{
         /* run the shell command */

         RunShell(yyvsp[-1]. strdef ->id, yyvsp[0]. listdef ) ;
         free(yyvsp[-1]. strdef ) ;
      ;
    break;}
case 10:
#line 311 "./iid.y"
{
         /* Turn on verbose query flag */

         VerboseQuery = 1 ;
      ;
    break;}
case 11:
#line 320 "./iid.y"
{
         /* Turn off verbose query flag */

         VerboseQuery = 0 ;
      ;
    break;}
case 12:
#line 329 "./iid.y"
{
         /* value of query is set associated with primitive */

         yyval. setdef  = yyvsp[0]. setdef  ;
      ;
    break;}
case 13:
#line 335 "./iid.y"
{
         /* value of query is intersection of the two query sets */

         yyval. setdef  = SetIntersect(yyvsp[-2]. setdef , yyvsp[0]. setdef ) ;
         if (VerboseQuery) {
            OneDescription(yyval. setdef ) ;
         }
      ;
    break;}
case 14:
#line 344 "./iid.y"
{
         /* value of query is union of the two query sets */

         yyval. setdef  = SetUnion(yyvsp[-2]. setdef , yyvsp[0]. setdef ) ;
         if (VerboseQuery) {
            OneDescription(yyval. setdef ) ;
         }
      ;
    break;}
case 15:
#line 353 "./iid.y"
{
         /* value of query is inverse of other query */
         
         yyval. setdef  = SetInverse(yyvsp[0]. setdef ) ;
         if (VerboseQuery) {
            OneDescription(yyval. setdef ) ;
         }
      ;
    break;}
case 16:
#line 365 "./iid.y"
{
         /* Value of primitive is value of recorded set */

         yyval. setdef  = yyvsp[0]. setdef  ;
      ;
    break;}
case 17:
#line 371 "./iid.y"
{
         /* Value of primitive is obtained by running an lid query */

         yyval. setdef  = RunProg(LidCommand, yyvsp[0]. listdef ) ;
         if (VerboseQuery) {
            OneDescription(yyval. setdef ) ;
         }
      ;
    break;}
case 18:
#line 380 "./iid.y"
{
         /* Value of primitive is obtained by running an aid query */

         yyval. setdef  = RunProg("aid -kmn", yyvsp[0]. listdef ) ;
         if (VerboseQuery) {
            OneDescription(yyval. setdef ) ;
         }
      ;
    break;}
case 19:
#line 389 "./iid.y"
{
         /* Match names from database against pattern */
         yyval. setdef  = RunProg("pid -kmn", yyvsp[0]. listdef ) ;
         if (VerboseQuery) {
            OneDescription(yyval. setdef ) ;
         }
      ;
    break;}
case 20:
#line 397 "./iid.y"
{
         /* value of primitive is value of query */

         yyval. setdef  = yyvsp[-1]. setdef  ;
      ;
    break;}
case 21:
#line 406 "./iid.y"
{
         /* make arg list holding single ID */

         yyval. listdef  = InitList() ;
         yyval. listdef  = ExtendList(yyval. listdef , yyvsp[0]. strdef ) ;
         LidCommand = DefaultCommand ;
      ;
    break;}
case 22:
#line 414 "./iid.y"
{
         /* arg list is Id_list */

         yyval. listdef  = yyvsp[0]. listdef  ;
         LidCommand = "lid -kmn" ;
      ;
    break;}
case 23:
#line 424 "./iid.y"
{
         /* arg list is Id_list */

         yyval. listdef  = yyvsp[0]. listdef  ;
      ;
    break;}
case 24:
#line 433 "./iid.y"
{
         /* make arg list holding single ID */

         yyval. listdef  = InitList() ;
         yyval. listdef  = ExtendList(yyval. listdef , yyvsp[0]. strdef ) ;
      ;
    break;}
case 25:
#line 440 "./iid.y"
{
         /* make arg list holding names from set */

         yyval. listdef  = InitList() ;
         yyval. listdef  = SetList(yyval. listdef , yyvsp[0]. setdef ) ;
      ;
    break;}
case 26:
#line 447 "./iid.y"
{
         /* extend arg list with additional ID */

         yyval. listdef  = ExtendList(yyvsp[-1]. listdef , yyvsp[0]. strdef ) ;
      ;
    break;}
case 27:
#line 453 "./iid.y"
{
         /* extend arg list with additional file names */

         yyval. listdef  = SetList(yyvsp[-1]. listdef , yyvsp[0]. setdef ) ;
      ;
    break;}
case 28:
#line 462 "./iid.y"
{
         /* make arg list holding single ID */

         yyval. listdef  = InitList() ;
         yyval. listdef  = ExtendList(yyval. listdef , yyvsp[0]. strdef ) ;
      ;
    break;}
case 29:
#line 469 "./iid.y"
{
         /* extend arg list with additional ID */

         yyval. listdef  = ExtendList(yyvsp[-1]. listdef , yyvsp[0]. strdef ) ;
      ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 557 "/usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug >= 3)
    {
      short *ssp1 = yyss - 1;
#ifndef __cplusplus
      fprintf (stderr, "state stack now");
#else /* __cplusplus */
      clog << "state stack now";
#endif /* __cplusplus */
      while (ssp1 != yyssp)
#ifndef __cplusplus
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
#else /* __cplusplus */
	clog << ' ' << *++ssp1;
      clog << endl;
#endif /* __cplusplus */
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
#ifndef __cplusplus
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#else /* __cplusplus */
	clog << "Discarding token " << yychar << " (" << yytname[yychar1] << ")." << endl;
#endif /* __cplusplus */
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
#ifndef __cplusplus
      fprintf (stderr, "Error: state stack now");
#else /* __cplusplus */
      clog << "Error: state stack now";
#endif /* __cplusplus */
      while (ssp1 != yyssp)
#ifndef __cplusplus
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
#else /* __cplusplus */
	clog << ' ' << *++ssp1;
      clog << endl;
#endif /* __cplusplus */
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
#ifndef __cplusplus
    fprintf(stderr, "Shifting error token, ");
#else /* __cplusplus */
    clog << "Shifting error token, ";
#endif /* __cplusplus */
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 476 "./iid.y"


/* ScanLine - a global variable holding a pointer to the current
 * command being scanned.
 */
char *             ScanLine ;

/* ScanPtr - a global pointer to the current scan position in ScanLine.
 */
char *             ScanPtr ;

/* yytext - buffer holding the token.
 */
char               yytext [ MAXCMD ] ;

/* yyerror - process syntax errors.
 */
int
yyerror( char const * s )
{
   if (*ScanPtr == '\0') {
      fprintf(stderr,"Syntax error near end of command.\n") ;
   } else {
      fprintf(stderr,"Syntax error on or before %s\n",ScanPtr) ;
   }
   return(0) ;
}

/* ScanInit - initialize the yylex routine for the new line of input.
 * Basically just initializes the global variables that hold the char
 * ptrs the scanner uses.
 */
void
ScanInit( char * line )
{
   /* skip the leading white space - the yylex routine is sensitive
    * to keywords in the first position on the command line.
    */
   
   while (isspace(*line)) ++line ;
   ScanLine = line ;
   ScanPtr = line ;
}

/* yylex - the scanner for iid. Basically a kludge ad-hoc piece of junk,
 * but what the heck, if it works...
 *
 * Mostly just scans for non white space strings and returns ID for them.
 * Does check especially for '(' and ')'. Just before returning ID it
 * checks for command names if it is the first token on line or
 * AND, OR, LID, AID if it is in the middle of a line.
 */
int
yylex( void )
{
   char *      bp ;
   char        c ;
   int         code = ID ;
   char *      dp ;
   char *      sp ;
   int         val ;
   
   bp = ScanPtr ;
   while (isspace(*bp)) ++bp ;
   sp = bp ;
   c = *sp++ ;
   if ((c == '(') || (c == ')') || (c == '\0')) {
      ScanPtr = sp ;
      if (c == '\0') {
         --ScanPtr ;
      }
      return(c) ;
   } else {
      dp = yytext ;
      while (! ((c == '(') || (c == ')') || (c == '\0') || isspace(c))) {
         *dp++ = c ;
         c = *sp++ ;
      }
      *dp++ = '\0' ;
      ScanPtr = sp - 1 ;
      if (bp == ScanLine) {

         /* first token on line, check for command names */

         if (strcaseequ(yytext, "SS")) return(SS) ;
         if (strcaseequ(yytext, "FILES")) return(FILES) ;
         if (strcaseequ(yytext, "F")) return(FILES) ;
         if (strcaseequ(yytext, "HELP")) return(HELP) ;
         if (strcaseequ(yytext, "H")) return(HELP) ;
         if (strcaseequ(yytext, "?")) return(HELP) ;
         if (strcaseequ(yytext, "BEGIN")) return(BEGIN) ;
         if (strcaseequ(yytext, "B")) return(BEGIN) ;
         if (strcaseequ(yytext, "SETS")) return(SETS) ;
         if (strcaseequ(yytext, "SHOW")) return(SHOW) ;
         if (strcaseequ(yytext, "P")) return(SHOW) ;
         if (strcaseequ(yytext, "OFF")) return(OFF) ;
         if (strcaseequ(yytext, "Q")) return(OFF) ;
         if (strcaseequ(yytext, "QUIT")) return(OFF) ;
         if (yytext[0] == '!') {
            code = SHELL_COMMAND ;
         } else {
            code = SHELL_QUERY ;
         }
      } else {

         /* not first token, check for operator names */

         if (strcaseequ(yytext, "LID")) return(LID) ;
         if (strcaseequ(yytext, "AID")) return(AID) ;
         if (strcaseequ(yytext, "AND")) return(AND) ;
         if (strcaseequ(yytext, "OR")) return(OR) ;
         if (strcaseequ(yytext, "NOT")) return(NOT) ;
         if (strcaseequ(yytext, "MATCH")) return(MATCH) ;
         if ((yytext[0] == 's' || yytext[0] == 'S') && isdigit(yytext[1])) {
            
            /* this might be a set specification */
            
            sp = &yytext[1] ;
            val = 0 ;
            for ( ; ; ) {
               c = *sp++ ;
               if (c == '\0') {
                  if (val < NextSetNum) {
                     yylval.setdef = TheSets[val] ;
                     return(SET) ;
                  }
               }
               if (isdigit(c)) {
                  val = (val * 10) + (c - '0') ;
               } else {
                  break ;
               }
            }
         }
      }
      yylval.strdef = (id_type *)malloc(sizeof(id_type) + strlen(yytext)) ;
      if (yylval.strdef == NULL) {
         fatal("Out of memory in yylex") ;
      }
      yylval.strdef->next_id = NULL ;
      if (code == SHELL_COMMAND) {
         strcpy(yylval.strdef->id, &yytext[1]) ;
      } else {
         strcpy(yylval.strdef->id, yytext) ;
      }
      return(code) ;
   }
}

/* The main program for iid - parse the command line, initialize processing,
 * loop processing one command at a time.
 */
int
main( int argc , char * argv [ ] )
{
   int         c ;                     /* current option */
   char *      CmdPtr ;                /* Points to the command string */
   char        Command [ MAXCMD ] ;    /* Buffer for reading commands */
   int         Do1 = 0 ;           /* 1 if should only do 1 command */
   int         DoPrompt ;              /* 1 if should write a prompt */
   int         errors = 0 ;            /* error count */

   DoPrompt = isatty(fileno(stdin)) ;
   while ((c = getopt(argc, argv, "Hac:")) != EOF) {
      switch(c) {
      case 'a':
         DefaultCommand = "aid -kmn" ;
         break ;
      case 'c':
         CmdPtr = optarg ;
         Do1 = 1 ;
         break ;
      case 'H':
         fputs("\
iid: interactive ID database query tool. Call with:\n\
   iid [-a] [-c] [-H]\n\
\n\
-a\tUse the aid as the default query command (not lid).\n\
-c cmd\tExecute the single query cmd and exit.\n\
-H\tPrint this message and exit.\n\
\n\
To get help after starting program type 'help'.\n\
",stderr) ;
         exit(0) ;
      default:
         ++errors ;
         break ;
      }
   }
   if (argc != optind) {
      fputs("iid: Excess arguments ignored.\n",stderr) ;
      ++errors ;
   }
   if (errors) {
      fputs("run iid -H for help.\n",stderr) ;
      exit(1) ;
   }

   /* initialize global data */

   InitIid() ;

   /* run the parser */

   if (Do1) {           
      ScanInit(CmdPtr) ;
      exit(yyparse()) ;
   } else {
      for ( ; ; ) {
         if (DoPrompt) {
            fputs(Prompt, stdout) ;
            fflush(stdout) ;
         }
         gets(Command) ;
         if (feof(stdin)) {
            if (DoPrompt) fputs("\n", stdout) ;
            strcpy(Command, "off") ;
         }
         ScanInit(Command) ;
         errors += yyparse() ;
      }
   }
}


/* ArgListSize - count the size of an arg list so can alloca() enough
 * space for the command.
 */
int
ArgListSize( id_list_type * idlp )
{
   id_type *      idep ;
   int            size = 0;
   
   idep = idlp->id_list ;
   while (idep != NULL) {
      size += 1 + strlen(idep->id);
      idep = idep->next_id;
   }
   return size;
}

/* SetListSize - count the size of a string build up from a set so we can
 * alloca() enough space for args.
 */
int
SetListSize( set_type * sp )
{
   int            i ;
   int            size = 0 ;
   
   for (i = 0; i < NextFileNum; ++i) {
      if (FileList[i]->mask_word < sp->set_size) {
         if (sp->set_data[FileList[i]->mask_word] & FileList[i]->mask_bit) {
            size += 1 + strlen(FileList[i]->name);
         }
      }
   }
   return size;
}

/* FlushFiles - clear out the TheFiles array for the start of a new
 * query.
 */
void
FlushFiles( void )
{
   int             i ;
   
   if (TheFiles != NULL) {
      for (i = 0; i <= MaxCurFile; ++i) {
         TheFiles[i] = 0 ;
      }
   }
   MaxCurFile = 0 ;
}

/* fatal - sometimes the only thing to do is die...
 */
void
fatal( char const * s )
{
   fprintf(stderr,"Fatal error: %s\n", s) ;
   exit(1) ;
}

/* CountBits - count the number of bits in a bit set. Actually fairly
 * tricky since it needs to deal with sets having infinite tails
 * as a result of a NOT operation.
 */
int
CountBits( set_type * sp )
{
   unsigned long      bit_mask ;
   int                count = 0 ;
   int                i ;
   
   i = 0;
   for ( ; ; ) {
      for (bit_mask = high_bit; bit_mask != 0; bit_mask >>= 1) {
         if (bit_mask == NextMaskBit && i == NextMaskWord) {
            return(count) ;
         }
         if (i < sp->set_size) {
            if (sp->set_data[i] & bit_mask) {
               ++count ;
            }
         } else {
            if (sp->set_tail == 0) return count;
            if (sp->set_tail & bit_mask) {
               ++count;
            }
         }
      }
      ++i;
   }
}

/* OneDescription - Print a description of a set. This includes
 * the set number, the number of files in the set, and the
 * set description string.
 */
void
OneDescription( set_type * sp )
{
   int        elt_count ;
   char       setnum[20] ;
   
   sprintf(setnum,"S%d",sp->set_num) ;
   elt_count = CountBits(sp) ;
   printf("%5s %6d  %s\n",setnum,elt_count,sp->set_desc) ;
}

/* DescribeSets - Print description of all the sets.
 */
void
DescribeSets( void )
{
   int            i ;

   if (NextSetNum > 0) {
      for (i = 0; i < NextSetNum; ++i) {
         OneDescription(TheSets[i]) ;
      }
   } else {
      printf("No sets defined yet.\n") ;
   }
}

/* SetList - Go through the bit set and add the file names in
 * it to an identifier list.
 */
id_list_type *
SetList( id_list_type * idlp , set_type * sp )
{
   int        i ;
   id_type *  idep ;
   
   for (i = 0; i < NextFileNum; ++i) {
      if (FileList[i]->mask_word < sp->set_size) {
         if (sp->set_data[FileList[i]->mask_word] & FileList[i]->mask_bit) {
            idep = (id_type *)malloc(sizeof(id_type) +
                                     strlen(FileList[i]->name)) ;
            if (idep == NULL) {
               fatal("Out of memory in SetList") ;
            }
            idep->next_id = NULL ;
            strcpy(idep->id, FileList[i]->name) ;
            idlp = ExtendList(idlp, idep) ;
         }
      }
   }
   return(idlp) ;
}

/* PrintSet - Go through the bit set and print the file names
 * corresponding to all the set bits.
 */
void
PrintSet( set_type * sp )
{
   int        i ;
   
   for (i = 0; i < NextFileNum; ++i) {
      if (FileList[i]->mask_word < sp->set_size) {
         if (sp->set_data[FileList[i]->mask_word] & FileList[i]->mask_bit) {
            printf("%s\n",FileList[i]->name) ;
         }
      }
   }
}

/* Free up all space used by current set of sets and reset all
 * set numbers.
 */
void
FlushSets( void )
{
   int         i ;
   
   for (i = 0; i < NextSetNum; ++i) {
      free(TheSets[i]->set_desc) ;
      free(TheSets[i]) ;
   }
   NextSetNum = 0 ;
}

/* InitList - create an empty identifier list.
 */
id_list_type *
InitList( void )
{
   id_list_type *   idlp ;
   
   idlp = (id_list_type *)malloc(sizeof(id_list_type)) ;
   if (idlp == NULL) {
      fatal("Out of memory in InitList") ;
   }
   idlp->id_count = 0 ;
   idlp->end_ptr_ptr = & (idlp->id_list) ;
   idlp->id_list = NULL ;
   return(idlp) ;
}

/* ExtendList - add one identifier to an ID list.
 */
id_list_type *
ExtendList( id_list_type * idlp , id_type * idp )
{
   *(idlp->end_ptr_ptr) = idp ;
   idlp->end_ptr_ptr = &(idp->next_id) ;
   return(idlp) ;
}

/* InitIid - do all initial processing for iid.
 *   1) Determine the size of a unsigned long for bit set stuff.
 *   2) Find out the name of the pager program to use.
 *   3) Create the HelpSet (pointing to the help file).
 *   4) Setup the prompt.
 */
void
InitIid( void )
{
   unsigned long      bit_mask = 1 ;   /* find number of bits in long */
   int                i ;
   char const *       page ;           /* pager program */
   
   do {
      high_bit = bit_mask ;
      bit_mask <<= 1 ;
   } while (bit_mask != 0) ;
   
   NextMaskBit = high_bit ;
   
   page = getenv("PAGER") ;
   if (page == NULL) {
      page = PAGER ;
   }
   strcpy(Pager, page) ;
   
   FlushFiles() ;
   InstallFile(IID_HELP_FILE) ;
   HelpSet = (set_type *)
      malloc(sizeof(set_type) + sizeof(unsigned long) * MaxCurFile) ;
   if (HelpSet == NULL) {
      fatal("No memory for set in InitIid") ;
   }
   HelpSet->set_tail = 0 ;
   HelpSet->set_desc = NULL ;
   HelpSet->set_size = MaxCurFile + 1 ;
   for (i = 0; i <= MaxCurFile; ++i) {
      HelpSet->set_data[i] = TheFiles[i] ;
   }
   
   page = getenv("PS1") ;
   if (page == NULL) {
      page = PROMPT ;
   }
   strcpy(Prompt, page) ;
}

/* InstallFile - install a file name in the symtab. Return the
 * symbol table pointer of the file.
 */
symtab_type *
InstallFile( char const * fp )
{
   char             c ;
   unsigned long    hash_code ;
   int              i ;
   char const *     sp ;
   symtab_type *    symp ;
   
   hash_code = 0 ;
   sp = fp ;
   while ((c = *sp++) != '\0') {
      hash_code <<= 1 ;
      hash_code ^= (unsigned long)(c) ;
      if (hash_code & high_bit) {
         hash_code &= ~ high_bit ;
         hash_code ^= 1 ;
      }
   }
   hash_code %= HASH_SIZE ;
   symp = HashTable[hash_code] ;
   while (symp != NULL && strcmp(symp->name, fp)) {
      symp = symp->hash_link ;
   }
   if (symp == NULL) {
      symp = (symtab_type *)malloc(sizeof(symtab_type) + strlen(fp)) ;
      if (symp == NULL) {
         fatal("No memory for symbol table entry in InstallFile") ;
      }
      strcpy(symp->name, fp) ;
      symp->hash_link = HashTable[hash_code] ;
      HashTable[hash_code] = symp ;
      if (NextMaskWord >= FileSpace) {
         FileSpace += 1000 ;
         if (TheFiles != NULL) {
            TheFiles = (unsigned long *)
               realloc(TheFiles, sizeof(unsigned long) * FileSpace) ;
         } else {
            TheFiles = (unsigned long *)
               malloc(sizeof(unsigned long) * FileSpace) ;
         }
         if (TheFiles == NULL) {
            fatal("No memory for TheFiles in InstallFile") ;
         }
         for (i = NextMaskWord; i < FileSpace; ++i) {
            TheFiles[i] = 0 ;
         }
      }
      symp->mask_word = NextMaskWord ;
      symp->mask_bit = NextMaskBit ;
      NextMaskBit >>= 1 ;
      if (NextMaskBit == 0) {
         NextMaskBit = high_bit ;
         ++NextMaskWord ;
      }
      if (NextFileNum >= ListSpace) {
         ListSpace += 1000 ;
         if (FileList == NULL) {
            FileList = (symtab_type **)
               malloc(sizeof(symtab_type *) * ListSpace) ;
         } else {
            FileList = (symtab_type **)
               realloc(FileList, ListSpace * sizeof(symtab_type *)) ;
         }
         if (FileList == NULL) {
            fatal("No memory for FileList in InstallFile") ;
         }
      }
      FileList[NextFileNum++] = symp ;
      /* put code here to sort the file list by name someday */
   }
   TheFiles[symp->mask_word] |= symp->mask_bit ;
   if (symp->mask_word > MaxCurFile) {
      MaxCurFile = symp->mask_word ;
   }
   return(symp) ;
}

/* RunPager - run the users pager program on the list of files
 * in the set.
 */
void
RunPager( char * pp , set_type * sp )
{
   char *         cmd ;
   int            i ;
   
   cmd = (char *)TEMP_ALLOC(SetListSize(sp) + strlen(pp) + 2);
   strcpy(cmd, pp) ;
   for (i = 0; i < NextFileNum; ++i) {
      if (FileList[i]->mask_word < sp->set_size) {
         if (sp->set_data[FileList[i]->mask_word] & FileList[i]->mask_bit) {
            strcat(cmd, " ") ;
            strcat(cmd, FileList[i]->name) ;
         }
      }
   }
   system(cmd) ;
   TEMP_FREE(cmd) ;
}

/* AddSet - add a new set to the universal list of sets. Assign
 * it the next set number.
 */
void
AddSet( set_type * sp )
{
   if (NextSetNum >= SetSpace) {
      SetSpace += 1000 ;
      if (TheSets != NULL) {
         TheSets = (set_type **)
            realloc(TheSets, sizeof(set_type *) * SetSpace) ;
      } else {
         TheSets = (set_type **)
            malloc(sizeof(set_type *) * SetSpace) ;
      }
      if (TheSets == NULL) {
         fatal("No memory for TheSets in AddSet") ;
      }
   }
   sp->set_num = NextSetNum ;
   TheSets[NextSetNum++] = sp ;
}

/* RunProg - run a program with arguments from id_list and
 * accept list of file names back from the program which
 * are installed in the symbol table and used to construct
 * a new set.
 */
set_type *
RunProg( char const * pp , id_list_type * idlp )
{
   int            c ;
   char *         cmd ;
   char *         dp ;
   char           file [ MAXCMD ] ;
   int            i ;
   id_type *      idep ;
   id_type *      next_id ;
   FILE *         prog ;
   set_type *     sp ;
   
   cmd = (char *)TEMP_ALLOC(ArgListSize(idlp) + strlen(pp) + 2);
   FlushFiles() ;
   strcpy(cmd, pp) ;
   idep = idlp->id_list ;
   while (idep != NULL) {
      strcat(cmd, " ") ;
      strcat(cmd, idep->id) ;
      next_id = idep->next_id ;
      free(idep) ;
      idep = next_id ;
   }
   free(idlp) ;
   
   /* run program with popen, reading the output. Assume each
    * white space terminated string is a file name.
    */
   prog = popen(cmd, "r") ;
   dp = file ;
   while ((c = getc(prog)) != EOF) {
      if (isspace(c)) {
         if (dp != file) {
            *dp++ = '\0' ;
            InstallFile(file) ;
            dp = file ;
         }
      } else {
         *dp++ = c ;
      }
   }
   if (dp != file) {
      *dp++ = '\0' ;
      InstallFile(file) ;
   }
   if (pclose(prog) != 0) {
      /* if there was an error make an empty set, who knows what
       * garbage the program printed.
       */
      FlushFiles() ;
   }
   
   sp = (set_type *)
      malloc(sizeof(set_type) + sizeof(unsigned long) * MaxCurFile) ;
   if (sp == NULL) {
      fatal("No memory for set in RunProg") ;
   }
   sp->set_tail = 0 ;
   sp->set_desc = (char *)malloc(strlen(cmd) + 1) ;
   if (sp->set_desc == NULL) {
      fatal("No memory for set description in RunProg") ;
   }
   strcpy(sp->set_desc, cmd) ;
   sp->set_size = MaxCurFile + 1 ;
   for (i = 0; i <= MaxCurFile; ++i) {
      sp->set_data[i] = TheFiles[i] ;
   }
   AddSet(sp) ;
   TEMP_FREE(cmd);
   return(sp) ;
}

/* SetDirectory - change the working directory. This will
 * determine which ID file is found by the subprograms.
 */
void
SetDirectory( id_type * dir )
{
   if (chdir(dir->id) != 0) {
      fprintf(stderr,"Directory %s not accessible.\n", dir->id) ;
   }
   free(dir) ;
}

/* SetIntersect - construct a new set from the intersection
 * of two others. Also construct a new description string.
 */
set_type *
SetIntersect( set_type * sp1 , set_type * sp2 )
{
   char *     desc ;
   int        i ;
   int        len1 ;
   int        len2 ;
   set_type * new_set ;
   int        new_size ;
   
   if (sp1->set_tail || sp2->set_tail) {
      new_size = MAX(sp1->set_size, sp2->set_size) ;
   } else {
      new_size = MIN(sp1->set_size, sp2->set_size) ;
   }
   new_set = (set_type *)malloc(sizeof(set_type) +
                                (new_size - 1) * sizeof(unsigned long)) ;
   if (new_set == NULL) {
      fatal("No memory for set in SetIntersect") ;
   }
   len1 = strlen(sp1->set_desc) ;
   len2 = strlen(sp2->set_desc) ;
   desc = (char *)malloc(len1 + len2 + 10) ;
   if (desc == NULL) {
      fatal("No memory for set description in SetIntersect") ;
   }
   new_set->set_desc = desc ;
   strcpy(desc,"(") ;
   ++desc ;
   strcpy(desc, sp1->set_desc) ;
   desc += len1 ;
   strcpy(desc, ") AND (") ;
   desc += 7 ;
   strcpy(desc, sp2->set_desc) ;
   desc += len2 ;
   strcpy(desc, ")") ;
   AddSet(new_set) ;
   new_set->set_size = new_size ;
   for (i = 0; i < new_size; ++i) {
      new_set->set_data[i] = 
         ((i < sp1->set_size) ? sp1->set_data[i] : sp1->set_tail) & 
         ((i < sp2->set_size) ? sp2->set_data[i] : sp2->set_tail) ;
   }
   new_set->set_tail = sp1->set_tail & sp2->set_tail ;
   return(new_set) ;
}

/* SetUnion - construct a new set from the union of two others.
 * Also construct a new description string.
 */
set_type *
SetUnion( set_type * sp1 , set_type * sp2 )
{
   char *     desc ;
   int        i ;
   int        len1 ;
   int        len2 ;
   set_type * new_set ;
   int        new_size ;
   
   new_size = MAX(sp1->set_size, sp2->set_size) ;
   new_set = (set_type *)malloc(sizeof(set_type) +
                                (new_size - 1) * sizeof(unsigned long)) ;
   if (new_set == NULL) {
      fatal("No memory for set in SetUnion") ;
   }
   len1 = strlen(sp1->set_desc) ;
   len2 = strlen(sp2->set_desc) ;
   desc = (char *)malloc(len1 + len2 + 9) ;
   if (desc == NULL) {
      fatal("No memory for set description in SetUnion") ;
   }
   new_set->set_desc = desc ;
   strcpy(desc,"(") ;
   ++desc ;
   strcpy(desc, sp1->set_desc) ;
   desc += len1 ;
   strcpy(desc, ") OR (") ;
   desc += 6 ;
   strcpy(desc, sp2->set_desc) ;
   desc += len2 ;
   strcpy(desc, ")") ;
   AddSet(new_set) ;
   new_set->set_size = new_size ;
   for (i = 0; i < new_size; ++i) {
      new_set->set_data[i] =
         ((i < sp1->set_size) ? (sp1->set_data[i]) : sp1->set_tail) |
         ((i < sp2->set_size) ? (sp2->set_data[i]) : sp2->set_tail) ;
   }
   new_set->set_tail = sp1->set_tail | sp2->set_tail ;
   return(new_set) ;
}

/* SetInverse - construct a new set from the inverse of another.
 * Also construct a new description string.
 *
 * This is kind of tricky. An inverse set in iid may grow during
 * the course of a session. By NOTing the set_tail extension the
 * inverse at any given time will be defined as the inverse against
 * a universe that grows as additional queries are made and new files
 * are added to the database.
 *
 * Several alternative definitions were possible (snapshot the
 * universe at the time of the NOT, go read the ID file to
 * determine the complete universe), but this one was the one
 * I picked.
 */
set_type *
SetInverse( set_type * sp )
{
   char *     desc ;
   int        i ;
   set_type * new_set ;
   
   new_set = (set_type *)malloc(sizeof(set_type) +
                                (sp->set_size - 1) * sizeof(unsigned long)) ;
   if (new_set == NULL) {
      fatal("No memory for set in SetInverse") ;
   }
   desc = (char *)malloc(strlen(sp->set_desc) + 5) ;
   if (desc == NULL) {
      fatal("No memory for set description in SetInverse") ;
   }
   new_set->set_desc = desc ;
   strcpy(desc,"NOT ") ;
   desc += 4 ;
   strcpy(desc, sp->set_desc) ;
   AddSet(new_set) ;
   new_set->set_size = sp->set_size ;
   for (i = 0; i < sp->set_size; ++i) {
      new_set->set_data[i] = ~ sp->set_data[i] ;
   }
   new_set->set_tail = ~ sp->set_tail ;
   return(new_set) ;
}

/* RunShell - run a program with arguments from id_list.
 */
void
RunShell( char * pp , id_list_type * idlp )
{
   char *         cmd ;
   id_type *      idep ;
   id_type *      next_id ;
   
   cmd = (char *)TEMP_ALLOC(ArgListSize(idlp) + strlen(pp) + 2);
   strcpy(cmd, pp) ;
   idep = idlp->id_list ;
   while (idep != NULL) {
      strcat(cmd, " ") ;
      strcat(cmd, idep->id) ;
      next_id = idep->next_id ;
      free(idep) ;
      idep = next_id ;
   }
   free(idlp) ;
   system(cmd) ;
   TEMP_FREE(cmd);
}
