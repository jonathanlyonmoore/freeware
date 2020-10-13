/*
 * $Header: /usr/build/vile/vile/RCS/regexp.c,v 1.118 2006/04/20 00:01:45 tom Exp $
 *
 * Copyright 2005, Thomas E. Dickey and Paul G. Fox
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, distribute with modifications, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization.
 *
 * ----------------------------------------------------------------------------
 *
 *	This code has been MODIFIED for use in vile (see the original
 *	copyright information down below) -- in particular:
 *	 - regexec no longer needs to scan a null terminated string
 *	 - regexec takes two extra arguments describing the first and
 *	 	just-past-last legal scan start offsets, to limit matches
 *		to beginning in that range
 *	 - inexact character matches are now handled, if the global ignorecase
 *	 	is set
 *	 - regexps are now relocatable, rather than locked down
 *
 *		pgf, 11/91
 *
 *	  - allow repeat counts in \{braces\}
 *	  - implement character-classes.
 *
 *		ted, 01/01
 *
 *	  - modify to support patterns with embedded nulls
 *
 *		ted, 02/03
 *
 *	  - add test-driver
 *
 *		ted, 05/03
 */

/*
 * regcomp and regexec -- regsub and regerror are elsewhere
 *
 *	Copyright (c) 1986 by University of Toronto.
 *	Written by Henry Spencer.  Not derived from licensed software.
 *
 *	Permission is granted to anyone to use this software for any
 *	purpose on any computer system, and to redistribute it freely,
 *	subject to the following restrictions:
 *
 *	1. The author is not responsible for the consequences of use of
 *		this software, no matter how awful, even if they arise
 *		from defects in it.
 *
 *	2. The origin of this software must not be misrepresented, either
 *		by explicit claim or by omission.
 *
 *	3. Altered versions must be plainly marked as such, and must not
 *		be misrepresented as being the original software.
 *
 * Beware that some of this code is subtly aware of the way operator
 * precedence is structured in regular expressions.  Serious changes in
 * regular-expression syntax might require a total rethink.
 */

#ifdef DEBUG_REGEXP
#define UNBUNDLED_VILE_REGEX
#endif

#ifdef UNBUNDLED_VILE_REGEX

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPT_VILE_CTYPE 0	/* use macros but not data */

#include <vl_regex.h>
#include <vl_alloc.h>
#include <vl_ctype.h>

#else

#include <estruct.h>

#endif

#if OPT_VILE_CTYPE
#include <edef.h>		/* use global data from vile */
#endif

#if !defined(OPT_WORKING) || defined(DEBUG_REGEXP)
# undef  beginDisplay
# define beginDisplay()		/* nothing */
# undef  endofDisplay
# define endofDisplay()		/* nothing */
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NO_LEAKS
#define NO_LEAKS 0
#endif

#ifndef OPT_TRACE
#define OPT_TRACE 0
#endif

#ifndef TRACE
#define TRACE(p)		/* nothing */
#endif

#ifdef DEBUG_REGEXP

/* #define OPT_TRACE 1 */
/* #define REGDEBUG  1 */

# define visible_buff(s,n,e) s
# if OPT_TRACE
#  undef  TRACE
#  define TRACE(p)		/* nothing */
# endif
#endif

#if OPT_TRACE
/* #define REGDEBUG  1 */
#endif

#ifdef REGDEBUG
#define REGTRACE(p) TRACE(p)
#else
#define REGTRACE(p)		/*nothing */
#endif

#undef PLUS			/* vile conflict */
#undef min
#undef max

#if !OPT_VILE_CTYPE
int ignorecase = FALSE;
#endif

static char *reg(int paren, int *flagp);
static char *regatom(int *flagp, int at_bop);
static char *regbranch(int *flagp);
static char *regnext(char *p);
static char *regnode(int op);
static char *regpiece(int *flagp, int at_bop);
static int regmatch(char *prog, int found);
static int regrepeat(const char *p);
static int regtry(regexp * prog, char *string, char *stringend);
static void regc(int b);
static void regninsert(int n, char *opnd);
static void regopinsert(int op, char *opnd);
static void regoptail(char *p, char *val);
static void regrange(int op, char *opnd, int lo, int hi);
static void regtail(char *p, char *val);
static void set_opsize(void);

#ifdef REGDEBUG
static int regnarrate = 1;
static char *reg_program;
static char *regdump1(char *base, char *op);
static void regdump(regexp * r);
static char *regprop(char *op);
#endif

/*
 * The "internal use only" fields in regexp.h are present to pass info from
 * compile to execute that permits the execute phase to run lots faster on
 * simple cases.  They are:
 *
 * regstart	char that must begin a match; -1 if none obvious
 * reganch	is the match anchored (at beginning-of-line only)?
 * regmust	string (starting at program[offset]) that match must
 *				include, or NULL
 * regmlen	length of regmust string
 *
 * Regstart and reganch permit very fast decisions on suitable starting points
 * for a match, cutting down the work a lot.  Regmust permits fast rejection
 * of lines that cannot possibly match.  The regmust tests are costly enough
 * that regcomp() supplies a regmust only if the r.e. contains something
 * potentially expensive (at present, the only such thing detected is * or +
 * at the start of the r.e., which can involve a lot of backup).  Regmlen is
 * supplied because the test in regexec() needs it and regcomp() is computing
 * it anyway.
 */

/*
 * Structure for regexp "program".  This is essentially a linear encoding
 * of a nondeterministic finite-state machine (aka syntax charts or
 * "railroad normal form" in parsing technology).  Each node is an opcode
 * plus a "next" pointer, possibly plus an operand.  "Next" pointers of
 * all nodes except BRANCH implement concatenation; a "next" pointer with
 * a BRANCH on both ends of it is connecting two alternatives.  (Here we
 * have one of the subtle syntax dependencies:  an individual BRANCH (as
 * opposed to a collection of them) is never concatenated with anything
 * because of operator precedence.)  The operand of some types of node is
 * a literal string; for others, it is a node leading into a sub-FSM.  In
 * particular, the operand of a BRANCH node is the first node of the branch.
 * (NB this is *not* a tree structure:  the tail of the branch connects
 * to the thing following the set of BRANCHes.)  The opcodes are:
 */

/* definition		opnd?	meaning */
typedef enum {
    END = 0			/* no   End of program. */
    ,BOL			/* no   Match "" at beginning of line. */
    ,EOL			/* no   Match "" at end of line. */
    ,ANY			/* no   Match any one character. */
    ,ANYOF			/* str  Match any character in this string. */
    ,ANYBUT			/* str  Match any character not in this string. */
    ,BRANCH			/* node Match this alternative, or the next... */
    ,BACK			/* no   Match "", "next" ptr points backward. */
    ,EXACTLY			/* str  Match this string. */
    ,NOTHING			/* no   Match empty string. */
    ,STAR			/* node Match this (simple) thing 0 or more times. */
    ,PLUS			/* node Match this (simple) thing 1 or more times. */
    ,RSIMPLE			/* node Match this (simple) thing min..max times */
    ,RCOMPLX			/* node Match this (complex) thing min..max times */

    ,BEGWORD			/* node Match "" between nonword and word. */
    ,ENDWORD			/* node Match "" between word and nonword. */

    ,ALNUM			/* node Match any alphanumeric, include _ */
    ,ALPHA			/* node Match any alphabetic */
    ,BLANK			/* node Match any space char */
    ,CNTRL			/* node Match any control char */
    ,DIGIT			/* node Match any digit */
    ,GRAPH			/* node Match any graphic char (no whitesp) */
    ,LOWER			/* node Match any lowercase char */
    ,NALNUM			/* node Match any non-identifier char */
    ,NALPHA			/* node Match any alphabetic */
    ,NBLANK			/* node Match any non-space char */
    ,NCNTRL			/* node Match any non-control char */
    ,NDIGIT			/* node Match any non-digit */
    ,NGRAPH			/* node Match any non-graphic char */
    ,NLOWER			/* node Match any non-lowercase char */
    ,NPRINT			/* node Match any non-printable char */
    ,NPUNCT			/* node Match any non-punctuation char  */
    ,NSPACE			/* node Match single nonwhitespace, excluding BOL and EOL */
    ,NUPPER			/* node Match any non-uppercase char */
    ,NXDIGIT			/* node Match any non-hex digit */
    ,PRINT			/* node Match any printable char (including whitesp) */
    ,PUNCT			/* node Match any punctuation char */
    ,SPACE			/* node Match single whitespace, excluding BOL and EOL */
    ,UPPER			/* node Match any uppercase char */
    ,XDIGIT			/* node Match any hex digit */

    ,NEVER			/* no   No match */
    ,OPEN			/* no   Mark this point in input as start of #n. */
    ,OPEN1, OPEN2, OPEN3, OPEN4, OPEN5, OPEN6, OPEN7, OPEN8, OPEN9
    ,CLOSE			/* no   Analogous to OPEN. */
    ,CLOSE1, CLOSE2, CLOSE3, CLOSE4, CLOSE5, CLOSE6, CLOSE7, CLOSE8, CLOSE9
} REGEXP_OP;

/*
 * Macros to ensure consistent use of character classes:
 */
#define is_ALPHA(c) isAlpha(c)
#define is_ALNUM(c) isident(c)
#define is_BLANK(c) (isSpace(c) && !isreturn(c))
#define is_CNTRL(c) isCntrl(c)
#define is_DIGIT(c) isDigit(c)
#define is_GRAPH(c) isGraph(c)
#define is_LOWER(c) isLower(c)
#define is_PRINT(c) (isPrint(c) || (isSpace(c) && !isCntrl(c)))
#define is_PUNCT(c) isPunct(c)
#define is_SPACE(c) isSpace(c)
#define is_UPPER(c) isUpper(c)

#ifdef isXDigit
#define is_XDIGIT(c) isXDigit(c)
#else
#define is_XDIGIT(c) (isDigit(c) || (isLower(c) && (c) - 'a' < 6) || (isUpper(c) && (c) - 'A' < 6))
#endif

/*
 * Opcode notes:
 *
 * BRANCH	The set of branches constituting a single choice are hooked
 *		together with their "next" pointers, since precedence prevents
 *		anything being concatenated to any individual branch.  The
 *		"next" pointer of the last BRANCH in a choice points to the
 *		thing following the whole choice.  This is also where the
 *		final "next" pointer of each individual branch points; each
 *		branch starts with the operand node of a BRANCH node.
 *
 * BACK		Normal "next" pointers all implicitly point forward; BACK
 *		exists to make loop structures possible.
 *
 * STAR,PLUS	'?', and complex '*' and '+', are implemented as circular
 *		BRANCH structures using BACK.  Simple cases (one character
 *		per match) are implemented with STAR and PLUS for speed
 *		and to minimize recursive plunges.
 *
 * OPEN,CLOSE	...are numbered at compile time.
 */

/*
 * A node is one char of opcode followed by two chars of "next" pointer.
 * "Next" pointers are stored as two 8-bit pieces, high order first.  The
 * value is a positive offset from the opcode of the node containing it.
 * An operand, if any, simply follows the node.  (Note that much of the
 * code generation knows about this implicit relationship.)
 *
 * Using two bytes for the "next" pointer is vast overkill for most things,
 * but allows patterns to get big without disasters.
 *
 * We added OPSIZE() to store the length of an ANYOF, ANYBUT or EXACTLY
 * OPERAND() so we can allow it to contain embedded nulls.  We cannot use
 * NEXT() for this purpose, since it may contain an offset past additional
 * operations following the current one.
 */

#define OP_HDR		5	/* header chars in normal operation */

#define	OP(p)		((p)[0])
#define	NEXT(p)		((unsigned)((CharOf((p)[1]) << 8) + CharOf((p)[2])))
#define	OPSIZE(p)	((unsigned)((CharOf((p)[3]) << 8) + CharOf((p)[4])))
#define	OPERAND(p)	((p) + OP_HDR)

#define SET_NEXT(p, n)	(p)[1] = CharOf(n >> 8),\
 			(p)[2] = CharOf(n)

#define SET_SIZE(p, n)	(p)[3] = CharOf(n >> 8),\
 			(p)[4] = CharOf(n)

/*
 * RSIMPLE/RCOMPLX operations have a different layout:  OP(), NEXT(), and
 * RR_MIN(), RR_MAX() are used to extract their data, i.e., the minimum and
 * maximum repeat counts, respectively.  A zero repeat-count means that the
 * value is unspecified.
 */
#define RR_BYTES  sizeof(int)
#define RR_LEN		3 + (2 * RR_BYTES)	/* sizeof(RSIMPLE/RCOMPLX) */
#define RR_MIN(p) ((p)[3])
#define RR_MAX(p) ((p)[3 + RR_BYTES])

#define SAME(a,b) (ignorecase ? nocase_eq(a,b) : (CharOf(a) == CharOf(b)))
#define STRSKIP(s) ((s) + strlen(s))

/*
 * See regmagic.h for one further detail of program structure.
 */

/*
 * Utility definitions.
 */

#define L_CURL '{'
#define R_CURL '}'

#define	ISMULT(c)	((c) == '*' || (c) == '+' || (c) == '?' || (c) == L_CURL)
#define	META		"^$.[()|?+*\\<>{}"

#define ANY_ESC "aAbBcCdDgGlLpPqQsSuUwWxX"

/*
 * Flags to be passed up and down.
 */
#define	HASWIDTH	01	/* Known never to match null string. */
#define	SIMPLE		02	/* Simple enough to be STAR/PLUS operand. */
#define	SPSTART		04	/* Starts with * or +. */
#define	WORST		0	/* Worst case. */

/*
 * Global work variables for regcomp().
 */
static char *regparse;		/* Input-scan pointer. */
static char *reglimit;		/* end of input-scan pointer */
static int regnpar;		/* () count. */
static char regdummy;
static char *regcode;		/* Code-emit pointer; &regdummy = don't. */
static long regsize;		/* Code size. */

static char *op_pointer;	/* cached from regnode() */
static int op_length;		/* ...corresponding operand-length */

/*
 *				regexp		in magic	in nomagic
 *				char		enter as	enter as
 *				-------		--------	--------
 *	0 or 1			?		\?		\?
 *	1 or more		+		\+		\+
 *	0 or more		*		*		\*
 *	beg. nest		(		\(		\(
 *	end nest		)		\(		\)
 *	beg chr class		[		[		\[
 *	beg "word"		<		\<		\<
 *	end "word"		>		\>		\>
 *	beg "limit"		{		\{		\{
 *	end "limit"		}		\}		\}
 *	beginning		^		^		^
 *	end			$		$		$
 *	any char		.		.		\.
 *	alternation		|		\|		\|
 *	flip or literal		\		\\		\\
 *	last replace		~		~		\~
 *	words			\w		\w		\w
 *	spaces			\s		\s		\s
 *	digits			\d		\d		\d
 *	printable		\p		\p		\p
 *
 *	So:  in magic mode, we remove \ from ? + ( ) < > { } |
 *			   and add \ to bare ? + ( ) < > { } |
 *	   in nomagic mode, we remove \ from ? + ( ) < > { } | * [ ] . ~
 *			   and add \ to bare ? + ( ) < > { } | * [ ] . ~
 */

#define MAGICMETA   "?+()<>{}|"
#define NOMAGICMETA "?+()<>{}|*[] .~"

static int
regmassage(const char *in_text,
	   size_t in_size,
	   char *out_text,
	   size_t *out_size,
	   int magic)
{
    const char *metas = ((magic > 0)
			 ? MAGICMETA
			 : (magic < 0)
			 ? "<>"
			 : NOMAGICMETA);
    char *nxt = out_text;
    size_t n;

    for (n = 0; n < in_size; ++n) {
	if (in_text[n] == BACKSLASH) {	/* remove \ from these metas */
	    if ((n + 1) >= in_size) {
#ifdef FAIL_TRAILING_BS
		mlforce("trailing backslash");
		return FALSE;
#else
		*nxt++ = BACKSLASH;
		break;
#endif
	    }
	    if (in_text[n + 1] != EOS && strchr(metas, in_text[n + 1])) {
		++n;
	    } else if (in_text[n + 1] == BACKSLASH) {
		*nxt++ = in_text[++n];	/* the escape */
	    }
	} else if (in_text[n] != EOS && strchr(metas, in_text[n])) {	/* add \ to these metas */
	    *nxt++ = BACKSLASH;
	}
	*nxt++ = in_text[n];	/* the char */
    }
    *nxt = EOS;
    *out_size = (nxt - out_text);
    return TRUE;
}

/*
 - regcomp - compile a regular expression into internal code
 *
 * We can't allocate space until we know how big the compiled form will be,
 * but we can't compile it (and thus know how big it is) until we've got a
 * place to put the code.  So we cheat:  we compile it twice, once with code
 * generation turned off and size counting turned on, and once "for real".
 * This also means that we don't allocate space until we are sure that the
 * thing really will compile successfully, and we never have to move the
 * code and thus invalidate pointers into it.  (Note that it has to be in
 * one piece because free() must be able to free it all.)
 *
 * Beware that the optimization-preparation code in here knows about some
 * of the structure of the compiled regexp.
 */

regexp *
regcomp(const char *exp_text, size_t exp_len, int magic)
{
    regexp *r;
    char *scan;
    char *longest;
    size_t len;
    size_t parsed_len;
    int flags;
    static char *exp;
    static size_t explen;

    if (exp_text == NULL) {
	regerror("NULL argument");
	return NULL;
    }

    TRACE(("regcomp(\"%s\", %smagic)\n",
	   visible_buff(exp_text, exp_len, 0),
	   magic ? "" : "no"));

    len = exp_len + 1;
    if (explen < 2 * len + 20) {

	beginDisplay();
	if (exp)
	    free(exp);
	exp = castalloc(char, 2 * len + 20);
	endofDisplay();

	if (exp == NULL) {
	    regerror("couldn't allocate exp copy");
	    return NULL;
	}
	explen = 2 * len + 20;
    }

    if (!regmassage(exp_text, exp_len, exp, &parsed_len, magic))
	return NULL;
    TRACE(("after regmassage: '%s'\n", exp));

    /* First pass: determine size, legality. */
    REGTRACE(("First pass: determine size, legality.\n"));
    regparse = exp;
    reglimit = exp + parsed_len;
    regnpar = 1;
    regsize = 0;
    regcode = &regdummy;
    regc(REGEXP_MAGIC);
    if (reg(0, &flags) == NULL)
	return (NULL);

    /* Small enough for pointer-storage convention? */
    if (regsize >= 32767) {	/* Probably could be 65535. */
	regerror("regexp too big");
	return NULL;
    }

    /* Allocate space. */
    beginDisplay();
    r = typeallocplus(regexp, regsize);
    endofDisplay();
    if (r == NULL) {
	regerror("out of space");
	return NULL;
    }

    /* how big is it?  (vile addition) */
    r->size = sizeof(regexp) + regsize;

    /* Second pass: emit code. */
    REGTRACE(("Second pass: emit code\n"));
    regparse = exp;
    reglimit = exp + parsed_len;
    regnpar = 1;
    regcode = r->program;
    regc(REGEXP_MAGIC);
    if (reg(0, &flags) == NULL)
	return (NULL);

    /* Dig out information for optimizations. */
    r->regstart = -1;		/* Worst-case defaults. */
    r->reganch = 0;
    r->regmust = -1;
    r->regmlen = 0;
    scan = r->program + 1;	/* First BRANCH. */
    if (OP(regnext(scan)) == END) {	/* Only one top-level choice. */
	scan = OPERAND(scan);

	/* Starting-point info. */
	if (OP(scan) == EXACTLY)
	    r->regstart = CharOf(*OPERAND(scan));
	else if (OP(scan) == BEGWORD && OP(regnext(scan)) == EXACTLY)
	    r->regstart = CharOf(*OPERAND(regnext(scan)));
	else if (OP(scan) == BOL)
	    r->reganch++;

	/*
	 * If there's something expensive in the r.e., find the
	 * longest literal string that must appear and make it the
	 * regmust.  Resolve ties in favor of later strings, since
	 * the regstart check works with the beginning of the r.e.
	 * and avoiding duplication strengthens checking.  Not a
	 * strong reason, but sufficient in the absence of others.
	 */
	if (flags & SPSTART) {
	    longest = NULL;
	    len = 0;
	    for (; scan != NULL; scan = regnext(scan))
		if (OP(scan) == EXACTLY && OPSIZE(scan) >= len) {
		    longest = OPERAND(scan);
		    len = OPSIZE(scan);
		}
	    if (longest) {
		r->regmust = (int) (longest - r->program);
		r->regmlen = len;
	    }
	}
    }
#if NO_LEAKS
    if (exp != 0) {
	beginDisplay();
	free(exp);
	exp = 0;
	explen = 0;
	endofDisplay();
    }
#endif
#ifdef REGDEBUG
    regdump(r);
#endif
    return (r);
}

/*
 - reg - regular expression, i.e. main body or parenthesized thing
 *
 * Caller must absorb opening parenthesis.
 *
 * Combining parenthesis handling with the base level of regular expression
 * is a trifle forced, but the need to tie the tails of the branches to what
 * follows makes it hard to avoid.
 */
static char *
reg(
       int paren,		/* Parenthesized? */
       int *flagp)
{
    char *ret;
    char *br;
    char *ender;
    int parno;
    int flags;

    *flagp = HASWIDTH;		/* Tentatively. */

    /* Make an OPEN node, if parenthesized. */
    if (paren) {
	if (regnpar >= NSUBEXP) {
	    regerror("too many ()");
	    return NULL;
	}
	parno = regnpar;
	regnpar++;
	ret = regnode(OPEN + parno);
    } else {
	ret = NULL;
	parno = 0;
    }

    /* Pick up the branches, linking them together. */
    br = regbranch(&flags);
    if (br == NULL)
	return (NULL);
    if (ret != NULL)
	regtail(ret, br);	/* OPEN -> first. */
    else
	ret = br;
    if (!(flags & HASWIDTH))
	*flagp &= ~HASWIDTH;
    *flagp |= flags & SPSTART;
    while (*regparse == '|') {
	regparse++;
	br = regbranch(&flags);
	if (br == NULL)
	    return (NULL);
	regtail(ret, br);	/* BRANCH -> BRANCH. */
	if (!(flags & HASWIDTH))
	    *flagp &= ~HASWIDTH;
	*flagp |= flags & SPSTART;
    }

    /* Make a closing node, and hook it on the end. */
    ender = regnode((paren) ? CLOSE + parno : END);
    regtail(ret, ender);

    /* Hook the tails of the branches to the closing node. */
    for (br = ret; br != NULL; br = regnext(br))
	regoptail(br, ender);

    /* Check for proper termination. */
    if (paren && *regparse++ != ')') {
	regerror("unmatched ()");
	return NULL;
    } else if (!paren && regparse < reglimit) {
	if (*regparse == ')')
	    regerror("unmatched ()");
	else
	    regerror("Can't happen");
	return NULL;
    }

    return (ret);
}

/*
 - regbranch - one alternative of an | operator
 *
 * Implements the concatenation operator.
 */
static char *
regbranch(int *flagp)
{
    char *ret;
    char *chain;
    char *latest;
    int flags;

    *flagp = WORST;		/* Tentatively. */

    ret = regnode(BRANCH);
    chain = NULL;
    while (regparse < reglimit && *regparse != '|' && *regparse != ')') {
	latest = regpiece(&flags, chain == NULL);
	if (latest == NULL)
	    return (NULL);
	if (chain && OP(chain) == EOL) {
	    regninsert(2, latest);
	    OP(chain) = EXACTLY;
	    if (chain != &regdummy) {
		SET_SIZE(chain, 1);
	    }
	    *latest++ = '$';
	    *latest++ = EOS;
	    flags |= HASWIDTH | SIMPLE;
	}
	*flagp |= flags & HASWIDTH;
	if (chain == NULL)	/* First piece. */
	    *flagp |= flags & SPSTART;
	else
	    regtail(chain, latest);
	chain = latest;
    }
    if (chain == NULL)		/* Loop ran zero times. */
	(void) regnode(NOTHING);

    return (ret);
}

static void
encode_zero_or_more(char *ret, int flags)
{
    if (flags & SIMPLE) {
	regopinsert(STAR, ret);
    } else {
	/* Emit x* as (x&|), where & means "self". */
	regopinsert(BRANCH, ret);	/* Either x */
	regoptail(ret, regnode(BACK));	/* and loop */
	regoptail(ret, ret);	/* back */
	regtail(ret, regnode(BRANCH));	/* or */
	regtail(ret, regnode(NOTHING));		/* null. */
    }
}

static void
encode_one_or_more(char *ret, int flags)
{
    char *next;

    if (flags & SIMPLE) {
	regopinsert(PLUS, ret);
    } else {
	/* Emit x+ as x(&|), where & means "self". */
	next = regnode(BRANCH);	/* Either */
	regtail(ret, next);
	regtail(regnode(BACK), ret);	/* loop back */
	regtail(next, regnode(BRANCH));		/* or */
	regtail(ret, regnode(NOTHING));		/* null. */
    }
}

static void
encode_zero_or_one(char *ret)
{
    char *next;

    /* Emit x? as (x|) */
    regopinsert(BRANCH, ret);	/* Either x */
    regtail(ret, regnode(BRANCH));	/* or */
    next = regnode(NOTHING);	/* null. */
    regtail(ret, next);
    regoptail(ret, next);
}

/*
 * The minimum/maximum repeat counts are stored as an unaligned integer.
 */
typedef union {
    int int_value;
    char chr_value[sizeof(int)];
} UNALIGNED_INT;

#define GET_UNALIGNED(macro) \
    UNALIGNED_INT temp; \
    memcpy(temp.chr_value, &(macro(op)), sizeof(int)); \
    return temp.int_value

static int
get_RR_MIN(char *op)
{
    GET_UNALIGNED(RR_MIN);
}

static int
get_RR_MAX(char *op)
{
    GET_UNALIGNED(RR_MAX);
}

#define SET_UNALIGNED(macro) \
    UNALIGNED_INT temp; \
    temp.int_value = value; \
    memcpy(&(macro(op)), temp.chr_value, sizeof(int))

static void
set_RR_MIN(char *op, int value)
{
    SET_UNALIGNED(RR_MIN);
}

static void
set_RR_MAX(char *op, int value)
{
    SET_UNALIGNED(RR_MAX);
}

/*
 - regpiece - something followed by possible [*+?]
 *
 * Note that the branching code sequences used for ? and the general cases
 * of * and + are somewhat optimized:  they use the same NOTHING node as
 * both the endmarker for their branch list and the body of the last branch.
 * It might seem that this node could be dispensed with entirely, but the
 * endmarker role is not redundant.
 */
static char *
regpiece(int *flagp, int at_bop)
{
    char *ret;
    char op;
    int flags;

    ret = regatom(&flags, at_bop);
    if (ret == NULL)
	return (NULL);

    op = *regparse;
    if (!ISMULT(op)) {
	*flagp = flags;
	return (ret);
    }

    if (!(flags & HASWIDTH) && op != '?') {
	regerror("*+ operand could be empty");
	return NULL;
    }
    *flagp = (op != '+') ? (WORST | SPSTART) : (WORST | HASWIDTH);

    if (op == '*') {
	encode_zero_or_more(ret, flags);
    } else if (op == '+') {
	encode_one_or_more(ret, flags);
    } else if (op == '?') {
	encode_zero_or_one(ret);
    } else if (op == L_CURL) {
	int lo = 0, hi, value = 0, comma = 0;
	while (++regparse < reglimit) {
	    if (*regparse == R_CURL)
		break;
	    if (isDigit(*regparse)) {
		value = (value * 10) + (*regparse - '0');
	    } else if (*regparse == ',') {
		if (comma++)
		    break;
		lo = value;
		value = 0;
	    } else if (*regparse != BACKSLASH) {
		break;
	    }
	}
	if (*regparse != R_CURL) {
	    regerror("unmatched {}");
	    return NULL;
	}
	if (comma)
	    hi = value;
	else
	    hi = lo = value;

	if (lo == 0 && hi == 0)
	    encode_zero_or_more(ret, flags);
	else if (lo == 1 && hi == 0)
	    encode_one_or_more(ret, flags);
	else if (lo == 0 && hi == 1)
	    encode_zero_or_one(ret);
	else if (flags & SIMPLE)
	    regrange(RSIMPLE, ret, lo, hi);
	else {
	    char *next, *loop;

	    next = regnode(BRANCH);
	    regtail(ret, next);	/* Either */
	    loop = regnode(RCOMPLX);
	    if (loop != &regdummy) {
		set_RR_MIN(loop, lo);
		set_RR_MAX(loop, hi);
	    }
	    regtail(loop, ret);	/* loop back */
	    regtail(next, regnode(BRANCH));	/* or */
	    if (lo == 0)
		regtail(ret, regnode(NOTHING));		/* null. */
	    else
		regtail(ret, regnode(NEVER));	/* not! */
	}
    }
    regparse++;
    if (ISMULT(*regparse)) {
	regerror("nested *?+");
	return NULL;
    }

    return (ret);
}

/*
 * To make this efficient, we represent the character classes internally
 * as escaped characters, which happen to be usable themselves for that
 * purpose.
 */
static int
parse_char_class(char **src)
{
    /* this matches ANY_ESC */
    /* *INDENT-OFF* */
    static struct {
	const char *name;
	char escape;
    } char_classes[] = {
	{ "[:alnum:]", 'w' },
	{ "[:alpha:]", 'a' },
	{ "[:blank:]", 'b' },
	{ "[:cntrl:]", 'c' },
	{ "[:digit:]", 'd' },
	{ "[:graph:]", 'g' },
	{ "[:lower:]", 'l' },
	{ "[:print:]", 'p' },
	{ "[:punct:]", 'q' },
	{ "[:space:]", 's' },
	{ "[:upper:]", 'u' },
	{ "[:xdigit:]", 'x' },
    };
    /* *INDENT-ON* */

    unsigned n;
    for (n = 0; n < sizeof(char_classes) / sizeof(char_classes[0]); n++) {
	size_t len = strlen(char_classes[n].name);
	if (!strncmp(*src, char_classes[n].name, len)) {
	    *src += len;
	    return char_classes[n].escape;
	}
    }
    return 0;
}

/*
 * We don't implement collating symbols or equivalence class expressions, which
 * have similar syntax.
 */
static int
parse_unsupported(const char *src)
{
    if (strncmp(src, "[.", 2) == 0 && strstr(src, ".]") != 0) {
	regerror("collating symbols are not implemented");
	return 1;
    }
    if (strncmp(src, "[=", 2) == 0 && strstr(src, "=]") != 0) {
	regerror("equivalence class expressions are not implemented");
	return 1;
    }
    return 0;
}

/*
 * Like strcspn(), but limited by reglimit rather than a null.
 */
static int
reg_strcspn(const char *s, const char *reject)
{
    const char *base = s;

    while (s < reglimit) {
	if (*s != EOS) {
	    if (strchr(reject, CharOf(*s)) != 0) {
		break;
	    }
	}
	++s;
    }
    return (int) (s - base);
}

/*
 - regatom - the lowest level
 *
 * Optimization:  gobbles an entire sequence of ordinary characters so that
 * it can turn them into a single node, which is smaller to store and
 * faster to run.  Backslashed characters are exceptions, each becoming a
 * separate node; the code is simpler that way and it's not worth fixing.
 */
static char *
regatom(int *flagp, int at_bop)
{
    char *ret;
    int flags;
    int len = 1;

    *flagp = WORST;		/* Tentatively. */

    REGTRACE(("REGATOM:%s\n", regparse));
    switch (*regparse++) {
    case '^':
	if (!at_bop) {
	    regparse--;
	    goto defchar;
	}
	ret = regnode(BOL);
	break;
    case '$':
	ret = regnode(EOL);
	break;
    case '<':
	ret = regnode(BEGWORD);
	break;
    case '>':
	ret = regnode(ENDWORD);
	break;
    case '.':
	ret = regnode(ANY);
	*flagp |= HASWIDTH | SIMPLE;
	break;
    case '[':{
	    int classbgn;
	    int classend;

	    if (*regparse == '^') {	/* Complement of range. */
		ret = regnode(ANYBUT);
		regparse++;
	    } else
		ret = regnode(ANYOF);
	    if (*regparse == ']' || *regparse == '-')
		regc(*regparse++);
	    while (regparse < reglimit && *regparse != ']') {
		if (*regparse == '-') {
		    regparse++;
		    if (*regparse == ']' || regparse >= reglimit) {
			regc('-');
		    } else {
			classbgn = UCHAR_AT(regparse - 2) + 1;
			classend = UCHAR_AT(regparse);
			if (classbgn > classend + 1) {
			    regerror("invalid [] range");
			    return NULL;
			}
			for (; classbgn <= classend; classbgn++) {
			    if (classbgn == BACKSLASH)
				regc(classbgn);
			    regc(classbgn);
			}
			regparse++;
		    }
		} else if (parse_unsupported(regparse)) {
		    return NULL;
		} else if ((classbgn = parse_char_class(&regparse)) != 0) {
		    regc(BACKSLASH);
		    regc(classbgn);
		} else {
		    if (*regparse == BACKSLASH) {
			regc(*regparse++);
			if (*regparse == EOS
			    || (strchr(ANY_ESC, *regparse) == 0
				&& isAlpha(*regparse))) {
			    regerror("unexpected escape in range");
			    return NULL;
			}
		    }
		    regc(*regparse++);
		}
	    }
	    set_opsize();
	    if (*regparse != ']') {
		regerror("unmatched []");
		return NULL;
	    }
	    regparse++;
	    *flagp |= HASWIDTH | SIMPLE;
	}
	break;
    case '(':
	ret = reg(1, &flags);
	if (ret == NULL)
	    return (NULL);
	*flagp |= flags & (HASWIDTH | SPSTART);
	break;
    case '|':
    case ')':			/* Supposed to be caught earlier. */
	regerror("internal urp");
	return NULL;

    case '?':			/* FALLTHROUGH */
    case '+':			/* FALLTHROUGH */
    case L_CURL:		/* FALLTHROUGH */
    case '*':
	regerror("?+* follows nothing");
	return NULL;

    case BACKSLASH:
	if (regparse >= reglimit) {
#ifdef FAIL_TRAILING_BS
	    regerror("trailing \\");
	    ret = NULL;
#else
	    /* as a special case, treat a trailing '\' char as
	     * a trailing '.'.  This makes '\' work in isearch
	     * most of the time */
	    ret = regnode(ANY);
	    *flagp |= HASWIDTH | SIMPLE;
#endif
	    return ret;
	}
	switch (*regparse) {
	case 'a':
	    ret = regnode(ALPHA);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'A':
	    ret = regnode(NALPHA);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'b':
	    ret = regnode(BLANK);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'B':
	    ret = regnode(NBLANK);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'c':
	    ret = regnode(CNTRL);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'C':
	    ret = regnode(NCNTRL);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'd':
	    ret = regnode(DIGIT);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'D':
	    ret = regnode(NDIGIT);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'g':
	    ret = regnode(GRAPH);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'G':
	    ret = regnode(NGRAPH);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'l':
	    ret = regnode(LOWER);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'L':
	    ret = regnode(NLOWER);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'p':
	    ret = regnode(PRINT);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'P':
	    ret = regnode(NPRINT);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'q':
	    ret = regnode(PUNCT);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'Q':
	    ret = regnode(NPUNCT);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 's':
	    ret = regnode(SPACE);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'S':
	    ret = regnode(NSPACE);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'u':
	    ret = regnode(UPPER);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'U':
	    ret = regnode(NUPPER);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'w':
	    ret = regnode(ALNUM);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'W':
	    ret = regnode(NALNUM);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'x':
	    ret = regnode(XDIGIT);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	case 'X':
	    ret = regnode(NXDIGIT);
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	default:
	    ret = regnode(EXACTLY);
	    regc(*regparse);
	    set_opsize();
	    *flagp |= HASWIDTH | SIMPLE;
	    break;
	}
	regparse++;
	break;
    default:{
	    char ender;

	    regparse--;
	    len = reg_strcspn(regparse, META);
	    if (len <= 0) {
		regerror("internal disaster");
		return NULL;
	    }
	    ender = *(regparse + len);
	    if (len > 1 && ISMULT(ender))
		len--;		/* Back off clear of ?+* operand. */
	  defchar:
	    *flagp |= HASWIDTH;
	    if (len == 1)
		*flagp |= SIMPLE;
	    ret = regnode(EXACTLY);
	    while (len > 0) {
		regc(*regparse++);
		len--;
	    }
	    set_opsize();
	}
	break;
    }

    return (ret);
}

/*
 - regnode - emit a node
 */
static char *			/* Location. */
regnode(int op)
{
    char *ret;
    char *ptr;
    int length = (op == RSIMPLE || op == RCOMPLX) ? RR_LEN : OP_HDR;

    ret = regcode;
    if (ret == &regdummy) {
	regsize += length;
	return (ret);
    }

    ptr = ret;
    OP(ptr) = (char) op;
    SET_NEXT(ptr, 0);		/* Null "next" pointer. */
    switch (op) {
    case RSIMPLE:
    case RCOMPLX:
	set_RR_MIN(ptr, 0);
	set_RR_MAX(ptr, 0);
	break;
    case ANYOF:
    case ANYBUT:
    case EXACTLY:
	SET_SIZE(ptr, 0);
	break;
    default:
	SET_SIZE(ptr, 0xffff);	/* just so it is known */
	break;
    }
    regcode = ptr + length;

    op_pointer = ret;
    op_length = 0;
    return (ret);
}

/*
 - set_opsize - set the operand size, if known
 */
static void
set_opsize(void)
{
    if (regcode != &regdummy) {
	SET_SIZE(op_pointer, op_length);
    }
    regc(EOS);			/* FIXME - obsolete */
}

/*
 - regc - emit (if appropriate) a byte of code
 */
static void
regc(int b)
{
    ++op_length;
    if (regcode != &regdummy) {
	*regcode++ = (char) b;
    } else {
	regsize++;
    }
}

/*
 - regrange - emit (if appropriate) a range-code
 */
static void
regrange(int op, char *opnd, int lo, int hi)
{
    regninsert(RR_LEN, opnd);	/* like regopinsert */
    if (regcode == &regdummy)
	return;
    *opnd = (char) op;
    set_RR_MIN(opnd, lo);
    set_RR_MAX(opnd, hi);
}

/*
 - regninsert - insert n bytes in front of already-emitted operand
 *
 * Means relocating the operand.
 */
static void
regninsert(int n, char *opnd)
{
    char *src;
    char *dst;
    char *place;

    if (regcode == &regdummy) {
	regsize += n;
	return;
    }

    src = regcode;
    regcode += n;
    dst = regcode;
    while (src > opnd)
	*--dst = *--src;

    place = opnd;		/* Op node, where operand used to be. */
    while (n--)
	*place++ = EOS;
}

/*
 - regopinsert - insert an operator in front of already-emitted operand
 *
 * Means relocating the operand.
 */
static void
regopinsert(int op, char *opnd)
{
    regninsert(OP_HDR, opnd);
    if (regcode == &regdummy)
	return;
    *opnd = (char) op;
}

/*
 - regtail - set the next-pointer at the end of a node chain
 */
static void
regtail(char *p, char *val)
{
    char *scan;
    char *temp;
    int offset;

    if (p == &regdummy)
	return;

    /* Find last node. */
    scan = p;
    for (;;) {
	temp = regnext(scan);
	if (temp == NULL)
	    break;
	scan = temp;
    }

    if (OP(scan) == BACK
	|| OP(scan) == RCOMPLX)
	offset = (int) (scan - val);
    else
	offset = (int) (val - scan);
    SET_NEXT(scan, offset);
}

/*
 - regoptail - regtail on operand of first argument; nop if operandless
 */
static void
regoptail(char *p, char *val)
{
    /* "Operandless" and "op != BRANCH" are synonymous in practice. */
    if (p == NULL || p == &regdummy || OP(p) != BRANCH)
	return;
    regtail(OPERAND(p), val);
}

/*
 * regexec and friends
 */

/*
 * Global work variables for regexec().
 */
static char *reginput;		/* String-input pointer. */
static char *regnomore;		/* String-input end pointer. */
static char *regbol;		/* Beginning of input, for ^ check. */
static char **regstartp;	/* Pointer to startp array. */
static char **regendp;		/* Ditto for endp. */

/* this very special copy of strncmp allows for caseless operation,
 * and also for non-null terminated strings:
 * txt_a arg ends at position end_a, or txt_a + lenb, whichever is first.
 * txt_b begins a string of at most len_b characters, against which txt_a
 *	is compared.
 */
static int
regstrncmp(const char *txt_a,
	   const char *txt_b,
	   size_t len_b,
	   const char *end_a)
{
    if (end_a == NULL
	|| (end_a < txt_a)
	|| (unsigned) (end_a - txt_a) > len_b) {
	end_a = txt_a + len_b;
    }
    while (txt_a < end_a) {
	if (!SAME(*txt_a, *txt_b)) {
	    break;
	}
	txt_a++, txt_b++;
    }

    return (txt_a == end_a) ? (-*txt_b) : (*txt_a - *txt_b);
}

static char *
regstrchr(char *s, int c, const char *e)
{
    while (s < e) {
	if (SAME(*s, c))
	    return s;
	++s;
    }
    return 0;
}

/*
 * Same as 'regstrchr()', except that the string has special characters
 * escaped.  The 's' argument is always null-terminated.
 */
static int
RegStrChr2(const char *s, unsigned length, int c)
{
    int matched = 0;
    const char *last = s + length;

    while ((s < last) && !matched) {
	if (*s == BACKSLASH) {
	    /* this matches ANY_ESC */
	    switch (*++s) {
	    default:
		matched = (CharOf(c) == CharOf(*s));
		break;
	    case 'a':
		matched = is_ALPHA(c);
		break;
	    case 'A':
		matched = !is_ALPHA(c);
		break;
	    case 'b':
		matched = is_BLANK(c);
		break;
	    case 'B':
		matched = !is_BLANK(c);
		break;
	    case 'c':
		matched = is_CNTRL(c);
		break;
	    case 'C':
		matched = !is_CNTRL(c);
		break;
	    case 'd':
		matched = is_DIGIT(c);
		break;
	    case 'D':
		matched = !is_DIGIT(c);
		break;
	    case 'g':
		matched = is_GRAPH(c);
		break;
	    case 'G':
		matched = !is_GRAPH(c);
		break;
	    case 'l':
		matched = is_LOWER(c);
		break;
	    case 'L':
		matched = !is_LOWER(c);
		break;
	    case 'p':
		matched = is_PRINT(c);
		break;
	    case 'P':
		matched = !is_PRINT(c);
		break;
	    case 'q':
		matched = is_PUNCT(c);
		break;
	    case 'Q':
		matched = !is_PUNCT(c);
		break;
	    case 's':
		matched = is_SPACE(c);
		break;
	    case 'S':
		matched = !is_SPACE(c);
		break;
	    case 'u':
		matched = is_UPPER(c);
		break;
	    case 'U':
		matched = !is_UPPER(c);
		break;
	    case 'w':
		matched = is_ALNUM(c);
		break;
	    case 'W':
		matched = !is_ALNUM(c);
		break;
	    case 'x':
		matched = is_XDIGIT(c);
		break;
	    case 'X':
		matched = !is_XDIGIT(c);
		break;
	    }
	} else {
	    matched = SAME(*s, c);
	}
	s++;
    }
    return matched;
}

/*
 - regexec - match a regexp against a string
 	prog is the compiled expression, string is the string, stringend
	points just after the string, and the match can begin at or after
	startoff, but must end before endoff
 */
int
regexec(
	   regexp * prog,
	   char *string,
	   char *stringend,	/* pointer to the null, if there were one */
	   int startoff,
	   int endoff)
{
    char *s, *endsrch;

    /* Be paranoid... */
    if (prog == NULL || string == NULL) {
	regerror("NULL parameter");
	return (0);
    }

    /* Check validity of program. */
    if (UCHAR_AT(prog->program) != REGEXP_MAGIC) {
	regerror("corrupted program");
	return (0);
    }

    /* supply an endpoint if none given */
    if (stringend == NULL) {
	stringend = &string[strlen(string)];
    } else if (stringend < string) {
	regerror("end less than start");
	return (0);
    }

    if (endoff < 0)
	endoff = (int) (stringend - string);

    endsrch = &string[endoff];

    /* if our outer limit is the end-of-string, let us scan there,
       in case we're trying to match a lone '$' */
    if (endsrch == stringend)
	endsrch++;

    /* If there is a "must appear" string, look for it. */
    if (prog->regmust != -1) {
	s = &string[startoff];
	while ((s = regstrchr(s, prog->program[prog->regmust],
			      stringend))
	       != NULL && s < endsrch) {
	    if (regstrncmp(s, &prog->program[prog->regmust],
			   prog->regmlen, stringend) == 0)
		break;		/* Found it. */
	    s++;
	}
	if (s >= endsrch || s == NULL)	/* Not present. */
	    return (0);
    }

    /* Mark beginning of line for ^ . */
    regbol = string;

    /* Simplest case:  anchored match need be tried only once. */
    if (startoff == 0 && prog->reganch)
	return (regtry(prog, string, stringend));

    /* Messy cases:  unanchored match. */
    s = &string[startoff];
    if (prog->regstart >= 0) {
	/* We know what char it must start with. */
	while ((s = regstrchr(s, prog->regstart, stringend)) != NULL &&
	       s < endsrch) {
	    if (regtry(prog, s, stringend))
		return (1);
	    s++;
	}
    } else {
	/* We don't -- general case. */
	do {
	    if (regtry(prog, s, stringend))
		return (1);
	} while (s++ != stringend && s < endsrch);
    }

    /* Failure. */
    return (0);
}

/*
 - regtry - try match at specific point
 */
static int			/* 0 failure, 1 success */
regtry(
	  regexp * prog,
	  char *string,
	  char *stringend)
{
    int i;
    char **sp;
    char **ep;

    reginput = string;
    regnomore = stringend;
    regstartp = prog->startp;
    regendp = prog->endp;
#ifdef REGDEBUG
    reg_program = prog->program;
#endif

    sp = prog->startp;
    ep = prog->endp;
    for (i = NSUBEXP; i > 0; i--) {
	*sp++ = NULL;
	*ep++ = NULL;
    }
    if (regmatch(prog->program + 1, 0)) {
	prog->startp[0] = string;
	prog->endp[0] = reginput;
	prog->mlen = reginput - string;
	return (1);
    } else {
	prog->mlen = 0;		/* not indicative of anything */
	return (0);
    }
}

/*
 - regmatch - main matching routine
 *
 * Conceptually the strategy is simple:  check to see whether the current
 * node matches, call self recursively to see whether the rest matches,
 * and then act accordingly.  In practice we make some effort to avoid
 * recursion, in particular by going through "ordinary" nodes (that don't
 * need to know whether the rest of the match failed) by a loop instead of
 * by recursion.
 */
static int			/* 0 failure, 1 success */
regmatch(char *prog, int found)
{
    char *scan;			/* Current node. */
    char *next;			/* Next node. */

    if ((scan = prog) != NULL) {
#ifdef REGDEBUG
	if (scan != NULL && regnarrate) {
	    if (reginput < regnomore) {
		TRACE(("%2d%s %d{%.*s}\n",
		       scan - reg_program, regprop(scan),
		       regnomore - reginput,
		       regnomore - reginput, reginput));
	    } else {
		TRACE(("%2d%s %d{}\n",
		       scan - reg_program, regprop(scan),
		       regnomore - reginput));
	    }
	}
#endif
    }
    while (scan != NULL) {
#ifdef REGDEBUG
	if (regnarrate)
	    (void) regdump1(reg_program, scan);
#endif
	next = regnext(scan);

	switch (OP(scan)) {
	case BOL:
	    if (reginput != regbol)
		return (0);
	    break;
	case EOL:
	    if (reginput != regnomore)
		return (0);
	    break;
	case BEGWORD:
	    /* Match if current char isident
	     * and previous char BOL or !ident */
	    if ((reginput == regnomore || !isident(*reginput))
		|| (reginput != regbol
		    && isident(reginput[-1])))
		return (0);
	    break;
	case ENDWORD:
	    /* Match if previous char isident
	     * and current char EOL or !ident */
	    if ((reginput != regnomore && isident(*reginput))
		|| reginput == regbol
		|| !isident(reginput[-1]))
		return (0);
	    break;
	case ALPHA:
	    if (reginput == regnomore || !is_ALPHA(*reginput))
		return 0;
	    reginput++;
	    break;
	case NALPHA:
	    if (reginput == regnomore || is_ALPHA(*reginput))
		return 0;
	    reginput++;
	    break;
	case ALNUM:		/* includes _ */
	    if (reginput == regnomore || !is_ALNUM(*reginput))
		return 0;
	    reginput++;
	    break;
	case NALNUM:
	    if (reginput == regnomore || is_ALNUM(*reginput))
		return 0;
	    reginput++;
	    break;
	case BLANK:
	    if (reginput == regnomore || !is_BLANK(*reginput))
		return 0;
	    reginput++;
	    break;
	case NBLANK:
	    if (reginput == regnomore || is_BLANK(*reginput))
		return 0;
	    reginput++;
	    break;
	case CNTRL:
	    if (reginput == regnomore || !is_CNTRL(*reginput))
		return 0;
	    reginput++;
	    break;
	case NCNTRL:
	    if (reginput == regnomore || is_CNTRL(*reginput))
		return 0;
	    reginput++;
	    break;
	case DIGIT:
	    if (reginput == regnomore || !is_DIGIT(*reginput))
		return 0;
	    reginput++;
	    break;
	case NDIGIT:
	    if (reginput == regnomore || is_DIGIT(*reginput))
		return 0;
	    reginput++;
	    break;
	case GRAPH:
	    if (reginput == regnomore || !is_GRAPH(*reginput))
		return 0;
	    reginput++;
	    break;
	case NGRAPH:
	    if (reginput == regnomore || is_GRAPH(*reginput))
		return 0;
	    reginput++;
	    break;
	case LOWER:
	    if (reginput == regnomore || !is_LOWER(*reginput))
		return 0;
	    reginput++;
	    break;
	case NLOWER:
	    if (reginput == regnomore || is_LOWER(*reginput))
		return 0;
	    reginput++;
	    break;
	case PRINT:
	    if (reginput == regnomore || !is_PRINT(*reginput))
		return 0;
	    reginput++;
	    break;
	case NPRINT:
	    if (reginput == regnomore || is_PRINT(*reginput))
		return 0;
	    reginput++;
	    break;
	case PUNCT:
	    if (reginput == regnomore || !is_PUNCT(*reginput))
		return 0;
	    reginput++;
	    break;
	case NPUNCT:
	    if (reginput == regnomore || is_PUNCT(*reginput))
		return 0;
	    reginput++;
	    break;
	case SPACE:
	    if (reginput == regnomore || !is_SPACE(*reginput))
		return 0;
	    reginput++;
	    break;
	case NSPACE:
	    if (reginput == regnomore || is_SPACE(*reginput))
		return 0;
	    reginput++;
	    break;
	case UPPER:
	    if (reginput == regnomore || !is_UPPER(*reginput))
		return 0;
	    reginput++;
	    break;
	case NUPPER:
	    if (reginput == regnomore || is_UPPER(*reginput))
		return 0;
	    reginput++;
	    break;
	case XDIGIT:
	    if (reginput == regnomore || !is_XDIGIT(*reginput))
		return 0;
	    reginput++;
	    break;
	case NXDIGIT:
	    if (reginput == regnomore || is_XDIGIT(*reginput))
		return 0;
	    reginput++;
	    break;
	case ANY:
	    if (reginput == regnomore)
		return (0);
	    reginput++;
	    break;
	case EXACTLY:{
		unsigned len;
		char *opnd;

		if (reginput == regnomore)
		    return (0);

		opnd = OPERAND(scan);
		/* Inline the first character, for speed. */
		if (!SAME(*opnd, *reginput)) {
		    return (0);
		}
		len = OPSIZE(scan);
		if (len > 1
		    && regstrncmp(reginput, opnd, len, regnomore) != 0)
		    return (0);
		reginput += len;
	    }
	    break;
	case ANYOF:
	    if (reginput == regnomore
		|| RegStrChr2(OPERAND(scan), OPSIZE(scan), *reginput) == 0)
		return (0);
	    reginput++;
	    break;
	case ANYBUT:
	    if (reginput == regnomore
		|| RegStrChr2(OPERAND(scan), OPSIZE(scan), *reginput) != 0)
		return (0);
	    reginput++;
	    break;
	case NEVER:
	    break;
	case NOTHING:
	    break;
	case BACK:
	    break;
	case OPEN1:
	case OPEN2:
	case OPEN3:
	case OPEN4:
	case OPEN5:
	case OPEN6:
	case OPEN7:
	case OPEN8:
	case OPEN9:{
		int no;
		char *save;

		no = OP(scan) - OPEN;
		save = reginput;

		if (regmatch(next, found)) {
		    /*
		     * Don't set startp if some later
		     * invocation of the same parentheses
		     * already has.
		     */
		    if (regstartp[no] == NULL)
			regstartp[no] = save;
		    return (1);
		} else
		    return (0);
	    }
	    /* NOTREACHED */

	case CLOSE1:
	case CLOSE2:
	case CLOSE3:
	case CLOSE4:
	case CLOSE5:
	case CLOSE6:
	case CLOSE7:
	case CLOSE8:
	case CLOSE9:{
		int no;
		char *save;

		no = OP(scan) - CLOSE;
		save = reginput;

		if (regmatch(next, found)) {
		    /*
		     * Don't set endp if some later
		     * invocation of the same parentheses
		     * already has.
		     */
		    if (regendp[no] == NULL)
			regendp[no] = save;
		    return (1);
		} else
		    return (0);
	    }
	    /* NOTREACHED */

	case BRANCH:
	    if (OP(next) != BRANCH) {	/* No choice. */
		next = OPERAND(scan);	/* Avoid recursion. */
		break;
	    }

	    /*
	     * Do looping (BACK, RCOMPLX) via recursion.
	     */
	    do {
		char *save = reginput;

		next = OPERAND(scan);
		if (OP(next) == RCOMPLX) {
		    int max = get_RR_MAX(next);
		    int min = get_RR_MIN(next);

		    if ((max == 0
			 || found + 1 < max)
			&& regmatch(next, found + 1)) {
			found = min;
		    }

		    if (min == 0
			|| found >= min)
			return (1);
		    return (0);

		} else if (OP(next) >= OPEN) {
		    if (regmatch(next, 0)) {
			return (1);
		    }
		} else {
		    if (regmatch(next, found)) {
			return (1);
		    }
		}
		reginput = save;
		scan = regnext(scan);
	    } while (scan != NULL && OP(scan) == BRANCH);
	    return (0);

	case RCOMPLX:
	    break;

	case RSIMPLE:		/* FALLTHROUGH */
	case STAR:		/* FALLTHROUGH */
	case PLUS:
	    {
		int nxtch;
		int no;
		char *save;
		int min;
		int max;
		char *rpt;

		/*
		 * Lookahead to avoid useless match attempts
		 * when we know what character comes next.
		 */
		nxtch = -1;
		if (OP(next) == EXACTLY)
		    nxtch = *OPERAND(next);

		switch (OP(scan)) {
		case STAR:
		    min = 0;
		    max = -1;
		    rpt = OPERAND(scan);
		    break;
		case PLUS:
		    min = 1;
		    max = -1;
		    rpt = OPERAND(scan);
		    break;
		default:	/* RSIMPLE */
		    min = get_RR_MIN(scan);
		    max = get_RR_MAX(scan);
		    rpt = scan + RR_LEN;
		    break;
		}

		save = reginput;
		no = regrepeat(rpt);

		if (max > 0 && no > max) {
		    no = max;
		    reginput = save + no;
		}

		while (no >= min) {
		    /* If it could work, try it. */
		    if ((nxtch == -1
			 || reginput >= regnomore
			 || SAME(*reginput, nxtch))
			&& regmatch(next, found))
			return (1);

		    /* Couldn't or didn't -- back up. */
		    no--;
		    reginput = save + no;
		}
		return (0);
	    }
	    /* NOTREACHED */

	case END:
	    return (1);		/* Success! */
	default:
	    regerror("memory corruption");
	    return (0);
	}

	scan = next;
    }

    /*
     * We get here only if there's trouble -- normally "case END" is
     * the terminating point.
     */
    regerror("corrupted pointers");
    return (0);
}

/*
 - regrepeat - repeatedly match something simple, report how many
 */
static int
regrepeat(const char *p)
{
    int count = 0;
    char *scan = reginput;
    unsigned size = OPSIZE(p);
    const char *opnd = OPERAND(p);

    switch (OP(p)) {
    case ANY:
	count = (int) (regnomore - scan);
	scan += count;
	break;
    case EXACTLY:
	while (scan < regnomore) {
	    if (!SAME(*opnd, *scan))
		break;
	    count++;
	    scan++;
	}
	break;
    case ANYOF:
	while (scan < regnomore && RegStrChr2(opnd, size, *scan) != 0) {
	    count++;
	    scan++;
	}
	break;
    case ANYBUT:
	while (scan < regnomore && RegStrChr2(opnd, size, *scan) == 0) {
	    count++;
	    scan++;
	}
	break;
    case ALPHA:
	while (scan < regnomore && is_ALPHA(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case NALPHA:
	while (scan < regnomore && !is_ALPHA(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case ALNUM:
	while (scan < regnomore && is_ALNUM(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case NALNUM:
	while (scan < regnomore && !is_ALNUM(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case BLANK:
	while (scan < regnomore && is_BLANK(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case NBLANK:
	while (scan < regnomore && !is_BLANK(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case CNTRL:
	while (scan < regnomore && is_CNTRL(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case NCNTRL:
	while (scan < regnomore && !is_CNTRL(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case DIGIT:
	while (scan < regnomore && is_DIGIT(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case NDIGIT:
	while (scan < regnomore && !is_DIGIT(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case GRAPH:
	while (scan < regnomore && is_GRAPH(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case NGRAPH:
	while (scan < regnomore && !is_GRAPH(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case LOWER:
	while (scan < regnomore && is_LOWER(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case NLOWER:
	while (scan < regnomore && !is_LOWER(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case PRINT:
	while (scan < regnomore && is_PRINT(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case NPRINT:
	while (scan < regnomore && !is_PRINT(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case PUNCT:
	while (scan < regnomore && is_PUNCT(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case NPUNCT:
	while (scan < regnomore && !is_PUNCT(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case SPACE:
	while (scan < regnomore && is_SPACE(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case NSPACE:
	while (scan < regnomore && !is_SPACE(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case UPPER:
	while (scan < regnomore && is_UPPER(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case NUPPER:
	while (scan < regnomore && !is_UPPER(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case XDIGIT:
	while (scan < regnomore && is_XDIGIT(*scan)) {
	    count++;
	    scan++;
	}
	break;
    case NXDIGIT:
	while (scan < regnomore && !is_XDIGIT(*scan)) {
	    count++;
	    scan++;
	}
	break;
    default:			/* Oh dear.  Called inappropriately. */
	regerror("internal foulup");
	count = 0;		/* Best compromise. */
	break;
    }
    reginput = scan;

    return (count);
}

/*
 - regnext - dig the "next" pointer out of a node
 */
static char *
regnext(char *p)
{
    int offset;

    if (p == &regdummy)
	return (NULL);

    offset = NEXT(p);
    if (offset == 0)
	return (NULL);

    if (OP(p) == BACK
	|| OP(p) == RCOMPLX)
	return (p - offset);
    else
	return (p + offset);
}

#ifdef REGDEBUG

/*
 - regdump1 - dump a single regexp operation in vaguely comprehensible form
 */
static char *
regdump1(char *program, char *s)
{
    static TBUFF *dump;
    char temp[80];
    char *next;
    int op;
    unsigned len;

    tb_init(&dump, EOS);
    op = OP(s);
    sprintf(temp, "%2d%s", s - program, regprop(s));	/* Where, what. */
    tb_sappend0(&dump, temp);
    next = regnext(s);
    if (next == NULL)		/* Next ptr. */
	strcpy(temp, "(0)");
    else
	sprintf(temp, "(%d)", (s - program) + (next - s));
    tb_sappend0(&dump, temp);
    len = OPSIZE(s);
    s += OP_HDR;
    if (op == ANYOF || op == ANYBUT || op == EXACTLY) {
	tb_sappend0(&dump, visible_buff(s, len, FALSE));
	tb_append(&dump, EOS);	/* FIXME: obsolete */
	s += (len + 1);
    } else if (op == RSIMPLE || op == RCOMPLX) {
	s += (RR_LEN - OP_HDR);
    }
    TRACE(("%s\n", tb_values(dump)));
    return s;
}

/*
 - regdump - dump a regexp in vaguely comprehensible form
 */
static void
regdump(regexp * r)
{
    static TBUFF *dump;
    char temp[80];
    char *s;
    char op = EXACTLY;		/* Arbitrary non-END op. */

    TRACE(("regdump\n"));
    s = r->program + 1;
    while (op != END) {		/* While that wasn't END last time... */
	op = OP(s);
	s = regdump1(r->program, s);
    }

    /* Header fields of interest. */
    tb_init(&dump, EOS);
    if (r->regstart >= 0) {
	sprintf(temp, "start `%#x' ", r->regstart);
	tb_sappend0(&dump, temp);
    }
    if (r->reganch) {
	sprintf(temp, "anchored ");
	tb_sappend0(&dump, temp);
    }
    if (r->regmust != -1) {
	tb_sappend0(&dump, "must have \"");
	tb_sappend0(&dump, &(r->program[r->regmust]));
	tb_sappend0(&dump, "\"");
    }
    TRACE(("%s\n", tb_values(dump)));
    tb_free(&dump);
}

/*
 - regprop - printable representation of opcode
 */
static char *
regprop(char *op)
{
    char *p = "?";
    static char buf[50];

    (void) strcpy(buf, ":");

    switch (OP(op)) {
    case BOL:
	p = "BOL";
	break;
    case EOL:
	p = "EOL";
	break;
    case ANY:
	p = "ANY";
	break;
    case ANYOF:
	p = "ANYOF";
	break;
    case ANYBUT:
	p = "ANYBUT";
	break;
    case BRANCH:
	p = "BRANCH";
	break;
    case EXACTLY:
	p = "EXACTLY";
	break;
    case NEVER:
	p = "NEVER";
	break;
    case NOTHING:
	p = "NOTHING";
	break;
    case BACK:
	p = "BACK";
	break;
    case END:
	p = "END";
	break;
    case BEGWORD:
	p = "BEGWORD";
	break;
    case ENDWORD:
	p = "ENDWORD";
	break;
    case ALPHA:
	p = "ALPHA";
	break;
    case NALPHA:
	p = "NALPHA";
	break;
    case ALNUM:
	p = "ALNUM";
	break;
    case NALNUM:
	p = "NALNUM";
	break;
    case BLANK:
	p = "BLANK";
	break;
    case NBLANK:
	p = "NBLANK";
	break;
    case CNTRL:
	p = "CNTRL";
	break;
    case NCNTRL:
	p = "NCNTRL";
	break;
    case DIGIT:
	p = "DIGIT";
	break;
    case NDIGIT:
	p = "NDIGIT";
	break;
    case GRAPH:
	p = "GRAPH";
	break;
    case NGRAPH:
	p = "NGRAPH";
	break;
    case LOWER:
	p = "LOWER";
	break;
    case NLOWER:
	p = "NLOWER";
	break;
    case PRINT:
	p = "PRINT";
	break;
    case NPRINT:
	p = "NPRINT";
	break;
    case PUNCT:
	p = "PUNCT";
	break;
    case NPUNCT:
	p = "NPUNCT";
	break;
    case SPACE:
	p = "SPACE";
	break;
    case NSPACE:
	p = "NSPACE";
	break;
    case UPPER:
	p = "UPPER";
	break;
    case NUPPER:
	p = "NUPPER";
	break;
    case XDIGIT:
	p = "XDIGIT";
	break;
    case NXDIGIT:
	p = "NXDIGIT";
	break;
    case OPEN1:
    case OPEN2:
    case OPEN3:
    case OPEN4:
    case OPEN5:
    case OPEN6:
    case OPEN7:
    case OPEN8:
    case OPEN9:
	(void) sprintf(STRSKIP(buf), "OPEN%d", OP(op) - OPEN);
	p = NULL;
	break;
    case CLOSE1:
    case CLOSE2:
    case CLOSE3:
    case CLOSE4:
    case CLOSE5:
    case CLOSE6:
    case CLOSE7:
    case CLOSE8:
    case CLOSE9:
	(void) sprintf(STRSKIP(buf), "CLOSE%d", OP(op) - CLOSE);
	p = NULL;
	break;
    case STAR:
	p = "STAR";
	break;
    case PLUS:
	p = "PLUS";
	break;
    case RSIMPLE:		/* FALLTHROUGH */
    case RCOMPLX:
	(void) sprintf(STRSKIP(buf), "%s%c", OP(op) == RSIMPLE
		       ? "RSIMPLE"
		       : "RCOMPLX", L_CURL);
	if (get_RR_MIN(op))
	    (void) sprintf(STRSKIP(buf), "%d", get_RR_MIN(op));
	strcat(buf, ",");
	if (get_RR_MAX(op))
	    (void) sprintf(STRSKIP(buf), "%d", get_RR_MAX(op));
	(void) sprintf(STRSKIP(buf), "%c", R_CURL);
	p = NULL;
	break;
    default:
	regerror("corrupted opcode");
	break;
    }
    if (p != NULL)
	(void) strcat(buf, p);
    return (buf);
}
#endif

#if defined(llength) && defined(lforw) && defined(lback)
/* vile support:
 * like regexec, but takes LINE * as input instead of char *
 */
int
lregexec(
	    regexp * prog,
	    LINE *lp,
	    int startoff,
	    int endoff)
{
    if (endoff < startoff)
	return 0;

    if (lp->l_text) {
	return regexec(prog, lp->l_text, &(lp->l_text[llength(lp)]),
		       startoff, endoff);
    } else {
	/* the prog might be ^$, or something legal on a null string */

	char *nullstr = "";
	int s;

	if (startoff > 0)
	    return 0;
	s = regexec(prog, nullstr, nullstr, 0, 0);
	if (s) {
	    if (prog->mlen > 0) {
		mlforce("BUG: non-zero match on null string");
		return 0;
	    }
	    prog->startp[0] = prog->endp[0] = NULL;
	}
	return s;
    }
}
#endif /* VILE LINE */

#ifdef DEBUG_REGEXP
void
mlforce(const char *dummy,...)
{
    printf("? %s\n", dummy);
}

void
regerror(const char *msg)
{
    printf("? %s\n", msg);
}

static char *
get_string(FILE *fp, unsigned *length, int linenum)
{
    static char *result = 0;
    static unsigned have = 0;

    int ch;
    int escaped = 0;
    int value = 0;
    int literal = -1;

    if (result == 0) {
	have = BUFSIZ;
	result = typeallocn(char, have);
    }
    *length = 0;
    for (;;) {
	ch = fgetc(fp);
	if (ch == EOF || ferror(fp)) {
	    *length = 0;
	    if (result != 0) {
		have = 0;
		free(result);
		result = 0;
	    }
	    break;
	} else if (ch == '\n') {
	    break;
	} else if (escaped) {
	    if (escaped == 1) {
		switch (ch) {
		case '\\':
		    value = ch;
		    break;
		case 's':
		    value = ' ';
		    break;
		case 'n':
		    value = '\n';
		    break;
		case 'r':
		    value = '\r';
		    break;
		case 't':
		    value = '\t';
		    break;
		default:
		    if (isdigit(ch)) {
			value = ch - '0';
			escaped = -2;
		    } else {
			value = ch;
		    }
		}
		if (escaped > 0)
		    escaped = 0;
	    } else if (isdigit(ch)) {
		value = (value * 8) + (ch - '0');
		++escaped;
	    } else {
		fprintf(stderr,
			"line %d: expected 3 digits per octal escape\n", linenum);
		escaped = 0;
	    }
	} else if (ch == '\\' && !literal) {
	    escaped = 1;
	    value = 0;
	} else {
	    value = ch;
	}
	if (!escaped) {
	    if (*length + 2 >= have) {
		have *= 2;
		result = typereallocn(char, result, have);
	    }
	    result[*length] = (char) value;
	    *length += 1;
	    if (literal < 0)
		literal = (value == '#' || isupper(value)) ? 1 : 0;
	}
    }
    if (result != 0)
	result[*length] = 0;
    return (*length != 0) ? result : 0;
}

static void
put_string(char *s, unsigned length, int literal)
{
    unsigned n;

    for (n = 0; n < length; ++n) {
	if (literal) {
	    putchar(CharOf(s[n]));
	} else {
	    switch (s[n]) {
	    case '\\':
		fputs("\\\\", stdout);
		break;
	    case ' ':
		fputs("\\s", stdout);
		break;
	    case '\n':
		fputs("\\n", stdout);
		break;
	    case '\r':
		fputs("\\r", stdout);
		break;
	    case '\t':
		fputs("\\t", stdout);
		break;
	    default:
		if (CharOf(s[n]) < ' ' || CharOf(s[n]) > '~') {
		    printf("\\%03o", CharOf(s[n]));
		} else {
		    putchar(CharOf(s[n]));
		}
	    }
	}
    }
    putchar('\n');
}

/*
 * Read script containing patterns (p), test-data (q) and results (r).  The
 * first character of each line is its type:
 * >	Comments begin with '#'.
 * >	Use an uppercase p/q/r to suppress backslash interpretation of the line,
 *	e.g., to simplify typing patterns.
 * >	Use M/m to switch magic on/off in the call to regcomp().
 * >	Use N to disable regmassage (used for vile), so expressions are POSIX.
 * >	Lines beginning with '?' are error messages.
 * >	Other lines are converted to comments.
 *
 * The output of the test driver contains the comment lines, p- and q-lines as
 * well as the computed r-lines.  Because special characters are encoded as
 * backslash sequences, it is simple to compare the driver's input and output
 * using "diff", to see where a test differs.
 */
static void
test_regexp(FILE *fp)
{
    char *s;
    int magic = TRUE;
    int linenum = 0;
    int literal;
    int subexp;
    unsigned offset;
    unsigned length;
    regexp *pattern = 0;

    while ((s = get_string(fp, &length, ++linenum)) != 0) {
	literal = 0;
	switch (*s) {
	case 'N':
	    magic = -1;
	    put_string(s, length, TRUE);
	    break;
	case 'M':
	    magic = 1;
	    put_string(s, length, TRUE);
	    break;
	case 'm':
	    magic = 0;
	    put_string(s, length, TRUE);
	    break;
	case 'P':
	    literal = 1;
	    /* FALLTHRU */
	case 'p':
	    put_string(s, length, literal);
	    /* compile pattern */
	    if (pattern != 0)
		free(pattern);
	    ++s, --length;
	    pattern = regcomp(s, length, magic);
	    break;
	case 'Q':
	    literal = 1;
	    /* FALLTHRU */
	case 'q':
	    put_string(s, length, literal);
	    /* compute results */
	    ++s, --length;
	    if (pattern != 0) {
		for (offset = 0; offset <= length; ++offset) {
		    if (regexec(pattern, s, s + length, offset, length)) {
			for (subexp = 0; subexp < NSUBEXP; ++subexp) {
			    if (pattern->startp[subexp] != 0
				&& pattern->endp[subexp] != 0
				&& (pattern->endp[subexp] >=
				    pattern->startp[subexp])) {
				unsigned mlen =
				pattern->endp[subexp] -
				pattern->startp[subexp];
				if (mlen <= 1) {
				    printf("r%d [%d:%d] -> [%d]",
					   subexp, offset, length - mlen,
					   pattern->startp[subexp] - s);
				} else {
				    printf("r%d [%d:%d] -> [%d:%d]",
					   subexp, offset, length - 1,
					   pattern->startp[subexp] - s,
					   pattern->endp[subexp] - s - 1);
				}
				put_string(pattern->startp[subexp], mlen, FALSE);
			    } else if (pattern->startp[subexp] != 0
				       && pattern->endp[subexp] != 0) {
				printf("? mlen = %d\n",
				       pattern->endp[subexp] -
				       pattern->startp[subexp]);
			    } else if (pattern->startp[subexp] != 0) {
				printf("? null end[%d] pointer\n", subexp);
			    } else if (pattern->endp[subexp] != 0) {
				printf("? null start[%d] pointer\n", subexp);
			    }
			}
		    }
		}
	    }
	    break;
	case '#':
	    put_string(s, length, TRUE);
	    break;
	case 'R':
	case 'r':
	    /* absorb - see 'q' case */
	    break;
	case '?':
	    /* absorb - see 'p' case */
	    break;
	default:
	    putchar('#');
	    put_string(s, length, TRUE);
	    break;
	}
    }
    if (pattern != 0)
	free(pattern);
}

int
main(int argc, char *argv[])
{
    int n;

#if 0
    for (n = 0; n < 256; ++n) {
	printf("%3d [%2X] %c%c%c%c%c%c%c%c%c%c%c%c\n",
	       n, n,
	       is_ALPHA(n) ? 'A' : '-',
	       is_ALNUM(n) ? 'a' : '-',
	       is_BLANK(n) ? 'B' : '-',
	       is_CNTRL(n) ? 'C' : '-',
	       is_DIGIT(n) ? 'D' : '-',
	       is_GRAPH(n) ? 'G' : '-',
	       is_LOWER(n) ? 'L' : '-',
	       is_PRINT(n) ? 'P' : '-',
	       is_PUNCT(n) ? 'p' : '-',
	       is_SPACE(n) ? 'S' : '-',
	       is_UPPER(n) ? 'U' : '-',
	       is_XDIGIT(n) ? 'X' : '-');
    }
#endif

    if (argc > 1) {
	for (n = 1; n < argc; ++n) {
	    FILE *fp = fopen(argv[n], "r");
	    if (fp != 0) {
		test_regexp(fp);
		fclose(fp);
	    } else {
		perror(argv[n]);
		return EXIT_FAILURE;
	    }
	}
    } else {
	test_regexp(stdin);
    }
    return EXIT_SUCCESS;
}
#endif
