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
/* Generated on Sun Nov  7 20:44:09 EST 1999 */

#include <fftw-int.h>
#include <fftw.h>

/* Generated by: ./genfft -magic-alignment-check -magic-twiddle-load-all -magic-variables 4 -magic-loopi -notwiddleinv 12 */

/*
 * This function contains 96 FP additions, 16 FP multiplications,
 * (or, 88 additions, 8 multiplications, 8 fused multiply/add),
 * 40 stack variables, and 48 memory accesses
 */
static const fftw_real K866025403 = FFTW_KONST(+0.866025403784438646763723170752936183471402627);
static const fftw_real K500000000 = FFTW_KONST(+0.500000000000000000000000000000000000000000000);

/*
 * Generator Id's : 
 * $Id: fni_12.c,v 1.1 2001-08-10 04:19:13 tom7 Exp $
 * $Id: fni_12.c,v 1.1 2001-08-10 04:19:13 tom7 Exp $
 * $Id: fni_12.c,v 1.1 2001-08-10 04:19:13 tom7 Exp $
 */

void fftwi_no_twiddle_12(const fftw_complex *input, fftw_complex *output, int istride, int ostride)
{
     fftw_real tmp5;
     fftw_real tmp35;
     fftw_real tmp57;
     fftw_real tmp27;
     fftw_real tmp58;
     fftw_real tmp36;
     fftw_real tmp10;
     fftw_real tmp38;
     fftw_real tmp60;
     fftw_real tmp32;
     fftw_real tmp61;
     fftw_real tmp39;
     fftw_real tmp16;
     fftw_real tmp82;
     fftw_real tmp42;
     fftw_real tmp47;
     fftw_real tmp76;
     fftw_real tmp83;
     fftw_real tmp21;
     fftw_real tmp85;
     fftw_real tmp49;
     fftw_real tmp54;
     fftw_real tmp77;
     fftw_real tmp86;
     ASSERT_ALIGNED_DOUBLE;
     {
	  fftw_real tmp1;
	  fftw_real tmp2;
	  fftw_real tmp3;
	  fftw_real tmp4;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp1 = c_re(input[0]);
	  tmp2 = c_re(input[4 * istride]);
	  tmp3 = c_re(input[8 * istride]);
	  tmp4 = tmp2 + tmp3;
	  tmp5 = tmp1 + tmp4;
	  tmp35 = tmp1 - (K500000000 * tmp4);
	  tmp57 = K866025403 * (tmp2 - tmp3);
     }
     {
	  fftw_real tmp23;
	  fftw_real tmp24;
	  fftw_real tmp25;
	  fftw_real tmp26;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp23 = c_im(input[0]);
	  tmp24 = c_im(input[4 * istride]);
	  tmp25 = c_im(input[8 * istride]);
	  tmp26 = tmp24 + tmp25;
	  tmp27 = tmp23 + tmp26;
	  tmp58 = tmp23 - (K500000000 * tmp26);
	  tmp36 = K866025403 * (tmp25 - tmp24);
     }
     {
	  fftw_real tmp6;
	  fftw_real tmp7;
	  fftw_real tmp8;
	  fftw_real tmp9;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp6 = c_re(input[6 * istride]);
	  tmp7 = c_re(input[10 * istride]);
	  tmp8 = c_re(input[2 * istride]);
	  tmp9 = tmp7 + tmp8;
	  tmp10 = tmp6 + tmp9;
	  tmp38 = tmp6 - (K500000000 * tmp9);
	  tmp60 = K866025403 * (tmp7 - tmp8);
     }
     {
	  fftw_real tmp28;
	  fftw_real tmp29;
	  fftw_real tmp30;
	  fftw_real tmp31;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp28 = c_im(input[6 * istride]);
	  tmp29 = c_im(input[10 * istride]);
	  tmp30 = c_im(input[2 * istride]);
	  tmp31 = tmp29 + tmp30;
	  tmp32 = tmp28 + tmp31;
	  tmp61 = tmp28 - (K500000000 * tmp31);
	  tmp39 = K866025403 * (tmp30 - tmp29);
     }
     {
	  fftw_real tmp12;
	  fftw_real tmp13;
	  fftw_real tmp14;
	  fftw_real tmp15;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp12 = c_re(input[3 * istride]);
	  tmp13 = c_re(input[7 * istride]);
	  tmp14 = c_re(input[11 * istride]);
	  tmp15 = tmp13 + tmp14;
	  tmp16 = tmp12 + tmp15;
	  tmp82 = tmp12 - (K500000000 * tmp15);
	  tmp42 = K866025403 * (tmp13 - tmp14);
     }
     {
	  fftw_real tmp43;
	  fftw_real tmp44;
	  fftw_real tmp45;
	  fftw_real tmp46;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp43 = c_im(input[3 * istride]);
	  tmp44 = c_im(input[7 * istride]);
	  tmp45 = c_im(input[11 * istride]);
	  tmp46 = tmp44 + tmp45;
	  tmp47 = tmp43 - (K500000000 * tmp46);
	  tmp76 = tmp43 + tmp46;
	  tmp83 = K866025403 * (tmp45 - tmp44);
     }
     {
	  fftw_real tmp17;
	  fftw_real tmp18;
	  fftw_real tmp19;
	  fftw_real tmp20;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp17 = c_re(input[9 * istride]);
	  tmp18 = c_re(input[istride]);
	  tmp19 = c_re(input[5 * istride]);
	  tmp20 = tmp18 + tmp19;
	  tmp21 = tmp17 + tmp20;
	  tmp85 = tmp17 - (K500000000 * tmp20);
	  tmp49 = K866025403 * (tmp18 - tmp19);
     }
     {
	  fftw_real tmp50;
	  fftw_real tmp51;
	  fftw_real tmp52;
	  fftw_real tmp53;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp50 = c_im(input[9 * istride]);
	  tmp51 = c_im(input[istride]);
	  tmp52 = c_im(input[5 * istride]);
	  tmp53 = tmp51 + tmp52;
	  tmp54 = tmp50 - (K500000000 * tmp53);
	  tmp77 = tmp50 + tmp53;
	  tmp86 = K866025403 * (tmp52 - tmp51);
     }
     {
	  fftw_real tmp11;
	  fftw_real tmp22;
	  fftw_real tmp33;
	  fftw_real tmp34;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp11 = tmp5 + tmp10;
	  tmp22 = tmp16 + tmp21;
	  c_re(output[6 * ostride]) = tmp11 - tmp22;
	  c_re(output[0]) = tmp11 + tmp22;
	  {
	       fftw_real tmp75;
	       fftw_real tmp78;
	       fftw_real tmp79;
	       fftw_real tmp80;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp75 = tmp5 - tmp10;
	       tmp78 = tmp76 - tmp77;
	       c_re(output[9 * ostride]) = tmp75 - tmp78;
	       c_re(output[3 * ostride]) = tmp75 + tmp78;
	       tmp79 = tmp27 + tmp32;
	       tmp80 = tmp76 + tmp77;
	       c_im(output[6 * ostride]) = tmp79 - tmp80;
	       c_im(output[0]) = tmp79 + tmp80;
	  }
	  tmp33 = tmp27 - tmp32;
	  tmp34 = tmp16 - tmp21;
	  c_im(output[3 * ostride]) = tmp33 - tmp34;
	  c_im(output[9 * ostride]) = tmp34 + tmp33;
	  {
	       fftw_real tmp67;
	       fftw_real tmp89;
	       fftw_real tmp88;
	       fftw_real tmp90;
	       fftw_real tmp70;
	       fftw_real tmp74;
	       fftw_real tmp73;
	       fftw_real tmp81;
	       ASSERT_ALIGNED_DOUBLE;
	       {
		    fftw_real tmp65;
		    fftw_real tmp66;
		    fftw_real tmp84;
		    fftw_real tmp87;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp65 = tmp35 - tmp36;
		    tmp66 = tmp38 - tmp39;
		    tmp67 = tmp65 - tmp66;
		    tmp89 = tmp65 + tmp66;
		    tmp84 = tmp82 - tmp83;
		    tmp87 = tmp85 - tmp86;
		    tmp88 = tmp84 - tmp87;
		    tmp90 = tmp84 + tmp87;
	       }
	       {
		    fftw_real tmp68;
		    fftw_real tmp69;
		    fftw_real tmp71;
		    fftw_real tmp72;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp68 = tmp47 - tmp42;
		    tmp69 = tmp54 - tmp49;
		    tmp70 = tmp68 - tmp69;
		    tmp74 = tmp68 + tmp69;
		    tmp71 = tmp58 - tmp57;
		    tmp72 = tmp61 - tmp60;
		    tmp73 = tmp71 + tmp72;
		    tmp81 = tmp71 - tmp72;
	       }
	       c_re(output[5 * ostride]) = tmp67 - tmp70;
	       c_re(output[11 * ostride]) = tmp67 + tmp70;
	       c_im(output[2 * ostride]) = tmp73 - tmp74;
	       c_im(output[8 * ostride]) = tmp73 + tmp74;
	       c_im(output[11 * ostride]) = tmp81 - tmp88;
	       c_im(output[5 * ostride]) = tmp81 + tmp88;
	       c_re(output[2 * ostride]) = tmp89 - tmp90;
	       c_re(output[8 * ostride]) = tmp89 + tmp90;
	  }
	  {
	       fftw_real tmp41;
	       fftw_real tmp95;
	       fftw_real tmp94;
	       fftw_real tmp96;
	       fftw_real tmp56;
	       fftw_real tmp64;
	       fftw_real tmp63;
	       fftw_real tmp91;
	       ASSERT_ALIGNED_DOUBLE;
	       {
		    fftw_real tmp37;
		    fftw_real tmp40;
		    fftw_real tmp92;
		    fftw_real tmp93;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp37 = tmp35 + tmp36;
		    tmp40 = tmp38 + tmp39;
		    tmp41 = tmp37 - tmp40;
		    tmp95 = tmp37 + tmp40;
		    tmp92 = tmp82 + tmp83;
		    tmp93 = tmp85 + tmp86;
		    tmp94 = tmp92 - tmp93;
		    tmp96 = tmp92 + tmp93;
	       }
	       {
		    fftw_real tmp48;
		    fftw_real tmp55;
		    fftw_real tmp59;
		    fftw_real tmp62;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp48 = tmp42 + tmp47;
		    tmp55 = tmp49 + tmp54;
		    tmp56 = tmp48 - tmp55;
		    tmp64 = tmp48 + tmp55;
		    tmp59 = tmp57 + tmp58;
		    tmp62 = tmp60 + tmp61;
		    tmp63 = tmp59 + tmp62;
		    tmp91 = tmp59 - tmp62;
	       }
	       c_re(output[ostride]) = tmp41 - tmp56;
	       c_re(output[7 * ostride]) = tmp41 + tmp56;
	       c_im(output[10 * ostride]) = tmp63 - tmp64;
	       c_im(output[4 * ostride]) = tmp63 + tmp64;
	       c_im(output[7 * ostride]) = tmp91 - tmp94;
	       c_im(output[ostride]) = tmp91 + tmp94;
	       c_re(output[10 * ostride]) = tmp95 - tmp96;
	       c_re(output[4 * ostride]) = tmp95 + tmp96;
	  }
     }
}

fftw_codelet_desc fftwi_no_twiddle_12_desc =
{
     "fftwi_no_twiddle_12",
     (void (*)()) fftwi_no_twiddle_12,
     12,
     FFTW_BACKWARD,
     FFTW_NOTW,
     276,
     0,
     (const int *) 0,
};
