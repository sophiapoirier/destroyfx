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
/* Generated on Sun Nov  7 20:44:48 EST 1999 */

#include <fftw-int.h>
#include <fftw.h>

/* Generated by: ./genfft -magic-alignment-check -magic-twiddle-load-all -magic-variables 4 -magic-loopi -hc2hc-forward 9 */

/*
 * This function contains 188 FP additions, 136 FP multiplications,
 * (or, 139 additions, 87 multiplications, 49 fused multiply/add),
 * 35 stack variables, and 72 memory accesses
 */
static const fftw_real K250000000 = FFTW_KONST(+0.250000000000000000000000000000000000000000000);
static const fftw_real K433012701 = FFTW_KONST(+0.433012701892219323381861585376468091735701313);
static const fftw_real K1_532088886 = FFTW_KONST(+1.532088886237956070404785301110833347871664914);
static const fftw_real K1_879385241 = FFTW_KONST(+1.879385241571816768108218554649462939872416269);
static const fftw_real K347296355 = FFTW_KONST(+0.347296355333860697703433253538629592000751354);
static const fftw_real K1_285575219 = FFTW_KONST(+1.285575219373078652645286819814526865815119768);
static const fftw_real K684040286 = FFTW_KONST(+0.684040286651337466088199229364519161526166735);
static const fftw_real K1_969615506 = FFTW_KONST(+1.969615506024416118733486049179046027341286503);
static const fftw_real K342020143 = FFTW_KONST(+0.342020143325668733044099614682259580763083368);
static const fftw_real K813797681 = FFTW_KONST(+0.813797681349373692844693217248393223289101568);
static const fftw_real K939692620 = FFTW_KONST(+0.939692620785908384054109277324731469936208134);
static const fftw_real K296198132 = FFTW_KONST(+0.296198132726023843175338011893050938967728390);
static const fftw_real K852868531 = FFTW_KONST(+0.852868531952443209628250963940074071936020296);
static const fftw_real K173648177 = FFTW_KONST(+0.173648177666930348851716626769314796000375677);
static const fftw_real K556670399 = FFTW_KONST(+0.556670399226419366452912952047023132968291906);
static const fftw_real K766044443 = FFTW_KONST(+0.766044443118978035202392650555416673935832457);
static const fftw_real K984807753 = FFTW_KONST(+0.984807753012208059366743024589523013670643252);
static const fftw_real K150383733 = FFTW_KONST(+0.150383733180435296639271897612501926072238258);
static const fftw_real K642787609 = FFTW_KONST(+0.642787609686539326322643409907263432907559884);
static const fftw_real K663413948 = FFTW_KONST(+0.663413948168938396205421319635891297216863310);
static const fftw_real K866025403 = FFTW_KONST(+0.866025403784438646763723170752936183471402627);
static const fftw_real K500000000 = FFTW_KONST(+0.500000000000000000000000000000000000000000000);

/*
 * Generator Id's : 
 * $Id: fhf_9.c,v 1.3 2001-08-10 12:51:40 tom7 Exp $
 * $Id: fhf_9.c,v 1.3 2001-08-10 12:51:40 tom7 Exp $
 * $Id: fhf_9.c,v 1.3 2001-08-10 12:51:40 tom7 Exp $
 */

void fftw_hc2hc_forward_9(fftw_real *A, const fftw_complex *W, int iostride, int m, int dist)
{
     int i;
     fftw_real *X;
     fftw_real *Y;
     X = A;
     Y = A + (9 * iostride);
     {
	  fftw_real tmp136;
	  fftw_real tmp150;
	  fftw_real tmp155;
	  fftw_real tmp154;
	  fftw_real tmp139;
	  fftw_real tmp162;
	  fftw_real tmp145;
	  fftw_real tmp153;
	  fftw_real tmp156;
	  fftw_real tmp137;
	  fftw_real tmp138;
	  fftw_real tmp140;
	  fftw_real tmp151;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp136 = X[0];
	  {
	       fftw_real tmp146;
	       fftw_real tmp147;
	       fftw_real tmp148;
	       fftw_real tmp149;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp146 = X[2 * iostride];
	       tmp147 = X[5 * iostride];
	       tmp148 = X[8 * iostride];
	       tmp149 = tmp147 + tmp148;
	       tmp150 = tmp146 + tmp149;
	       tmp155 = tmp146 - (K500000000 * tmp149);
	       tmp154 = tmp148 - tmp147;
	  }
	  tmp137 = X[3 * iostride];
	  tmp138 = X[6 * iostride];
	  tmp139 = tmp137 + tmp138;
	  tmp162 = tmp138 - tmp137;
	  {
	       fftw_real tmp141;
	       fftw_real tmp142;
	       fftw_real tmp143;
	       fftw_real tmp144;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp141 = X[iostride];
	       tmp142 = X[4 * iostride];
	       tmp143 = X[7 * iostride];
	       tmp144 = tmp142 + tmp143;
	       tmp145 = tmp141 + tmp144;
	       tmp153 = tmp141 - (K500000000 * tmp144);
	       tmp156 = tmp143 - tmp142;
	  }
	  Y[-3 * iostride] = K866025403 * (tmp150 - tmp145);
	  tmp140 = tmp136 + tmp139;
	  tmp151 = tmp145 + tmp150;
	  X[3 * iostride] = tmp140 - (K500000000 * tmp151);
	  X[0] = tmp140 + tmp151;
	  {
	       fftw_real tmp164;
	       fftw_real tmp160;
	       fftw_real tmp161;
	       fftw_real tmp163;
	       fftw_real tmp152;
	       fftw_real tmp157;
	       fftw_real tmp158;
	       fftw_real tmp159;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp164 = K866025403 * tmp162;
	       tmp160 = (K663413948 * tmp156) - (K642787609 * tmp153);
	       tmp161 = (K150383733 * tmp154) - (K984807753 * tmp155);
	       tmp163 = tmp160 + tmp161;
	       tmp152 = tmp136 - (K500000000 * tmp139);
	       tmp157 = (K766044443 * tmp153) + (K556670399 * tmp156);
	       tmp158 = (K173648177 * tmp155) + (K852868531 * tmp154);
	       tmp159 = tmp157 + tmp158;
	       X[iostride] = tmp152 + tmp159;
	       X[4 * iostride] = tmp152 + (K866025403 * (tmp160 - tmp161)) - (K500000000 * tmp159);
	       X[2 * iostride] = tmp152 + (K173648177 * tmp153) - (K296198132 * tmp154) - (K939692620 * tmp155) - (K852868531 * tmp156);
	       Y[-iostride] = tmp164 + tmp163;
	       Y[-4 * iostride] = (K866025403 * (tmp162 + (tmp158 - tmp157))) - (K500000000 * tmp163);
	       Y[-2 * iostride] = (K813797681 * tmp154) - (K342020143 * tmp155) - (K150383733 * tmp156) - (K984807753 * tmp153) - tmp164;
	  }
     }
     X = X + dist;
     Y = Y - dist;
     for (i = 2; i < m; i = i + 2, X = X + dist, Y = Y - dist, W = W + 8) {
	  fftw_real tmp24;
	  fftw_real tmp122;
	  fftw_real tmp75;
	  fftw_real tmp121;
	  fftw_real tmp128;
	  fftw_real tmp127;
	  fftw_real tmp35;
	  fftw_real tmp72;
	  fftw_real tmp70;
	  fftw_real tmp92;
	  fftw_real tmp109;
	  fftw_real tmp118;
	  fftw_real tmp97;
	  fftw_real tmp108;
	  fftw_real tmp53;
	  fftw_real tmp81;
	  fftw_real tmp105;
	  fftw_real tmp117;
	  fftw_real tmp86;
	  fftw_real tmp106;
	  ASSERT_ALIGNED_DOUBLE;
	  {
	       fftw_real tmp29;
	       fftw_real tmp73;
	       fftw_real tmp34;
	       fftw_real tmp74;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp24 = X[0];
	       tmp122 = Y[-8 * iostride];
	       {
		    fftw_real tmp26;
		    fftw_real tmp28;
		    fftw_real tmp25;
		    fftw_real tmp27;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp26 = X[3 * iostride];
		    tmp28 = Y[-5 * iostride];
		    tmp25 = c_re(W[2]);
		    tmp27 = c_im(W[2]);
		    tmp29 = (tmp25 * tmp26) - (tmp27 * tmp28);
		    tmp73 = (tmp27 * tmp26) + (tmp25 * tmp28);
	       }
	       {
		    fftw_real tmp31;
		    fftw_real tmp33;
		    fftw_real tmp30;
		    fftw_real tmp32;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp31 = X[6 * iostride];
		    tmp33 = Y[-2 * iostride];
		    tmp30 = c_re(W[5]);
		    tmp32 = c_im(W[5]);
		    tmp34 = (tmp30 * tmp31) - (tmp32 * tmp33);
		    tmp74 = (tmp32 * tmp31) + (tmp30 * tmp33);
	       }
	       tmp75 = K866025403 * (tmp73 - tmp74);
	       tmp121 = tmp73 + tmp74;
	       tmp128 = tmp122 - (K500000000 * tmp121);
	       tmp127 = K866025403 * (tmp34 - tmp29);
	       tmp35 = tmp29 + tmp34;
	       tmp72 = tmp24 - (K500000000 * tmp35);
	  }
	  {
	       fftw_real tmp58;
	       fftw_real tmp94;
	       fftw_real tmp63;
	       fftw_real tmp89;
	       fftw_real tmp68;
	       fftw_real tmp90;
	       fftw_real tmp69;
	       fftw_real tmp95;
	       ASSERT_ALIGNED_DOUBLE;
	       {
		    fftw_real tmp55;
		    fftw_real tmp57;
		    fftw_real tmp54;
		    fftw_real tmp56;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp55 = X[2 * iostride];
		    tmp57 = Y[-6 * iostride];
		    tmp54 = c_re(W[1]);
		    tmp56 = c_im(W[1]);
		    tmp58 = (tmp54 * tmp55) - (tmp56 * tmp57);
		    tmp94 = (tmp56 * tmp55) + (tmp54 * tmp57);
	       }
	       {
		    fftw_real tmp60;
		    fftw_real tmp62;
		    fftw_real tmp59;
		    fftw_real tmp61;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp60 = X[5 * iostride];
		    tmp62 = Y[-3 * iostride];
		    tmp59 = c_re(W[4]);
		    tmp61 = c_im(W[4]);
		    tmp63 = (tmp59 * tmp60) - (tmp61 * tmp62);
		    tmp89 = (tmp61 * tmp60) + (tmp59 * tmp62);
	       }
	       {
		    fftw_real tmp65;
		    fftw_real tmp67;
		    fftw_real tmp64;
		    fftw_real tmp66;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp65 = X[8 * iostride];
		    tmp67 = Y[0];
		    tmp64 = c_re(W[7]);
		    tmp66 = c_im(W[7]);
		    tmp68 = (tmp64 * tmp65) - (tmp66 * tmp67);
		    tmp90 = (tmp66 * tmp65) + (tmp64 * tmp67);
	       }
	       tmp69 = tmp63 + tmp68;
	       tmp95 = tmp89 + tmp90;
	       {
		    fftw_real tmp88;
		    fftw_real tmp91;
		    fftw_real tmp93;
		    fftw_real tmp96;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp70 = tmp58 + tmp69;
		    tmp88 = tmp58 - (K500000000 * tmp69);
		    tmp91 = K866025403 * (tmp89 - tmp90);
		    tmp92 = tmp88 + tmp91;
		    tmp109 = tmp88 - tmp91;
		    tmp118 = tmp94 + tmp95;
		    tmp93 = K866025403 * (tmp68 - tmp63);
		    tmp96 = tmp94 - (K500000000 * tmp95);
		    tmp97 = tmp93 + tmp96;
		    tmp108 = tmp96 - tmp93;
	       }
	  }
	  {
	       fftw_real tmp41;
	       fftw_real tmp83;
	       fftw_real tmp46;
	       fftw_real tmp78;
	       fftw_real tmp51;
	       fftw_real tmp79;
	       fftw_real tmp52;
	       fftw_real tmp84;
	       ASSERT_ALIGNED_DOUBLE;
	       {
		    fftw_real tmp38;
		    fftw_real tmp40;
		    fftw_real tmp37;
		    fftw_real tmp39;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp38 = X[iostride];
		    tmp40 = Y[-7 * iostride];
		    tmp37 = c_re(W[0]);
		    tmp39 = c_im(W[0]);
		    tmp41 = (tmp37 * tmp38) - (tmp39 * tmp40);
		    tmp83 = (tmp39 * tmp38) + (tmp37 * tmp40);
	       }
	       {
		    fftw_real tmp43;
		    fftw_real tmp45;
		    fftw_real tmp42;
		    fftw_real tmp44;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp43 = X[4 * iostride];
		    tmp45 = Y[-4 * iostride];
		    tmp42 = c_re(W[3]);
		    tmp44 = c_im(W[3]);
		    tmp46 = (tmp42 * tmp43) - (tmp44 * tmp45);
		    tmp78 = (tmp44 * tmp43) + (tmp42 * tmp45);
	       }
	       {
		    fftw_real tmp48;
		    fftw_real tmp50;
		    fftw_real tmp47;
		    fftw_real tmp49;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp48 = X[7 * iostride];
		    tmp50 = Y[-iostride];
		    tmp47 = c_re(W[6]);
		    tmp49 = c_im(W[6]);
		    tmp51 = (tmp47 * tmp48) - (tmp49 * tmp50);
		    tmp79 = (tmp49 * tmp48) + (tmp47 * tmp50);
	       }
	       tmp52 = tmp46 + tmp51;
	       tmp84 = tmp78 + tmp79;
	       {
		    fftw_real tmp77;
		    fftw_real tmp80;
		    fftw_real tmp82;
		    fftw_real tmp85;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp53 = tmp41 + tmp52;
		    tmp77 = tmp41 - (K500000000 * tmp52);
		    tmp80 = K866025403 * (tmp78 - tmp79);
		    tmp81 = tmp77 + tmp80;
		    tmp105 = tmp77 - tmp80;
		    tmp117 = tmp83 + tmp84;
		    tmp82 = K866025403 * (tmp51 - tmp46);
		    tmp85 = tmp83 - (K500000000 * tmp84);
		    tmp86 = tmp82 + tmp85;
		    tmp106 = tmp85 - tmp82;
	       }
	  }
	  {
	       fftw_real tmp119;
	       fftw_real tmp36;
	       fftw_real tmp71;
	       fftw_real tmp116;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp119 = K866025403 * (tmp117 - tmp118);
	       tmp36 = tmp24 + tmp35;
	       tmp71 = tmp53 + tmp70;
	       tmp116 = tmp36 - (K500000000 * tmp71);
	       X[0] = tmp36 + tmp71;
	       X[3 * iostride] = tmp116 + tmp119;
	       Y[-6 * iostride] = tmp116 - tmp119;
	  }
	  {
	       fftw_real tmp125;
	       fftw_real tmp120;
	       fftw_real tmp123;
	       fftw_real tmp124;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp125 = K866025403 * (tmp70 - tmp53);
	       tmp120 = tmp117 + tmp118;
	       tmp123 = tmp121 + tmp122;
	       tmp124 = tmp123 - (K500000000 * tmp120);
	       Y[0] = tmp120 + tmp123;
	       Y[-3 * iostride] = tmp125 + tmp124;
	       X[6 * iostride] = -(tmp124 - tmp125);
	  }
	  {
	       fftw_real tmp76;
	       fftw_real tmp129;
	       fftw_real tmp99;
	       fftw_real tmp131;
	       fftw_real tmp103;
	       fftw_real tmp126;
	       fftw_real tmp100;
	       fftw_real tmp130;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp76 = tmp72 + tmp75;
	       tmp129 = tmp127 + tmp128;
	       {
		    fftw_real tmp87;
		    fftw_real tmp98;
		    fftw_real tmp101;
		    fftw_real tmp102;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp87 = (K766044443 * tmp81) + (K642787609 * tmp86);
		    tmp98 = (K173648177 * tmp92) + (K984807753 * tmp97);
		    tmp99 = tmp87 + tmp98;
		    tmp131 = K866025403 * (tmp98 - tmp87);
		    tmp101 = (K766044443 * tmp86) - (K642787609 * tmp81);
		    tmp102 = (K173648177 * tmp97) - (K984807753 * tmp92);
		    tmp103 = K866025403 * (tmp101 - tmp102);
		    tmp126 = tmp101 + tmp102;
	       }
	       X[iostride] = tmp76 + tmp99;
	       tmp100 = tmp76 - (K500000000 * tmp99);
	       Y[-7 * iostride] = tmp100 - tmp103;
	       X[4 * iostride] = tmp100 + tmp103;
	       Y[-iostride] = tmp126 + tmp129;
	       tmp130 = tmp129 - (K500000000 * tmp126);
	       X[7 * iostride] = -(tmp130 - tmp131);
	       Y[-4 * iostride] = tmp131 + tmp130;
	  }
	  {
	       fftw_real tmp104;
	       fftw_real tmp133;
	       fftw_real tmp111;
	       fftw_real tmp132;
	       fftw_real tmp115;
	       fftw_real tmp134;
	       fftw_real tmp112;
	       fftw_real tmp135;
	       ASSERT_ALIGNED_DOUBLE;
	       tmp104 = tmp72 - tmp75;
	       tmp133 = tmp128 - tmp127;
	       {
		    fftw_real tmp107;
		    fftw_real tmp110;
		    fftw_real tmp113;
		    fftw_real tmp114;
		    ASSERT_ALIGNED_DOUBLE;
		    tmp107 = (K173648177 * tmp105) + (K984807753 * tmp106);
		    tmp110 = (K342020143 * tmp108) - (K939692620 * tmp109);
		    tmp111 = tmp107 + tmp110;
		    tmp132 = K866025403 * (tmp110 - tmp107);
		    tmp113 = (K173648177 * tmp106) - (K984807753 * tmp105);
		    tmp114 = (K342020143 * tmp109) + (K939692620 * tmp108);
		    tmp115 = K866025403 * (tmp113 + tmp114);
		    tmp134 = tmp113 - tmp114;
	       }
	       X[2 * iostride] = tmp104 + tmp111;
	       tmp112 = tmp104 - (K500000000 * tmp111);
	       Y[-8 * iostride] = tmp112 - tmp115;
	       Y[-5 * iostride] = tmp112 + tmp115;
	       Y[-2 * iostride] = tmp134 + tmp133;
	       tmp135 = tmp133 - (K500000000 * tmp134);
	       X[5 * iostride] = -(tmp132 + tmp135);
	       X[8 * iostride] = -(tmp135 - tmp132);
	  }
     }
     if (i == m) {
	  fftw_real tmp12;
	  fftw_real tmp18;
	  fftw_real tmp4;
	  fftw_real tmp7;
	  fftw_real tmp10;
	  fftw_real tmp9;
	  fftw_real tmp14;
	  fftw_real tmp23;
	  fftw_real tmp16;
	  fftw_real tmp21;
	  fftw_real tmp5;
	  fftw_real tmp8;
	  fftw_real tmp6;
	  fftw_real tmp11;
	  fftw_real tmp22;
	  fftw_real tmp13;
	  fftw_real tmp17;
	  fftw_real tmp20;
	  fftw_real tmp1;
	  fftw_real tmp3;
	  fftw_real tmp2;
	  fftw_real tmp15;
	  fftw_real tmp19;
	  ASSERT_ALIGNED_DOUBLE;
	  tmp1 = X[0];
	  tmp3 = X[3 * iostride];
	  tmp2 = X[6 * iostride];
	  tmp12 = K866025403 * (tmp3 + tmp2);
	  tmp18 = tmp1 - (K500000000 * (tmp2 - tmp3));
	  tmp4 = tmp1 + tmp2 - tmp3;
	  tmp7 = X[4 * iostride];
	  tmp10 = X[7 * iostride];
	  tmp9 = X[iostride];
	  tmp14 = (K1_969615506 * tmp7) + (K684040286 * tmp9) + (K1_285575219 * tmp10);
	  tmp23 = (K1_285575219 * tmp9) - (K1_969615506 * tmp10) - (K684040286 * tmp7);
	  tmp16 = (K347296355 * tmp7) + (K1_879385241 * tmp9) - (K1_532088886 * tmp10);
	  tmp21 = (K1_879385241 * tmp7) + (K1_532088886 * tmp9) + (K347296355 * tmp10);
	  tmp5 = X[2 * iostride];
	  tmp8 = X[5 * iostride];
	  tmp6 = X[8 * iostride];
	  tmp11 = tmp8 - (tmp5 + tmp6);
	  tmp22 = (K1_285575219 * tmp6) - (K684040286 * tmp8) - (K1_969615506 * tmp5);
	  tmp13 = (K1_285575219 * tmp5) + (K1_969615506 * tmp8) + (K684040286 * tmp6);
	  tmp17 = (K1_532088886 * tmp5) - (K1_879385241 * tmp6) - (K347296355 * tmp8);
	  tmp20 = (K347296355 * tmp5) + (K1_879385241 * tmp8) + (K1_532088886 * tmp6);
	  Y[-iostride] = K866025403 * (tmp11 + tmp7 - (tmp9 + tmp10));
	  X[iostride] = tmp4 + (K500000000 * (tmp11 + tmp9 + tmp10 - tmp7));
	  X[4 * iostride] = tmp4 + tmp5 + tmp6 + tmp7 - (tmp8 + tmp9 + tmp10);
	  X[2 * iostride] = tmp18 + (K433012701 * (tmp22 - tmp23)) + (K250000000 * (tmp21 - tmp20));
	  Y[-2 * iostride] = tmp12 - (K433012701 * (tmp20 + tmp21)) - (K250000000 * (tmp22 + tmp23));
	  tmp15 = tmp13 + tmp14;
	  Y[0] = -(tmp12 + (K500000000 * tmp15));
	  Y[-3 * iostride] = (K250000000 * tmp15) - (K433012701 * (tmp16 - tmp17)) - tmp12;
	  tmp19 = tmp17 + tmp16;
	  X[0] = tmp18 + (K500000000 * tmp19);
	  X[3 * iostride] = tmp18 + (K433012701 * (tmp13 - tmp14)) - (K250000000 * tmp19);
     }
}

static const int twiddle_order[] =
{1, 2, 3, 4, 5, 6, 7, 8};
fftw_codelet_desc fftw_hc2hc_forward_9_desc =
{
     "fftw_hc2hc_forward_9",
     (void (*)()) fftw_hc2hc_forward_9,
     9,
     FFTW_FORWARD,
     FFTW_HC2HC,
     201,
     8,
     twiddle_order,
};
