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
/* Generated on Sun Nov  7 20:44:46 EST 1999 */

#include <fftw-int.h>
#include <fftw.h>

/* Generated by: ./genfft -magic-alignment-check -magic-twiddle-load-all -magic-variables 4 -magic-loopi -hc2hc-forward 7 */

/*
 * This function contains 120 FP additions, 96 FP multiplications,
 * (or, 108 additions, 84 multiplications, 12 fused multiply/add),
 * 25 stack variables, and 56 memory accesses
 */
static const fftw_real K222520933 = FFTW_KONST(+0.222520933956314404288902564496794759466355569);
static const fftw_real K900968867 = FFTW_KONST(+0.900968867902419126236102319507445051165919162);
static const fftw_real K623489801 = FFTW_KONST(+0.623489801858733530525004884004239810632274731);
static const fftw_real K433883739 = FFTW_KONST(+0.433883739117558120475768332848358754609990728);
static const fftw_real K974927912 = FFTW_KONST(+0.974927912181823607018131682993931217232785801);
static const fftw_real K781831482 = FFTW_KONST(+0.781831482468029808708444526674057750232334519);

/*
 * Generator Id's : 
 * $Id: fhf_7.c,v 1.3 2001-08-10 21:46:13 tom7 Exp $
 * $Id: fhf_7.c,v 1.3 2001-08-10 21:46:13 tom7 Exp $
 * $Id: fhf_7.c,v 1.3 2001-08-10 21:46:13 tom7 Exp $
 */

void fftw_hc2hc_forward_7(fftw_real *A, const fftw_complex *W, int iostride, int m, int dist)
{
     int i;
     fftw_real *X;
     fftw_real *Y;
     X = A;
     Y = A + (7 * iostride);
     {
	  fftw_real tmp85;
	  fftw_real tmp84;
	  fftw_real tmp88;
	  fftw_real tmp78;
	  fftw_real tmp86;
	  fftw_real tmp81;
	  fftw_real tmp87;
	  fftw_real tmp82;
	  fftw_real tmp83;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp85 = X[0];
	  tmp82 = X[iostride];
	  tmp83 = X[6 * iostride];
	  tmp84 = tmp82 - tmp83;
	  tmp88 = tmp82 + tmp83;
	  {
	       fftw_real tmp76;
	       fftw_real tmp77;
	       fftw_real tmp79;
	       fftw_real tmp80;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp76 = X[2 * iostride];
	       tmp77 = X[5 * iostride];
	       tmp78 = tmp76 - tmp77;
	       tmp86 = tmp76 + tmp77;
	       tmp79 = X[3 * iostride];
	       tmp80 = X[4 * iostride];
	       tmp81 = tmp79 - tmp80;
	       tmp87 = tmp79 + tmp80;
	  }
	  Y[-3 * iostride] = (K781831482 * tmp78) - (K974927912 * tmp81) - (K433883739 * tmp84);
	  Y[-iostride] = -((K781831482 * tmp84) + (K974927912 * tmp78) + (K433883739 * tmp81));
	  Y[-2 * iostride] = (K433883739 * tmp78) + (K781831482 * tmp81) - (K974927912 * tmp84);
	  X[2 * iostride] = tmp85 + (K623489801 * tmp87) - (K900968867 * tmp86) - (K222520933 * tmp88);
	  X[iostride] = tmp85 + (K623489801 * tmp88) - (K900968867 * tmp87) - (K222520933 * tmp86);
	  X[3 * iostride] = tmp85 + (K623489801 * tmp86) - (K222520933 * tmp87) - (K900968867 * tmp88);
	  X[0] = tmp85 + tmp88 + tmp86 + tmp87;
     }
     X = X + dist;
     Y = Y - dist;
     for (i = 2; i < m; i = i + 2, X = X + dist, Y = Y - dist, W = W + 6) {
	  fftw_real tmp14;
	  fftw_real tmp66;
	  fftw_real tmp25;
	  fftw_real tmp68;
	  fftw_real tmp51;
	  fftw_real tmp63;
	  fftw_real tmp36;
	  fftw_real tmp69;
	  fftw_real tmp57;
	  fftw_real tmp64;
	  fftw_real tmp47;
	  fftw_real tmp70;
	  fftw_real tmp54;
	  fftw_real tmp65;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp14 = X[0];
	  tmp66 = Y[-6 * iostride];
	  {
	       fftw_real tmp19;
	       fftw_real tmp49;
	       fftw_real tmp24;
	       fftw_real tmp50;
	       ASSERT_ALIGNED_DOUBLE;
	       {
		    fftw_real tmp16;
		    fftw_real tmp18;
		    fftw_real tmp15;
		    fftw_real tmp17;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp16 = X[iostride];
		    tmp18 = Y[-5 * iostride];
		    tmp15 = c_re(W[0]);
		    tmp17 = c_im(W[0]);
		    tmp19 = (tmp15 * tmp16) - (tmp17 * tmp18);
		    tmp49 = (tmp17 * tmp16) + (tmp15 * tmp18);
	       }
	       {
		    fftw_real tmp21;
		    fftw_real tmp23;
		    fftw_real tmp20;
		    fftw_real tmp22;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp21 = X[6 * iostride];
		    tmp23 = Y[0];
		    tmp20 = c_re(W[5]);
		    tmp22 = c_im(W[5]);
		    tmp24 = (tmp20 * tmp21) - (tmp22 * tmp23);
		    tmp50 = (tmp22 * tmp21) + (tmp20 * tmp23);
	       }
	       tmp25 = tmp19 + tmp24;
	       tmp68 = tmp24 - tmp19;
	       tmp51 = tmp49 - tmp50;
	       tmp63 = tmp49 + tmp50;
	  }
	  {
	       fftw_real tmp30;
	       fftw_real tmp55;
	       fftw_real tmp35;
	       fftw_real tmp56;
	       ASSERT_ALIGNED_DOUBLE;
	       {
		    fftw_real tmp27;
		    fftw_real tmp29;
		    fftw_real tmp26;
		    fftw_real tmp28;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp27 = X[2 * iostride];
		    tmp29 = Y[-4 * iostride];
		    tmp26 = c_re(W[1]);
		    tmp28 = c_im(W[1]);
		    tmp30 = (tmp26 * tmp27) - (tmp28 * tmp29);
		    tmp55 = (tmp28 * tmp27) + (tmp26 * tmp29);
	       }
	       {
		    fftw_real tmp32;
		    fftw_real tmp34;
		    fftw_real tmp31;
		    fftw_real tmp33;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp32 = X[5 * iostride];
		    tmp34 = Y[-iostride];
		    tmp31 = c_re(W[4]);
		    tmp33 = c_im(W[4]);
		    tmp35 = (tmp31 * tmp32) - (tmp33 * tmp34);
		    tmp56 = (tmp33 * tmp32) + (tmp31 * tmp34);
	       }
	       tmp36 = tmp30 + tmp35;
	       tmp69 = tmp35 - tmp30;
	       tmp57 = tmp55 - tmp56;
	       tmp64 = tmp55 + tmp56;
	  }
	  {
	       fftw_real tmp41;
	       fftw_real tmp52;
	       fftw_real tmp46;
	       fftw_real tmp53;
	       ASSERT_ALIGNED_DOUBLE;
	       {
		    fftw_real tmp38;
		    fftw_real tmp40;
		    fftw_real tmp37;
		    fftw_real tmp39;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp38 = X[3 * iostride];
		    tmp40 = Y[-3 * iostride];
		    tmp37 = c_re(W[2]);
		    tmp39 = c_im(W[2]);
		    tmp41 = (tmp37 * tmp38) - (tmp39 * tmp40);
		    tmp52 = (tmp39 * tmp38) + (tmp37 * tmp40);
	       }
	       {
		    fftw_real tmp43;
		    fftw_real tmp45;
		    fftw_real tmp42;
		    fftw_real tmp44;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp43 = X[4 * iostride];
		    tmp45 = Y[-2 * iostride];
		    tmp42 = c_re(W[3]);
		    tmp44 = c_im(W[3]);
		    tmp46 = (tmp42 * tmp43) - (tmp44 * tmp45);
		    tmp53 = (tmp44 * tmp43) + (tmp42 * tmp45);
	       }
	       tmp47 = tmp41 + tmp46;
	       tmp70 = tmp46 - tmp41;
	       tmp54 = tmp52 - tmp53;
	       tmp65 = tmp52 + tmp53;
	  }
	  {
	       fftw_real tmp60;
	       fftw_real tmp59;
	       fftw_real tmp73;
	       fftw_real tmp72;
	       ASSERT_ALIGNED_DOUBLE;
	       X[0] = tmp14 + tmp25 + tmp36 + tmp47;
	       tmp60 = (K781831482 * tmp51) + (K974927912 * tmp57) + (K433883739 * tmp54);
	       tmp59 = tmp14 + (K623489801 * tmp25) - (K900968867 * tmp47) - (K222520933 * tmp36);
	       Y[-6 * iostride] = tmp59 - tmp60;
	       X[iostride] = tmp59 + tmp60;
	       {
		    fftw_real tmp62;
		    fftw_real tmp61;
		    fftw_real tmp58;
		    fftw_real tmp48;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp62 = (K433883739 * tmp51) + (K974927912 * tmp54) - (K781831482 * tmp57);
		    tmp61 = tmp14 + (K623489801 * tmp36) - (K222520933 * tmp47) - (K900968867 * tmp25);
		    Y[-4 * iostride] = tmp61 - tmp62;
		    X[3 * iostride] = tmp61 + tmp62;
		    tmp58 = (K974927912 * tmp51) - (K781831482 * tmp54) - (K433883739 * tmp57);
		    tmp48 = tmp14 + (K623489801 * tmp47) - (K900968867 * tmp36) - (K222520933 * tmp25);
		    Y[-5 * iostride] = tmp48 - tmp58;
		    X[2 * iostride] = tmp48 + tmp58;
	       }
	       Y[0] = tmp63 + tmp64 + tmp65 + tmp66;
	       tmp73 = (K974927912 * tmp68) - (K781831482 * tmp70) - (K433883739 * tmp69);
	       tmp72 = (K623489801 * tmp65) + tmp66 - (K900968867 * tmp64) - (K222520933 * tmp63);
	       X[5 * iostride] = -(tmp72 - tmp73);
	       Y[-2 * iostride] = tmp73 + tmp72;
	       {
		    fftw_real tmp75;
		    fftw_real tmp74;
		    fftw_real tmp71;
		    fftw_real tmp67;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp75 = (K433883739 * tmp68) + (K974927912 * tmp70) - (K781831482 * tmp69);
		    tmp74 = (K623489801 * tmp64) + tmp66 - (K222520933 * tmp65) - (K900968867 * tmp63);
		    X[4 * iostride] = -(tmp74 - tmp75);
		    Y[-3 * iostride] = tmp75 + tmp74;
		    tmp71 = (K781831482 * tmp68) + (K974927912 * tmp69) + (K433883739 * tmp70);
		    tmp67 = (K623489801 * tmp63) + tmp66 - (K900968867 * tmp65) - (K222520933 * tmp64);
		    X[6 * iostride] = -(tmp67 - tmp71);
		    Y[-iostride] = tmp71 + tmp67;
	       }
	  }
     }
     if (i == m) {
	  fftw_real tmp1;
	  fftw_real tmp10;
	  fftw_real tmp13;
	  fftw_real tmp4;
	  fftw_real tmp11;
	  fftw_real tmp7;
	  fftw_real tmp12;
	  fftw_real tmp8;
	  fftw_real tmp9;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp1 = X[0];
	  tmp8 = X[iostride];
	  tmp9 = X[6 * iostride];
	  tmp10 = tmp8 - tmp9;
	  tmp13 = tmp8 + tmp9;
	  {
	       fftw_real tmp2;
	       fftw_real tmp3;
	       fftw_real tmp5;
	       fftw_real tmp6;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp2 = X[2 * iostride];
	       tmp3 = X[5 * iostride];
	       tmp4 = tmp2 - tmp3;
	       tmp11 = tmp2 + tmp3;
	       tmp5 = X[3 * iostride];
	       tmp6 = X[4 * iostride];
	       tmp7 = tmp5 - tmp6;
	       tmp12 = tmp5 + tmp6;
	  }
	  Y[0] = -((K781831482 * tmp11) + (K974927912 * tmp12) + (K433883739 * tmp13));
	  Y[-iostride] = (K781831482 * tmp12) - (K974927912 * tmp13) - (K433883739 * tmp11);
	  Y[-2 * iostride] = (K974927912 * tmp11) - (K781831482 * tmp13) - (K433883739 * tmp12);
	  X[iostride] = tmp1 + (K222520933 * tmp10) - (K623489801 * tmp7) - (K900968867 * tmp4);
	  X[2 * iostride] = tmp1 + (K900968867 * tmp7) - (K623489801 * tmp10) - (K222520933 * tmp4);
	  X[3 * iostride] = tmp1 + tmp4 - (tmp7 + tmp10);
	  X[0] = tmp1 + (K623489801 * tmp4) + (K222520933 * tmp7) + (K900968867 * tmp10);
     }
}

static const int twiddle_order[] =
{1, 2, 3, 4, 5, 6};
fftw_codelet_desc fftw_hc2hc_forward_7_desc =
{
     "fftw_hc2hc_forward_7",
     (void (*)()) fftw_hc2hc_forward_7,
     7,
     FFTW_FORWARD,
     FFTW_HC2HC,
     157,
     6,
     twiddle_order,
};