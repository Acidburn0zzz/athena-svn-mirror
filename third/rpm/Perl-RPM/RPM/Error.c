/*
 * This file was generated automatically by xsubpp version 1.9507 from the 
 * contents of Error.xs. Do not edit this file, edit Error.xs instead.
 *
 *	ANY CHANGES MADE HERE WILL BE LOST! 
 *
 */

#line 1 "Error.xs"
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "RPM.h"

static char * const rcsid = "$Id: Error.c,v 1.1.1.1 2002-09-14 23:13:22 ghudson Exp $";

static CV* err_callback;

#if (RPM_VERSION >= 0x040002)
#  define ERR_STR_CONST const
#else
#  define ERR_STR_CONST
#endif

/*
  This was static, but it needs to be accessible from other modules, as well.
*/
SV* rpm_errSV;

/*
  This is a callback routine that the bootstrapper will register with the RPM
  lib so as to catch any errors. (I hope)
*/
static void rpm_catch_errors(void)
{
    /* Because rpmErrorSetCallback expects (void)fn(void), we have to declare
       our thread context here */
    dTHX;
    int error_code;
    ERR_STR_CONST char* error_string;

    error_code = rpmErrorCode();
    error_string = rpmErrorString();

    /* Set the string part, first */
    sv_setsv(rpm_errSV, newSVpv(error_string, strlen(error_string)));
    /* Set the IV part */
    sv_setiv(rpm_errSV, error_code);
    /* Doing that didn't erase the PV part, but it cleared the flag: */
    SvPOK_on(rpm_errSV);

    /* If there is a current callback, invoke it: */
    if (err_callback != Nullcv)
    {
        /* This is just standard boilerplate for calling perl from C */
        dSP;
        ENTER;
        SAVETMPS;
        PUSHMARK(sp);
        XPUSHs(sv_2mortal(newSViv(error_code)));
        XPUSHs(sv_2mortal(newSVpv(error_string, strlen(error_string))));
        PUTBACK;

        /* The actual call */
        perl_call_sv((SV *)err_callback, G_DISCARD);

        /* More boilerplate */
        SPAGAIN;
        PUTBACK;
        FREETMPS;
        LEAVE;
    }

    return;
}

/* This is just to offer an easy way to clear both sides of $RPM::err */
void clear_errors(pTHX)
{
    sv_setsv(rpm_errSV, newSVpv("", 0));
    sv_setiv(rpm_errSV, 0);
    SvPOK_on(rpm_errSV);

    return;
}

SV* set_error_callback(pTHX_ SV* newcb)
{
    SV* oldcb;

    oldcb = (err_callback) ? newRV((SV *)err_callback) : newSVsv(&PL_sv_undef);

    if (SvROK(newcb)) newcb = SvRV(newcb);
    if (SvTYPE(newcb) == SVt_PVCV)
        err_callback = (CV *)newcb;
    else if (SvPOK(newcb))
    {
        char* fn_name;
        char* sv_name;

        sv_name = SvPV(newcb, PL_na);
        if (! strstr(sv_name, "::"))
        {
            Newz(TRUE, fn_name, strlen(sv_name) + 7, char);
            strncat(fn_name, "main::", 6);
            strcat(fn_name + 6, sv_name);
        }
        else
            fn_name = sv_name;

        err_callback = perl_get_cv(fn_name, FALSE);
    }
    else
    {
        err_callback = Nullcv;
    }

    return oldcb;
}

void rpm_error(pTHX_ int code, const char* message)
{
    rpmError(code, (char *)message);
}


#line 129 "Error.c"
XS(XS_RPM__Error_set_error_callback)
{
    dXSARGS;
    if (items != 1)
	Perl_croak(aTHX_ "Usage: RPM::Error::set_error_callback(newcb)");
    {
	SV*	newcb = ST(0);
	SV *	RETVAL;
#line 127 "Error.xs"
    RETVAL = set_error_callback(aTHX_ newcb);
#line 140 "Error.c"
	ST(0) = RETVAL;
	sv_2mortal(ST(0));
    }
    XSRETURN(1);
}

XS(XS_RPM__Error_clear_errors)
{
    dXSARGS;
    if (items != 0)
	Perl_croak(aTHX_ "Usage: RPM::Error::clear_errors()");
    {
#line 135 "Error.xs"
    clear_errors(aTHX);
#line 155 "Error.c"
    }
    XSRETURN_EMPTY;
}

XS(XS_RPM__Error_rpm_error)
{
    dXSARGS;
    if (items != 2)
	Perl_croak(aTHX_ "Usage: RPM::Error::rpm_error(code, message)");
    {
	int	code = (int)SvIV(ST(0));
	char*	message = (char *)SvPV(ST(1),PL_na);
#line 143 "Error.xs"
    rpm_error(aTHX_ code, message);
#line 170 "Error.c"
    }
    XSRETURN_EMPTY;
}

#ifdef __cplusplus
extern "C"
#endif
XS(boot_RPM__Error)
{
    dXSARGS;
    char* file = __FILE__;

        newXSproto("RPM::Error::set_error_callback", XS_RPM__Error_set_error_callback, file, "$");
        newXSproto("RPM::Error::clear_errors", XS_RPM__Error_clear_errors, file, "");
        newXSproto("RPM::Error::rpm_error", XS_RPM__Error_rpm_error, file, "$$");

    /* Initialisation Section */

#line 147 "Error.xs"
{
    rpm_errSV = perl_get_sv("RPM::err", GV_ADD|GV_ADDMULTI);
    rpmErrorSetCallback(rpm_catch_errors);
    err_callback = Nullcv;
}

#line 196 "Error.c"

    /* End of Initialisation Section */

    XSRETURN_YES;
}

