/*
 * Common utility functions for vile syntax/highlighter programs
 *
 * $Header: /usr/build/vile/vile/filters/RCS/filters.c,v 1.99 2005/09/30 00:59:45 tom Exp $
 *
 */

#include <filters.h>

#define QUOTE '\''

#ifdef HAVE_LONG_FILE_NAMES
#define KEYFILE_SUFFIX ".keywords"
#else
#define KEYFILE_SUFFIX ".key"
#endif

#if DOT_HIDES_FILE
#define DOT_TO_HIDE_IT "."
#else
#define DOT_TO_HIDE_IT ""
#endif

#define VERBOSE(level,params)	if (FltOptions('v') >= level) mlforce params

#define HASH_LENGTH 256

struct _keyword {
    char *kw_name;
    char *kw_attr;
    unsigned kw_size;		/* strlen(kw_name) */
    unsigned short kw_flag;	/* nonzero for classes */
    unsigned short kw_used;	/* nonzero for classes */
    KEYWORD *next;
};

typedef struct _classes CLASS;

struct _classes {
    char *name;
    KEYWORD **data;
    CLASS *next;
};

char *default_attr;
int zero_or_more = '*';		/* zero or more of the following */
int zero_or_all = '?';		/* zero or all of the following */
int meta_ch = '.';
int eqls_ch = ':';
int vile_keywords;
int flt_options[256];

static KEYWORD **hashtable;
static CLASS *classes;

/*
 * flt_bfr_*() function data
 */
static char *flt_bfr_text = 0;
static char *flt_bfr_attr = "";
static unsigned flt_bfr_used = 0;
static unsigned flt_bfr_size = 0;

/*
 * OpenKeywords() function data
 */
static char *str_keyword_name = 0;
static char *str_keyword_file = 0;
static unsigned len_keyword_name = 0;
static unsigned len_keyword_file = 0;

/******************************************************************************
 * Private functions                                                          *
 ******************************************************************************/

static void
CannotAllocate(const char *where)
{
    VERBOSE(1, ("%s: cannot allocate", where));
}

static const char *
AttrsOnce(KEYWORD * entry)
{
    entry->kw_used = 999;
    return NONNULL(entry->kw_attr);
}

static void
ExecAbbrev(char *param)
{
    zero_or_more = *param;
}

static void
ExecBrief(char *param)
{
    zero_or_all = *param;
}

static void
ExecClass(char *param)
{
    parse_keyword(param, 1);
}

static void
ExecDefault(char *param)
{
    char *s = skip_ident(param);
    int save = *s;

    *s = 0;
    if (!*param)
	param = NAME_KEYWORD;
    if (is_class(param)) {
	free(default_attr);
	default_attr = strmalloc(param);
	VERBOSE(1, ("set default_attr '%s' %p\n", default_attr, default_attr));
    } else {
	*s = save;
	VERBOSE(1, ("not a class:%s", param));
    }
}

static void
ExecEquals(char *param)
{
    eqls_ch = *param;
}

static void
ExecMeta(char *param)
{
    meta_ch = *param;
}

/*
 * Include a symbol table from another key-file.
 */
static void
ExecSource(char *param)
{
    int save_meta = meta_ch;
    int save_eqls = eqls_ch;

    flt_make_symtab(param);
    flt_read_keywords(MY_NAME);	/* provide default values for this table */
    flt_read_keywords(param);
    set_symbol_table(flt_name());

    meta_ch = save_meta;
    eqls_ch = save_eqls;
}

/*
 * Useful for diverting to another symbol table in the same key-file, for
 * performance.
 */
static void
ExecTable(char *param)
{
    flt_make_symtab(param);
    flt_read_keywords(MY_NAME);	/* provide default values for this table */
}

static KEYWORD *
FindIdentifier(const char *name)
{
    unsigned size;
    int Index;
    KEYWORD *hash_id = 0;

    if (name != 0 && (size = strlen(name)) != 0) {
	Index = hash_function(name);
	hash_id = hashtable[Index];
	while (hash_id != NULL) {
	    if (hash_id->kw_size == size
		&& strcmp(hash_id->kw_name, name) == 0) {
		break;
	    }
	    hash_id = hash_id->next;
	}
    }
    return hash_id;
}

static void
Free(char *ptr)
{
    if (ptr != 0)
	free(ptr);
}

/*
 * Find the first occurrence of a file in the canonical search list:
 *	the current directory
 *	the home directory
 *	vile's subdirectory of the home directory
 *	the vile library-directory
 *
 * On Unix we look for file/directory names with a "." prefixed since that
 * hides them.
 */
static FILE *
OpenKeywords(char *classname)
{
#define OPEN_IT(p) if ((fp = fopen(p, "r")) != 0) { \
			VERBOSE(1,("Opened %s", p)); return fp; } else { \
			VERBOSE(2,("..skip %s", p)); }
#define FIND_IT(p) sprintf p; OPEN_IT(str_keyword_name)

    static char suffix[] = KEYFILE_SUFFIX;

    FILE *fp;
    char *path;
    unsigned need;
    char leaf[20];

    need = sizeof(suffix) + strlen(classname) + 2;
    str_keyword_file = do_alloc(str_keyword_file, need, &len_keyword_file);
    if (str_keyword_file == 0) {
	CannotAllocate("OpenKeywords");
	return 0;
    }
    sprintf(str_keyword_file, "%s%s", classname, suffix);

    if (strchr(str_keyword_file, PATHSEP) != 0) {
	OPEN_IT(str_keyword_file);
    }

    if ((path = home_dir()) == 0)
	path = "";

    need = strlen(path)
	+ strlen(str_keyword_file)
	+ 20;

    str_keyword_name = do_alloc(str_keyword_name, need, &len_keyword_name);
    if (str_keyword_name == 0) {
	CannotAllocate("OpenKeywords");
	return 0;
    }

    FIND_IT((str_keyword_name, "%s%c%s%s", PATHDOT, PATHSEP, DOT_TO_HIDE_IT, str_keyword_file));
    FIND_IT((str_keyword_name, "%s%c%s%s", path, PATHSEP, DOT_TO_HIDE_IT, str_keyword_file));
    sprintf(leaf, "%s%s%c", DOT_TO_HIDE_IT, MY_NAME, PATHSEP);

    FIND_IT((str_keyword_name, "%s%c%s%s", path, PATHSEP, leaf, str_keyword_file));

    path = vile_getenv("VILE_STARTUP_PATH");
#ifdef VILE_STARTUP_PATH
    if (path == 0)
	path = VILE_STARTUP_PATH;
#endif
    if (path != 0) {
	int n = 0, m;

	need = strlen(path) + strlen(str_keyword_file) + 2;
	str_keyword_name = do_alloc(str_keyword_name, need, &len_keyword_name);
	if (str_keyword_name == 0) {
	    CannotAllocate("OpenKeywords");
	    return 0;
	}
	while (path[n] != 0) {
	    for (m = n; path[m] != 0 && path[m] != PATHCHR; m++)
		/*LOOP */ ;
	    FIND_IT((str_keyword_name, "%.*s%c%s", m - n, path + n, PATHSEP, str_keyword_file));
	    if (path[m])
		n = m + 1;
	    else
		n = m;
	}
    }

    return 0;
}

static int
ParseDirective(char *line)
{
    static struct {
	const char *name;
	void (*func) (char *param);
    } table[] = {
	/* *INDENT-OFF* */
	{ "abbrev",  ExecAbbrev   },
	{ "brief",   ExecBrief    },
	{ "class",   ExecClass    },
	{ "default", ExecDefault  },
	{ "equals",  ExecEquals   },
	{ "include", flt_read_keywords },
	{ "merge",   ExecSource   },
	{ "meta",    ExecMeta     },
	{ "source",  ExecSource   },
	{ "table",   ExecTable    },
	/* *INDENT-ON* */

    };
    unsigned n, len;

    if (*(line = skip_blanks(line)) == meta_ch) {
	line = skip_blanks(line + 1);
	if ((len = (skip_ident(line) - line)) != 0) {
	    for (n = 0; n < sizeof(table) / sizeof(table[0]); n++) {
		if (!strncmp(line, table[n].name, len)) {
		    (*table[n].func) (skip_blanks(line + len));
		    break;
		}
	    }
	}
	return 1;
    }
    return 0;
}

static void
RemoveList(KEYWORD * k)
{
    if (k != NULL) {
	if (k->next != NULL)
	    RemoveList(k->next);
	free((char *) k);
    }
}

static unsigned
TrimBlanks(char *src)
{
    unsigned len = strlen(src);

    while (len != 0
	   && isspace(CharOf(src[len - 1])))
	src[--len] = 0;
    return (len);
}

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/

char *
ci_keyword_attr(char *text)
{
    return keyword_attr(lowercase_of(text));
}

char *
class_attr(char *name)
{
    KEYWORD *hash_id;
    char *result = 0;

    while ((hash_id = is_class(name)) != 0) {
	VERBOSE(hash_id->kw_used, ("class_attr(%s) = %s",
				   name, AttrsOnce(hash_id)));
	name = result = hash_id->kw_attr;
	VERBOSE(1, ("-> %p\n", result));
    }
    return result;
}

void *
flt_alloc(void *ptr, unsigned need, unsigned *have, unsigned size)
{
    need += (2 * size);		/* allow for trailing null, etc */
    if (need > *have) {
	need *= 2;
	if (ptr != 0)
	    ptr = realloc(ptr, need);
	else
	    ptr = malloc(need);
	*have = need;
    }
    return ptr;
}

/*
 * The flt_bfr_*() functions are used for managing a possibly multi-line buffer
 * that will be written as one attributed region.
 */
void
flt_bfr_append(char *text, int length)
{
    flt_bfr_text = do_alloc(flt_bfr_text, flt_bfr_used + length, &flt_bfr_size);
    if (flt_bfr_text != 0) {
	strncpy(flt_bfr_text + flt_bfr_used, text, length);
	flt_bfr_used += length;
    } else {
	CannotAllocate("flt_bfr_append");
    }
}

void
flt_bfr_begin(char *attr)
{
    flt_bfr_finish();
    flt_bfr_attr = attr;
}

void
flt_bfr_embed(char *text, int length, char *attr)
{
    char *save = flt_bfr_attr;

    if ((save == 0 && attr == 0) ||
	(save != 0 && attr != 0 && !strcmp(save, attr))) {
	flt_bfr_append(text, length);
    } else {
	flt_bfr_finish();
	flt_puts(text, length, attr);
	flt_bfr_attr = save;
    }
}

void
flt_bfr_error(void)
{
    if (flt_bfr_used) {
	flt_error("unterminated buffer");
	flt_bfr_attr = class_attr(NAME_ERROR);
	flt_bfr_finish();
    }
}

void
flt_bfr_finish(void)
{
    if (flt_bfr_used) {
	flt_puts(flt_bfr_text, flt_bfr_used,
		 (flt_bfr_attr
		  ? flt_bfr_attr
		  : ""));
    }
    flt_bfr_used = 0;
    flt_bfr_attr = "";
}

int
flt_bfr_length(void)
{
    return flt_bfr_used;
}

void
flt_free_keywords(char *classname)
{
    CLASS *p, *q;
    KEYWORD *ptr;
    int i;

    for (p = classes, q = 0; p != 0; q = p, p = p->next) {
	if (!strcmp(classname, p->name)) {
	    hashtable = p->data;

	    for (i = 0; i < HASH_LENGTH; i++) {
		while ((ptr = hashtable[i]) != 0) {
		    hashtable[i] = ptr->next;
		    free(ptr->kw_name);
		    free(ptr->kw_attr);
		    free(ptr);
		}
	    }

	    free(p->name);
	    free(p->data);
	    if (q != 0)
		q->next = p->next;
	    else
		classes = p->next;
	    free(p);
	    break;
	}
    }
    hashtable = (classes != 0) ? classes->data : 0;
}

void
flt_free_symtab(void)
{
    while (classes != 0)
	flt_free_keywords(classes->name);
}

/*
 * We drop the old symbol table each time we read the data, to allow keywords
 * to be removed from the table.  Also, it is necessary for some filters such
 * as m4 to be able to restart to a known state.
 */
void
flt_initialize(void)
{
    if (default_attr != 0)
	free(default_attr);
    default_attr = strmalloc(NAME_KEYWORD);

    zero_or_more = '*';
    zero_or_all = '?';
    meta_ch = '.';
    eqls_ch = ':';
    FltOptions('v') = 0;

    flt_free_symtab();
}

void
flt_make_symtab(char *classname)
{
    if (!set_symbol_table(classname)) {
	CLASS *p;

	if ((p = typecallocn(CLASS, 1)) == 0) {
	    CannotAllocate("flt_make_symtab");
	    return;
	}

	p->name = strmalloc(classname);
	p->data = typecallocn(KEYWORD *, HASH_LENGTH);
	if (p->name == 0 || p->data == 0) {
	    if (p->name != 0)
		free(p->name);
	    free(p);
	    CannotAllocate("flt_make_symtab");
	    return;
	}

	p->next = classes;
	classes = p;
	hashtable = p->data;

	VERBOSE(1, ("flt_make_symtab(%s)", classname));

	/*
	 * Mark all of the standard predefined classes when we first create a
	 * symbol table.  Some filters may define their own special classes,
	 * and not all filters use all of these classes, but it's a lot simpler
	 * than putting the definitions into every ".key" file.
	 */
	insert_keyword(NAME_ACTION, ATTR_ACTION, 1);
	insert_keyword(NAME_COMMENT, ATTR_COMMENT, 1);
	insert_keyword(NAME_ERROR, ATTR_ERROR, 1);
	insert_keyword(NAME_IDENT, ATTR_IDENT, 1);
	insert_keyword(NAME_IDENT2, ATTR_IDENT2, 1);
	insert_keyword(NAME_KEYWORD, ATTR_KEYWORD, 1);
	insert_keyword(NAME_KEYWRD2, ATTR_KEYWRD2, 1);
	insert_keyword(NAME_LITERAL, ATTR_LITERAL, 1);
	insert_keyword(NAME_NUMBER, ATTR_NUMBER, 1);
	insert_keyword(NAME_PREPROC, ATTR_PREPROC, 1);
	insert_keyword(NAME_TYPES, ATTR_TYPES, 1);
    }
}

void
flt_read_keywords(char *classname)
{
    FILE *kwfile;
    char *line = 0;
    char *name;
    unsigned line_len = 0;

    VERBOSE(1, ("flt_read_keywords(%s)", classname));
    if ((kwfile = OpenKeywords(classname)) != NULL) {
	int linenum = 0;
	while (readline(kwfile, &line, &line_len) != 0) {

	    name = skip_blanks(line);
	    if (TrimBlanks(name) == 0)
		continue;
	    if (ParseDirective(name))
		continue;

	    VERBOSE(2, ("line %3d:", ++linenum));
	    parse_keyword(name, 0);
	}
	fclose(kwfile);
    }
    Free(line);
}

/*
 * Iterate over the names in the hash table, not ordered.  This assumes that
 * the called function does not remove any entries from the table.  It is okay
 * to add names, since that will not alter the hash links.
 */
void
for_each_keyword(EachKeyword func)
{
    int i;
    KEYWORD *ptr;

    for (i = 0; i < HASH_LENGTH; i++) {
	for (ptr = hashtable[i]; ptr != 0; ptr = ptr->next) {
	    (*func) (ptr->kw_name, ptr->kw_size, ptr->kw_attr);
	}
    }
}

char *
get_symbol_table(void)
{
    CLASS *p;
    for (p = classes; p != 0; p = p->next) {
	if (hashtable == p->data) {
	    return p->name;
	}
    }
    return "?";
}

long
hash_function(const char *id)
{
    /*
     * Build more elaborate hashing scheme. If you want one.
     */
    if ((id[0] == 0) || (id[1] == 0))
	return (CharOf(id[0]));

    return ((CharOf(id[0]) ^ (CharOf(id[1]) << 3) ^ (CharOf(id[2]) >> 1)) & 0xff);
}

static KEYWORD *
alloc_keyword(const char *ident, const char *attribute, int classflag)
{
    KEYWORD *first;
    KEYWORD *nxt;
    int Index;

    if ((nxt = FindIdentifier(ident)) != 0) {
	Free(nxt->kw_attr);
	if ((nxt->kw_attr = strmalloc(attribute)) == NULL) {
	    free(nxt);
	    nxt = 0;
	}
    } else {
	nxt = first = NULL;
	Index = hash_function(ident);
	first = hashtable[Index];
	if ((nxt = typecallocn(KEYWORD, 1)) != NULL) {
	    nxt->kw_name = strmalloc(ident);
	    nxt->kw_size = strlen(nxt->kw_name);
	    nxt->kw_attr = strmalloc(attribute);
	    nxt->kw_flag = classflag;
	    nxt->kw_used = 2;
	    nxt->next = first;
	    if (nxt->kw_name != 0
		&& nxt->kw_attr != 0) {
		hashtable[Index] = nxt;
	    } else {
		if (nxt->kw_name != 0)
		    free(nxt->kw_name);
		free(nxt);
		nxt = 0;
	    }
	}
    }
    return nxt;
}

void
insert_keyword(const char *ident, const char *attribute, int classflag)
{
    KEYWORD *nxt;
    char *mark;
    char *temp;

    VERBOSE(2, ("insert_keyword(%s, %s, %d)\n",
		ident,
		attribute,
		classflag));

    if ((mark = strchr(ident, zero_or_more)) != 0
	&& (mark != ident)) {
	if ((temp = strmalloc(ident)) != 0) {

	    mark = temp + (mark - ident);
	    while (*mark == zero_or_more) {
		*mark = 0;
		insert_keyword(temp, attribute, classflag);
		if ((mark[0] = mark[1]) != 0) {
		    *(++mark) = zero_or_more;
		}
	    }
	    free(temp);
	} else {
	    CannotAllocate("insert_keyword");
	}
    } else if ((mark = strchr(ident, zero_or_all)) != 0
	       && (mark != ident)) {
	if ((temp = strmalloc(ident)) != 0) {

	    mark = temp + (mark - ident);
	    if (*mark == zero_or_all) {
		*mark = 0;
		insert_keyword(temp, attribute, classflag);
		while ((mark[0] = mark[1]) != 0)
		    ++mark;
		insert_keyword(temp, attribute, classflag);
	    }
	    free(temp);
	} else {
	    CannotAllocate("insert_keyword");
	}
    } else if ((nxt = alloc_keyword(ident, attribute, classflag)) == 0) {
	CannotAllocate("insert_keyword");
    } else {
	VERBOSE(3, ("...\tname \"%s\"\tattr \"%s\"",
		    nxt->kw_name,
		    NONNULL(nxt->kw_attr)));
    }
}

KEYWORD *
is_class(char *name)
{
    KEYWORD *hash_id;
    if ((hash_id = FindIdentifier(name)) != 0
	&& hash_id->kw_flag != 0) {
	return hash_id;
    }
    return 0;
}

KEYWORD *
is_keyword(char *name)
{
    KEYWORD *hash_id;
    if ((hash_id = FindIdentifier(name)) != 0
	&& hash_id->kw_flag == 0) {
	return hash_id;
    }
    return 0;
}

char *
keyword_attr(char *name)
{
    KEYWORD *hash_id = is_keyword(name);
    char *result = 0;

    if (hash_id != 0) {
	result = hash_id->kw_attr;
	while ((hash_id = is_class(result)) != 0)
	    result = hash_id->kw_attr;
    }
    VERBOSE(1, ("keyword_attr(%s) = %p %s\n", name, result, NONNULL(result)));
    return result;
}

char *
lowercase_of(char *text)
{
    static char *name;
    static unsigned used;
    unsigned n;

    name = do_alloc(name, strlen(text), &used);
    for (n = 0; text[n] != 0; n++) {
	if (isalpha(CharOf(text[n])) && isupper(CharOf(text[n])))
	    name[n] = tolower(CharOf(text[n]));
	else
	    name[n] = text[n];
    }
    name[n] = 0;
    return (name);
}

void
parse_keyword(char *name, int classflag)
{
    char *args = 0;
    char *s, *t;
    int quoted = 0;

    if ((s = strchr(name, eqls_ch)) != 0) {
	*s++ = 0;
	s = skip_blanks(s);
	if (*s != 0) {
	    args = t = s;
	    while (*s != 0) {
		if (quoted) {
		    if (*s == QUOTE) {
			if (*++s != QUOTE)
			    quoted = 0;
		    }
		} else {
		    if (*s == QUOTE) {
			quoted = 1;
			s++;
		    } else if (!isalnum(CharOf(*s))) {
			args = 0;	/* error: ignore */
			break;
		    }
		}
		*t++ = *s++;
	    }
	    *t = 0;
	}
	TrimBlanks(name);
    }

    VERBOSE(2, ("parsed\tname \"%s\"\tattr \"%s\"%s",
		name,
		NONNULL(args),
		(classflag || is_class(name)) ? " - class" : ""));

    if (*name && args) {
	insert_keyword(name, args, classflag);
    } else if (*name) {
	KEYWORD *hash_id;
	if (args == 0) {
	    args = default_attr;
	    VERBOSE(2, ("using attr \"%s\"", args));
	}
	if ((hash_id = FindIdentifier(args)) != 0) {
	    /*
	     * Insert the classname rather than the hash_id->kw_attr value,
	     * since insert_keyword makes a copy of the string we pass to it.
	     * Retrieving the attribute from the copy will give the unique
	     * attribute string belonging to the class, so it is possible at
	     * runtime to compare pointers from keyword_attr() to determine if
	     * two tokens have the same class.  sql-filt.l uses this feature.
	     */
	    insert_keyword(name, args, classflag);
	}
    }
}

char *
readline(FILE *fp, char **ptr, unsigned *len)
{
    char *buf = *ptr;
    unsigned used = 0;

    if (buf == 0) {
	*len = BUFSIZ;
	buf = typeallocn(char, *len);
    }
    while (!feof(fp)) {
	int ch = fgetc(fp);
	if (ch == EOF || feof(fp) || ferror(fp)) {
	    break;
	}
	if (used + 2 >= *len) {
	    *len = 3 * (*len) / 2;
	    buf = typereallocn(char, buf, *len);
	    if (buf == 0)
		return 0;
	}
	buf[used++] = ch;
	if (ch == '\n')
	    break;
    }
    buf[used] = '\0';
    return used ? (*ptr = buf) : 0;
}

int
set_symbol_table(const char *classname)
{
    CLASS *p;
    for (p = classes; p != 0; p = p->next) {
	if (!strcmp(classname, p->name)) {
	    hashtable = p->data;
	    VERBOSE(3, ("set_symbol_table:%s", classname));
	    return 1;
	}
    }
    return 0;
}

char *
skip_ident(char *src)
{
    while (*src != '\0' && isprint(CharOf(*src)) && !isspace(CharOf(*src))) {
	if (*src == eqls_ch
	    || *src == meta_ch) {
	    break;
	}
	src++;
    }
    return (src);
}

int
yywrap(void)
{
    return 1;
}

#if NO_LEAKS
void
filters_leaks(void)
{
    FreeAndNull(str_keyword_name);
    FreeAndNull(str_keyword_file);
    len_keyword_name = 0;
    len_keyword_file = 0;
}
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                