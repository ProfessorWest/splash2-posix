/* $Header: /usr/people/sam/tiff/libtiff/RCS/t4.h,v 1.9 92/02/10 19:06:22 sam Exp $ */

/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992 Sam Leffler
 * Copyright (c) 1991, 1992 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

#ifndef _T4_
#define	_T4_
/*
 * CCITT T.4 1D Huffman runlength codes and
 * related definitions.  Given the small sizes
 * of these tables it might does not seem
 * worthwhile to make code & length 8 bits.
 */
typedef struct tableentry {
    unsigned short length;	/* bit length of g3 code */
    unsigned short code;	/* g3 code */
    short	runlen;		/* run length in bits */
} tableentry;

#define	EOL	0x001	/* EOL code value - 0000 0000 0000 1 */

/* status values returned instead of a run length */
#define	G3CODE_INVALID	-1
#define	G3CODE_INCOMP	-2
#define	G3CODE_EOL	-3
#define	G3CODE_EOF	-4

/*
 * Note that these tables are ordered such that the
 * index into the table is known to be either the
 * run length, or (run length / 64) + a fixed offset.
 *
 * NB: The G3CODE_INVALID entries are only used
 *     during state generation (see mkg3states.c).
 */
extern const tableentry TIFFFaxWhiteCodes[];
extern const tableentry TIFFFaxBlackCodes[];

#else
#if defined(__STDC__) || defined(__EXTENDED__) || USE_CONST
extern	const tableentry TIFFFaxWhiteCodes[];
extern	const tableentry TIFFFaxBlackCodes[];
#else
extern	tableentry TIFFFaxWhiteCodes[];
extern	tableentry TIFFFaxBlackCodes[];
#endif /* !__STDC__ */
#endif
