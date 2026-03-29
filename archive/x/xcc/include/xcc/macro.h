/**
 * Macro string evaluation and substitution utilities.
 *
 * Provides formatted printing and macro expansion using a hash table.
 * Commonly used in diagnostics and template substitution with value maps.
 *
 * Original Author: Sandeep Dutta <sandeep.dutta@usa.net>, 1998
 * Released under the GNU General Public License v2 or later.
 */

#pragma once

#include <stdio.h>
#include <stdarg.h>

#include <xcc/hashtab.h>

#ifdef __cplusplus
extern "C"
{
#endif

    char *eval_macros(hTab *pvals, const char *pfrom);
    char *mvsprintf(hTab *pvals, const char *pformat, va_list ap);
    char *msprintf(hTab *pvals, const char *pformat, ...);
    void mfprintf(FILE *fp, hTab *pvals, const char *pformat, ...);

#ifdef __cplusplus
}
#endif
