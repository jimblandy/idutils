%{
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

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include <config.h>
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

int yyerror __P(( char const * s )) ;
void ScanInit __P(( char * line )) ;
int yylex __P(( void )) ;
int ArgListSize __P(( id_list_type * idlp )) ;
int SetListSize __P(( set_type * sp )) ;
void FlushFiles __P(( void )) ;
void fatal __P(( char const * s )) ;
int CountBits __P(( set_type * sp )) ;
void OneDescription __P(( set_type * sp )) ;
void DescribeSets __P(( void )) ;
id_list_type * SetList __P(( id_list_type * idlp , set_type * sp )) ;
void PrintSet __P(( set_type * sp )) ;
void FlushSets __P(( void )) ;
id_list_type * InitList __P(( void )) ;
id_list_type * ExtendList __P(( id_list_type * idlp , id_type * idp )) ;
void InitIid __P(( void )) ;
symtab_type * InstallFile __P(( char const * fp )) ;
void RunPager __P(( char * pp , set_type * sp )) ;
void AddSet __P(( set_type * sp )) ;
set_type * RunProg __P(( char const * pp , id_list_type * idlp )) ;
void SetDirectory __P(( id_type * dir )) ;
set_type * SetIntersect __P(( set_type * sp1 , set_type * sp2 )) ;
set_type * SetUnion __P(( set_type * sp1 , set_type * sp2 )) ;
set_type * SetInverse __P(( set_type * sp )) ;
void RunShell __P(( char * pp , id_list_type * idlp )) ;

%}

%union {
   set_type *     setdef ;   
   id_type *      strdef ;
   id_list_type * listdef ;
}

%token < setdef > SET

%token < strdef > ID SHELL_QUERY SHELL_COMMAND

%type < setdef > Query Primitive

%type < listdef > Lid_group Aid_group Id_list Command_list

%token LID AID BEGIN SETS SS FILES SHOW HELP OFF MATCH

%left OR

%left AND

%left NOT

%start Command

%%

Command :
   BEGIN ID
      {
         /* cd to the directory specified as argument, flush sets */

         SetDirectory($2) ;
         FlushSets() ;
      }
|  Set_query Query
|  File_query Query
      {
         /* print the list of files resulting from Query */

         PrintSet($2) ;
      }
|  SHOW SET
      {
         /* run PAGER on the list of files in SET */

         RunPager(Pager, $2) ;
      }
|  SETS
      {
         /* describe sets created so far */

         DescribeSets() ;
      }
|  HELP
      {
         /* run PAGER on the help file */

         RunPager(Pager, HelpSet) ;
      }
|  OFF
      {
         exit(0) ;
      }
| SHELL_QUERY Command_list
      {
         /* run the shell command and eat the results as a file set */

         OneDescription(RunProg($1->id, $2)) ;
         free($1) ;
      }
| SHELL_COMMAND Command_list
      {
         /* run the shell command */

         RunShell($1->id, $2) ;
         free($1) ;
      }
;

Set_query :
   SS
      {
         /* Turn on verbose query flag */

         VerboseQuery = 1 ;
      }
;

File_query :
   FILES
      {
         /* Turn off verbose query flag */

         VerboseQuery = 0 ;
      }
;

Query :
   Primitive
      {
         /* value of query is set associated with primitive */

         $$ = $1 ;
      }
|  Query AND Query
      {
         /* value of query is intersection of the two query sets */

         $$ = SetIntersect($1, $3) ;
         if (VerboseQuery) {
            OneDescription($$) ;
         }
      }
|  Query OR Query
      {
         /* value of query is union of the two query sets */

         $$ = SetUnion($1, $3) ;
         if (VerboseQuery) {
            OneDescription($$) ;
         }
      }
|  NOT Query
      {
         /* value of query is inverse of other query */
         
         $$ = SetInverse($2) ;
         if (VerboseQuery) {
            OneDescription($$) ;
         }
      }
;

Primitive :
   SET
      {
         /* Value of primitive is value of recorded set */

         $$ = $1 ;
      }
|  Lid_group
      {
         /* Value of primitive is obtained by running an lid query */

         $$ = RunProg(LidCommand, $1) ;
         if (VerboseQuery) {
            OneDescription($$) ;
         }
      }
|  Aid_group
      {
         /* Value of primitive is obtained by running an aid query */

         $$ = RunProg("aid -kmn", $1) ;
         if (VerboseQuery) {
            OneDescription($$) ;
         }
      }
|  MATCH Id_list
      {
         /* Match names from database against pattern */
         $$ = RunProg("pid -kmn", $2) ;
         if (VerboseQuery) {
            OneDescription($$) ;
         }
      }
|  '(' Query ')'
      {
         /* value of primitive is value of query */

         $$ = $2 ;
      }
;

Lid_group :
   ID
      {
         /* make arg list holding single ID */

         $$ = InitList() ;
         $$ = ExtendList($$, $1) ;
         LidCommand = DefaultCommand ;
      }
|  LID Id_list
      {
         /* arg list is Id_list */

         $$ = $2 ;
         LidCommand = "lid -kmn" ;
      }
;

Aid_group :
   AID Id_list
      {
         /* arg list is Id_list */

         $$ = $2 ;
      }
;

Command_list :
   ID
      {
         /* make arg list holding single ID */

         $$ = InitList() ;
         $$ = ExtendList($$, $1) ;
      }
|  SET
      {
         /* make arg list holding names from set */

         $$ = InitList() ;
         $$ = SetList($$, $1) ;
      }
|  Command_list ID
      {
         /* extend arg list with additional ID */

         $$ = ExtendList($1, $2) ;
      }
|  Command_list SET
      {
         /* extend arg list with additional file names */

         $$ = SetList($1, $2) ;
      }
;

Id_list :
   ID
      {
         /* make arg list holding single ID */

         $$ = InitList() ;
         $$ = ExtendList($$, $1) ;
      }
|  Id_list ID
      {
         /* extend arg list with additional ID */

         $$ = ExtendList($1, $2) ;
      }
;

%%

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
   char *      CmdPtr = NULL ;         /* Points to the command string */
   char        Command [ MAXCMD ] ;    /* Buffer for reading commands */
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

   if (CmdPtr) {           
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
