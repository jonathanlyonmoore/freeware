/*
 * w32cbrd:  collection of common clipboard manipulation routines shared by
 *           the Win32 console- and GUI-based vile editor.
 *
 * Caveats
 * =======
 * -- On a stock Win95 host, the first copy to the clipboard from the
 *    console version of vile causes the busy thread cursor to be displayed
 *    (i.e., cursor changes to a pointer/hourglass icon).  This cursor stays
 *    active for 5-10 seconds (all apps are active) and then goes away.
 *    Subsequent copies do not show this cursor.  On an NT host, this
 *    phenomenon does not occur.
 *
 * $Header: /usr/build/vile/vile/RCS/w32cbrd.c,v 1.25 2001/12/21 13:05:12 tom Exp $
 */

#include "estruct.h"
#include "edef.h"

#include <stdlib.h>
#include <search.h>

#define  CLIPBOARD_BUSY      "[Clipboard currently busy]"
#define  CLIPBOARD_COPY_MB   "[Clipboard copy from minibuffer not supported]"
#define  CLIPBOARD_COPYING   "[Copying...]"
#define  CLIPBOARD_COPY_FAIL "[Clipboad copy failed]"
#define  CLIPBOARD_COPY_MEM  "[Insufficient memory for copy operation]"

typedef struct rgn_cpyarg_struct
{
    UINT nbyte, nline;
    UCHAR *dst;
} RGN_CPYARG;

static int print_low, print_high;

/* ------------------------------------------------------------------ */

static void
minibuffer_abort(void)
{
    char str[3];

    /*
     * Aborting out of the minibuffer is not easy.  When in doubt, use a
     * sledge hammer.
     */

    str[0] = ESC;
    str[1] = '\0';
    (void) w32_keybrd_write(str);
    update(TRUE);
}




static void
report_cbrdstats(UINT nbyte, UINT nline, int pasted)
{
    char buf[128];

    if (! global_b_val(MDTERSE))
    {
        lsprintf(buf,
                 "[%s %d line%s, %d bytes %s clipboard]",
                 (pasted) ? "Pasted" : "Copied",
                 nline,
                 PLURAL(nline),
                 nbyte,
                 (pasted) ? "from" : "to");
        mlwrite(buf);
    }
    else
        mlforce("[%d lines]", nline);
}



/* The memory block handle _must_ be unlocked before calling this fn. */
static int
setclipboard(HGLOBAL hClipMem, UINT nbyte, UINT nline)
{
    int rc, i;

    for (rc = i = 0; i < 8 && (! rc); i++)
    {
        /* Try to open clipboard */

        if (! OpenClipboard(NULL))
            Sleep(500);
        else
            rc = 1;
    }
    if (! rc)
    {
        mlforce(CLIPBOARD_BUSY);
        GlobalFree(hClipMem);
        return (FALSE);
    }
    EmptyClipboard();
    rc = (SetClipboardData(CF_TEXT, hClipMem) != NULL);
    CloseClipboard();
    if (! rc)
    {
        mlforce(CLIPBOARD_COPY_FAIL);
        GlobalFree(hClipMem);
    }
    else
    {
        /* success */

        report_cbrdstats(nbyte - 1,  /* subtract terminating NUL */
                         nline,
                         FALSE);
    }
    return (rc);
}



/* Count lines and nonbuffer data added during "copy to clipboard" operation. */
static void
cbrd_count_meta_data(int        len,
                     UINT      *nbyte,
                     UINT      *nline,
                     char      *src)
{
    register UINT c;

    while (len--)
    {
        if ((c = (UCHAR) *src++) == '\n')
        {
            (*nline)++;
            (*nbyte)++;                    /* API requires CR/LF terminator */
        }
        else if (c < _SPC_ && c != _TAB_)  /* assumes ASCII char set        */
            (*nbyte)++;                    /* account for '^' meta char     */
        else if (c > _TILDE_ && (! PASS_HIGH(c))) /* assumes ASCII char set */
            (*nbyte) += 3;                 /* account for '\xdd' meta chars */
    }
}



/*
 * This function is called to process each logical line of data in a
 * user-selected region.  It counts the number of bytes of data in the line.
 */
static int
count_rgn_data(void *argp, int l, int r)
{
    RGN_CPYARG *cpyp;
    int        len;
    LINE       *lp;

    lp = DOT.l;

    /* Rationalize offsets */
    if (llength(lp) < l)
        return (TRUE);
    if (r > llength(lp))
        r = llength(lp);
    cpyp = argp;
    if (r == llength(lp) || regionshape == RECTANGLE)
    {
        /* process implied newline */

        cpyp->nline++;
        cpyp->nbyte += 2;   /* CBRD API maps NL -> CR/LF */
    }
    len          = r - l;
    cpyp->nbyte += len;
    cbrd_count_meta_data(len, &cpyp->nbyte, &cpyp->nline, lp->l_text + l);
    return (TRUE);
}



static void
cbrd_copy_and_xlate(int len, UCHAR **cbrd_ptr, char *src)
{
    register UINT c;
    UCHAR     *dst = *cbrd_ptr;

    while (len--)
    {
        if ((c = (UCHAR) *src++) == '\n')
        {
            *dst++ = '\r';
            *dst++ = '\n';
        }
        else if ((c >= _SPC_ && c <= _TILDE_) || (c == _TAB_))
            *dst++ = (UCHAR) c;
        else if (c < _SPC_)
        {
            *dst++ = '^';
            *dst++ = ctrldigits[c];
        }
        else
        {
            /* c > _TILDE_ */

            if (! PASS_HIGH(c))
            {
                *dst++ = '\\';
                *dst++ = 'x';
                *dst++ = hexdigits[(c & 0xf0) >> 4];
                *dst++ = hexdigits[c & 0xf];
            }
            else
                *dst++ = (UCHAR) c;
        }
    }
    *cbrd_ptr = dst;
}



/*
 * This function is called to process each logical line of data in a
 * user-selected region.  It copies region data to a buffer allocated on
 * the heap.
 */
static int
copy_rgn_data(void *argp, int l, int r)
{
    RGN_CPYARG *cpyp;
    int        len;
    LINE       *lp;

    lp = DOT.l;

    /* Rationalize offsets */
    if (llength(lp) < l)
        return (TRUE);
    if (r > llength(lp))
        r = llength(lp);
    cpyp = argp;
    len  = r - l;
    cbrd_copy_and_xlate(len, &cpyp->dst, lp->l_text + l);
    if (r == llength(lp) || regionshape == RECTANGLE)
    {
        /* process implied newline */

        *cpyp->dst++ = '\r';
        *cpyp->dst++ = '\n';
    }
    return (TRUE);
}



/*
 * Copy contents of [un]named register to Windows clipboard.  The control
 * flow is shamelessly copied from kwrite().
 */
static int
cbrd_reg_copy(void)
{
    HGLOBAL                 hClipMem;
    register int            i;
    KILL                    *kp;      /* pointer into [un]named register */
    UINT                    nbyte;
    UINT                    nline;
    UCHAR                  *dst;

    /* make sure there is something to put */
    if (kbs[ukb].kbufh == NULL)
    {
        mlforce("[Nothing to copy]");
        return (FALSE);     /* not an error, just nothing */
    }

    print_high = global_g_val(GVAL_PRINT_HIGH);
    print_low  = global_g_val(GVAL_PRINT_LOW);

    /* tell us we're writing */
    mlwrite(CLIPBOARD_COPYING);
    nline = 0;
    nbyte = 0;

    /*
     * Make 2 passes over the data.  1st pass counts the data and
     * adjusts for the fact that:
     *
     * 1) each '\n' must be warped to "\r\n" to satisfy clipboard APIs.
     * 2) unprintable data (modulo tabs) must be warped to a printable
     *    equivalent.
     */
    for (kp = kbs[ukb].kbufh; kp; kp = kp->d_next)
    {
        i      = KbSize(ukb, kp);
        nbyte += i;
        cbrd_count_meta_data(i, &nbyte, &nline, (char *) kp->d_chunk);
    }
    nbyte++;   /* Add room for terminating null */

    /* 2nd pass -- alloc storage for data and copy to clipboard. */
    if ((hClipMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, nbyte)) == NULL)
    {
        mlforce(CLIPBOARD_COPY_MEM);
        return (FALSE);
    }
    dst = GlobalLock(hClipMem);
    for (kp = kbs[ukb].kbufh; kp; kp = kp->d_next)
        cbrd_copy_and_xlate((int) KbSize(ukb, kp), &dst, (char *) kp->d_chunk);
    *dst = '\0';
    GlobalUnlock(hClipMem);
    return (setclipboard(hClipMem, nbyte, nline));
}



/*
 * Copy contents of unnamed register to Windows clipboard.
 *
 * Bound to Alt+Insert.
 */
int
cbrdcpy_unnamed(int unused1, int unused2)
{
    int rc;

    if (reading_msg_line)
    {
        minibuffer_abort();   /* FIXME -- goes away some day? */
        mlerase();
        mlforce(CLIPBOARD_COPY_MB);
        return (ABORT);
    }
    kregcirculate(FALSE);
    rc  = cbrd_reg_copy();
    ukb = 0;
    return (rc);
}


/*
 * Copy the currently-selected region (i.e., the range of lines from DOT to
 * MK, inclusive) to the windows clipboard.  Lots of code has been borrowed
 * and/or adapted from operyank() and writereg().
 */
static int
cbrdcpy_region(void)
{
    RGN_CPYARG              cpyarg;
    DORGNLINES              dorgn;
    HGLOBAL                 hClipMem;
    MARK                    odot;
    int                     rc;

    mlwrite(CLIPBOARD_COPYING);
    print_high   = global_g_val(GVAL_PRINT_HIGH);
    print_low    = global_g_val(GVAL_PRINT_LOW);
    odot         = DOT;          /* do_lines_in_region() moves DOT. */
    cpyarg.nbyte = cpyarg.nline = 0;
    dorgn        = get_do_lines_rgn();

    /*
     * Make 2 passes over the data.  1st pass counts the data and
     * adjusts for the fact that:
     *
     * 1) each '\n' must be warped to "\r\n" to satisfy clipboard APIs.
     * 2) unprintable data (modulo tabs) must be warped to a printable
     *    equivalent.
     */
    rc  = dorgn(count_rgn_data, &cpyarg, TRUE);
    DOT = odot;
    if (!rc)
        return (FALSE);
    cpyarg.nbyte++;        /* Terminating nul */

    /* 2nd pass -- alloc storage for data and copy to clipboard. */
    if ((hClipMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,
                                cpyarg.nbyte)) == NULL)
    {
        mlforce(CLIPBOARD_COPY_MEM);
        return (FALSE);
    }
    cpyarg.dst = GlobalLock(hClipMem);

    /*
     * Pass #2 -> The actual copy.  Don't need to restore DOT, that
     * is handled by opercbrdcpy().
     */
    rc = dorgn(copy_rgn_data, &cpyarg, TRUE);
    GlobalUnlock(hClipMem);
    if (! rc)
    {
        GlobalFree(hClipMem);
        return (FALSE);
    }
    *cpyarg.dst = '\0';
    return (setclipboard(hClipMem, cpyarg.nbyte, cpyarg.nline));
}



/*
 * Copy contents of specified region or register to Windows clipboard.
 * This command is an operaor and mimics the functionality of ^W, but
 * mimics operyank()'s implemenation.
 *
 * Bound to Ctrl+Insert.
 */
int
opercbrdcpy(int f, int n)
{
    if (reading_msg_line)
    {
        minibuffer_abort();   /* FIXME -- goes away some day? */
        mlerase();
        mlforce(CLIPBOARD_COPY_MB);
        return (ABORT);
    }
    if (ukb != 0)
        return (cbrd_reg_copy());
    else
    {
        MARK odot;
        int  rc;

        odot  = DOT;
        opcmd = OPDEL;
        rc    = vile_op(f, n, cbrdcpy_region, "Clipboard copy");
        DOT   = odot;   /* cursor does not move */
        return (rc);
    }
}

/* ------------------- Paste Functionality ----------------------- */

#define MAX_MAPPED_STR 16     /* fairly conservative :-) */

static int  map_and_insert(UINT, UINT *);

typedef struct { UINT val; char *str; } MAP;

/* Keep this table sorted by "val" . */
static MAP cbrdmap[] =
{
    { 0x85, "..."  },
    { 0x8B, "<"    },
    { 0x91, "'"    },
    { 0x92, "'"    },
    { 0x93, "\""   },
    { 0x94, "\""   },
    { 0x96, "-"    },
    { 0x97, "--"   },
    { 0x99, "(TM)" },
    { 0x9B, ">"    },
    { 0xA6, "|"    },
    { 0xA9, "(C)"  },
    { 0xAB, "<<"   },
    { 0xAD, "-"    },
    { 0xAE, "(R)"  },
    { 0xB1, "+/-"  },
    { 0xBB, ">>"   },
    { 0xBC, "1/4"  },
    { 0xBD, "1/2"  },
    { 0xBE, "3/4"  },
};

/* --------------------------------------------------------------- */

static int
map_compare(const void *elem1, const void *elem2)
{
    return (((MAP *) elem1)->val - ((MAP *) elem2)->val);
}



static int
map_cbrd_char(UINT c, UCHAR mapped_rslt[MAX_MAPPED_STR])
{
    MAP  key, *rslt_p;
    int  nmapped = 0;
    char *str;

    key.val = c;
    rslt_p  = bsearch(&key,
                      cbrdmap,
                      sizeof(cbrdmap) / sizeof(cbrdmap[0]),
                      sizeof(cbrdmap[0]),
                      map_compare);
    if (! rslt_p)
        mapped_rslt[nmapped++] = (UCHAR) c;
    else
    {
        for (str = rslt_p->str; *str; str++)
            mapped_rslt[nmapped++] = *str;
    }
    mapped_rslt[nmapped] = '\0';
    return (nmapped);
}



/* paste a single line from the clipboard to the minibuffer. */
static int
paste_to_minibuffer(UCHAR *cbrddata)
{
    int   rc = TRUE;
    UCHAR *cp = cbrddata, *eol = NULL, map_str[MAX_MAPPED_STR + 1],
          one_char[2];

    while(*cp)
    {
        if (*cp == '\r' && *(cp + 1) == '\n')
        {
            eol = cp;

            /*
             * Don't allow more than one line of data to be pasted into the
             * minibuffer (to protect the user from seriously bad side
             * effects when s/he pastes in the wrong buffer).  We don't
             * report an error here in an effort to retain compatibility
             * with a couple of "significant" win32 apps (e.g., IE and
             * Outlook) that simply truncate a paste at one line when it
             * only makes sense to take a single line of input.
             */
            break;
        }
        cp++;
    }
    if (eol)
        *eol = '\0';  /* chop delimiter */
    one_char[1] = '\0';
    while (*cbrddata && rc)
    {
        if (*cbrddata > _TILDE_)
        {
            (void) map_cbrd_char(*cbrddata, map_str);
            rc = w32_keybrd_write((char *) map_str);
        }
        else
        {
            one_char[0] = *cbrddata;
            rc = w32_keybrd_write((char *) one_char);
        }
        cbrddata++;
    }
    return (rc);
}



/*
 * Paste contents of windows clipboard (if TEXT) to current buffer.
 * Bound to Shift+Insert.
 */
int
cbrdpaste(int f, int n)
{
    register UINT      c;
    register UCHAR *data;
    HANDLE   hClipMem;
    int      i, rc, suppressnl;
    UINT     nbyte, nline;

    for (rc = i = 0; i < 8 && (! rc); i++)
    {
        /* Try to open clipboard */

        if (! OpenClipboard(NULL))
            Sleep(500);
        else
            rc = 1;
    }
    if (! rc)
    {
        if (reading_msg_line)
        {
            minibuffer_abort();   /* FIXME -- goes away some day? */
            rc = ABORT;
        }
        else
            rc = FALSE;
        mlforce(CLIPBOARD_BUSY);
        return (rc);
    }
    if ((hClipMem = GetClipboardData(CF_TEXT)) == NULL)
    {
        CloseClipboard();
        if (reading_msg_line)
        {
            minibuffer_abort();   /* FIXME -- goes away some day? */
            rc = ABORT;
        }
        else
            rc = FALSE;
        mlforce("[Clipboard empty or not TEXT data]");
        return (rc);
    }
    if ((data = GlobalLock(hClipMem)) == NULL)
    {
        CloseClipboard();
        if (reading_msg_line)
        {
            minibuffer_abort();   /* FIXME -- goes away some day? */
            rc = ABORT;
        }
        else
            rc = FALSE;
        mlforce("[Can't lock clipboard memory]");
        return (rc);
    }
    if (reading_msg_line)
    {
        rc = paste_to_minibuffer(data);
        GlobalUnlock(hClipMem);
        CloseClipboard();
        return (rc);
    }
    mlwrite(CLIPBOARD_COPYING);
    nbyte = nline = 0;
    rc    = TRUE;

    /*
     * Before stuffing data in the current buffer, save info regarding dot
     * and mark.  The dot/mark manipulation code is more or less cribbed
     * from doput() and PutChar().  Note also that clipboard data is always
     * copied into the current region as if it were an "exact" shape, which
     * should be the most intuitive result for Windows users who work with
     * the clipboard (I hope).
     */
    suppressnl = is_header_line(DOT, curbp);
    /* this pastes after DOT instead of at DOT - very annoying
    if (! is_at_end_of_line(DOT))
        forwchar(TRUE,1);
    */
    (void) setmark();
    while(*data && rc)
    {
        if ((c = *data) == '\n')
        {
            nbyte++;
            nline++;
            rc = lnewline();
        }
        else if (c == '\r' && *(data + 1) == '\n')
        {

            /* Clipboard end of line delimiter is crlf.  Ignore cr. */

            ;
        }
        else if (c > _TILDE_)
            rc = map_and_insert(c, &nbyte);
        else
        {
            nbyte++;
            rc = linsert(1, (int) c);
        }
        data++;
    }
    if (rc)
    {
        if (nbyte > 0 && (data[-1] == '\n') && suppressnl)
        {
            /*
             * Last byte inserted was a newline and DOT was originally
             * pointing at the beginning of the buffer(??).  In this
             * situation, linsert() has added an additional newline to the
             * buffer.  Remove it.
             */

            (void) ldelete(1, FALSE);
        }
    }
    GlobalUnlock(hClipMem);
    CloseClipboard();
    if (! rc)
        (void) no_memory("cbrdpaste()");
    else
    {
        /*
         * Success.  Fiddle with dot and mark again (another chunk of doput()
         * code).  "Tha' boy shore makes keen use of cut and paste."
         * Don't swap if inserting - this allows you to continue typing
         * after a paste operation without additional futzing with DOT.
         */

        if (!insertmode) swapmark();          /* I understand this. */
        if (is_header_line(DOT, curbp))
            DOT.l = lback(DOT.l);             /* This is a mystery. */
        report_cbrdstats(nbyte, nline, TRUE);
    }
    return (rc);
}



/*
 * Map selected characters from the ANSI character set to their ASCII
 * equivalents and insert same in the current buffer.
 */
static int
map_and_insert(UINT c,       /* ANSI char to insert   */
               UINT *nbyte   /* total #chars inserted */
               )
{
    UCHAR mapped_str[MAX_MAPPED_STR];
    int           i, nmapped, rc;

    nmapped = map_cbrd_char(c, mapped_str);
    *nbyte += nmapped;
    for (rc = TRUE, i = 0; i < nmapped && rc; i++)
        rc = linsert(1, mapped_str[i]);
    return (rc);
}

/* --------------------- Cut Functionality ----------------------- */

/*
 * Cut current [win]vile selection to windows clipboard.
 * Bound to Shift+Delete.
 */
int
cbrdcut(int f, int n)
{
    return (w32_del_selection(TRUE));
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            