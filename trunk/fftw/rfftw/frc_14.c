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
/* Generated on Sun Nov  7 20:43:58 EST 1999 */

#include <fftw-int.h>
#include <fftw.h>

/* Generated by: ./genfft -magic-alignment-check -magic-twiddle-load-all -magic-variables 4 -magic-loopi -real2hc 14 */

/*
 * This function contains 62 FP additions, 36 FP multiplications,
 * (or, 62 additions, 36 multiplications, 0 fused multiply/add),
 * 22 stack variables, and 28 memory accesses
 */
static const fftw_real K900968867 = FFTW_KONST(+0.900968867902419126236102319507445051165919162);
static const fftw_real K222520933 = FFTW_KONST(+0.222520933956314404288902564496794759466355569);
static const fftw_real K623489801 = FFTW_KONST(+0.623489801858733530525004884004239810632274731);
static const fftw_real K433883739 = FFTW_KONST(+0.433883739117558120475768332848358754609990728);
static const fftw_real K974927912 = FFTW_KONST(+0.974927912181823607018131682993931217232785801);
static const fftw_real K781831482 = FFTW_KONST(+0.781831482468029808708444526674057750232334519);

/*
 * Generator Id's : 
 * $Id: frc_14.c,v 1.5 2001-08-10 21:46:13 tom7 Exp $
 * $Id: frc_14.c,v 1.5 2001-08-10 21:46:13 tom7 Exp $
 * $Id: frc_14.c,v 1.5 2001-08-10 21:46:13 tom7 Exp $
 */

void fftw_real2hc_14(const fftw_real *input, fftw_real *real_output, fftw_real *imag_output, int istride, int real_ostride, int imag_ostride)
{
     fftw_real tmp3;
     fftw_real tmp37;
     fftw_real tmp6;
     fftw_real tmp31;
     fftw_real tmp23;
     fftw_real tmp28;
     fftw_real tmp20;
     fftw_real tmp29;
     fftw_real tmp13;
     fftw_real tmp34;
     fftw_real tmp9;
     fftw_real tmp32;
     fftw_real tmp16;
     fftw_real tmp35;
     fftw_real tmp1;
     fftw_real tmp2;
     ASSERT_ALIGNED_DOUBLE;
     tmp1 = input[0];
     tmp2 = input[7 * istride];
     tmp3 = tmp1 - tmp2;
     tmp37 = tmp1 + tmp2;
     {
	  fftw_real tmp4;
	  fftw_real tmp5;
	  fftw_real tmp21;
	  fftw_real tmp22;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp4 = input[4 * istride];
	  tmp5 = input[11 * istride];
	  tmp6 = tmp4 - tmp5;
	  tmp31 = tmp4 + tmp5;
	  tmp21 = input[12 * istride];
	  tmp22 = input[5 * istride];
	  tmp23 = tmp21 - tmp22;
	  tmp28 = tmp21 + tmp22;
     }
     {
	  fftw_real tmp18;
	  fftw_real tmp19;
	  fftw_real tmp11;
	  fftw_real tmp12;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp18 = input[2 * istride];
	  tmp19 = input[9 * istride];
	  tmp20 = tmp18 - tmp19;
	  tmp29 = tmp18 + tmp19;
	  tmp11 = input[6 * istride];
	  tmp12 = input[13 * istride];
	  tmp13 = tmp11 - tmp12;
	  tmp34 = tmp11 + tmp12;
     }
     {
	  fftw_real tmp7;
	  fftw_real tmp8;
	  fftw_real tmp14;
	  fftw_real tmp15;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp7 = input[10 * istride];
	  tmp8 = input[3 * istride];
	  tmp9 = tmp7 - tmp8;
	  tmp32 = tmp7 + tmp8;
	  tmp14 = input[8 * istride];
	  tmp15 = input[istride];
	  tmp16 = tmp14 - tmp15;
	  tmp35 = tmp14 + tmp15;
     }
     {
	  fftw_real tmp25;
	  fftw_real tmp27;
	  fftw_real tmp26;
	  fftw_real tmp10;
	  fftw_real tmp24;
	  fftw_real tmp17;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp25 = tmp23 - tmp20;
	  tmp27 = tmp16 - tmp13;
	  tmp26 = tmp9 - tmp6;
	  imag_output[imag_ostride] = (K781831482 * tmp25) + (K974927912 * tmp26) + (K433883739 * tmp27);
	  imag_output[5 * imag_ostride] = -((K974927912 * tmp25) - (K781831482 * tmp27) - (K433883739 * tmp26));
	  imag_output[3 * imag_ostride] = (K433883739 * tmp25) + (K974927912 * tmp27) - (K781831482 * tmp26);
	  tmp10 = tmp6 + tmp9;
	  tmp24 = tmp20 + tmp23;
	  tmp17 = tmp13 + tmp16;
	  real_output[3 * real_ostride] = tmp3 + (K623489801 * tmp10) - (K222520933 * tmp17) - (K900968867 * tmp24);
	  real_output[7 * real_ostride] = tmp3 + tmp24 + tmp10 + tmp17;
	  real_output[real_ostride] = tmp3 + (K623489801 * tmp24) - (K900968867 * tmp17) - (K222520933 * tmp10);
	  real_output[5 * real_ostride] = tmp3 + (K623489801 * tmp17) - (K900968867 * tmp10) - (K222520933 * tmp24);
     }
     {
	  fftw_real tmp30;
	  fftw_real tmp36;
	  fftw_real tmp33;
	  fftw_real tmp38;
	  fftw_real tmp40;
	  fftw_real tmp39;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp30 = tmp28 - tmp29;
	  tmp36 = tmp34 - tmp35;
	  tmp33 = tmp31 - tmp32;
	  imag_output[2 * imag_ostride] = (K974927912 * tmp30) + (K433883739 * tmp33) + (K781831482 * tmp36);
	  imag_output[6 * imag_ostride] = -((K781831482 * tmp30) - (K433883739 * tmp36) - (K974927912 * tmp33));
	  imag_output[4 * imag_ostride] = -((K433883739 * tmp30) + (K781831482 * tmp33) - (K974927912 * tmp36));
	  tmp38 = tmp29 + tmp28;
	  tmp40 = tmp31 + tmp32;
	  tmp39 = tmp34 + tmp35;
	  real_output[6 * real_ostride] = tmp37 + (K623489801 * tmp38) - (K900968867 * tmp39) - (K222520933 * tmp40);
	  real_output[2 * real_ostride] = tmp37 + (K623489801 * tmp39) - (K900968867 * tmp40) - (K222520933 * tmp38);
	  real_output[4 * real_ostride] = tmp37 + (K623489801 * tmp40) - (K222520933 * tmp39) - (K900968867 * tmp38);
	  real_output[0] = tmp37 + tmp38 + tmp40 + tmp39;
     }
}

fftw_codelet_desc fftw_real2hc_14_desc =
{
     "fftw_real2hc_14",
     (void (*)()) fftw_real2hc_14,
     14,
     FFTW_FORWARD,
     FFTW_REAL2HC,
     310,
     0,
     (const int *) 0,
};
