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
/* Generated on Sun Nov  7 20:44:27 EST 1999 */

#include <fftw-int.h>
#include <fftw.h>

/* Generated by: ./genfft -magic-alignment-check -magic-twiddle-load-all -magic-variables 4 -magic-loopi -hc2real 16 */

/*
 * This function contains 58 FP additions, 18 FP multiplications,
 * (or, 54 additions, 14 multiplications, 4 fused multiply/add),
 * 26 stack variables, and 32 memory accesses
 */
static const fftw_real K765366864 = FFTW_KONST(+0.765366864730179543456919968060797733522689125);
static const fftw_real K1_847759065 = FFTW_KONST(+1.847759065022573512256366378793576573644833252);
static const fftw_real K1_414213562 = FFTW_KONST(+1.414213562373095048801688724209698078569671875);
static const fftw_real K2_000000000 = FFTW_KONST(+2.000000000000000000000000000000000000000000000);

/*
 * Generator Id's : 
 * $Id: fcr_16.c,v 1.3 2001-08-10 21:46:13 tom7 Exp $
 * $Id: fcr_16.c,v 1.3 2001-08-10 21:46:13 tom7 Exp $
 * $Id: fcr_16.c,v 1.3 2001-08-10 21:46:13 tom7 Exp $
 */

void fftw_hc2real_16(const fftw_real *real_input, const fftw_real *imag_input, fftw_real *output, int real_istride, int imag_istride, int ostride)
{
     fftw_real tmp9;
     fftw_real tmp54;
     fftw_real tmp42;
     fftw_real tmp21;
     fftw_real tmp6;
     fftw_real tmp18;
     fftw_real tmp39;
     fftw_real tmp53;
     fftw_real tmp13;
     fftw_real tmp29;
     fftw_real tmp16;
     fftw_real tmp26;
     fftw_real tmp23;
     fftw_real tmp49;
     fftw_real tmp57;
     fftw_real tmp56;
     fftw_real tmp46;
     fftw_real tmp30;
     ASSERT_ALIGNED_DOUBLE;
     {
	  fftw_real tmp7;
	  fftw_real tmp8;
	  fftw_real tmp40;
	  fftw_real tmp19;
	  fftw_real tmp20;
	  fftw_real tmp41;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp7 = real_input[2 * real_istride];
	  tmp8 = real_input[6 * real_istride];
	  tmp40 = tmp7 - tmp8;
	  tmp19 = imag_input[2 * imag_istride];
	  tmp20 = imag_input[6 * imag_istride];
	  tmp41 = tmp20 + tmp19;
	  tmp9 = K2_000000000 * (tmp7 + tmp8);
	  tmp54 = K1_414213562 * (tmp40 + tmp41);
	  tmp42 = K1_414213562 * (tmp40 - tmp41);
	  tmp21 = K2_000000000 * (tmp19 - tmp20);
     }
     {
	  fftw_real tmp5;
	  fftw_real tmp38;
	  fftw_real tmp3;
	  fftw_real tmp36;
	  ASSERT_ALIGNED_DOUBLE;
	  {
	       fftw_real tmp4;
	       fftw_real tmp37;
	       fftw_real tmp1;
	       fftw_real tmp2;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp4 = real_input[4 * real_istride];
	       tmp5 = K2_000000000 * tmp4;
	       tmp37 = imag_input[4 * imag_istride];
	       tmp38 = K2_000000000 * tmp37;
	       tmp1 = real_input[0];
	       tmp2 = real_input[8 * real_istride];
	       tmp3 = tmp1 + tmp2;
	       tmp36 = tmp1 - tmp2;
	  }
	  tmp6 = tmp3 + tmp5;
	  tmp18 = tmp3 - tmp5;
	  tmp39 = tmp36 - tmp38;
	  tmp53 = tmp36 + tmp38;
     }
     {
	  fftw_real tmp44;
	  fftw_real tmp48;
	  fftw_real tmp47;
	  fftw_real tmp45;
	  ASSERT_ALIGNED_DOUBLE;
	  {
	       fftw_real tmp11;
	       fftw_real tmp12;
	       fftw_real tmp27;
	       fftw_real tmp28;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp11 = real_input[real_istride];
	       tmp12 = real_input[7 * real_istride];
	       tmp13 = tmp11 + tmp12;
	       tmp44 = tmp11 - tmp12;
	       tmp27 = imag_input[imag_istride];
	       tmp28 = imag_input[7 * imag_istride];
	       tmp29 = tmp27 - tmp28;
	       tmp48 = tmp27 + tmp28;
	  }
	  {
	       fftw_real tmp14;
	       fftw_real tmp15;
	       fftw_real tmp24;
	       fftw_real tmp25;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp14 = real_input[3 * real_istride];
	       tmp15 = real_input[5 * real_istride];
	       tmp16 = tmp14 + tmp15;
	       tmp47 = tmp15 - tmp14;
	       tmp24 = imag_input[3 * imag_istride];
	       tmp25 = imag_input[5 * imag_istride];
	       tmp26 = tmp24 - tmp25;
	       tmp45 = tmp25 + tmp24;
	  }
	  tmp23 = tmp13 - tmp16;
	  tmp49 = tmp47 + tmp48;
	  tmp57 = tmp48 - tmp47;
	  tmp56 = tmp44 + tmp45;
	  tmp46 = tmp44 - tmp45;
	  tmp30 = tmp26 + tmp29;
     }
     {
	  fftw_real tmp10;
	  fftw_real tmp17;
	  fftw_real tmp34;
	  fftw_real tmp35;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp10 = tmp6 + tmp9;
	  tmp17 = K2_000000000 * (tmp13 + tmp16);
	  output[8 * ostride] = tmp10 - tmp17;
	  output[0] = tmp10 + tmp17;
	  tmp34 = tmp6 - tmp9;
	  tmp35 = K2_000000000 * (tmp29 - tmp26);
	  output[4 * ostride] = tmp34 - tmp35;
	  output[12 * ostride] = tmp34 + tmp35;
     }
     {
	  fftw_real tmp22;
	  fftw_real tmp31;
	  fftw_real tmp32;
	  fftw_real tmp33;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp22 = tmp18 - tmp21;
	  tmp31 = K1_414213562 * (tmp23 - tmp30);
	  output[10 * ostride] = tmp22 - tmp31;
	  output[2 * ostride] = tmp22 + tmp31;
	  tmp32 = tmp18 + tmp21;
	  tmp33 = K1_414213562 * (tmp23 + tmp30);
	  output[6 * ostride] = tmp32 - tmp33;
	  output[14 * ostride] = tmp32 + tmp33;
     }
     {
	  fftw_real tmp43;
	  fftw_real tmp50;
	  fftw_real tmp51;
	  fftw_real tmp52;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp43 = tmp39 + tmp42;
	  tmp50 = (K1_847759065 * tmp46) - (K765366864 * tmp49);
	  output[9 * ostride] = tmp43 - tmp50;
	  output[ostride] = tmp43 + tmp50;
	  tmp51 = tmp39 - tmp42;
	  tmp52 = (K765366864 * tmp46) + (K1_847759065 * tmp49);
	  output[5 * ostride] = tmp51 - tmp52;
	  output[13 * ostride] = tmp51 + tmp52;
     }
     {
	  fftw_real tmp55;
	  fftw_real tmp58;
	  fftw_real tmp59;
	  fftw_real tmp60;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp55 = tmp53 - tmp54;
	  tmp58 = (K765366864 * tmp56) - (K1_847759065 * tmp57);
	  output[11 * ostride] = tmp55 - tmp58;
	  output[3 * ostride] = tmp55 + tmp58;
	  tmp59 = tmp53 + tmp54;
	  tmp60 = (K1_847759065 * tmp56) + (K765366864 * tmp57);
	  output[7 * ostride] = tmp59 - tmp60;
	  output[15 * ostride] = tmp59 + tmp60;
     }
}

fftw_codelet_desc fftw_hc2real_16_desc =
{
     "fftw_hc2real_16",
     (void (*)()) fftw_hc2real_16,
     16,
     FFTW_BACKWARD,
     FFTW_HC2REAL,
     367,
     0,
     (const int *) 0,
};
