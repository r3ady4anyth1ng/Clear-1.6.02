/*
 *+***********************************************************************
 *
 *  Module:
 *	image.h
 *
 *  Purpose:
 *	This is the sample header file for an image DSO.
 *
 *  Author/Date:
 */

/*
//
//-
// ==========================================================================
// Copyright 1995,2006,2008 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+
//
*/

#ifndef XPG3_EXISTS
#define XPG3_EXISTS
#endif
#include <math.h>

#ifndef IMF_EXPORT

	#ifdef	_WIN32

		#ifdef	IMF_DLL
			#define IMF_EXPORT	_declspec( dllexport )
		#else	/* IMF_DLL */
			#define IMF_EXPORT	_declspec( dllimport )
		#endif	/* IMF_DLL */
	#else   /* not WIN32 */
		#include <nl_types.h>
		#define IMF_EXPORT
	#endif	/*WIN32*/

#endif	/*IMF_EXPORT*/


#ifndef	PLUGINEXPORT

#ifdef	_WIN32

#ifdef	PLUGIN_DLL
#define PLUGINEXPORT	__declspec( dllexport )
#else	/*PLUGIN_DLL*/
#define PLUGINEXPORT
#endif	/*PLUGIN_DLL*/

#else	/* WIN32 */

#define PLUGINEXPORT

#endif	/*WIN32*/

#endif	/* PLUGINEXPORT */
/*
 * $Header: /home/rwoods/IMF_1.0/SOURCE/IMF/include/wf_std.h /main/8 1999/02/23 01:35:42 rwoods Exp $
 *
 * ************************************************************************
 * ******************		wf_std.h	    ***********************
 * ************************************************************************
 *  Copyright 1986  Wavefront Technologies, Inc, All rights reserved.
 *		    1421 State Street, Suite H
 *		    Santa Barbara, CA 93101
 *  created by: Roy Hall
 *
 *  PURPOSE AND CONTENTS:
 *	General data type definitions and parameters for use with
 *	the rendering systems, compositors, modelers, and motion
 *	planning and control systems provided by Wavefront Technologies
 */


#ifndef	WF_STD
#define	WF_STD

#ifdef  __cplusplus
extern "C" {			/* Stops C++ compiler renaming functions*/
#endif  /* __cplusplus */


/*
 *  Standard definitions which may also be declared elsewhere...
 */

#ifndef	TRUE
#define	TRUE		1
#endif	/* TRUE */

#ifndef	FALSE
#define	FALSE		0
#endif	/* FALSE */

#ifndef	YES
#define	YES		1
#endif	/* YES */

#ifndef	NO
#define	NO		0
#endif	/* NO */

#ifndef	NULL
#define	NULL		0
#endif	/* NULL */


/*
 *  Standard data types...
 *  (In the spirit of not changing anything already existing in this file,
 *  we'll leave U_CHAR, U_SHORT and FLAG as #defines.)
 */

#define	U_CHAR		unsigned char
#define	U_SHORT		unsigned short
#define	FLAG		char

#ifdef	_BOOL
#define BOOLEAN BOOLEANSAFE
#endif	/*_BOOL*/
//typedef	char		BOOLEAN;	/* TRUE/FALSE variable type	*/
typedef	double		MATRIX_4D[4][4];/* A 4 by 4 matrix of doubles	*/
typedef	float		MATRIX_4F[4][4];/* A 4 by 4 matrix of floats	*/
#ifdef __STDC__
typedef	void		*POINTER;	/* Generic pointer type		*/
#else
typedef	char		*POINTER;	/* Generic pointer type		*/
#endif
typedef	signed char	S_CHAR;		/* Signed character type	*/
typedef	signed int	S_INT;		/* Signed integer type		*/
typedef	signed short	S_SHORT;	/* Signed short type		*/
typedef unsigned int	U_INT;		/* Unsigned integer type	*/
#ifndef	U_LONG
typedef unsigned int	U_LONG;		/* Unsigned longword type	*/
#endif	/* U_LONG */

#ifdef LINUX
typedef char BOOLEAN;
#endif
typedef BOOLEAN		(*FP_BOOLEAN)();/* Pointer to a BOOLEAN routine	*/
typedef double		(*FP_DOUBLE)();	/* Pointer to a double routine	*/
typedef float		(*FP_FLOAT)();	/* Pointer to a float routine	*/
typedef int		(*FP_INT)();	/* Pointer to an integer routine*/
typedef POINTER		(*FP_POINTER)();/* Pointer to a POINTER routine	*/
typedef void		(*FP_VOID)();	/* Pointer to a void routine	*/

/* Cryptic pointer-to-function types */
typedef int		(*FUNC_PTR)();
typedef int		(*FUNC_PTR_I)();
typedef long		(*FUNC_PTR_L)();
typedef short		(*FUNC_PTR_S)();
typedef float		(*FUNC_PTR_F)();
typedef char		*(*FUNC_PTR_P)();


/*
 *  Standard argument list passing structure
 */

typedef struct {
    char		*arg_name;
    union  {short	s;
	    long	l;
	    int		i;
	    float	f;
	    double	d;
	    POINTER	p;} value;
} LIST_ARG;

/* file system path segment for access to Wavefront data
 * Use this in conjunction with the environment variable WAVE_DIR
 * to build the complete pathname to the data area.
 * Note that it DOES have a trailing '/'
 */
#define WAVE_DATA_DIR "/Site/data/.db/"

/* declaration for external variables */
#define EXTERNAL extern

/* exit handling codes
 */
#define EXH_C_RETURN	0
#define EXH_C_EXIT	1
#define EXH_C_RET_EXIT	2
#define EXH_C_SIGALL	32

/* 3D points and transformation matricies
 */
#define MT_C_X_AXIS	0
#define MT_C_Y_AXIS	1
#define MT_C_Z_AXIS	2

/* defines the mapping between a texture and the 'real' world
 */
typedef struct {
    float	u,v,w;
} MAP_COORD;

/* defines the lookup parameters for a texture - u, v, w, and level
 */
typedef struct {
    float	u,v,w,l;
} TEX_COORD;

typedef struct	point2d			/* 2D double-precision point	*/
{
    double	x;
    double	y;
} POINT_2D;

typedef struct	point2f			/* 2D single-precision point	*/
{
    float	x;
    float	y;
} POINT_2F;

typedef struct {
    int		x,y;
} POINT_2I;

typedef struct {
    int		x,y,z;
} POINT_3I;

typedef struct	point3d			/* 3D double-precision point	*/
{
    double	xd;
    double	yd;
    double	zd;
} POINT_3D;

typedef struct {
    float	x,y,z;
} POINT_3F;

typedef struct	point4d			/* 3D point w/ homogeneous coord*/
{
    double	xd;
    double	yd;
    double	zd;
    double	hd;
} POINT_4D;

typedef struct {
    float	x,y,z,h;		/* h is the homogenious coord	*/
} POINT_4F;

typedef struct {
    float	i,j,k;
} UNIT_VECTOR;

typedef struct {
    POINT_3F	origin;			/* origin of the unit vector	*/
    UNIT_VECTOR	direction;		/* direction of the unit vector	*/
} VECTOR;

typedef struct	vector2d		/* 2D double-precision vector	*/
{
    double	i;
    double	j;
} VECTOR_2D;

typedef struct	vector2f		/* 2D single-precision vector	*/
{
    float	i;
    float	j;
} VECTOR_2F;

typedef struct	vector3d		/* 3D double-precision vector	*/
{
    double	i;
    double	j;
    double	k;
} VECTOR_3D;

typedef struct	vector3f		/* 3D single-precision vector	*/
{
    float	i;
    float	j;
    float	k;
} VECTOR_3F;

typedef	struct	eptr			/* Efficient pointer array	*/
{
    void	**e_pointer;		/* Array of pointer values	*/
    int		e_allocd;		/* Actual length of e_pointer	*/
    int		e_quantum;		/* Memory allocation increment	*/
    int		e_used;			/* Number of active list entries*/
} EPTR;

typedef struct {
    float	a,b,c,d;    /* plane equation: ax + by + cz + d = 0.0 */
} PLANE_EQ;

typedef union {
    float	all[16];
    float	elem[4][4];
} TRANSFORM;


/* data types for windows and image related stuff
 */
typedef struct {
    int		left, right, bottom, top;
} WINDOW_I;

typedef struct {
    short	left, right, bottom, top;
} WINDOW_S;

typedef struct {
    float	left, right, bottom, top;
} WINDOW_F;

/* declarations of standard utility routines
 */
int		UTL_valid_file();


/*
 *  Declarations for internationalization...
 */


typedef struct		msgcat_defn
{
    int			mcd_set;	/* Set of string		*/
    int			mcd_id;		/* Id of string			*/
    char		*mcd_dflt;	/* Default string		*/
} MSGCAT_DEFN;

#define	MSGCAT_DEFN_NONE		{ 0, 0, (char *) NULL }

#ifndef	XPG3_EXISTS
#define	catgets( catd, set_num, msg_num, s )	( s )
#endif	/* XPG3_EXISTS */

#define IF_FALSE_GOTO_ERROR( a, label ) \
            { if ( !(a) ) goto label; }
#define IF_FALSE_RETURN_FALSE( a ) \
            { if ( !(a) ) return( FALSE ); }
#define IF_FALSE_RETURN_NULL( a ) \
            { if ( !(a) ) return( NULL ); }
#define IF_NULL_GOTO_ERROR( a, label ) \
            { if ( (a) == (void *) NULL ) goto label; }
#define IF_NULL_RETURN_FALSE( a ) \
            { if ( (a) == (void *) NULL ) return( FALSE ); }
#define IF_NULL_RETURN_NULL( a ) \
            { if ( (a) == (void *) NULL ) return( NULL ); }
#define IF_NEG_GOTO_ERROR( a, label ) \
            { if ( (a) < 0 ) goto label; }
#define IF_NEG_RETURN_FALSE( a ) \
            { if ( (a) < 0 ) return( FALSE ); }
#define IF_NEG_RETURN_NULL( a ) \
            { if ( (a) < 0 ) return( NULL ); }

#ifdef  __cplusplus
}
#endif  /* __cplusplus */

#endif	/* WF_STD */
/*
 *<
 * $Header: /home/rwoods/IMF_1.0/SOURCE/IMF/include/wf_error.h /main/14 1999/05/03 23:03:17 rwoods Exp $
 *
 * ************************************************************************
 * *******************         wf_error.h           ***********************
 * ************************************************************************
 *>
 *  Copyright 1991  Wavefront Technologies, Inc, All rights reserved.
 *                  530 E. Montecito St., Suite 106
 *                  Santa Barbara, CA 93103
 *
 *  created by: chris kitrick           date: feb 1991
 *<
 *  MODULE PURPOSE:
 *
 *      This is an include file containing all of the structure definitions
 *      and defines for the Wavefront error handler.
 *
 *  HISTORY:
 *
 *      $Log: wf_error.h $
 * Revision /main/14  1999/05/03  23:03:17  rwoods
 * Removed old Wavefront ERR_MSG stuff and
 * action stuff.  A number of files were including wf_stderr.h when they just 
 * needed to include wf_error.h.  Moved imf_error to be included in liberror 
 * rather than libimf for things that need ERR_report_cat but not IMF.
 * 
 * Revision /main/13  1999/02/23  01:29:20  rwoods
 * Merge from Apache src to add NT changes.
 * 
 * Revision 1.12  1995/10/19  19:48:31  swbld
 * corrected defn of ERR_dcl_handler
 *
 * Revision 1.11  1994/12/14  19:58:19  rwoods
 * Added routine to control when multiprocess error locking is in effect.
 *
 * Revision 1.10  1994/10/31  21:27:37  swbld
 * prototypes now always enabled
 *
 * Revision 1.9  1994/10/31  20:02:15  swbld
 * added C++ compatability
 *
 * Revision 1.8  1994/10/31  19:59:09  bhickey
 * Changed POINTER to void * in prototype.
 *
 * Revision 1.7  1994/08/08  21:44:01  bhickey
 * Added prototypes.
 *
 * Revision 1.6  1994/03/22  01:36:08  bdiack
 * added a decvlaration for ERR_realloc
 *
 * Revision 1.5  1994/02/16  20:40:40  bdiack
 * added prototypes declarations to ERR_printf, ERR_report_cat,
 * ERR_report_errno
 *
 * Revision 1.1  1994/02/16  20:38:40  bdiack
 * Initial revision
 *
 * Revision 1.4  1994/01/22  03:16:08  bdiack
 * adedd XPG3_EXISTS brackets around the language catalogue references
 *
 * Revision 1.3  1993/11/12  11:37:05  bdiack
 * added ERR_report_errno, ERR_printf, ERR_report_cat. the last one
 * is used with xpg/3 message catalogues.
 *
 * Revision 1.2  1991/06/17  14:42:07  chrisk
 * Added ERR_malloc declaration
 *
 * Revision 1.1  91/04/11  10:23:33  swsrc
 * Initial revision
 * 
 * Revision 3.0  91/02/20  11:14:54  chrisk
 * Initial Version
 * 
 *>
 */


#ifndef WF_ERROR_H
#define WF_ERROR_H

#ifdef	XPG3_EXISTS
#endif


#ifdef  __cplusplus
extern "C" {			/* Stops C++ compiler renaming functions*/
#endif  /* __cplusplus */

#define ERR__VERSION    1       /* internal version number              */

#define ERR__ALL        -1      /* return info for all errors           */
#define ERR__GLOBAL     0       /* declare handler for all errors       */

#define ERR__SAVE       16      /* number of previous messages saved    */

/* severity levels for errors */
#define ERR__DEBUG      0       /* debug messages, no adverse effects   */
#define ERR__INFO       1       /* informational, no adverse effects    */
#define ERR__WARNING    2       /* warning, operation not complete      */
#define ERR__ERROR      3       /* error, severe but exec continued     */
#define ERR__FATAL      4       /* fatal error, execution terminated    */

/* exit status for error handler routines */
#define ERR__QUIT       0       /* quit handling this error             */
#define ERR__CONTINUE   1       /* continue calling handlers            */

typedef int (*ERR__HANDLER)();

/* error handler linked list element */
typedef struct err__hdlr {
	struct err__hdlr *next;    /* pointer to next list element         */
	int          (*handler)(); /* error handling routine               */
	char         *handler_arg; /* argument to be passed to handler     */
} ERR__HDLR;

/* structure to contain all error messages */
typedef struct {
	unsigned char version;   /* internal software version number     */
	unsigned char severity;  /* severity level                       */
	unsigned  masked : 1;    /* "this error is masked"               */
	unsigned  filler : 15;   /* not used                             */
	char      *identifier;   /* unique id string for this error      */
	char      *msg_template; /* message text control string          */
	int       ref_count;     /* number of errors reported            */
	int       (*def_hdlr)(); /* default handler for this error       */
	ERR__HDLR *handler_list; /* list of handlers for this error      */
	char      **explanation; /* descriptive help text                */
	char      **user_action; /* steps required to recover from error */
	char      **fill;        /* fill in text                         */
} ERR__MSG;

/* error routines */
/*VARARGS*/
IMF_EXPORT	int          ERR_report( ERR__MSG*,  ... );
IMF_EXPORT	char         *ERR_format( char*, ERR__MSG*, ... );
IMF_EXPORT	void         ERR_query( ERR__MSG*, int*, int*, int*, char**,
				char**, char***, char*** );
IMF_EXPORT	int          ERR_stats( int );
IMF_EXPORT	int          ERR_mask( ERR__MSG* );
IMF_EXPORT	int          ERR_mask_s( int);
IMF_EXPORT	int          ERR_unmask( ERR__MSG* );
IMF_EXPORT	int          ERR_unmask_s( int );
IMF_EXPORT	ERR__MSG     *ERR_last( int );
IMF_EXPORT	int          ERR_dcl_handler( ERR__MSG*,
				int (*)( char *, int, char *, ERR__MSG *,
				    char * ), char * );
IMF_EXPORT	ERR__HANDLER *ERR_can_handler( ERR__MSG* );
IMF_EXPORT	int          ERR_compare( char*, ERR__MSG*, int );
IMF_EXPORT	int          ERR_def_handler( char*, int, char*, ERR__MSG*,
				char* );
IMF_EXPORT	int	     ERR_free( void* );
IMF_EXPORT	void         *ERR_malloc( unsigned );
IMF_EXPORT	void         *ERR_realloc( void*, unsigned );
IMF_EXPORT	char         *ERR_type( int );
IMF_EXPORT	char         *ERR_format_fill( ERR__MSG*, char* );


/*
 *  Function declarations...
 */

/* PRINTFLIKE2 */
IMF_EXPORT	void	ERR_printf(
			    int type, char *format, ...
			);

#ifdef 	_WIN32
/* PRINTFLIKE1 */
#else	/* _WIN32 */
/* PRINTFLIKE5 */
#endif	/* _WIN32 */
IMF_EXPORT	void	ERR_report_cat(
#ifndef _WIN32
#ifdef	XPG3_EXISTS
			    nl_catd catd, int set, int msg, int priority,
#else	/* XPG3_EXISTS */
			    void *catd, int set, int msg, int priority,
#endif	/* XPG3_EXISTS */
#endif	/* !_WIN32 */
			    char *str, ...
			);

IMF_EXPORT	void	ERR_report_errno(
			    int error, char *string
			);

IMF_EXPORT	int	ERR_report_mp(
			    void *sema, int enable
			);

#ifdef  __cplusplus
}
#endif  /* __cplusplus */


/*
 *  Message cataloguing for internationalised applications.
 *  `imf__msg_cat' points to the catalogue and is set via IMF_init().
 *  It takes on the value NULL by default -- which does not use
 *  cataloguing.
 */
#ifndef _WIN32
extern	IMF_EXPORT	nl_catd	imf__msg_cat;
#endif
#endif	/* WF_ERROR_H */
/*
 *+***********************************************************************
 *
 *  Module:
 *	wf_vmath.h
 *
 *  Purpose:
 *	This module is used to declare mathematical definitions
 *
 *  Author/Date:
 *	Bill Diack, 6 September 1991
 *
 *  Functions/Global Variables:
 *
 *  RCS Data:
 *	$Header: /tmp_mnt/remote/ute/usr/ute1/vobForIMF/rcs/include/wf_vmath.h,v 1.13 1995/04/27 23:42:10 bdiack Exp $
 *
 *  Copyright (c) 1991 by Wavefront Canada Ltd., Vancouver, BC and
 *  Wavefront Technologies, Inc., Santa Barbara, CA.  All rights reserved.
 *
 *-***********************************************************************
 */


#ifndef	WF_VMATH_H
#define	WF_VMATH_H


#ifdef  __cplusplus
extern "C" {			/* Stops C++ compiler renaming functions*/
#endif  /* __cplusplus */


/*
 *  Define general floating-point tolerances and miscellaneous constants
 */

#define D_EPSILON	(5e-16)		/* Standard double-precision tol*/
#define F_EPSILON	(5e-5)		/* Standard single-precision tol*/
#define	ONE_K		1024		/* Binary one thousand (2**10)	*/
#define	ONE_MEG		1048576		/* Binary one million (2**20)	*/


/*
 *  Macros which test if two values are equal within tolerance...
 *
 *	D_ macros use the double-precision tolerance
 *	F_ macros use the single-precision tolerance
 *	TOL_ macros accept a tolerance as an argument
 *
 *  The set includes:
 *	_EQ	tests if equal
 *	_GE	tests if greater than or equal to
 *	_GT	tests if strictly greater than
 *	_LE	tests if less than or equal to
 *	_LT	tests if strictly less than
 *	_NE	tests if not equal
 */

#define D_EQ(a,b)	( ABS( (a) - (b) ) < D_EPSILON )
#define D_GE(a,b)	( (a) - (b) > -D_EPSILON )
#define D_GT(a,b)	( (a) - (b) >= D_EPSILON )
#define D_LE(a,b)	( (a) - (b) < D_EPSILON )
#define D_LT(a,b)	( (a) - (b) <= -D_EPSILON )
#define D_NE(a,b)	( ABS( (a) - (b) ) >= D_EPSILON )

#define F_EQ(a,b)	( ABS( (a) - (b) ) < F_EPSILON )
#define F_GE(a,b)	( (a) - (b) > -F_EPSILON )
#define F_GT(a,b)	( (a) - (b) >= F_EPSILON )
#define F_LE(a,b)	( (a) - (b) < F_EPSILON )
#define F_LT(a,b)	( (a) - (b) <= -F_EPSILON )
#define F_NE(a,b)	( ABS( (a) - (b) ) >= F_EPSILON )

#define TOL_EQ(a,b,t)	( ABS( (a) - (b) ) < (t) )
#define TOL_GE(a,b,t)	( (a) - (b) > -(t) )
#define TOL_GT(a,b,t)	( (a) - (b) >= (t) )
#define TOL_LE(a,b,t)	( (a) - (b) < (t) )
#define TOL_LT(a,b,t)	( (a) - (b) <= -(t) )
#define TOL_NE(a,b,t)	( ABS( (a) - (b) ) >= (t) )


/*
 *  General mathematical macros...
 *	ABS	returns the absolute value
 *	CLAMP	limits a value to a range
 *	DTOR	converts an angle in degrees to radians
 *	MAX	computes the maximum of two values
 *	MAX3	computes the maximum of three values
 *	MIN	computes the minimum of two values
 *	MIN3	computes the minimum of three values
 *	RTOD	converts an angle in radians to degrees
 *	SIGN	returns -1 if negative, 0 if zero and +1 if positive
 *	SQR	squares its argument
 *	SWAP	switches two values of type `type', e.g. "SWAP(float,x,y)"
 *	TOL_SIGN
 *		same as `SIGN', but uses a tolerance for equality test
 */

#define ABS(a)		( (a) < 0.0 ? -(a) : (a) )
#define	CLAMP(a,min,max) \
	    ( (a) < (min) ? (min) : ( (a) > (max) ? (max) : (a) ) )
#define DTOR(d)		( (d) * M_PI * 2 / 360.0 )
#define	MAX3(a,b,c)	( ( (a) > (b) )				\
			    ? ( ( (a) > (c) ) ? (a) : (c) )	\
			    : ( ( (b) > (c) ) ? (b) : (c) ) )
#define	MIN3(a,b,c)	( ( (a) < (b) )				\
			    ? ( ( (a) < (c) ) ? (a) : (c) )	\
			    : ( ( (b) < (c) ) ? (b) : (c) ) )
#define RTOD(r)		( (r) * 360.0 / ( M_PI * 2 ) )
#define SIGN(a)		( (a) > 0 ? 1 : (a) < 0 ? -1 : 0 )
#define SQR(a)		( (a) * (a) )
#define	SWAP(type,a,b)	{ type tmp; tmp = (a), (a) = (b), (b) = tmp; }
#define TOL_SIGN(a,t)	( (a) > (t) ? 1 : (a) < -(t) ? -1 : 0 )


/*
 *  The following are also defined in <sys/param.h>
 */

#ifndef MAX
#define MAX(a,b)	( (a) > (b) ? (a) : (b) )
#endif	/* MAX */

#ifndef MIN
#define MIN(a,b)	( (a) < (b) ? (a) : (b) )
#endif	/* MIN */


/*
 *  Mathematical macros limited to integer range (plus or minus two
 *  billion when using 32 bit integers).  They may be faster than using
 *  the standard library calls.
 *
 *	EVEN	tests if its argument is an even number
 *	FCEIL	returns the ceiling of its argument: ceil(a)
 *	FFLOOR	returns the floor of its argument: floor(a)
 *	FRAC	returns the fractional part (the part after the decimal
 *		point, thus FRAC(3.1415) returns 0.1415)
 *	IDIV	integer divide computes the quotient rounded up to the
 *		nearest integer (same as ceil((float)a/b)
 *	ODD	tests if its argument is an odd number
 *	ROUND	rounds its argument to the nearest integer
 */

#define EVEN(a)		( !ODD( a ) )
#define FCEIL(a)	( (int)( (a) <= 0 || (a) == (int)(a) ? (a) : (a) + 1 ) )
#define FFLOOR(a)	( (int)( (a) >= 0 || (a) == (int)(a) ? (a) : (a) - 1 ) )
#define FRAC(a)		ABS( ( (a) - (int) (a) ) )
#define IDIV(x,y)	( ( (x) + (y) - 1 ) / (y) )
#define ODD(a)		( ( (int) (a) ) & 0x1 )
#define ROUND(a)	( (int) ( (a) >= 0 ? (a) + 0.5 : (a) - 0.5 ) )


/*
 *  These have disappeared from the SGI OS 
 */

#ifndef M_PI_2
#define M_PI_2                  1.57079632679489661923
#endif


/*
 *  Operations on vectors and points...
 *  Note that a VECTOR is a direction, and a POINT is a location in space,
 *  and only the meaningful operations between points and vectors are
 *  supported.
 *
 *	POINT*_ADD_VECTOR	point = point + vector
 *	POINT*_ONE		point = { initialise to 1.0 }
 *	POINT*_SUB		vector = point - point
 *	POINT*_SUB_VECTOR	point = point - vector
 *	POINT*_TO_VECTOR	point = vector <- for convenience
 *	POINT*_ZERO		point = { initialise to 0.0 }
 *	VECTOR*_ADD		vector = vector + vector
 *	VECTOR3_CROSS		vector = cross_product( vector, vector )
 *	VECTOR*_DOT		scalar = dot_product( vector, vector )
 *	VECTOR*_LENGTH		scalar = length( vector )
 *	VECTOR*_NORM		void	normalize( vector )
 *	VECTOR*_NEGATE		vector = -1 * vector
 *	VECTOR*_SCALE		vector = scalar * vector
 *	VECTOR*_SUB		vector = vector - vector
 *	VECTOR*_ZERO		vector = { initialise to zero }
 */

#define POINT2_ADD_VECTOR(r,p,v)	( (r).x = (p).x + (v).i, \
					  (r).y = (p).y + (v).j )
#define POINT3_ADD_VECTOR(r,p,v)	( (r).x = (p).x + (v).i, \
					  (r).y = (p).y + (v).j, \
					  (r).z = (p).z + (v).k )

#define	POINT2_ONE(p)			( (p).x = (p).y = 1 )
#define	POINT3_ONE(p)			( (p).x = (p).y = (p).z = 1 )

#define POINT2_SUB(r,p,q)		( (r).i = (p).x - (q).x, \
					  (r).j = (p).y - (q).y )
#define POINT3_SUB(r,p,q)		( (r).i = (p).x - (q).x, \
					  (r).j = (p).y - (q).y, \
					  (r).k = (p).z - (q).z )

#define POINT2_SUB_VECTOR(r,p,v)	( (r).x = (p).x - (v).i, \
					  (r).y = (p).y - (v).j )
#define POINT3_SUB_VECTOR(r,p,v)	( (r).x = (p).x - (v).i, \
					  (r).y = (p).y - (v).j, \
					  (r).z = (p).z - (v).k )

#define	POINT2_TO_VECTOR(v,p)		( (v).i = (p).x, (v).j = (p).y )
#define	POINT3_TO_VECTOR(v,p)		( (v).i = (p).x, (v).j = (p).y,	\
					    (v).k = (p).z )

#define	POINT2_ZERO(p)			( (p).x = (p).y = 0 )
#define	POINT3_ZERO(p)			( (p).x = (p).y = (p).z = 0 )

#define VECTOR2_ADD(r,a,b)		( (r).i = (a).i + (b).i, \
					  (r).j = (a).j + (b).j )
#define VECTOR3_ADD(r,a,b)		( (r).i = (a).i + (b).i, \
					  (r).j = (a).j + (b).j, \
					  (r).k = (a).k + (b).k )

#define	VECTOR3_CROSS(r,a,b)	( (r).i = (a).j * (b).k - (a).k * (b).j, \
				  (r).j = (a).k * (b).i - (a).i * (b).k, \
				  (r).k = (a).i * (b).j - (a).j * (b).i )

#define	VECTOR2_DOT(a,b)		( (a).i * (b).i + (a).j * (b).j )
#define	VECTOR3_DOT(a,b)		( (a).i * (b).i + (a).j * (b).j \
					    + (a).k * (b).k )

#define	VECTOR2_LENGTH(v)		sqrt( VECTOR2_DOT( v, v ) )
#define	VECTOR3_LENGTH(v)		sqrt( VECTOR3_DOT( v, v ) )

#define	VECTOR2_NEGATE(r,a)		( (r).i = -(a).i, \
					  (r).j = -(a).j )
#define	VECTOR3_NEGATE(r,a)		( (r).i = -(a).i, \
					  (r).j = -(a).j, \
					  (r).k = -(a).k )

#define	VECTOR2_NORM(r,a)		{ float LEN; \
					    LEN = 1.0 / (float) sqrt(	\
					      VECTOR2_DOT( (a), (a) ) );\
					    (r).i = (a).i * LEN;	\
					    (r).j = (a).j * LEN		\
					}
#define	VECTOR3_NORM(r,a)		{ float LEN; \
					    LEN = 1.0 / (float) sqrt(	\
					      VECTOR3_DOT( (a), (a) ) );\
					    (r).i = (a).i * LEN;	\
					    (r).j = (a).j * LEN;	\
					    (r).k = (a).k * LEN;	\
					}

#define	VECTOR2_SCALE(r,a,s)		( (r).i = (a).i * (s), \
					  (r).j = (a).j * (s) )
#define	VECTOR3_SCALE(r,a,s)		( (r).i = (a).i * (s), \
					  (r).j = (a).j * (s), \
					  (r).k = (a).k * (s) )

#define	VECTOR2_SUB(r,a,b)		( (r).i = (a).i - (b).i, \
					  (r).j = (a).j - (b).j )
#define	VECTOR3_SUB(r,a,b)		( (r).i = (a).i - (b).i, \
					  (r).j = (a).j - (b).j, \
					  (r).k = (a).k - (b).k )

#define	VECTOR2_ZERO(v)			( (v).i = (v).j = 0 )
#define	VECTOR3_ZERO(v)			( (v).i = (v).j = (v).k = 0 )


/*
 *  4x4 matrix macros and routines.
 *  The following handle matrices of doubles.
 */

#define MX4D_COPY(d,s)		(double *) WF_MEM_COPY( (d), (s),	\
						sizeof 	(MATRIX_4D) )

extern	double	*MX4D_identity( 	/* Assign identity matrix	*/
		MATRIX_4D 
		);

extern	BOOLEAN	MX4D_invert( 		/* Invert a matrix		*/
		MATRIX_4D,
		MATRIX_4D 
		);

extern	double	*MX4D_multiply(	 	/* Multiply matrices		*/
		MATRIX_4D,
		MATRIX_4D,
		MATRIX_4D 
		);

extern	double	*MX4D_multiply_POINT_3D(/* Multiply  matrix and POINT_3D*/
		POINT_3D*,
		MATRIX_4D,
		POINT_3D*
		);

extern	void	MX4D_print(		/* Print a matrix to stderr	*/
		MATRIX_4D
		);

extern	double	*MX4D_rotate( 		/* Build a rotation matrix	*/
		MATRIX_4D,
		int,
		double 
		);	

extern	double	*MX4D_scale( 		/* Build a scalining matrix	*/
		MATRIX_4D,
		double,
		double,
		double 
		);

extern	double	*MX4D_translate( 	/* Build a translation matrix	*/
		MATRIX_4D,
		double,
		double,
		double 
		);
extern	double	*MX4D_transpose( 	/* Transpose a matrix		*/
		MATRIX_4D 
		);

extern	double	*MX4D_zero( 		/* Initialise a matrix to zeros	*/
		MATRIX_4D 
		);


/*
 *  4x4 matrix macros and routines.
 *  The following handle matrices of floats.
 */

#define MX4F_COPY(f,s)		(float *)  WF_MEM_COPY( (f), (s),	\
						sizeof (MATRIX_4F) )

extern	float	*MX4F_identity( 	/* Assign identity matrix	*/
		MATRIX_4F 
		);

extern	BOOLEAN	MX4F_invert( 		/* Invert a matrix		*/
		MATRIX_4F,
		MATRIX_4F 
		);

extern	BOOLEAN	MX4F_invert_lu_decompose(
					/* Alternative invert alg.	*/
		MATRIX_4F,
		MATRIX_4F 
		);

extern	float	*MX4F_multiply( 	/* Multiply matrices		*/
		MATRIX_4F,
		MATRIX_4F,
		MATRIX_4F 
		);

extern	float	*MX4F_multiply_POINT_3F(/* Multiply  matrix and POINT_3F*/
		POINT_3F*,
		MATRIX_4F,
		POINT_3F*
		);

extern	void	MX4F_print(		/* Print a matrix to stderr	*/
		MATRIX_4F
		);

extern	float	*MX4F_rotate( 		/* Build a rotation matrix	*/
		MATRIX_4F,
		int,
		float 
		);
extern	float	*MX4F_scale( 		/* Build a scalining matrix	*/
		MATRIX_4F,
		float,
		float,
		float 
		);								
extern	float	*MX4F_translate( 	/* Build a translation matrix	*/
		MATRIX_4F,
		float,
		float,
		float 
		);	
				
extern	float	*MX4F_transpose(	 /* Transpose a matrix		*/
		MATRIX_4F 
		);
extern	float	*MX4F_zero( 		/* Initialise a matrix to zeros	*/
		MATRIX_4F
		);


/*
 *  Arbitrary sized (NxM) matrix macros and routines.
 *  The following handle matrices of doubles.
 */

#define	MXND_COPY(n,m,dst,src)	(double *) WF_MEM_COPY( (dst), (src),	\
					sizeof( double ) * (n) * (m) )

extern	BOOLEAN	MXND_lu_back_substitute( /* Back substitue to solve mtrx*/
		int,
		int, 
		double*,
		int*,
		double*,
		double* 
		); 

extern	BOOLEAN	MXND_lu_decompose(	 /* Decompose mx via lu decompos*/
		int,
		int,
		double*,
		double*,
		double*,
		int*, 
		double*,
		BOOLEAN*,
		BOOLEAN* 
		);

extern	BOOLEAN	MXND_solve_linear( 	/* Solve a linear system of eqns*/
		int,
		int,
		double*,
		double*,
		double*,
		BOOLEAN*,
		BOOLEAN* 
		);


/*
 *  Arbitrary sized (NxM) matrix macros and routines.
 *  The following handle matrices of doubles.
 */

#define	MXNF_COPY(n,m,dst,src)	(float *) WF_MEM_COPY( (dst), (src),	\
					sizeof( float ) * (n) * (m) )

extern	BOOLEAN	MXNF_lu_back_substitute( /* Back substitue to solve mtrx*/
		int,
		int, 
		float*,
		int*,
		float*,
		float* 
		); 

extern	BOOLEAN	MXNF_lu_decompose(	 /* Decompose mx via lu decompos*/
		int,
		int,
		float*,
		float*,
		float*,
		int*, 
		float*,
		BOOLEAN*,
		BOOLEAN* 
		);

extern	BOOLEAN	MXNF_solve_linear( 	/* Solve a linear system of eqns*/
		int,
		int,
		float*,
		float*,
		float*,
		BOOLEAN*,
		BOOLEAN* 
		);


/*
 *  2D point macros and routines
 */

extern  BOOLEAN POINT_2D_is_colinear( 	/* Test 3 POINT_2D's for colin. */
		POINT_2D*,
		POINT_2D*,
		POINT_2D*
		);

extern  BOOLEAN POINT_2F_is_colinear( 	/* Test 3 POINT_2F's for colin. */
		POINT_2F*,
		POINT_2F*,
		POINT_2F*
		);

#ifdef  __cplusplus
}
#endif  /* __cplusplus */

#endif	/* WF_VMATH_H */
/*
 * ************************************************************************
 * ******************		wf_fmt.h	    ***********************
 * ************************************************************************
 *
 *  Copyright 1991  Wavefront Technologies, Inc.
 *		    530 E. Montecito St
 *		    Santa Barbara CA 93103
 *
 *		    All Rights Reserved
 *
 *  WRITTEN BY: 
 *	Norman Crafts, March 1991
 *
 *  PURPOSE AND CONTENTS:
 *	This module contains the information required for manipulating
 *	FMT configuration table.  
 */

#ifndef WF_FMT
#define WF_FMT



#ifdef	XPG3_EXISTS
#ifdef	__unix
#else	/* __unix */
#define	nl_catd			POINTER
#endif	/* __unix */
#else	/* XPG3_EXISTS */
#define nl_catd			POINTER
#endif	/* XPG3_EXISTS */

#ifdef	__cplusplus
extern	"C" {			/* Stops C++ compiler renaming functions*/
#endif	/* __cplusplus */

#define FMT_C_SUCCESS		0
#define FMT_C_FAILURE	       -1

typedef struct {
    char		       *name;
    int				width;
    int				height;
    float			ratio;
} FMT_ASPECT_INFO;

typedef struct {
    char		       *type;	   /* name of the storage type */
    int				chan;	   /* default number of channels */
} FMT_CHAN_TYPE;


/*
 *  Function prototypes for the FMT library...
 */

extern IMF_EXPORT int		FMT_init(		/* Init FMT	*/
#ifdef					__STDC__
					nl_catd msg_catalog
#endif					/* __STDC__ */
					);

extern IMF_EXPORT FMT_ASPECT_INFO *FMT_find(		/* Get type/info*/
#ifdef					__STDC__
					char *name
#endif					/* __STDC__ */
					);

extern IMF_EXPORT FMT_ASPECT_INFO *FMT_info(		/* Get type info*/
#ifdef					__STDC__
					void
#endif					/* __STDC__ */
					);

extern IMF_EXPORT int		FMT_def_dominance(	/* Get dflt dom	*/
#ifdef					__STDC__
					void
#endif					/* __STDC__ */
					);

extern IMF_EXPORT int		FMT_def_frame_rate(	/* Get dflt fps	*/
#ifdef					__STDC__
					void
#endif					/* __STDC__ */
					);

extern IMF_EXPORT float		FMT_def_gamma(		/* Get dflt gmma*/
#ifdef					__STDC__
					void
#endif					/* __STDC__ */
					);

extern IMF_EXPORT int		FMT_def_gamma_tab_res(	/* Get dflt size*/
#ifdef					__STDC__
					void
#endif					/* __STDC__ */
					);

extern IMF_EXPORT float		FMT_def_input_gamma(	/* Get dflt gmma*/
#ifdef					__STDC__
					void
#endif					/* __STDC__ */
					);

extern IMF_EXPORT char		*FMT_def_aspect(	/* Get dflt aspt*/
#ifdef					__STDC__
					void
#endif					/* __STDC__ */
					);

extern IMF_EXPORT char		*FMT_def_image_data(	/* Get dflt type*/
#ifdef					__STDC__
					void
#endif					/* __STDC__ */
					);

extern IMF_EXPORT int		FMT_exit(		/* Terminate FMT*/
#ifdef					__STDC__
					void
#endif					/* __STDC__ */
					);
#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif /* WF_FMT */
/*
 * ************************************************************************
 * ******************         wf_color.h            ***********************
 * ************************************************************************
 *
 *  Copyright 1987  Wavefront Technologies, Inc., All Rights Reserved
 *                  530 East Montecito Street
 *                  Santa Barbara, CA   93103
 *
 *  created by: Roy Hall        date:
 *
 *  PURPOSE AND CONTENTS:
 *      standard data types for manipulating color information
 */
#ifndef WF_CLR
#define WF_CLR

#ifdef  __cplusplus
extern "C" {			/* Stops C++ compiler renaming functions*/
#endif  /* __cplusplus */

/* data types for color manipulation
 */
typedef struct {
    U_CHAR          r,g,b;
} COLOR_RGB_3C;

typedef struct {
    U_CHAR          r,g,b,a;
} COLOR_RGB_4C;

typedef struct {
    float           r,g,b;
} COLOR_RGB_3F;

typedef struct {
    float           r,g,b,a;
} COLOR_RGB_4F;


/*
 *  Colour data type with alpha and zed.  There are several flavours of
 *  Z which trade off space and precision.  The unsigned long type should
 *  be used when the originating data was U_LONG and we wish to maintain
 *  the 16 digits of precision (floats are only accurate to 6 digits)
 *  without the space penalty of doubles.
 */

typedef struct {
    float           r,g,b,a,z;
} COLOR_RGB_5F;

typedef struct {
    float           r,g,b,a;
    double	    z;
} COLOR_RGB_5FD;

typedef struct {
    float           r,g,b,a;
    U_LONG	    z;
} COLOR_RGB_5FU;

typedef struct {
    U_CHAR          x,y,z;
} COLOR_XYZ_3C;

typedef struct {
    U_CHAR          x,y,z,a;
} COLOR_XYZ_4C;

typedef struct {
    float           x,y,z;
} COLOR_XYZ_3F;

typedef struct {
    float           x,y,z,a;
} COLOR_XYZ_4F;

typedef union {
    float           all[9];
    float           elem[3][3];
} COLOR_TRANS;

#define COLOR_SAMP          float
#define COLOR_SAMP_MAT      float
#define MAX_SPECTRAL_SAMP   1000

/* Declaration of utility routines in the $WF_DIR/lib/libutl library
 *  > RFL routine set
 */
int             UTL_valid_file();
int             CLR_init();
int             CLR_close();
int             CLR_status();
int             CLR_spectral_read();
int             CLR_spectral_samp();
int             CLR_spectral_xyz();
int             CLR_CIEXYZ();
int             CLR_samp_spectral();
int             CLR_xyz_samp();
COLOR_SAMP_MAT  *CLR_samp_xyz_mat();
int             CLR_rgb_samp();
COLOR_SAMP_MAT  *CLR_samp_rgb_mat();
COLOR_TRANS     *CLR_xyz_rgb_mat();
COLOR_TRANS     *CLR_rgb_xyz_mat();
COLOR_TRANS     *CLR_rgb_rgb_mat();
int             CLR_clip_rgb();
int             CLR_xyz_lab();
int             CLR_xyz_luv();
COLOR_TRANS     *CLR_xyz_yiq_mat();
COLOR_TRANS     *CLR_yiq_xyz_mat();
COLOR_TRANS     *CLR_encode_mat();
COLOR_TRANS     *CLR_decode_mat();
int             CLR_trans_inv();

float           *CLR_gamma_btof();
short           *CLR_gamma_btos();
U_CHAR          *CLR_gamma_tob();
float           *CLR_gamma_tof();
int             *CLR_gamma_done();


#ifdef  __cplusplus
}
#endif  /* __cplusplus */

#endif	/* WF_CLR */

/*
 * ************************************************************************
 * ******************           wf_fb.h             ***********************
 * ************************************************************************
 *
 *  Copyright 1991  Wavefront Technologies, Inc., All Rights Reserved
 *                  530 East Montecito St.
 *                  Santa Barbara, CA 93103
 *
 *  WRITTEN BY: 
 *      Norman Crafts, April 1991
 *
 *  PURPOSE AND CONTENTS:
 *      Structures that make it possible to deal with frame buffer
 *      devices.
 */

#ifndef WF_FB
#define WF_FB

#ifdef	XPG3_EXISTS
#else	/* XPG3_EXISTS */
#define nl_catd			POINTER
#endif	/* XPG3_EXISTS */


#ifdef  __cplusplus
extern "C" {                    /* Stops C++ compiler renaming functions*/
#endif  /* __cplusplus */

typedef	struct	fb_info	FB_INFO;
struct	fb_info
{
    char            *type;          /* WF_BUFFER reference -- i.e. "rtech" */
    int             x_off;          /* lower left corner x offset */
    int             y_off;          /* lower left corner y offset */
    int             x_res;          /* x resolution for display */
    int             y_res;          /* y resolution for display */
    float           aspect;         /* display aspect ratio */
    COLOR_XYZ_3F    red;            /* red phosphor chromaticity */
    COLOR_XYZ_3F    green;          /* green phosphor chromaticity */
    COLOR_XYZ_3F    blue;           /* blue phosphor chromaticity */
    COLOR_XYZ_3F    white;          /* white phosphor chromaticity */
    float           gamma;          /* monitor gamma (used for r,g,b,olay) */
    int             bitplanes;      /* number of bitplanes */
};

#define FB_C_SUCCESS        0
#define FB_C_FAILURE        (-1)

/* declaration of the global frame buffer routines
 */
int                          FB_init(			/* Init fb systm*/
#ifdef				__STDC__
				nl_catd	msg_cat
#endif				/* __STDC__ */
				);

FB_INFO                     *FB_find(			/* Find fb by nm*/
#ifdef				__STDC__
				char *name
#endif				/* __STDC__ */
				);

FB_INFO                     *FB_info(			/* Find in seq	*/
#ifdef				__STDC__
				void
#endif				/* __STDC__ */
				);

char                        *FB_def_framebuffer(	/* Get dflt fb	*/
#ifdef				__STDC__
				void
#endif				/* __STDC__ */
				);

int                          FB_exit(			/* Terminate fb	*/
#ifdef				__STDC__
				void
#endif				/* __STDC__ */
				);

#ifdef  __cplusplus
}
#endif  /* __cplusplus */

#endif /* WF_FB */
/*
 * ************************************************************************
 * ******************		wf_imf.h		*******************
 * ************************************************************************
 *
 *  Copyright 1991  Wavefront Technologies, Inc.
 *		    530 E. Montecito St
 *		    Santa Barbara CA 93103
 *
 *		    All Rights Reserved
 *
 *  WRITTEN BY:
 *      Norman Crafts, March 1991
 *
 *  PURPOSE AND CONTENTS:
 *      This module contains the information required for manipulating
 *      image files.  The image file routine set supports a number
 *      of options for the storage of images in formats that are
 *      compatable with other systems and image recording software.
 */

#ifndef WF_IMF
#define WF_IMF



#ifdef	XPG3_EXISTS
#ifdef	__unix
#else	/* __unix */
#define nl_catd			POINTER
#endif	/* __unix */
#else	/* XPG3_EXISTS */
#define nl_catd			POINTER
#endif	/* XPG3_EXISTS */

#if	defined( SGI )

typedef	abilock_t	imfLock_t;

#elif	defined( _WIN32 )

typedef	HANDLE		imfLock_t;

#elif	defined( LINUX )

typedef	pthread_mutex_t	imfLock_t;

#else

typedef	void*		imfLock_t;

#endif


#ifdef	__cplusplus
extern "C" {			/* Stops C++ compiler renaming functions*/
#endif	/* __cplusplus */


/*
 *  I/O support types...
 *  The bitfields tell us if various aspects of a file can be read or
 *  written.  Sequential access implies starting at the image origin and
 *  advancing scanline-by-scanline.  Random access implies the ability to
 *  read or write scanlines in arbitrary order.  Lut refers to lookup
 *  tables stored with some image file types.  MP refers to multiple
 *  access, which, if set, allows multiple IMF_open's of the same image
 *  with more than one scanline read occuring at a given instant.
 */

#define IMF_C_LUT_READ    	 0x01		/* A readable lut?	*/
#define IMF_C_LUT_WRITE      	 0x02		/* A writable lut?	*/
#define IMF_C_READ       	 0x04		/* Sequent. read scans?	*/
#define IMF_C_WRITE      	 0x08		/* Sequent. write scans?*/
#define IMF_C_READ_RANDOM      	(0x10 | IMF_C_READ)
#define IMF_C_WRITE_RANDOM     	(0x20 | IMF_C_WRITE)
#define IMF_C_READ_MP		 0x40		/* Multi file opens ok?	*/


/*
 *  Multiframe/multitrack image file settings...
 *  These values are assigned to the "multiframe" field of the IMF_INFO
 *  structure.
 */

#define	IMF_C_MULTI_FRAME_NONE	0x00		/* No multiframe abilities */
#define	IMF_C_MULTI_FRAME_AUDIO	0x01		/* Is audio supported?	*/
#define	IMF_C_MULTI_FRAME_VIDEO	0x02		/* >1 img frame allowed?*/


/*
 *  Miscellaneous multiframe/multitrack image file defines.
 */

#define	IMF_C_FRAME_NONE		(-1)	/* Current frame number	*/

#define	IMF_C_PLAYBACK_FIT_HARDWARE	0	/* Hardware zoom for dsp*/
#define	IMF_C_PLAYBACK_FIT_MIXED	1	/* Sw if zoom down,hw=up*/
#define	IMF_C_PLAYBACK_FIT_SOFTWARE	2	/* Use hardware to displ*/

#define	IMF_C_PLAYBACK_LOOP_ONCE	0	/* Play sequence once	*/
#define	IMF_C_PLAYBACK_LOOP_REPEAT	1	/* Repeat in same dir'n	*/
#define	IMF_C_PLAYBACK_LOOP_SWING	2	/* Swing back and forth	*/


/*
 * Channel data types
 */
#define IMF_C_INTEGER    		1
#define IMF_C_FLOAT      		2
#define IMF_C_DOUBLE      		3

/*
 * Field dominance types
 */
#define IMF_C_FIELD_BOTH		0
#define IMF_C_FIELD_ODD			1
#define IMF_C_FIELD_EVEN		2


/*
 *  Image orientation types.
 *  IMF handles images in the following orientations:
 *	IMF_C_ORIENT_BOT_LEFT	: Scans are left->right, scan 0 at bottom
 *	IMF_C_ORIENT_TOP_LEFT	: Scans are left->right, scan 0 at top
 *  When querying a driver's orientation abilities (see IMF_info), the
 *  orientation flags are or'ed together as a bitmask.  When opening an
 *  image (see IMF_open), exactly one of the flags will be assigned.
 */

#define IMF_C_ORIENT_NONE		0x00	/* Scanline orientation	*/
#define IMF_C_ORIENT_BOT_LEFT		0x01	/* Scanline orientation	*/
#define IMF_C_ORIENT_TOP_LEFT		0x02	/* Scanline orientation	*/


/*
 * Image channel organization types
 */
#define IMF_C_PLANAR_CONTIGUOUS     	0
#define IMF_C_PLANAR_SEPARATE       	1

/*
 * Color response curve types
 */
#define IMF_C_CORRECTION_GAMMA      	1
#define IMF_C_CORRECTION_CRC		2

#define IMF_C_CHAN_RED			0x01
#define IMF_C_CHAN_GREEN		0x02
#define IMF_C_CHAN_BLUE			0x04
#define IMF_C_CHAN_MATTE		0x08
#define IMF_C_CHAN_LUMINANCE		0x10
#define IMF_C_CHAN_Z			0x20
#define IMF_C_CHAN_DEFAULT		0x40
#define IMF_C_CHAN_INDEX_LUT		0x80

/*
 * Bit fields describing image types
 */
#define IMF_C_IMAGE_ANY      		0xff
#define IMF_C_GENERIC      		0x01
#define IMF_C_SWATCH       		0x02
#define IMF_C_TEX_COLOR    		0x04
#define IMF_C_TEX_SCALAR   		0x08
#define IMF_C_TEX_BUMP     		0x10
#define IMF_C_INDEX_LUT			0x20

typedef struct
{
    float		 minimum;       /* channel minimum */
    float		 maximum;       /* channel maximum */
} IMF_NORMALIZE;

typedef struct
{
    int                  type;          /* channel data type */
    int                  bits;          /* channel precision */
} IMF_CHAN_TYPE;

typedef struct
{
    int                  count;         /* channel count */
    IMF_CHAN_TYPE        type;          /* channel precision */
} IMF_CHAN_GROUP;

typedef struct
{
    char                *name;          /* channel name */
    int			 count;		/* channel count */
    int			 elems;		/* channel elements */
    int			 type;		/* channel element storage type */
    int			 bits;		/* channel element data width in bits */
} IMF_CHANNEL_INFO;

typedef struct
{
    int                  usage;         /* color correction type */
    float		 gamma;		/* gamma storage value */
    IMF_CHANNEL_INFO     info;          /* color response curve info */
    POINTER             *response;	/* color response curve */
} IMF_COLOR_RESPONSE;


/*
 *  Structures for driver-specific capabilities...
 *  See the routines IMF_info_get_specific() and IMF_info_set_specific()
 *  for details.
 *
 *  Different image file types may have specific capabilities such as
 *  encoding channel order, bit packing methods, etc.  The routine
 *  IMF_info_get_specific() returns an array of type IMF_CAPABILITY for
 *  the chosen file type.  This information is also settable when creating
 *  a file using IMF_info_set_specific().
 */

#define	IMF_CAPABILITY_CODE_NONE	(-1)	/* Unassigned code numbr*/

#define	IMF_CAPABILITY_TYPE_LIST	0	/* List of value names	*/
#define	IMF_CAPABILITY_TYPE_NUMBER	1	/* Numeric param type	*/

#define	IMF_CAPABILITY_WHEN_ALWAYS \
	    ( IMF_CAPABILITY_WHEN_INPUT | IMF_CAPABILITY_WHEN_OUTPUT )
						/* Avail. never		*/
#define IMF_CAPABILITY_WHEN_DISABLED    0x04    /* Avail. according to  */
                                                /* the settings         */
#define	IMF_CAPABILITY_WHEN_INPUT	0x01	/* Avail. on input	*/
#define	IMF_CAPABILITY_WHEN_NEVER	0x00	/* Avail. never		*/
#define	IMF_CAPABILITY_WHEN_OUTPUT	0x02	/* Avail. on output	*/

typedef struct	imf_capability_entry	/* Entry in list		*/
{
    U_SHORT		ice_code;	/* Code for this entry		*/
    char		*ice_name;	/* Name of this list item	*/
    MSGCAT_DEFN		ice_name_msg;	/* Internationalised name	*/
} IMF_CAPABILITY_ENTRY;

typedef struct	imf_capability_list	/* Entry in list		*/
{
    int			icl_default;	/* Default value		*/
    int			icl_n_entries;	/* Number of entries in list	*/
    IMF_CAPABILITY_ENTRY
			*icl_entries;	/* Entries in list		*/
} IMF_CAPABILITY_LIST;

typedef struct	imf_capability_number	/* Numeric value		*/
{
    float		icn_default;	/* Default value		*/
    BOOLEAN		icn_minimum_dfnd;
    float		icn_minimum;	/* Minimum value		*/
    BOOLEAN		icn_maximum_dfnd;
    float		icn_maximum;	/* Maximum value		*/
    float		icn_increment;	/* Increment value		*/
} IMF_CAPABILITY_NUMBER;

typedef struct	imf_capability		/* Per-driver capabilities	*/
{
    U_SHORT		imc_code;	/* Numberic code for this entry	*/
    char		*imc_name;	/* Capability name		*/
    MSGCAT_DEFN		imc_name_msg;	/* Internationalised name	*/
    U_CHAR		imc_type;	/* Type of imc_value (float etc)*/
    POINTER		imc_value;	/* IMF_CAPABILITY_{NUMBER,LIST}	*/
    U_CHAR		imc_when_avail;	/* Valid on input/output?	*/
} IMF_CAPABILITY;

typedef struct	imf_cap_setting		/* Binds capability on file open*/
{
    U_SHORT		imcs_code;	/* From iml_code if a list entry*/
    union
    {
	float		imcs_number;	/* Parameter if a numeric type	*/
	int		imcs_list;	/* Parameter if a integer type	*/
    } imcs_value;
} IMF_CAP_SETTING;


/*
 *  Data structures for accessing lookup tables...
 *  See the routines IMF_lut_read(), IMF_open() and IMF_lut_alloc() for
 *  details.  There is no IMF_lut_write() because we need to pass the
 *  lut on file creation so it gets written to the header.  There IS a
 *  lut read routine because we don't always want to go to the expense
 *  of allocating it (we may simply be doing a file query).
 *
 *  The lut is a contiguous array of tuples.  For a 256-element lut of
 *  RGB data, imu_n_entries would be 256.  For a pre-defined pallete such
 *  as for the SGI 3000 (which has 4096 entries, 8 pre-defined values,
 *  5 of which are locked), imu_n_entries is 4096, and 5 entries would
 *  be tagged "locked" and (8-5) tagged as "used".
 */

#define	IMF_LUT_MODE_FREE	0	/* Unused			*/
#define	IMF_LUT_MODE_LOCKED	1	/* Entry is locked, but usable 	*/
#define	IMF_LUT_MODE_RESERVED	2	/* Entry is reserved, unusable	*/
#define	IMF_LUT_MODE_USED	3	/* Entry is in use, reassignable*/

typedef	struct		imf_lut_entry	/* Lookup table entry		*/
{
    U_SHORT		ile_red;	/* Red value of this entry	*/
    U_SHORT		ile_green;	/* Green value of this entry	*/
    U_SHORT		ile_blue;	/* Blue value of this entry	*/
    U_CHAR		ile_mode:7;	/* Reserved/used/unused etc	*/
    U_CHAR		ile_transparent:1;
					/* Is entry transaprent or not?	*/
} IMF_LUT_ENTRY;

typedef struct		imf_lut		/* IMF lookup table structure	*/
{
    IMF_LUT_ENTRY	*imu_lut;	/* Lookup table	(imu_entries)	*/
    int			imu_maximum;	/* Max value lut can represent	*/
    int			imu_n_entries;	/* Length of imu_lut array	*/
    float		imu_gamma;	/* Gamma in existing entries	*/
} IMF_LUT;


/*
 *  How to transfer data to the caller.  This prevents unnecessary data
 *  conversion and copying.
 */

typedef	enum	ImfImagePacking
{
    ImfImagePackingNone,	/* 16 bit monochrome: MMMM...	*/
    ImfImagePackingM16,		/* 16 bit monochrome: MMMM...	*/
    ImfImagePackingIndexM16,	/* Index and Mask 16 bits 	*/
    ImfImagePackingRGBA16,	/* Pack as 16 bit RGBARGBA...	*/
    ImfImagePackingRGBA16Z31,	/* 16 bit RGBA with 31 bit Z	*/
    ImfImagePackingRGBA8,	/* Pack as 8 bit RGBARGBA...	*/
    ImfImagePackingXBGR8,	/* Pack as 8 bit XBGRXBGR...	*/
    ImfImagePackingM8,		/* 8 bit monochrome: MMMM...	*/
    ImfImagePackingRGB8		/* Pack as 8 bit RGBRGB...	*/
} ImfImagePacking;


/*
 *  Result desired from pathname generator.
 */

typedef	enum	ImfPathName
{
    IMF_PATHNAME_FILENAME,	/* Produce a file name			*/
    IMF_PATHNAME_PATTERN,	/* Produce a pattern			*/
    IMF_PATHNAME_PRINTF,	/* Produce a printf format		*/
    IMF_PATHNAMES		/* Number of possible results		*/
} ImfPathName;


/*
 *  Image file field naming conventions.
 */

typedef	enum	ImfFieldType
{
    IMF_FIELD_TYPE_NONE,	/* No field naming convention		*/
    IMF_FIELD_TYPE_TAV,		/* Advanced Visualizer field naming	*/
    IMF_FIELD_TYPE_ALIAS_EVEN,	/* Alias even field name		*/
    IMF_FIELD_TYPE_ALIAS_ODD,	/* Alias odd field name			*/
    IMF_FIELD_TYPES
} ImfFieldType;


/*
 *  Components of the name syntax string.
 */

typedef	enum	ImfNSPType
{
    IMF_NSP_TYPE_END,		/* The end of the name syntax parts	*/
				/* array				*/
    IMF_NSP_TYPE_EXT,		/* The extension token of name syntax	*/
    IMF_NSP_TYPE_FIELD,		/* The field token of name syntax	*/
    IMF_NSP_TYPE_FRAME,		/* The frame token of name syntax	*/
    IMF_NSP_TYPE_NAME,		/* A name token of name syntax		*/
    IMF_NSP_TYPE_PUNCT,		/* A punctuation token of name syntax	*/
    IMF_NSP_TYPES		/* Number of different name syntax parts*/
} ImfNSPType;


typedef struct  imf_name_syntax_part
{
    ImfFieldType
		nsp_field_type;	/* Type of field naming to use		*/
    EPTR        nsp_finds;      /* Where in the pathname this token is  */
    				/* found                                */
    int         nsp_ns_len;     /* Length of token string in name syntax*/
    char        *nsp_ns_ptr;    /* Start of token in name syntax        */
    int         nsp_pn_len;     /* Length of pathname corresponding to  */
    				/* this token                           */
    char        *nsp_pn_ptr;    /* Start of pathname corresponding to   */
				/* this token                           */
    ImfNSPType  nsp_type;       /* Type of syntax token part this is    */
} IMF_NAME_SYNTAX_PART;


/*
 *  Type declarations for handler functions...
 *  NOTE: most of these functions take an IMF_OBJECT* as their first
 *  argument.  However, since the function pointers which use these
 *  typedefs are decared inside the IMF_OBJECT structure itself, we'll
 *  have to declare them as void*.
 */

typedef	struct	imf_object	IMF_OBJECT;

typedef void		(*IMF_capabilityProc)( IMF_CAPABILITY **,
			    int *, int *, int * );
typedef BOOLEAN		(*IMF_capsAvailUpdateProc)( IMF_CAP_SETTING **, 
			    BOOLEAN );
typedef	int		(*IMF_closeProc)( IMF_OBJECT * );
typedef	int		(*IMF_doneProc)( void );
typedef	int		(*IMF_directScanProc)( POINTER, int, void *, U_SHORT *,
			    float, ImfImagePacking );
typedef int		(*IMF_getFrameProc)( POINTER, int * );
typedef int		(*IMF_getRasterProc)( POINTER, int, int, U_CHAR *,
			    COLOR_RGB_4C *, ImfImagePacking );
typedef int		(*IMF_getRegionProc)( POINTER, int, int, int, int,
			    POINTER, int, ImfImagePacking );
typedef BOOLEAN         (*IMF_initDsoProc)( void **, char ** );
typedef	BOOLEAN		(*IMF_isFileProc)( char *, FILE * );
typedef int		(*IMF_lutReadProc)( POINTER, IMF_LUT ** );
typedef int		(*IMF_playbackBindProc)( POINTER, POINTER,
			    POINTER );
typedef int		(*IMF_playbackGotoProc)( POINTER, int );
typedef int		(*IMF_playbackParamsProc)( POINTER, int, int,
			    int, int, int, U_CHAR, U_CHAR, U_CHAR );
typedef	void		(*IMF_playbackPlayCallbackProc)( int );
typedef int		(*IMF_playbackPlayProc)( POINTER, int,
			    float, BOOLEAN, BOOLEAN,
			    IMF_playbackPlayCallbackProc );
typedef int		(*IMF_playbackStopProc)( POINTER, int * );
typedef	int		(*IMF_scanProc)( POINTER, int, POINTER ** );
typedef int		(*IMF_seekProc)( POINTER, int );
typedef int		(*IMF_setFrameProc)( POINTER, int );
typedef BOOLEAN		(*IMF_execMuxAudioProc)( POINTER, BOOLEAN * );
typedef int		(*IMF_privateUIPopdownProc)( void );
typedef int		(*IMF_privateUIPopupProc)( POINTER );


/*
 *  Information about an image or sub-image
 */

typedef struct
{
    int                  usage;         /* bit field describing image type */
    IMF_COLOR_RESPONSE   curve;         /* color correction */
    FMT_ASPECT_INFO      aspect;        /* image aspect information */
    WINDOW_I		 window;	/* image window bounds */
    WINDOW_I		 active;	/* active window bounds(non-NULL data)*/

    int                  chan_config;   /* channel planarity */
    int                  chan_orient;   /* channel orientation */

    char		*chan_format;	/* color format */
    int			 chan_count;	/* color count */
    int			 chan_type;	/* color storage type */
    int			 chan_bits;	/* color width in bits */

    char		*matte_format;	/* matte format */
    int			 matte_count;	/* matte channel count */
    int			 matte_type;	/* matte storage type */
    int			 matte_bits;	/* matte width in bits */

    char		*aux_format;	/* auxiliary channel format */
    int			 aux_count;	/* auxiliary channel count */
    int			 aux_type;	/* auxiliary channel storage type */
    int			 aux_bits;	/* auxiliary channel width in bits */

    char		*index_format;	/* color index format */
    int			 index_count;	/* color index count */
    int			 index_type;	/* color index storage type */
    int			 index_bits;	/* color index width in bits */
} IMF_IMAGE;

typedef struct imf_mux_imfo  
{
    double      imi_t_start;		
    double      imi_t_end;
    int         imi_g_start;
    int         imi_g_end;
    int         imi_l_end;
    POINTER	imi_buff_stereo_to_x;
    POINTER	imi_buff_x_to_fmt;
    POINTER     imi_gaf;
    POINTER     imi_imf;
    int	     	imi_n_channels;
} IMF_MUX_INFO;      

typedef	struct
{
    char		*key;		/* Name of the key		*/
    char		*handle;	/* Unix handle of the image file*/
    BOOLEAN		handle_complete;/* Is `handle' completely built?*/
					/* else uses imf__build_handle	*/
    char		*name;		/* Name of the image file	*/
    char		*ext;		/* Name of the extension	*/
    char		*desc;		/* Description			*/
    char		*program;	/* Program that created image	*/
    char		*machine;	/* Machine where image created	*/
    char		*user;		/* User that created image	*/
    char		*date;		/* Creation date		*/
    char		*time;		/* Time to create		*/
    char		*filter;	/* Filter function		*/
    char		*compress;	/* Compression function		*/
    IMF_CAP_SETTING    **settings;	/* Pointer array: type specific	*/
					/* capability settings		*/
    BOOLEAN		 lut_exists;	/* TRUE if a LUT exists		*/
    IMF_LUT		*write_lut;	/* Optional lut when writing	*/
    int			 job_num;	/* Job accounting info		*/
    int			 frame;		/* Frame number			*/
    int			 field;		/* Field rendered flag		*/
    U_LONG		 init_flag;	/* Has user called IMF_info_init*/

    COLOR_XYZ_3F	 red_pri;	/* Red chroma			*/
    COLOR_XYZ_3F	 green_pri;	/* Green chroma			*/
    COLOR_XYZ_3F	 blue_pri;	/* Blue chroma			*/
    COLOR_XYZ_3F	 white_pt;	/* White chroma			*/

    int			 count;		/* Image sub header count	*/
    IMF_IMAGE		*image;		/* Image sub header array	*/
    int			 track_num_frames; /* Num frames in cur trac	*/
    float		 track_frame_rate; /* Frames/second for track	*/
    int			 track_start_frame;/* Base frame # (cur track)	*/
    int			 num_audio_tracks; /* Track count in file	*/
    int			 num_video_tracks; /* Track count in file	*/
    BOOLEAN		 read_rgb_or_indexed;/* T = RGB, F = lut indices*/
    POINTER		 mux_audio_buff;/* audio ring buffer for mux	*/
    IMF_execMuxAudioProc mux_audio_cb;	/* audio callback		*/
    IMF_MUX_INFO	 *mux_audio_data;/* audio callback		*/
    double		 mux_bitrate;    /* mux bitrate for mux check */
    double		 video_bitrate;  /* video bitrate for mux check */
} IMF_INFO;


typedef struct
{
    char		*key;		/* search key */
    char		*ext;           /* file name extension */
    char		*name;          /* image format name */
    char	        *desc;          /* image format description */
    MSGCAT_DEFN		 desc_msg;	/* internationalised description */
    char		*handle_format; /* unix handle format string */
    char		*name_syntax;	/* syntax-format name string */
    int			 add_extension; /* add file extension to path? */
    int			 usage;         /* image classes supported */
    int			 orient;        /* scanline order */
    U_INT		 lut_bits;	/* Bit depth of lookup table	*/
    int			 chan_count;    /* Maximum color channels supported */
    U_INT		 chan_bits;     /* color precision capability */
    int			 matte_count;   /* Maximum matte channels supported */
    U_INT		 matte_bits;    /* matte precision capability */
    int			 z_count;       /* Maximum z channels supported */
    U_INT		 z_bits;        /* z precision capability */
    int                  act_window;    /* stores active window? */
    U_INT		 access;        /* read/write access */
    IMF_capabilityProc	 capability;	/* get type-specific capability data*/
    IMF_capsAvailUpdateProc
    			 caps_avail_update;
			 		/* Update caps availability */
    U_INT		 multiframe;	/* multitrack/multiframe abilities */
    BOOLEAN		 hard_link_duplicates;
					/*if TRUE, application can use	*/
					/* links if duplicate frames	*/
    BOOLEAN		 support_remote_access;
					/* if TRUE, driver handles rcp	*/
					/* based on name "lisa:/data/x"	*/
    IMF_isFileProc	is_file;	/* optional check filetype routine */
    IMF_privateUIPopdownProc		/* Popdown a IMF driver private UI*/
    			 private_ui_popdown;
    IMF_privateUIPopupProc		/* Popup a IMF driver private UI*/
    			 private_ui_popup;
} IMF_OBJECT_INFO;

struct		imf_object
{
    POINTER             *data;          /* vector of image file data ptrs*/
    IMF_INFO             info;          /* image file information	*/
    int			 access;	/* image access type		*/
    IMF_lutReadProc	 lut_read;	/* fetch lookup table from file	*/
    IMF_scanProc	 scan;		/* read-write a scan		*/
    IMF_closeProc        close;		/* close file file		*/
    IMF_playbackBindProc playback_bind;	/* bind multiframe img->Xwindow	*/
    IMF_playbackGotoProc playback_goto;	/* jump (scrub) to given frame	*/
    IMF_playbackPlayProc playback_play;	/* start multiframe playback	*/
    IMF_playbackParamsProc
			 playback_params;/*set multiframe playback parms*/
    IMF_playbackStopProc playback_stop;	/* stop multiframe playback	*/
    IMF_getFrameProc	 get_frame;	/* get cur frame(multiframe img)*/
    IMF_getRasterProc	 get_raster;	/* load image into raster	*/
    IMF_getRegionProc	 get_region;	/* load a region of an image	*/
    IMF_setFrameProc	 set_frame;	/* set cur frame(multiframe img)*/
    IMF_seekProc	 seek;		/* seek to a different scan	*/
    int			 last_scan_idx;	/* last scanline that was read	*/
    IMF_OBJECT_INFO	 *type_info;	/* info about this file type	*/
    IMF_directScanProc	 direct_scan;	/* read-write a scan direct to  */
    					/* cache			*/
    imfLock_t		 objectLock;	/* Multithreaded IMF object lock*/
};

typedef struct
{
    char                *key;           /* search key */
    char                *ext;           /* filename extension */
    char                *name;          /* image format name */
    char                *desc;          /* image format description */
    MSGCAT_DEFN		 desc_msg;	/* internationalised description */
    char		*handle_format; /* unix handle format string */
    char		*name_syntax;	/* syntax-format name string */
    int			 add_extension; /* add file extension to path? */
    int			 usage;         /* image classes supported */
    int			 orient;        /* scanline order */
    U_INT		 lut_bits;	/* Bit depth of lookup table	*/
    int			 chan_count;    /* Maximum color channels supported */
    U_INT		 chan_bits;     /* color precision capability */
    int			 matte_count;   /* Maximum matte channels supported */
    U_INT		 matte_bits;    /* matte precision capability */
    int			 z_count;       /* Maximum z channels supported */
    U_INT		 z_bits;        /* z precision capability */
    int                  act_window;    /* stores active window? */
    U_INT		 access;        /* read/write access */
    IMF_capabilityProc   capability;	/* get type-specific capability data*/
    IMF_capsAvailUpdateProc
    			 caps_avail_update;
			 		/* Update caps availability */
    int		       (*init)();	/* optional init routine */
    IMF_doneProc         done;		/* optional done routine */
    int		       (*r_open)();	/* file open for read routine */
    int		       (*w_open)();	/* file open for write routine */
    U_INT		 multiframe;	/* multitrack/multiframe abilities */
    BOOLEAN		 hard_link_duplicates;
					/*if TRUE, application can use	*/
					/* links if duplicate frames	*/
    BOOLEAN		 support_remote_access;
					/* if TRUE, driver handles rcp	*/
					/* based on name "lisa:/data/x"	*/
    IMF_isFileProc	is_file;	/* optional check filetype routine */
    IMF_privateUIPopdownProc		/* Popdown a IMF driver private UI*/
    			 private_ui_popdown;
    IMF_privateUIPopupProc		/* Popup a IMF driver private UI*/
    			 private_ui_popup;
} IMF_OBJECT_LIST;


/*
 *  Declare the image file open and close routines and image utility
 *  routines, arranged alphabetically:
 */

extern	IMF_EXPORT char	*imf__build_handle(
				char *path,
				char *handle,
				char *ext
				);

extern	IMF_EXPORT POINTER *imf__chan_alloc(
				IMF_IMAGE *image,
				int res,
				char *key
				);

extern	IMF_EXPORT int	imf__chan_free(
				POINTER *chan_data
				);

extern	IMF_EXPORT int	imf__init_ifd(
				IMF_OBJECT *imf
				);

extern  IMF_EXPORT int	IMF_direct_scan_read(	/* Read a scan from a file */
						/* to cache 	      	   */
				IMF_OBJECT	*object,
				int		scan,
				void		*cache,
				U_SHORT		*gamma,
				float		step,
				ImfImagePacking packing
				);

extern  IMF_EXPORT int	IMF_direct_scan_write(	/* Write a scan to a file  */
						/* cache 	      	   */
				IMF_OBJECT	*object,
				int		scan,
				void		*cache,
				U_SHORT		*gamma,
				float		step,
				ImfImagePacking packing
				);

//typedef	struct fb_info	FB_INFO;

extern	IMF_EXPORT IMF_OBJECT *IMF__display( /* Convert an image for display*/
				IMF_OBJECT *imf,
				int flags,
				FB_INFO *monitor,
				IMF_COLOR_RESPONSE *crc
				);

extern	IMF_EXPORT IMF_OBJECT *IMF__select( /* Select an image for display*/
				IMF_OBJECT *imf,
				int ind
				);

extern IMF_EXPORT IMF_CAP_SETTING	*IMF_cap_setting_alloc(
					/* Allocate a setting	*/
				int	num
				);

extern IMF_EXPORT BOOLEAN IMF_cap_setting_compare(/* Cpmpare some settings */
				IMF_CAP_SETTING  **settings1,
				IMF_CAP_SETTING  **settings2
				);

extern IMF_EXPORT IMF_CAP_SETTING	**IMF_cap_setting_copy(	
					/* Copy some settings	*/
				IMF_CAP_SETTING  **settings
				);


extern IMF_EXPORT BOOLEAN	IMF_capabilities_available(
				IMF_OBJECT_INFO *imf, 
				IMF_CAP_SETTING **settings,
				BOOLEAN
				);

#define IMF_cap_setting_free(c)	\
				( WF_FREE( c ), (IMF_CAP_SETTING *) NULL )

extern IMF_EXPORT POINTER *IMF_chan_alloc(	/* Allocate scanline buf*/
				IMF_IMAGE *image,
				int res,
				char *key,
				int *size
				);

extern IMF_EXPORT void	IMF_chan_free(		/* Free scanline buffer	*/
				POINTER *chan_data
				);

extern IMF_EXPORT int	IMF_close(		/* Close open image file*/
						/* manage multiframes	*/
				IMF_OBJECT *object
				);

extern IMF_EXPORT int 	IMF_close_do_it( 	/* Real close */
				IMF_OBJECT *object 
				);


extern IMF_EXPORT char	*IMF_def_base_name(	/* Get default base name*/
				void
				);

extern IMF_EXPORT char	*IMF_def_key(		/* Get default file type*/
				void
				);

extern IMF_EXPORT void	IMF_dso_init(		/* Pre-DSO initialisation*/
				char		*bin_dir
				);

extern IMF_EXPORT int	IMF_exit(		/* Terminate IMF	*/
				void
				);

extern IMF_EXPORT IMF_OBJECT_INFO *IMF_find(	/* Get info about a type*/
				char *name
				);

extern IMF_EXPORT float	*IMF_gamma_btof(	/* Degamma[256]->floats	*/
				float gamma
				);

extern IMF_EXPORT short	*IMF_gamma_btos(	/* Degamma[256]->shorts	*/
				float gamma
				);

extern IMF_EXPORT U_CHAR *IMF_gamma_tob(	/* Gamma[256]->bytes	*/
				float gamma
				);

extern IMF_EXPORT float	*IMF_gamma_tof(		/* Gamma[256]->floats	*/
				float gamma
				);

extern IMF_EXPORT int	IMF_gamma_done(		/* Free up gamma table	*/
				POINTER table
				);

extern IMF_EXPORT char	*IMF_get_file_type(	/* Give back the type	*/
				char		*filename,
				FILE		*fp,
				BOOLEAN		*all_checked );

extern IMF_EXPORT int	IMF_get_frame(	/* Frame# in a multiframe file	*/
				IMF_OBJECT	*object,
				int		*frame
				);

extern IMF_EXPORT int	IMF_get_raster(	/* Efficiently load frame into raster */
				IMF_OBJECT	*object,
				int		raster_width,
				int		raster_height,
				U_CHAR		*raster,
				COLOR_RGB_4C	*bkgd_clr,
				ImfImagePacking	packing );

extern IMF_EXPORT int	IMF_get_region(	/* Efficiently get region of image */
				IMF_OBJECT	**object,
				int		left,
				int		right,
				int		bottom,
				int		top,
				POINTER		raster,
				int		raster_stride_bytes,
				ImfImagePacking	packing );

extern IMF_EXPORT IMF_OBJECT_INFO *IMF_info(	/* Get type info in seq.*/
				void
				);

extern IMF_EXPORT IMF_INFO *IMF_info_alloc(/* Alloc & init IMF_INFO*/
				void
				);

extern IMF_EXPORT void	IMF_info_cleanup(	/* Cleanup (don't free)	*/
				IMF_INFO *info,
				BOOLEAN	 free_all_fields
				);

extern IMF_EXPORT IMF_INFO *IMF_info_free(	/* Cleanup & free	*/
				IMF_INFO *info,
				BOOLEAN	 free_all_fields
				);

extern IMF_EXPORT void	IMF_info_get_specific(	/* Get type-specific data*/
				char		*key,
				IMF_OBJECT_INFO	*type_info,
				IMF_CAPABILITY	**capabilities,
				int		*num_input,
				int		*num_output,
				int		*total
				);

extern IMF_EXPORT void	IMF_info_init(		/* Init an IMF_INFO	*/
				IMF_INFO *info
				);

extern	IMF_EXPORT void IMF_info_num_specific(	/* Get max # of specific*/
				int *max_input,
				int *max_output
				);

extern IMF_EXPORT int	IMF_init(		/* Initialise IMF system*/
				nl_catd		msg_catalog,
				IMF_initDsoProc	proc
				);

extern IMF_EXPORT char	*IMF_is_file_type(	/* Checks a file	*/
				char        *filename,
				FILE        *fp,
				char        *key,
				BOOLEAN	    *all_checked );

extern IMF_EXPORT IMF_LUT *IMF_lut_alloc(	/* Allocate a lookup table */
				int	entries
				);

extern IMF_EXPORT IMF_LUT *IMF_lut_copy(	/* Duplicate a lookup table */
				IMF_LUT *lut
				);

extern IMF_EXPORT IMF_LUT *IMF_lut_free(	/* Free a lookup table	*/
				IMF_LUT *lut
				);

extern IMF_EXPORT BOOLEAN IMF_lut_read(		/* Read lut from open image */
				IMF_OBJECT *object,
				IMF_LUT    **lut
				);

extern IMF_EXPORT IMF_LUT *IMF_lut_realloc(	/* Resizes a lookup table */
				IMF_LUT	*lut,
				int	entries
				);

extern IMF_EXPORT char	*IMF_name_from_key(	/* Get name given key	*/
				char *key
				);

extern IMF_EXPORT char	*IMF_name_syntax_name(	/* Get the "Name" symbol */
				void
				);

extern IMF_EXPORT BOOLEAN IMF_name_syntax_ends_ext( /* Ends in extension? */
				char *syntax
				);

extern IMF_EXPORT BOOLEAN IMF_name_syntax_has_ext(  /* Has an extension? */
				char *syntax
				);

extern IMF_EXPORT BOOLEAN IMF_name_syntax_has_frame(  /* Has a frame? */
				char *syntax
				);

extern IMF_EXPORT BOOLEAN IMF_name_syntax_verify(/* Verify a name-syntax str */
				char *string
				);

extern IMF_EXPORT IMF_OBJECT *IMF_open(		/* Open an image file	*/
				IMF_INFO *imf_info,
				char     *access
				);

extern IMF_EXPORT void	IMF_multiframe_add( 	/* Add an new object */
				IMF_OBJECT  *imf,
				IMF_INFO    *info
				);

extern IMF_EXPORT void 	IMF_multiframe_init(   /* Initialise multiframe */
				void 
				);

extern IMF_EXPORT IMF_OBJECT *IMF_multiframe_open(
					/* Get ibject if it's already */
					/* opened  */
				IMF_INFO *info
				);

extern IMF_EXPORT void  IMF_multiframe_set(   /* Set multiframe on */
				BOOLEAN	use,
				int	max_open
				);

extern IMF_EXPORT int	IMF_multiframe_set_closed(  /* Perform a false close */
				IMF_OBJECT *imf
				);

extern IMF_EXPORT BOOLEAN IMF_multiframe_used(   /* Is multiframe used */
				void 
				);

extern	IMF_EXPORT BOOLEAN IMF_parse_name_syntax(/* Parse a name syntax str  */
				char *syntax,
				IMF_NAME_SYNTAX_PART **parts,
				int n_parts

				);

extern	IMF_EXPORT BOOLEAN IMF_pathname_build(  /* build filename */
				char *syntax, char *root, char *ftype,
				char *ext, ImfPathName want, BOOLEAN for_write,
				int *frame, char result[], char short_name[],
				char **imfkey, IMF_OBJECT_INFO **imf_info
				);

extern	IMF_EXPORT BOOLEAN IMF_pathname_build_from_name_syntax(
					/* build filename */
				char *syntax, char *root, char *imfkey,
				char *ext, ImfPathName want, int *frame,
				char result[], char short_name[],
				IMF_OBJECT_INFO **imf_info, BOOLEAN for_write
				);

extern	IMF_EXPORT BOOLEAN IMF_pathname_build_no_name_syntax(
					/* build filename */
				char *root, char *ftype, ImfPathName want,
				BOOLEAN for_write, int *frame, char result[],
				char short_name[], char **imfkey,
				IMF_OBJECT_INFO **imf_info
				);

extern IMF_EXPORT int	IMF_playback_bind(	/* Bind playback to Xwin*/
				IMF_OBJECT *object,
				POINTER	   Xdisplay,	/* (Display *)	*/
				POINTER    Xwindow	/* (Window)	*/
				);

extern IMF_EXPORT int	IMF_playback_goto(	/* Goto frame in playbck*/
				IMF_OBJECT *object,
				int	frame
				);

extern IMF_EXPORT int	IMF_playback_params(	/* Set view parameters	*/
				IMF_OBJECT *object,
				int	width,		/* New view size*/
				int	height,		/* New view size*/
				int	xorigin,	/* Window origin*/
				int	yorigin,	/* Window origin*/
				int	fit_mode,	/* Fit method?	*/
				U_CHAR	bkgd_red,	/* Bkgd colour	*/
				U_CHAR	bkgd_green,	/* Bkgd colour	*/
				U_CHAR	bkgd_blue	/* Bkgd colour	*/
				);

extern IMF_EXPORT int	IMF_playback_play(	/* Start playing	*/
				IMF_OBJECT *object,
				int	loop_mode,	/* IMF_C_...LOOP*/
				float	speed,		/* FPS mltiplier*/
				BOOLEAN	show_all_frames,/* Skip/show all*/
				BOOLEAN	mute,		/* No audio	*/
				IMF_playbackPlayCallbackProc
					done_rtn	/* Optional rtn	*/
				);

extern IMF_EXPORT int	IMF_playback_stop(	/* Stop playing		*/
				IMF_OBJECT *object,
				int	   *cur_frame
				);

extern IMF_EXPORT int 	IMF_private_ui_popdown(	/* Popdown private UI */
				void
				);

extern IMF_EXPORT int 	IMF_private_ui_popup(	/* Popup private UI */
				IMF_OBJECT_INFO	*imf_info,
				POINTER		main_widget
				);

extern IMF_OBJECT	*IMF__quantize(		/* Init pix depth cnvrtr*/
				IMF_OBJECT *imf_obj,
				IMF_CHAN_TYPE *imf_chan_type[3]
				);

extern IMF_EXPORT int	IMF_response_curve_init(/* Initialise a curve	*/
				IMF_COLOR_RESPONSE *curve
				);

extern IMF_EXPORT int	IMF_response_curve_comp(/* Compose two curves	*/
				IMF_COLOR_RESPONSE *f,
				IMF_COLOR_RESPONSE *g,
				IMF_COLOR_RESPONSE *h
				);

extern IMF_EXPORT int	IMF_response_curve_done(/* Free a response curve*/
				IMF_COLOR_RESPONSE *curve
				);

extern IMF_EXPORT int	IMF_scan_read(		/* Read a scan from a file*/
				IMF_OBJECT	*object,
				int		scan,
				POINTER		**line_buff
				);

extern IMF_EXPORT int	IMF_scan_write(		/* Write a scan to a file*/
				IMF_OBJECT	*object,
				int		scan,
				POINTER		*line_buff
				);

extern IMF_EXPORT int	IMF_seek(		/* Seek or rewind file	*/
				IMF_OBJECT	**object,
				int		scan
				);

extern IMF_EXPORT BOOLEAN IMF_set_available(
				IMF_CAPABILITY *,
				BOOLEAN
				);

extern IMF_EXPORT int	IMF_set_frame(	/* Frame # in a multiframe file	*/
				IMF_OBJECT	*object,
				int		frame
				);

extern IMF_EXPORT int	TEX_is_filtered_texture(/* Test if a filtered tx*/
				char *name,
				int type
				);

extern	IMF_EXPORT BOOLEAN aware_licence_check(	/* Check Aware/MPEG lic	*/
				unsigned int	licence
				);
extern  IMF_EXPORT int	imf__free_obj( 
				IMF_OBJECT *imf 
				);


/*
 *  Protocol numbers for plug-ins.  This should only be bumped once per
 *  minor version of imflibs and only if any of the data structures are
 *  changed.  The version number defined in the plug-in is checked on
 *  startup against the current version number, and if they differ, the
 *  plug-in is rejected.
 */

#define	IMF_PROTOCOL_2_3	"2.3"	/* Compatible with imflibs_2.3+	*/
#define	IMF_PROTOCOL_3_0	"3.0"	/* Compatible with imflibs_3.0+	*/
#define	IMF_PROTOCOL_3_2	"3.2"	/* Compatible with imflibs_3.2+	*/

#define	IMF_PROTOCOL_CURRENT	IMF_PROTOCOL_3_2


/*
 *  Error codes returned by the image file routine set for boolean-style
 *  success/fail routines:
 */

#define IMF_C_SUCCESS            	0
#define IMF_C_FAILURE			(-1)


/*
 *  General error codes returned by the image file routine set and also
 *  assigned to the variable "imf__err".
 */

#define IMF_C_NORMAL			0	/* No errors		*/
#define IMF_C_BAD_FRAME			(-1)	/* Frame # out of range	*/
#define IMF_C_BAD_SCAN			(-2)	/* Scanline out of range*/
#define IMF_C_BAD_TRACK			(-3)	/* Bad data type found	*/
#define IMF_C_BAD_TYPE			(-4)	/* Bad data type found	*/
#define IMF_C_CANNOT_OPEN		(-5)	/* Could not open file	*/
#define IMF_C_COLOR_INDEX_ONLY		(-6)	/* Can only write index	*/
						/* image file.		*/
#define IMF_C_ENC_DEC_ERR		(-7)	/* Scan de/encode problm*/
#define IMF_C_INVLD_HDR			(-8)	/* File header corrupt	*/
#define	IMF_C_KEY_INVALID		(-9)	/* Invalid IMF key	*/
#define IMF_C_MEM_ALLOC			(-10)	/* Memory alloc error	*/
#define	IMF_C_NAME_SYNTAX_INVALID	(-11)	/* Invalid name syntax	*/
#define	IMF_C_NAME_SYNTAX_MISMATCH	(-12)	/* Mismatched name syntax*/
#define	IMF_C_NAME_SYNTAX_UNDEFINED	(-13)	/* Undefined name syntax*/
#define	IMF_C_NAME_TOO_LONG		(-14)	/* Name too long	*/
#define	IMF_C_NAME_UNDEFINED		(-15)	/* Name undefined	*/
#define IMF_C_NO_SUPPORT		(-16)	/* Type/capabty. not sup*/
#define	IMF_C_PROGRAMMER_ERROR		(-17)	/* Programming error	*/
#define IMF_C_READ_ERR			(-18)	/* File read error	*/
#define	IMF_C_SWATCHES_NOT_SUPPORTED	(-19)	/* Not supportd by ftype*/
#define IMF_C_WRITE_ERR			(-20)	/* File write error	*/

extern	IMF_EXPORT	int	imf__err;	/* Error code IMF_C_...	*/


/*
 *  Global variables for IMF plug-ins
 *	*program			: File creator
 *	*version			: IMF version
 *	*type				: Image type
 *	*imageKey			: Image key
 *	*imageExtension			: Image file extension
 *	*imageName			: Image type name
 *	*imageDescription		: Image description
 *	*imageFormatString		: Format string
 *	*imageNameSyntax		: Filename syntax
 *	imageAddExtension		: Add file extension?
 *	imageUsage			: Image usage
 *	imageOrientation		: Image orientation
 *	imageNumberOfLuts		: # of look-up tables
 *	imageBitsPerLut			: # bits per LUT
 *	imageBitsPerPaletteEntry	: # bits per pal entry
 *	imageNumberOfChannels		: # of color channels
 *	imageBitsPerChannel		: # bits per channels
 *	imageNumberOfMattes		: # of matte channels
 *	imageBitsPerMatte		: # of bits/matte chan
 *	imageNumberOfZChannels		: # of Z channels
 *	imageBitsPerZChannel		: # of bits per Z chan
 *	imageSupportsActiveWindow	: Active win supported?
 *	imageAccess			: Valid access methods
 *	imageMultiFrame			: Multiframe info
 *	imageHardLinkDuplicates		: Can link when dup gen
 *	imageSupportRemoteAccess	: Driver handles rcp?
 */
#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

extern	PLUGINEXPORT	char	*program;
extern	PLUGINEXPORT	char	*version;
extern	PLUGINEXPORT	char	*type;
extern	PLUGINEXPORT	char	*imageKey;
extern	PLUGINEXPORT	char	*imageExtension;
extern	PLUGINEXPORT	char	*imageName;
extern	PLUGINEXPORT	char	*imageDescription;
extern	PLUGINEXPORT	char	*imageFormatString;
extern	PLUGINEXPORT	char	*imageNameSyntax;
extern	PLUGINEXPORT	int	imageAddExtension;
extern	PLUGINEXPORT	int	imageUsage;
extern	PLUGINEXPORT	int	imageOrientation;
extern	PLUGINEXPORT	int	imageNumberOfLuts;
extern	PLUGINEXPORT	U_INT	imageBitsPerLut;
extern	PLUGINEXPORT	U_INT	imageBitsPerPaletteEntry;
extern	PLUGINEXPORT	int	imageNumberOfChannels;
extern	PLUGINEXPORT	U_INT	imageBitsPerChannel;
extern	PLUGINEXPORT	int	imageNumberOfMattes;
extern	PLUGINEXPORT	U_INT	imageBitsPerMatte;
extern	PLUGINEXPORT	int	imageNumberOfZChannels;
extern	PLUGINEXPORT	U_INT	imageBitsPerZChannel;
extern	PLUGINEXPORT	int	imageSupportsActiveWindow;
extern	PLUGINEXPORT	U_INT	imageAccess;
extern	PLUGINEXPORT	U_INT	imageMultiFrame;
extern	PLUGINEXPORT	BOOLEAN	imageHardLinkDuplicates;
extern	PLUGINEXPORT	BOOLEAN	imageSupportRemoteAccess;

#ifdef  __cplusplus
};
#endif /* __cplusplus */

/*
 *  Function prototypes for IMF plug-ins
 */

extern	PLUGINEXPORT	void	imageCapability(
				    IMF_CAPABILITY **capabilities,
				    int *num_input, int *num_output,
				    int *total );
extern	PLUGINEXPORT	BOOLEAN	imageCapsAvailUpdate(
				    IMF_CAP_SETTING **settings, BOOLEAN );
extern	PLUGINEXPORT	void	imageDone( void );
extern	PLUGINEXPORT	int	imageInit( void );
extern	PLUGINEXPORT	BOOLEAN imageIsFile( char *fn, FILE *fp );
extern	PLUGINEXPORT	int	imageReadOpen( IMF_OBJECT *imf );
extern	PLUGINEXPORT	int	imageWriteOpen( IMF_OBJECT *imf );
extern  PLUGINEXPORT	int	imagePrivateUIPopdown( void );
extern  PLUGINEXPORT	int	imagePrivateUIPopup( POINTER main_widget );


#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif /* WF_IMF */
/*
 *<
 * $Header: /tmp_mnt/remote/ute/usr/ute1/vobForIMF/rcs/include/wf_dst.h,v 1.4 1994/10/31 20:09:09 swbld Exp $
 *
 * ************************************************************************
 * *******************           wf_dst.h           ***********************
 * ************************************************************************
 *>
 *  Copyright 1987  Wavefront Technologies, Inc, All rights reserved.
 *                  530 E. Montecito St., Suite 106
 *                  Santa Barbara, CA 93103
 *  created by: jon pittman    date: jan 1987
 *<
 *  MODULE PURPOSE:
 *
 *      This is an include file containing all of the structure definitions
 *      and defines for the Wavefront data structures routines.
 *
 *  MODULE CONTENTS:
 *
 *      wf_dst.h               - include file
 *
 *  INCLUDE FILES:
 *
 *      n/a
 *
 *  LOADING AND LINKING:
 *
 *      This file should be included in all modules that use the Wavefront
 *      data structures routines.
 *
 *  HISTORY:
 *
 *  $Log: wf_dst.h,v $
 * Revision 1.4  1994/10/31  20:09:09  swbld
 * added C++ compatability
 *
 * Revision 1.3  1993/05/25  12:13:45  perry
 * Added prototypes for new functions needed by Kinemation.
 *
 * Revision 1.2  91/05/16  09:11:32  chrisk
 * Added two routine declarations to DST_symbol_ set
 * 
 * Revision 1.1  91/04/11  10:23:32  swsrc
 * Initial revision
 * 
 *>
 */

#ifndef WF_DST_H
#define WF_DST_H

#ifdef  __cplusplus
extern "C" {			/* Stops C++ compiler renaming functions*/
#endif  /* __cplusplus */

#define DST__NIL        0       /* end of list character                */

typedef char *DST__POINTER;

/* DST routine declarations */

DST__POINTER    DST_malloc();
DST__POINTER    DST_free();
DST__POINTER    DST_purge();

DST__POINTER    DST_q_init();
void            DST_q_insh();
void            DST_q_inst();
DST__POINTER    DST_q_remh();
DST__POINTER    DST_q_remt();
int             DST_q_traverse();
int             DST_q_delete();

char            *DST_ds_alloc();
void            DST_ds_free();
char            *DST_ds_copy();

DST__POINTER    DST_tree_init();
int             DST_tree_insert();
DST__POINTER    DST_tree_lookup();
DST__POINTER    DST_tree_remove();
int             DST_tree_empty();
int             DST_tree_count();
int             DST_tree_traverse();
int             DST_tree_pretrav();
int             DST_tree_delete();

void		DST_sort();

DST__POINTER    DST_sym_init();
DST__POINTER    DST_sym_exit();
int             DST_sym_create();
int             DST_sym_static();
int             DST_sym_function();
char            *DST_sym_lookup();
int             DST_sym_remove();
int             DST_sym_replace();
int             DST_sym_traverse();
int             DST_sym_float();
int             DST_sym_int();

/* dst_mem module declarations */
DST__POINTER    DST_mem_init();
char            *DST_mem_ptr(),
		*DST_mem_str_ptr();
void            DST_mem_free();

/* dst_llist module declarations */

typedef struct link {
	struct link     *next;
} DST__LINK;

typedef struct {
	DST__LINK       *first,
			*last,
			*current;
	int             length;
} DST__LLIST;

DST__LLIST     *DST_ll_new(),
	       *DST_ll_new_mem();
void            DST_ll_init(),
		DST_ll_free(),
		DST_ll_addo(),
		DST_ll_addt(),
		DST_ll_addh(),
		DST_ll_initl(),
	       *DST_ll_nextl(),
	       *DST_ll_getl(),
	       *DST_ll_setl();
DST__LINK      *DST_ll_rem(),
	       *DST_ll_remh(),
	       *DST_ll_remt(),
	       *DST_ll_first(),
	       *DST_ll_last();
int             DST_ll_length(),
		DST_ll_traverse();

char           *DST_array_new();
int             DST_array_add(),
		DST_array_free(),
		DST_array_size();

/* dst_dllist module declarations */

typedef struct dlink {
	struct dlink    *next,
			*prev;
} DST__DLINK;

typedef struct {
	DST__DLINK      *first,
			*last;
	int             length;
} DST__DLIST;

void *DST_ll_finddata(/* DST__LLIST *,int (*)(),char * */);
DST__DLINK *DST_dl_iterate(/* DST__DLIST      *,DST__DLINK * */);
DST__DLIST     *DST_dl_new();
DST__DLIST     *DST_dl_new_mem();
void            DST_dl_init();
void            DST_dl_free();
void            DST_dl_addt();
void            DST_dl_addh();
void            DST_dl_addo();
DST__DLINK     *DST_dl_rem();
DST__DLINK     *DST_dl_remh();
DST__DLINK     *DST_dl_remt();
DST__DLINK     *DST_dl_first();
DST__DLINK     *DST_dl_last();
DST__DLINK     *DST_dl_next();
DST__DLINK     *DST_dl_previous();
int             DST_dl_traverse();
int             DST_dl_length();

#define DST__SYM_INTEGER        1
#define DST__SYM_FLOAT          2
#define DST__SYM_STRING         3
#define DST__SYM_INT_PTR        4
#define DST__SYM_FLT_PTR        5
#define DST__SYM_STR_PTR        6
#define DST__SYM_STR_HNDL       7
#define DST__SYM_INT_FUNC       8
#define DST__SYM_FLT_FUNC       9
#define DST__SYM_STR_FUNC       10

typedef struct {
	short   type;
	char    *name,
		*format,
		*value;
} DST__SYMBOL;

char         *DST_symbol_init      (); /* returns handle */
void          DST_symbol_exit      ( /* handle */ );
int           DST_symbol_add       ( /* hndl, name, type, value, type, format, protected */ );
int           DST_symbol_create    ( /* hndl, sym, name, type, value, type, format, protected */ );
char         *DST_symbol_lookup    ( /* hndl, name, arg */ );
char         *DST_symbol_lookup_table ( /* hndl, count, name, arg */ );
int           DST_symbol_remove    ( /* hndl, name, override_protection */ );
int           DST_symbol_traverse  ( /* hndl, function(), arg */ );
int           DST_symbol_traverse_table ( /* hndl, count, function(), arg */ );
int           DST_symbol_change    ( /* hndl, value, type, format, override */ );
DST__SYMBOL  *DST_symbol_find      ( /* hndl, name, override_protection */ );
int           DST_symbol_transfer  ( /* from_hndl, to_hndl, override_protection */ );


#ifdef  __cplusplus
}
#endif  /* __cplusplus */

#endif
