/*
 * Copyright (c) 1997-1999 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* This file was automatically generated --- DO NOT EDIT */
/* Generated on Sun Nov  7 20:43:47 EST 1999 */

#include <fftw-int.h>
#include <fftw.h>

/* Generated by: ./genfft -magic-alignment-check -magic-twiddle-load-all -magic-variables 4 -magic-loopi -notwiddle 1 */

/*
 * This function contains 0 FP additions, 0 FP multiplications,
 * (or, 0 additions, 0 multiplications, 0 fused multiply/add),
 * 2 stack variables, and 4 memory accesses
 */

/*
 * Generator Id's : 
 * $Id: fn_1.c,v 1.3 2001-08-10 21:31:55 tom7 Exp $
 * $Id: fn_1.c,v 1.3 2001-08-10 21:31:55 tom7 Exp $
 * $Id: fn_1.c,v 1.3 2001-08-10 21:31:55 tom7 Exp $
 */

void fftw_no_twiddle_1(const fftw_complex *input, fftw_complex *output, int istride, int ostride)
{
     fftw_real tmp1;
     fftw_real tmp2;
     ASSERT_ALIGNED_DOUBLE;
     tmp1 = c_re(input[0]);
     c_re(output[0]) = tmp1;
     tmp2 = c_im(input[0]);
     c_im(output[0]) = tmp2;
}

fftw_codelet_desc fftw_no_twiddle_1_desc =
{
     "fftw_no_twiddle_1",
     (void (*)()) fftw_no_twiddle_1,
     1,
     FFTW_FORWARD,
     FFTW_NOTW,
     23,
     0,
     (const int *) 0,
};
