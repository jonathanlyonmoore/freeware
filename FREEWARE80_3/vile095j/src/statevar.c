/*
 * statevar.c -- the functions named var_XXXX() referred to by modetbl
 *	for getting and setting the values of the vile state variables,
 *	plus helper utility functions.
 *
 * $Header: /usr/build/vile/vile/RCS/statevar.c,v 1.98 2006/04/19 23:51:34 tom Exp $
 */

#include	"estruct.h"
#include	"edef.h"
#include	"nevars.h"
#include	"patchlev.h"

#if SYS_WINNT
#include	<process.h>
#endif

#define		WRITE_ONLY	"[write only]"

#if OPT_FINDPATH
static char *findpath;		/* $VILE_FINDPATH environment is "$findpath" state
				 * variable.
				 */
#endif

#if OPT_EVAL && OPT_SHELL
static char *shell;		/* $SHELL environment is "$shell" state variable */
static char *directory;		/* $TMP environment is "$directory" state variable */
#if DISP_X11
static char *x_display;		/* $DISPLAY environment is "$xdisplay" state variable */
static char *x_shell;		/* $XSHELL environment is "$xshell" state variable */
static char *x_shellflags;	/* flags separating "$xshell" from command */
#endif
#endif
#if OPT_PATHLOOKUP
static char *cdpath;		/* $CDPATH environment is "$cdpath" state variable. */
#endif

static const char *
DftEnv(const char *name, const char *dft)
{
#if OPT_EVAL && OPT_SHELL
    name = safe_getenv(name);
    return isEmpty(name) ? dft : name;
#else
    return dft;
#endif
}

static void
SetEnv(char **namep, const char *value)
{
    char *newvalue;

    beginDisplay();
    if ((newvalue = strmalloc(value)) != 0) {
#if OPT_EVAL && OPT_SHELL
	FreeIfNeeded(*namep);
#endif
	*namep = newvalue;
    }
    endofDisplay();
}

static int
any_ro_BOOL(TBUFF **rp, const char *vp, int value)
{
    if (rp) {
	render_boolean(rp, value);
	return TRUE;
    } else if (vp) {
	return ABORT;		/* read-only */
    } else {
	return FALSE;
    }
}

static int
any_rw_BOOL(TBUFF **rp, const char *vp, int *value)
{
    if (rp) {
	render_boolean(rp, *value);
	return TRUE;
    } else if (vp) {
	*value = scan_bool(vp);
	return TRUE;
    } else {
	return FALSE;
    }
}

static int
any_ro_INT(TBUFF **rp, const char *vp, int value)
{
    if (rp) {
	render_int(rp, value);
	return TRUE;
    } else if (vp) {
	return ABORT;		/* read-only */
    } else {
	return FALSE;
    }
}

static int
any_rw_INT(TBUFF **rp, const char *vp, int *value)
{
    if (rp) {
	render_int(rp, *value);
	return TRUE;
    } else if (vp) {
	*value = scan_int(vp);
	return TRUE;
    } else {
	return FALSE;
    }
}

static int
any_ro_STR(TBUFF **rp, const char *vp, const char *value)
{
    if (rp) {
	if (value != 0) {
	    tb_scopy(rp, value);
	    return TRUE;
	}
    } else if (vp) {
	return ABORT;		/* read-only */
    }
    return FALSE;
}

static int
any_rw_STR(TBUFF **rp, const char *vp, TBUFF **value)
{
    if (rp) {
	if (value != 0 && *value != 0) {
	    tb_copy(rp, *value);
	    return TRUE;
	}
    } else if (vp) {
	tb_scopy(value, vp);
	return TRUE;
    }
    return FALSE;
}

static int
any_rw_EXPR(TBUFF **rp, const char *vp, TBUFF **value)
{
    if (rp) {
	if (value != 0) {
	    tb_copy(rp, *value);
	    return TRUE;
	}
    } else if (vp) {
	regexp *exp = regcomp(vp, strlen(vp), TRUE);
	if (exp != 0) {
	    beginDisplay();
	    free(exp);
	    endofDisplay();
	    tb_scopy(value, vp);
	    return TRUE;
	}
    }
    return FALSE;
}

static int
any_ro_TBUFF(TBUFF **rp, const char *vp, TBUFF **value)
{
    if (rp) {
	if (value != 0) {
	    tb_copy(rp, *value);
	    return TRUE;
	}
    } else if (vp) {
	return ABORT;		/* read-only */
    }
    return FALSE;
}

static int
any_rw_TBUFF(TBUFF **rp, const char *vp, TBUFF **value)
{
    if (rp) {
	if (value != 0) {
	    tb_copy(rp, *value);
	    return TRUE;
	}
    } else if (vp) {
	tb_scopy(value, vp);
	return TRUE;
    }
    return FALSE;
}

static int
any_rw_TXT(TBUFF **rp, const char *vp, char **value)
{
    if (rp) {
	tb_scopy(rp, *value);
	return TRUE;
    } else if (vp) {
	SetEnv(value, vp);
	return TRUE;
    } else {
	return FALSE;
    }
}

static int
any_REPL(TBUFF **rp, const char *vp, CHARTYPE type)
{
    if (rp) {
	lgrabtext(rp, type);
	return TRUE;
    } else if (vp && valid_buffer(curbp)) {
	return lrepltext(type, vp, (int) strlen(vp));
    } else {
	return FALSE;
    }
}

#if OPT_HOOKS
static int
any_HOOK(TBUFF **rp, const char *vp, HOOK * hook)
{
    if (rp) {
	tb_scopy(rp, hook->proc);
	return TRUE;
    } else if (vp) {
	(void) strncpy0(hook->proc, vp, NBUFN);
	return TRUE;
    } else {
	return FALSE;
    }
}
#endif

const char *
safe_getenv(const char *s)
{
#if OPT_EVAL && OPT_SHELL
    char *v = getenv(s);
#if defined(_WIN32)
    if (v == 0)
	v = vile_getenv(s);
#endif
    return NONNULL(v);
#else
    return "";
#endif
}

#if OPT_EVAL && OPT_SHELL

#if SYS_OS2 || SYS_OS2_EMX
#  define SHELL_NAME "COMSPEC"
#  define SHELL_PATH "cmd.exe"
#else
#  if SYS_WINNT
#    define SHELL_NAME "COMSPEC"
#    define SHELL_PATH (is_winnt() ? "cmd.exe" : "command.com")
#  else
#    if SYS_MSDOS
#      define SHELL_NAME "COMSPEC"
#      define SHELL_PATH "command.com"
#    else
#      define SHELL_NAME "SHELL"
#      define SHELL_PATH "/bin/sh"
#    endif
#  endif
#endif

char *
get_shell(void)
{
    if (shell == 0)
	SetEnv(&shell, DftEnv(SHELL_NAME, SHELL_PATH));
    return shell;
}
#endif

#if OPT_FINDPATH
char *
get_findpath(void)
{
    if (findpath == 0)
	SetEnv(&findpath, DftEnv("VILE_FINDPATH", ""));
    return (findpath);
}
#endif

#if OPT_EVAL && DISP_X11 && OPT_SHELL
char *
get_xshell(void)
{
    if (x_shell == 0)
	SetEnv(&x_shell, DftEnv("XSHELL", "xterm"));
    return x_shell;
}

char *
get_xshellflags(void)
{
    if (x_shellflags == 0)
	SetEnv(&x_shellflags, DftEnv("XSHELLFLAGS", "-e"));
    return x_shellflags;
}

char *
get_xdisplay(void)
{
    if (x_display == 0)
	SetEnv(&x_display, DftEnv("DISPLAY", x_get_display_name()));
    return x_display;
}
#endif

#if OPT_EVAL && OPT_SHELL
char *
get_directory(void)
{
    if (directory == 0)
	SetEnv(&directory, DftEnv("TMP", P_tmpdir));
    return directory;
}
#endif

/* access the default kill buffer */
static void
getkill(TBUFF **rp)
{
    tb_init(rp, EOS);
    if (kbs[0].kbufh != 0) {
	int n = index2ukb(0);
	tb_bappend(rp, (char *) (kbs[n].kbufh->d_chunk), kbs[n].kused);
    }
    tb_append(rp, EOS);
}

#if OPT_PATHLOOKUP
char *
get_cdpath(void)
{
    if (cdpath == 0)
	SetEnv(&cdpath, DftEnv("CDPATH", ""));
    return cdpath;
}

int
var_CDPATH(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, get_cdpath());
	return TRUE;
    } else if (vp) {
	SetEnv(&cdpath, vp);
	return TRUE;
    } else {
	return FALSE;
    }
}
#endif

#if OPT_EVAL
/* Return comma-delimited list of "interesting" options. */
static char *
cfgopts(void)
{
    static const char *opts[] =
    {
#if !OPT_SHELL
	"noshell",
#endif
#if SYS_WINNT && defined(VILE_OLE)
	"oleauto",
#endif
#if OPT_LOCALE
	"locale",
#endif
#if OPT_PERL
	"perl",
#endif
#if DISP_CURSES
	"curses",
#endif
#if DISP_TERMCAP
# if USE_TERMINFO
	"terminfo",
# else
	"termcap",
# endif
#endif
#if DISP_X11
# if OL_WIDGETS
	"openlook",
# endif
# if MOTIF_WIDGETS
	"motif",
# endif
# if ATHENA_WIDGETS
	"athena",
#  ifdef HAVE_LIB_XAW
	"xaw",
#  endif
#  ifdef HAVE_LIB_XAW3D
	"xaw3d",
#  endif
#  ifdef HAVE_LIB_NEXTAW
	"nextaw",
#  endif
# endif
#endif
	NULL			/* End of list marker */
    };
    static TBUFF *optstring;

    if (optstring == 0) {
	const char **lclopt;

	optstring = tb_init(&optstring, EOS);
	for (lclopt = opts; *lclopt; lclopt++) {
	    if (tb_length(optstring))
		optstring = tb_append(&optstring, ',');
	    optstring = tb_sappend(&optstring, *lclopt);
	}
	optstring = tb_append(&optstring, EOS);
    }
    return tb_values(optstring);
}
#endif

#if OPT_EVAL
int
var_ABUFNAME(TBUFF **rp, const char *vp)
{
    BUFFER *bp;
    return any_ro_STR(rp, vp, ((bp = find_alt()) != 0) ? bp->b_bname : "");
}

#if OPT_HOOKS
int
var_AUTOCOLORHOOK(TBUFF **rp, const char *vp)
{
    return any_HOOK(rp, vp, &autocolorhook);
}
#endif

int
var_BCHARS(TBUFF **rp, const char *vp)
{
    bsizes(curbp);
    return (valid_buffer(curbp)
	    ? any_ro_INT(rp, vp, curbp->b_bytecount)
	    : FALSE);
}

int
var_BFLAGS(TBUFF **rp, const char *vp)
{
    char temp[80];

    if (rp) {
	buffer_flags(temp, curbp);
	tb_scopy(rp, temp);
	return TRUE;
    } else if (vp) {
	return ABORT;		/* read-only */
    } else {
	return FALSE;
    }
}

int
var_BLINES(TBUFF **rp, const char *vp)
{
    bsizes(curbp);
    return (valid_buffer(curbp)
	    ? any_ro_INT(rp, vp, curbp->b_linecount)
	    : FALSE);
}

#if DISP_NTWIN
static int
parse_rgb(const char **src, int dft)
{
    char *dst = 0;
    int result;

    *src = skip_cblanks(*src);
    result = strtol(*src, &dst, 0);
    if (dst != 0)
	*src = dst;
    if (dst == 0 || (*dst != 0 && !isSpace(*dst)) || result <= 0 || result > 255)
	result = dft;
    return result;
}

int
var_BRIGHTNESS(TBUFF **rp, const char *vp)
{
    char temp[80];

    if (rp) {
	lsprintf(temp, "%d %d %d", rgb_gray, rgb_normal, rgb_bright);
	tb_scopy(rp, temp);
	return TRUE;
    } else if (vp) {
	rgb_gray = parse_rgb(&vp, 140);
	rgb_normal = parse_rgb(&vp, 180);
	rgb_bright = parse_rgb(&vp, 255);
	return vile_refresh(FALSE, 1);
    } else {
	return FALSE;
    }
}
#endif

#if OPT_HOOKS
int
var_BUFHOOK(TBUFF **rp, const char *vp)
{
    return any_HOOK(rp, vp, &bufhook);
}
#endif

int
var_CBUFNAME(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, curbp->b_bname);
	return TRUE;
    } else if (vp) {
	if (valid_buffer(curbp)) {
	    set_bname(curbp, vp);
	    curwp->w_flag |= WFMODE;
	}
	return TRUE;
    } else {
	return FALSE;
    }
}

int
var_BWINDOWS(TBUFF **rp, const char *vp)
{
    return (valid_buffer(curbp)
	    ? any_ro_INT(rp, vp, curbp->b_nwnd)
	    : FALSE);
}

#if OPT_HOOKS
int
var_CDHOOK(TBUFF **rp, const char *vp)
{
    return any_HOOK(rp, vp, &cdhook);
}
#endif

int
var_CFGOPTS(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp, cfgopts());
}

int
var_CFNAME(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, ((valid_buffer(curbp) && curbp->b_fname)
		      ? curbp->b_fname
		      : ""));
	return TRUE;
    } else if (vp) {
	if (valid_buffer(curbp)) {
	    ch_fname(curbp, vp);
	    curwp->w_flag |= WFMODE;
	}
	return TRUE;
    } else {
	return FALSE;
    }
}

int
var_CHAR(TBUFF **rp, const char *vp)
{
    if (rp) {
	if (valid_buffer(curbp) && !is_empty_buf(curbp)) {
	    if (is_at_end_of_line(DOT))
		render_int(rp, '\n');
	    else
		render_int(rp, char_at(DOT));
	} else {
	    tb_scopy(rp, error_val);
	}
	return TRUE;
    } else if (vp) {
	if (valid_buffer(curbp) && !b_val(curbp, MDVIEW)) {
	    int s, c;
	    mayneedundo();
	    (void) ldelete(1L, FALSE);	/* delete 1 char */
	    c = scan_int(vp);
	    if (c == '\n')
		s = lnewline();
	    else
		s = linsert(1, c);
	    (void) backchar(FALSE, 1);
	    return s;
	} else {
	    return rdonly();
	}
    } else {
	return FALSE;
    }
}

#if OPT_ENCRYPT
int
var_CRYPTKEY(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, WRITE_ONLY);
	return TRUE;
    } else if (vp) {
	beginDisplay();
	FreeIfNeeded(cryptkey);
	cryptkey = typeallocn(char, NKEYLEN);
	endofDisplay();
	if (cryptkey == 0)
	    return no_memory("var_CRYPTKEY");
	vl_make_encrypt_key(cryptkey, vp);
	return TRUE;
    } else {
	return FALSE;
    }
}
#endif

#if !SMALLER
int
var_CURCHAR(TBUFF **rp, const char *vp)
{
    if (rp) {
	render_int(rp, vl_getcchar() + 1);
	return TRUE;
    } else if (vp && valid_buffer(curbp)) {
	return gotochr(TRUE, strtol(vp, 0, 0));
    } else {
	return FALSE;
    }
}
#endif

int
var_CURCOL(TBUFF **rp, const char *vp)
{
    if (rp) {
	render_int(rp, getccol(FALSE) + 1);
	return TRUE;
    } else if (vp && valid_buffer(curbp)) {
	return gotocol(TRUE, strtol(vp, 0, 0));
    } else {
	return FALSE;
    }
}

int
var_CURLINE(TBUFF **rp, const char *vp)
{
    if (rp) {
	render_int(rp, getcline());
	return TRUE;
    } else if (vp && valid_buffer(curbp)) {
	return gotoline(TRUE, strtol(vp, 0, 0));
    } else {
	return FALSE;
    }
}

#if OPT_SHELL
int
var_CWD(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, current_directory(FALSE));
	return TRUE;
    } else if (vp) {
	return set_directory(vp);
    } else {
	return FALSE;
    }
}
#endif

int
var_CWLINE(TBUFF **rp, const char *vp)
{
    if (rp) {
	render_int(rp, getlinerow());
	return TRUE;
    } else if (vp && valid_buffer(curbp)) {
	return forwline(TRUE, strtol(vp, 0, 0) - getlinerow());
    } else {
	return FALSE;
    }
}

int
var_DEBUG(TBUFF **rp, const char *vp)
{
    return any_rw_BOOL(rp, vp, &tracemacros);
}

#if OPT_SHELL
int
var_DIRECTORY(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, get_directory());
	return TRUE;
    } else if (vp) {
	SetEnv(&directory, vp);
	return TRUE;
    } else {
	return FALSE;
    }
}
#endif

int
var_CMD_COUNT(TBUFF **rp, const char *vp)
{
    return any_ro_INT(rp, vp, cmd_count);
}

int
var_DISCMD(TBUFF **rp, const char *vp)
{
    return any_rw_BOOL(rp, vp, &vl_msgs);
}

int
var_DISINP(TBUFF **rp, const char *vp)
{
    return any_rw_BOOL(rp, vp, &vl_echo);
}

#if OPT_TRACE
int
var_GOAL_COLUMN(TBUFF **rp, const char *vp)
{
    return any_rw_INT(rp, vp, &curgoal);
}
#endif

#if OPT_LOCALE
int
var_ENCODING(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp, vl_encoding);
}
#endif

int
var_EOC(TBUFF **rp, const char *vp)
{
    return any_ro_BOOL(rp, vp, ev_end_of_cmd ? 1 : 0);
}

int
var_FILENAME_IC(TBUFF **rp, const char *vp)
{
#if OPT_CASELESS || SYS_VMS
    static int myvalue = TRUE;
#else
    static int myvalue = FALSE;
#endif
    return any_ro_BOOL(rp, vp, myvalue);
}

#if OPT_FINDERR
int
var_FILENAME_EXPR(TBUFF **rp, const char *vp)
{
    int code = any_rw_EXPR(rp, vp, &filename_expr);
    if (rp != 0 && code == TRUE) {
	free_err_exps(curbp);
	update_err_regex();
    }
    return code;
}

int
var_ERROR_EXPR(TBUFF **rp, const char *vp)
{
    return any_rw_EXPR(rp, vp, &error_expr);
}

int
var_ERROR_MATCH(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, tb_values(error_match));
	return TRUE;
    } else if (vp) {
	tb_scopy(&error_match, vp);
	return TRUE;
    }
    return FALSE;
}

int
var_ERROR_BUFFER(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, get_febuff());
	return TRUE;
    } else if (vp) {
	set_febuff(vp);
	return TRUE;
    } else {
	return FALSE;
    }
}

int
var_ERROR_TABSTOP(TBUFF **rp, const char *vp)
{
    return any_rw_INT(rp, vp, &error_tabstop);
}
#endif

int
var_EXEC_PATH(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp, exec_pathname);
}

#if OPT_MSDOS_PATH || SYS_VMS
#define the_EXEC_SUFFIX ".exe"
#else
#define the_EXEC_SUFFIX ""
#endif

int
var_EXEC_SUFFIX(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp, the_EXEC_SUFFIX);
}

#if OPT_HOOKS
int
var_EXITHOOK(TBUFF **rp, const char *vp)
{
    return any_HOOK(rp, vp, &exithook);
}
#endif

#if SYS_WINNT
int
var_FAVORITES(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp, get_favorites());
}
#endif

#if OPT_FINDPATH
int
var_FINDPATH(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, get_findpath());
	return TRUE;
    } else if (vp) {
	SetEnv(&findpath, vp);
	return TRUE;
    } else {
	return FALSE;
    }
}

/* debug aid -- this may go away */
int
var_FINDCMD(TBUFF **rp, const char *vp)
{
    char *cmd = last_findcmd();

    return (any_ro_STR(rp, vp, NONNULL(cmd)));
}
#endif

#if DISP_X11||DISP_NTWIN
int
var_FONT(TBUFF **rp, const char *vp)
{
#if SYS_WINNT && DISP_NTWIN
    if (rp) {
	tb_scopy(rp, ntwinio_current_font());
	return TRUE;
    } else if (vp) {
	return ntwinio_font_frm_str(vp, FALSE);
    } else {
	return FALSE;
    }
#endif
#if DISP_X11
    if (rp) {
	tb_scopy(rp, x_current_fontname());
	return TRUE;
    } else if (vp) {
	return x_setfont(vp);
    } else {
	return FALSE;
    }
#endif
}
#endif

int
var_FWD_SEARCH(TBUFF **rp, const char *vp)
{
    return any_rw_BOOL(rp, vp, &last_srch_direc);
}

int
var_HELPFILE(TBUFF **rp, const char *vp)
{
    return any_rw_TXT(rp, vp, &helpfile);
}

#if DISP_X11
int
var_ICONNAM(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, x_get_icon_name());
	return TRUE;
    } else if (vp) {
	x_set_icon_name(vp);
	return TRUE;
    } else {
	return FALSE;
    }
}
#endif

int
var_IDENTIF(TBUFF **rp, const char *vp)
{
    return any_REPL(rp, vp, vl_ident);
}

int
var_KBDMACRO(TBUFF **rp, const char *vp)
{
    if (rp) {
	get_kbd_macro(rp);
	return TRUE;
    } else if (vp) {
	return ABORT;		/* read-only */
    } else {
	return FALSE;
    }
}

int
var_KILL(TBUFF **rp, const char *vp)
{
    if (rp) {
	getkill(rp);
	return TRUE;
    } else if (vp) {
	return ABORT;		/* read-only */
    } else {
	return FALSE;
    }
}

int
var_LASTKEY(TBUFF **rp, const char *vp)
{
    return any_rw_INT(rp, vp, &lastkey);
}

int
var_LCOLS(TBUFF **rp, const char *vp)
{
    if (rp) {
	int col;
#ifdef WMDLINEWRAP
	/* temporarily set linewrap to avoid having the sideways value
	 * subtracted from the returned column.
	 */
	int save = w_val(curwp, WMDLINEWRAP);
	w_val(curwp, WMDLINEWRAP) = 1;
#endif
	col = offs2col(curwp, DOT.l, llength(DOT.l));
#ifdef WMDLINEWRAP
	w_val(curwp, WMDLINEWRAP) = save;
#endif
	render_int(rp, col);
	return TRUE;
    } else if (vp) {
	return ABORT;		/* read-only */
    } else {
	return FALSE;
    }
}

int
var_LIBDIR_PATH(TBUFF **rp, const char *vp)
{
    return any_rw_TXT(rp, vp, &libdir_path);
}

int
var_LINE(TBUFF **rp, const char *vp)
{
    return any_REPL(rp, vp, (CHARTYPE) 0);
}

int
var_LLENGTH(TBUFF **rp, const char *vp)
{
    return valid_buffer(curbp) ? any_ro_INT(rp, vp, llength(DOT.l)) : FALSE;
}

#if OPT_LOCALE
int
var_LOCALE(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp, vl_locale);
}
#endif

#if OPT_MAJORMODE
int
var_MAJORMODE(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp, ((valid_buffer(curbp) && curbp->majr != 0)
			       ? curbp->majr->shortname
			       : ""));
}

int
var_MAJORMODEHOOK(TBUFF **rp, const char *vp)
{
    return any_HOOK(rp, vp, &majormodehook);
}
#endif

int
var_MATCH(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp,
		      (tb_length(tb_matched_pat)
		       ? tb_values(tb_matched_pat)
		       : ""));
}

#if OPT_MENUS
int
var_MENU_FILE(TBUFF **rp, const char *vp)
{
    return any_rw_TXT(rp, vp, &menu_file);
}
#endif

int
var_MODE(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp, current_modename());
}

#if OPT_MLFORMAT
int
var_MLFORMAT(TBUFF **rp, const char *vp)
{
    if (rp) {
	if (modeline_format == 0)
	    mlforce("BUG: modeline_format uninitialized");
	else
	    tb_scopy(rp, modeline_format);
	return TRUE;
    } else if (vp) {
	SetEnv(&modeline_format, vp);
	upmode();
	return TRUE;
    } else {
	return FALSE;
    }
}
#endif

int
var_MODIFIED(TBUFF **rp, const char *vp)
{
    return any_ro_BOOL(rp, vp, valid_buffer(curbp) && b_is_changed(curbp));
}

#if OPT_COLOR
int
var_NCOLORS(TBUFF **rp, const char *vp)
{
    if (rp) {
	render_int(rp, ncolors);
	return TRUE;
    } else if (vp) {
	return set_colors(strtol(vp, 0, 0));
    }
    return FALSE;
}
#else
int
var_NCOLORS(TBUFF **rp GCC_UNUSED, const char *vp GCC_UNUSED)
{
    return FALSE;
}
#endif

int
var_NTILDES(TBUFF **rp, const char *vp)
{
    if (rp) {
	render_int(rp, ntildes);
	return TRUE;
    } else if (vp) {
	ntildes = strtol(vp, 0, 0);
	if (ntildes > 100)
	    ntildes = 100;
	return TRUE;
    } else {
	return FALSE;
    }
}

#if OPT_SHELL
int
var_OCWD(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp, previous_directory());
}
#endif

int
var_OS(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp, opersys);
}

int
var_PAGELEN(TBUFF **rp, const char *vp)
{
    if (rp) {
	render_int(rp, term.rows);
	return TRUE;
    } else if (vp) {
#if DISP_X11 || DISP_NTWIN
	gui_resize(term.cols, strtol(vp, 0, 0));
	return TRUE;
#else
	return newlength(TRUE, strtol(vp, 0, 0));
#endif
    } else {
	return FALSE;
    }
}

/* separator for lists of pathnames */
int
var_PATHCHR(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_append(rp, vl_pathchr);
	tb_append(rp, EOS);
	return TRUE;
    } else if (vp) {
	if (strlen(vp) == 1) {
	    vl_pathchr = *vp;
	    return TRUE;
	}
    }
    return FALSE;
}

/* separator for levels of pathnames */
int
var_PATHSEP(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_append(rp, vl_pathsep);
	tb_append(rp, EOS);
	return TRUE;
    } else if (vp) {
	if (strlen(vp) == 1) {
	    vl_pathsep = *vp;
	    return TRUE;
	}
    }
    return FALSE;
}

#if OPT_POSFORMAT
int
var_POSFORMAT(TBUFF **rp, const char *vp)
{
    if (rp) {
	if (position_format == 0)
	    mlforce("BUG: position_format uninitialized");
	else
	    tb_scopy(rp, position_format);
	return TRUE;
    } else if (vp) {
	SetEnv(&position_format, vp);
	return TRUE;
    } else {
	return FALSE;
    }
}
#endif

int
var_CURWIDTH(TBUFF **rp, const char *vp)
{
    if (rp) {
	render_int(rp, term.cols);
	return TRUE;
    } else if (vp) {
#if DISP_X11 || DISP_NTWIN
	gui_resize(strtol(vp, 0, 0), term.rows);
	return TRUE;
#else
	return newwidth(TRUE, strtol(vp, 0, 0));
#endif
    } else {
	return FALSE;
    }
}

int
var_PALETTE(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, (tb_length(tb_curpalette)
		      ? tb_values(tb_curpalette)
		      : ""));
	return TRUE;
    } else if (vp) {
	return set_palette(vp);
    } else {
	return FALSE;
    }
}

int
var_PATCHLEVEL(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp, VILE_PATCHLEVEL);
}

int
var_PATHNAME(TBUFF **rp, const char *vp)
{
    return any_REPL(rp, vp, vl_pathn);
}

int
var_PENDING(TBUFF **rp, const char *vp)
{
    return any_ro_BOOL(rp, vp, keystroke_avail());
}

int
var_PROCESSID(TBUFF **rp, const char *vp)
{
    if (rp) {
#if SYS_UNIX
	render_int(rp, getpid());
#else
# if (SYS_WINNT || SYS_VMS)
	/* translate pid to hex, because:
	 *    a) most Win32 PID's are huge,
	 *    b) VMS PID's are traditionally displayed in hex.
	 */
	render_hex(rp, getpid());
# endif
#endif
	return TRUE;
    } else if (vp) {
	return ABORT;		/* read-only */
    } else {
	return FALSE;
    }
}

int
var_PROGNAME(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp, prognam);
}

int
var_PROMPT(TBUFF **rp, const char *vp)
{
    return any_rw_STR(rp, vp, &prompt_string);
}

int
var_QIDENTIF(TBUFF **rp, const char *vp)
{
    return any_REPL(rp, vp, vl_qident);
}

#if OPT_HOOKS
int
var_RDHOOK(TBUFF **rp, const char *vp)
{
    return any_HOOK(rp, vp, &readhook);
}
#endif

/*
 * Note that replacepat is stored without a trailing null.
 */
int
var_REPLACE(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_copy(rp, replacepat);
	return TRUE;
    } else if (vp) {
	(void) tb_init(&replacepat, EOS);
	(void) tb_sappend(&replacepat, vp);
	return TRUE;
    } else {
	return FALSE;
    }
}

/*
 * Note that searchpat is stored without a trailing null.
 */
int
var_SEARCH(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_copy(rp, searchpat);
	return TRUE;
    } else if (vp && valid_buffer(curbp)) {
	(void) tb_init(&searchpat, EOS);
	(void) tb_sappend(&searchpat, vp);
	beginDisplay();
	FreeIfNeeded(gregexp);
	endofDisplay();
	gregexp = regcomp(tb_values(searchpat),
			  tb_length(searchpat),
			  b_val(curbp, MDMAGIC));
	return TRUE;
    } else {
	return FALSE;
    }
}

int
var_SEED(TBUFF **rp, const char *vp)
{
    if (rp) {
	render_int(rp, seed);
	return TRUE;
    } else if (vp) {
	seed = strtol(vp, 0, 0);
	srand(seed);
	return TRUE;
    } else {
	return FALSE;
    }
}

#if OPT_SHELL
int
var_SHELL(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, get_shell());
	return TRUE;
    } else if (vp) {
	SetEnv(&shell, vp);
	return TRUE;
    } else {
	return FALSE;
    }
}
#endif

int
var_SRES(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, screen_desc);
	return TRUE;
    } else if (vp) {
	return term.setdescrip(vp);
    } else {
	return FALSE;
    }
}

int
var_STARTUP_FILE(TBUFF **rp, const char *vp)
{
    return any_rw_TXT(rp, vp, &startup_file);
}

int
var_STARTUP_PATH(TBUFF **rp, const char *vp)
{
    return any_rw_TXT(rp, vp, &startup_path);
}

static int lastcmdstatus = TRUE;

void
setcmdstatus(int s)
{
    lastcmdstatus = s;
}

int
var_STATUS(TBUFF **rp, const char *vp)
{
    return any_ro_BOOL(rp, vp, lastcmdstatus);
}

#if OPT_EVAL
int
var__STATUS(TBUFF **rp, const char *vp)
{
    return any_ro_TBUFF(rp, vp, &last_macro_result);
}

int
var_RETURN(TBUFF **rp, const char *vp)
{
    return any_rw_TBUFF(rp, vp, &this_macro_result);
}
#endif /* OPT_EVAL */

#if OPT_TITLE
int
var_TITLE(TBUFF **rp, const char *vp)
{
    if (rp) {
#if DISP_X11
	tb_scopy(rp, x_get_window_name());
	return TRUE;
#elif SYS_WINNT
	tb_scopy(rp, w32_wdw_title());
	return TRUE;
#endif
    } else if (vp) {
#if DISP_X11 || SYS_WINNT
	tb_scopy(&request_title, vp);
	return TRUE;
#else
	return ABORT;		/* read-only */
#endif
    }
    return FALSE;
}

int
var_TITLEFORMAT(TBUFF **rp, const char *vp)
{
    int code = any_rw_STR(rp, vp, &title_format);
    if (!rp && vp)
	set_editor_title();
    return code;
}
#endif /* OPT_TITLE */

int
var_TPAUSE(TBUFF **rp, const char *vp)
{
    return any_rw_INT(rp, vp, &term.pausecount);
}

int
var_VERSION(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp, version);
}

int
var_WITHPREFIX(TBUFF **rp, const char *vp)
{
    return any_ro_STR(rp, vp, tb_values(with_prefix));
}

int
var_WLINES(TBUFF **rp, const char *vp)
{
    if (rp) {
	render_int(rp, curwp->w_ntrows);
	return TRUE;
    } else if (vp && curwp) {
	return resize(TRUE, strtol(vp, 0, 0));
    } else {
	return FALSE;
    }
}

int
var_WORD(TBUFF **rp, const char *vp)
{
    return any_REPL(rp, vp, vl_nonspace);
}

#if OPT_HOOKS
int
var_WRHOOK(TBUFF **rp, const char *vp)
{
    return any_HOOK(rp, vp, &writehook);
}
#endif

#if DISP_X11 && OPT_SHELL
int
var_XDISPLAY(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, get_xdisplay());
	return TRUE;
    } else if (vp) {
	SetEnv(&x_display, vp);
	return TRUE;
    } else {
	return FALSE;
    }
}

int
var_XSHELL(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, get_xshell());
	return TRUE;
    } else if (vp) {
	SetEnv(&x_shell, vp);
	return TRUE;
    } else {
	return FALSE;
    }
}

int
var_XSHELLFLAGS(TBUFF **rp, const char *vp)
{
    if (rp) {
	tb_scopy(rp, get_xshellflags());
	return TRUE;
    } else if (vp) {
	SetEnv(&x_shellflags, vp);
	return TRUE;
    } else {
	return FALSE;
    }
}
#endif
#endif

#if NO_LEAKS
void
ev_leaks(void)
{
#if OPT_EVAL
    UVAR *p;
    while ((p = temp_vars) != 0)
	rmv_tempvar(p->u_name);

    FreeAndNull(shell);
    FreeAndNull(directory);
#if DISP_X11
    FreeAndNull(x_display);
    FreeAndNull(x_shell);
#endif
#endif
}
#endif /* NO_LEAKS */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          