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
/* Generated on Sun Nov  7 20:44:08 EST 1999 */

#include <fftw-int.h>
#include <fftw.h>

/* Generated by: ./genfft -magic-alignment-check -magic-twiddle-load-all -magic-variables 4 -magic-loopi -notwiddleinv 11 */

/*
 * This function contains 140 FP additions, 100 FP multiplications,
 * (or, 140 additions, 100 multiplications, 0 fused multiply/add),
 * 30 stack variables, and 44 memory accesses
 */
static const fftw_real K959492973 = FFTW_KONST(+0.959492973614497389890368057066327699062454848);
static const fftw_real K654860733 = FFTW_KONST(+0.654860733945285064056925072466293553183791199);
static const fftw_real K142314838 = FFTW_KONST(+0.142314838273285140443792668616369668791051361);
static const fftw_real K415415013 = FFTW_KONST(+0.415415013001886425529274149229623203524004910);
static const fftw_real K841253532 = FFTW_KONST(+0.841253532831181168861811648919367717513292498);
static const fftw_real K540640817 = FFTW_KONST(+0.540640817455597582107635954318691695431770608);
static const fftw_real K909631995 = FFTW_KONST(+0.909631995354518371411715383079028460060241051);
static const fftw_real K989821441 = FFTW_KONST(+0.989821441880932732376092037776718787376519372);
static const fftw_real K755749574 = FFTW_KONST(+0.755749574354258283774035843972344420179717445);
static const fftw_real K281732556 = FFTW_KONST(+0.281732556841429697711417915346616899035777899);

/*
 * Generator Id's : 
 * $Id: fni_11.c,v 1.1 2001-08-10 04:19:13 tom7 Exp $
 * $Id: fni_11.c,v 1.1 2001-08-10 04:19:13 tom7 Exp $
 * $Id: fni_11.c,v 1.1 2001-08-10 04:19:13 tom7 Exp $
 */

void fftwi_no_twiddle_11(const fftw_complex *input, fftw_complex *output, int istride, int ostride)
{
     fftw_real tmp1;
     fftw_real tmp23;
     fftw_real tmp4;
     fftw_real tmp17;
     fftw_real tmp38;
     fftw_real tmp49;
     fftw_real tmp26;
     fftw_real tmp53;
     fftw_real tmp7;
     fftw_real tmp21;
     fftw_real tmp10;
     fftw_real tmp18;
     fftw_real tmp35;
     fftw_real tmp50;
     fftw_real tmp13;
     fftw_real tmp20;
     fftw_real tmp29;
     fftw_real tmp51;
     fftw_real tmp32;
     fftw_real tmp52;
     fftw_real tmp16;
     fftw_real tmp19;
     ASSERT_ALIGNED_DOUBLE;
     {
	  fftw_real tmp2;
	  fftw_real tmp3;
	  fftw_real tmp36;
	  fftw_real tmp37;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp1 = c_re(input[0]);
	  tmp23 = c_im(input[0]);
	  tmp2 = c_re(input[istride]);
	  tmp3 = c_re(input[10 * istride]);
	  tmp4 = tmp2 + tmp3;
	  tmp17 = tmp2 - tmp3;
	  tmp36 = c_im(input[istride]);
	  tmp37 = c_im(input[10 * istride]);
	  tmp38 = tmp36 + tmp37;
	  tmp49 = tmp37 - tmp36;
	  {
	       fftw_real tmp24;
	       fftw_real tmp25;
	       fftw_real tmp5;
	       fftw_real tmp6;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp24 = c_im(input[2 * istride]);
	       tmp25 = c_im(input[9 * istride]);
	       tmp26 = tmp24 + tmp25;
	       tmp53 = tmp25 - tmp24;
	       tmp5 = c_re(input[2 * istride]);
	       tmp6 = c_re(input[9 * istride]);
	       tmp7 = tmp5 + tmp6;
	       tmp21 = tmp5 - tmp6;
	  }
     }
     {
	  fftw_real tmp8;
	  fftw_real tmp9;
	  fftw_real tmp27;
	  fftw_real tmp28;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp8 = c_re(input[3 * istride]);
	  tmp9 = c_re(input[8 * istride]);
	  tmp10 = tmp8 + tmp9;
	  tmp18 = tmp8 - tmp9;
	  {
	       fftw_real tmp33;
	       fftw_real tmp34;
	       fftw_real tmp11;
	       fftw_real tmp12;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp33 = c_im(input[3 * istride]);
	       tmp34 = c_im(input[8 * istride]);
	       tmp35 = tmp33 + tmp34;
	       tmp50 = tmp34 - tmp33;
	       tmp11 = c_re(input[4 * istride]);
	       tmp12 = c_re(input[7 * istride]);
	       tmp13 = tmp11 + tmp12;
	       tmp20 = tmp11 - tmp12;
	  }
	  tmp27 = c_im(input[4 * istride]);
	  tmp28 = c_im(input[7 * istride]);
	  tmp29 = tmp27 + tmp28;
	  tmp51 = tmp28 - tmp27;
	  {
	       fftw_real tmp30;
	       fftw_real tmp31;
	       fftw_real tmp14;
	       fftw_real tmp15;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp30 = c_im(input[5 * istride]);
	       tmp31 = c_im(input[6 * istride]);
	       tmp32 = tmp30 + tmp31;
	       tmp52 = tmp31 - tmp30;
	       tmp14 = c_re(input[5 * istride]);
	       tmp15 = c_re(input[6 * istride]);
	       tmp16 = tmp14 + tmp15;
	       tmp19 = tmp14 - tmp15;
	  }
     }
     {
	  fftw_real tmp56;
	  fftw_real tmp55;
	  fftw_real tmp44;
	  fftw_real tmp45;
	  ASSERT_ALIGNED_DOUBLE;
	  c_re(output[0]) = tmp1 + tmp4 + tmp7 + tmp10 + tmp13 + tmp16;
	  {
	       fftw_real tmp62;
	       fftw_real tmp61;
	       fftw_real tmp58;
	       fftw_real tmp57;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp62 = (K281732556 * tmp49) + (K755749574 * tmp50) + (K989821441 * tmp52) - (K909631995 * tmp51) - (K540640817 * tmp53);
	       tmp61 = tmp1 + (K841253532 * tmp7) + (K415415013 * tmp13) - (K142314838 * tmp16) - (K654860733 * tmp10) - (K959492973 * tmp4);
	       c_re(output[6 * ostride]) = tmp61 - tmp62;
	       c_re(output[5 * ostride]) = tmp61 + tmp62;
	       tmp58 = (K540640817 * tmp49) + (K909631995 * tmp53) + (K989821441 * tmp50) + (K755749574 * tmp51) + (K281732556 * tmp52);
	       tmp57 = tmp1 + (K841253532 * tmp4) + (K415415013 * tmp7) - (K959492973 * tmp16) - (K654860733 * tmp13) - (K142314838 * tmp10);
	       c_re(output[10 * ostride]) = tmp57 - tmp58;
	       c_re(output[ostride]) = tmp57 + tmp58;
	  }
	  tmp56 = (K909631995 * tmp49) + (K755749574 * tmp53) - (K540640817 * tmp52) - (K989821441 * tmp51) - (K281732556 * tmp50);
	  tmp55 = tmp1 + (K415415013 * tmp4) + (K841253532 * tmp16) - (K142314838 * tmp13) - (K959492973 * tmp10) - (K654860733 * tmp7);
	  c_re(output[9 * ostride]) = tmp55 - tmp56;
	  c_re(output[2 * ostride]) = tmp55 + tmp56;
	  {
	       fftw_real tmp60;
	       fftw_real tmp59;
	       fftw_real tmp54;
	       fftw_real tmp48;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp60 = (K989821441 * tmp49) + (K540640817 * tmp51) + (K755749574 * tmp52) - (K909631995 * tmp50) - (K281732556 * tmp53);
	       tmp59 = tmp1 + (K415415013 * tmp10) + (K841253532 * tmp13) - (K654860733 * tmp16) - (K959492973 * tmp7) - (K142314838 * tmp4);
	       c_re(output[8 * ostride]) = tmp59 - tmp60;
	       c_re(output[3 * ostride]) = tmp59 + tmp60;
	       tmp54 = (K755749574 * tmp49) + (K540640817 * tmp50) + (K281732556 * tmp51) - (K909631995 * tmp52) - (K989821441 * tmp53);
	       tmp48 = tmp1 + (K841253532 * tmp10) + (K415415013 * tmp16) - (K959492973 * tmp13) - (K142314838 * tmp7) - (K654860733 * tmp4);
	       c_re(output[7 * ostride]) = tmp48 - tmp54;
	       c_re(output[4 * ostride]) = tmp48 + tmp54;
	  }
	  c_im(output[0]) = tmp23 + tmp38 + tmp26 + tmp35 + tmp29 + tmp32;
	  {
	       fftw_real tmp22;
	       fftw_real tmp39;
	       fftw_real tmp42;
	       fftw_real tmp43;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp22 = (K281732556 * tmp17) + (K755749574 * tmp18) + (K989821441 * tmp19) - (K909631995 * tmp20) - (K540640817 * tmp21);
	       tmp39 = tmp23 + (K841253532 * tmp26) + (K415415013 * tmp29) - (K142314838 * tmp32) - (K654860733 * tmp35) - (K959492973 * tmp38);
	       c_im(output[5 * ostride]) = tmp22 + tmp39;
	       c_im(output[6 * ostride]) = tmp39 - tmp22;
	       tmp42 = (K540640817 * tmp17) + (K909631995 * tmp21) + (K989821441 * tmp18) + (K755749574 * tmp20) + (K281732556 * tmp19);
	       tmp43 = tmp23 + (K841253532 * tmp38) + (K415415013 * tmp26) - (K959492973 * tmp32) - (K654860733 * tmp29) - (K142314838 * tmp35);
	       c_im(output[ostride]) = tmp42 + tmp43;
	       c_im(output[10 * ostride]) = tmp43 - tmp42;
	  }
	  tmp44 = (K909631995 * tmp17) + (K755749574 * tmp21) - (K540640817 * tmp19) - (K989821441 * tmp20) - (K281732556 * tmp18);
	  tmp45 = tmp23 + (K415415013 * tmp38) + (K841253532 * tmp32) - (K142314838 * tmp29) - (K959492973 * tmp35) - (K654860733 * tmp26);
	  c_im(output[2 * ostride]) = tmp44 + tmp45;
	  c_im(output[9 * ostride]) = tmp45 - tmp44;
	  {
	       fftw_real tmp40;
	       fftw_real tmp41;
	       fftw_real tmp46;
	       fftw_real tmp47;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp40 = (K989821441 * tmp17) + (K540640817 * tmp20) + (K755749574 * tmp19) - (K909631995 * tmp18) - (K281732556 * tmp21);
	       tmp41 = tmp23 + (K415415013 * tmp35) + (K841253532 * tmp29) - (K654860733 * tmp32) - (K959492973 * tmp26) - (K142314838 * tmp38);
	       c_im(output[3 * ostride]) = tmp40 + tmp41;
	       c_im(output[8 * ostride]) = tmp41 - tmp40;
	       tmp46 = (K755749574 * tmp17) + (K540640817 * tmp18) + (K281732556 * tmp20) - (K909631995 * tmp19) - (K989821441 * tmp21);
	       tmp47 = tmp23 + (K841253532 * tmp35) + (K415415013 * tmp32) - (K959492973 * tmp29) - (K142314838 * tmp26) - (K654860733 * tmp38);
	       c_im(output[4 * ostride]) = tmp46 + tmp47;
	       c_im(output[7 * ostride]) = tmp47 - tmp46;
	  }
     }
}

fftw_codelet_desc fftwi_no_twiddle_11_desc =
{
     "fftwi_no_twiddle_11",
     (void (*)()) fftwi_no_twiddle_11,
     11,
     FFTW_BACKWARD,
     FFTW_NOTW,
     254,
     0,
     (const int *) 0,
};
