/*
 ** Copyright 2003-2010, VisualOn, Inc.
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

/***********************************************************************
*      File: c4t64fx_mips.c                                            *
*                                                                      *
*      Description:Performs algebraic codebook search for higher modes *
*                                                                      *
************************************************************************
*                                                                      *
*      Optimized for MIPS architecture                                 *
*                                                                      *
************************************************************************/

/************************************************************************
* Function: ACELP_4t64_fx_mips()                                        *
*                                                                       *
* 20, 36, 44, 52, 64, 72, 88 bits algebraic codebook.                   *
* 4 tracks x 16 positions per track = 64 samples.                       *
*                                                                       *
* 20 bits --> 4 pulses in a frame of 64 samples.                        *
* 36 bits --> 8 pulses in a frame of 64 samples.                        *
* 44 bits --> 10 pulses in a frame of 64 samples.                       *
* 52 bits --> 12 pulses in a frame of 64 samples.                       *
* 64 bits --> 16 pulses in a frame of 64 samples.                       *
* 72 bits --> 18 pulses in a frame of 64 samples.                       *
* 88 bits --> 24 pulses in a frame of 64 samples.                       *
*                                                                       *
* All pulses can have two (2) possible amplitudes: +1 or -1.            *
* Each pulse can have sixteen (16) possible positions.                  *
*************************************************************************/

#include "typedef.h"
#include "basic_op.h"
#include "math_op.h"
#include "acelp.h"
#include "cnst.h"

#include "q_pulse.h"

static Word16 tipos[36] = {
    0, 1, 2, 3,                            /* starting point &ipos[0], 1st iter */
    1, 2, 3, 0,                            /* starting point &ipos[4], 2nd iter */
    2, 3, 0, 1,                            /* starting point &ipos[8], 3rd iter */
    3, 0, 1, 2,                            /* starting point &ipos[12], 4th iter */
    0, 1, 2, 3,
    1, 2, 3, 0,
    2, 3, 0, 1,
    3, 0, 1, 2,
    0, 1, 2, 3};                           /* end point for 24 pulses &ipos[35], 4th iter */

#define NB_PULSE_MAX  24

#define L_SUBFR   64
#define NB_TRACK  4
#define STEP      4
#define NB_POS    16
#define MSIZE     256
#define NB_MAX    8
#define NPMAXPT   ((NB_PULSE_MAX+NB_TRACK-1)/NB_TRACK)

/* Private functions */
void cor_h_vec_012_mips(
        Word16 h[],                           /* (i) scaled impulse response                 */
        Word16 vec[],                         /* (i) scaled vector (/8) to correlate with h[] */
        Word16 track,                         /* (i) track to use                            */
        Word16 sign[],                        /* (i) sign vector                             */
        Word16 rrixix[][NB_POS],              /* (i) correlation of h[x] with h[x]      */
        Word16 cor_1[],                       /* (o) result of correlation (NB_POS elements) */
        Word16 cor_2[]                        /* (o) result of correlation (NB_POS elements) */
        );

void cor_h_vec_30_mips(
        Word16 h[],                           /* (i) scaled impulse response                 */
        Word16 vec[],                         /* (i) scaled vector (/8) to correlate with h[] */
        Word16 track,                         /* (i) track to use                            */
        Word16 sign[],                        /* (i) sign vector                             */
        Word16 rrixix[][NB_POS],              /* (i) correlation of h[x] with h[x]      */
        Word16 cor_1[],                       /* (o) result of correlation (NB_POS elements) */
        Word16 cor_2[]                        /* (o) result of correlation (NB_POS elements) */
        );

void search_ixiy_mips(
        Word16 nb_pos_ix,                     /* (i) nb of pos for pulse 1 (1..8)       */
        Word16 track_x,                       /* (i) track of pulse 1                   */
        Word16 track_y,                       /* (i) track of pulse 2                   */
        Word16 * ps,                          /* (i/o) correlation of all fixed pulses  */
        Word16 * alp,                         /* (i/o) energy of all fixed pulses       */
        Word16 * ix,                          /* (o) position of pulse 1                */
        Word16 * iy,                          /* (o) position of pulse 2                */
        Word16 dn[],                          /* (i) corr. between target and h[]       */
        Word16 dn2[],                         /* (i) vector of selected positions       */
        Word16 cor_x[],                       /* (i) corr. of pulse 1 with fixed pulses */
        Word16 cor_y[],                       /* (i) corr. of pulse 2 with fixed pulses */
        Word16 rrixiy[][MSIZE]                /* (i) corr. of pulse 1 with pulse 2   */
        );

#if defined(MIPS_DSP_R1_LE)
void ACELP_4t64_fx_mips(
        Word16 dn[],                          /* (i) <12b : correlation between target x[] and H[]      */
        Word16 cn[],                          /* (i) <12b : residual after long term prediction         */
        Word16 H[],                           /* (i) Q12: impulse response of weighted synthesis filter */
        Word16 code[],                        /* (o) Q9 : algebraic (fixed) codebook excitation         */
        Word16 y[],                           /* (o) Q9 : filtered fixed codebook excitation            */
        Word16 nbbits,                        /* (i) : 20, 36, 44, 52, 64, 72 or 88 bits                */
        Word16 ser_size,                      /* (i) : bit rate                                         */
        Word16 _index[]                       /* (o) : index (20): 5+5+5+5 = 20 bits.                   */
        /* (o) : index (36): 9+9+9+9 = 36 bits.                   */
        /* (o) : index (44): 13+9+13+9 = 44 bits.                 */
        /* (o) : index (52): 13+13+13+13 = 52 bits.               */
        /* (o) : index (64): 2+2+2+2+14+14+14+14 = 64 bits.       */
        /* (o) : index (72): 10+2+10+2+10+14+10+14 = 72 bits.     */
        /* (o) : index (88): 11+11+11+11+11+11+11+11 = 88 bits.   */
        )
{
    Word32 i, j, k;
    Word16 st, ix, iy, pos, index, track, nb_pulse, nbiter, j_temp;
    Word16 psk, ps, alpk, alp, val, k_cn, k_dn, exp;
    Word16 *p0, *p1, *p2, *p3, *psign;
    Word16 *h, *h_inv, *ptr_h1, *ptr_h2, *ptr_hf, h_shift;
    Word32 s, cor, cor1, L_tmp, L_index;
    Word16 dn2[L_SUBFR], sign[L_SUBFR], vec[L_SUBFR];
    Word16 ind[NPMAXPT * NB_TRACK];
    Word16 codvec[NB_PULSE_MAX], nbpos[10];
    Word16 cor_x[NB_POS], cor_y[NB_POS], pos_max[NB_TRACK];
    Word16 h_buf[4 * L_SUBFR];
    Word16 rrixix[NB_TRACK][NB_POS], rrixiy[NB_TRACK][MSIZE];
    Word16 ipos[NB_PULSE_MAX];
    Word16 *ptr, *ptr_end;
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5, Temp6, Temp7, Temp8, Temp9;

    switch (nbbits)
    {
        case 20:                               /* 20 bits, 4 pulses, 4 tracks */
            nbiter = 4;                          /* 4x16x16=1024 loop */
            alp = 8192;                          /* alp = 2.0 (Q12) */
            nb_pulse = 4;
            nbpos[0] = 4;
            nbpos[1] = 8;
            break;
        case 36:                               /* 36 bits, 8 pulses, 4 tracks */
            nbiter = 4;                          /* 4x20x16=1280 loop */
            alp = 4096;                          /* alp = 1.0 (Q12) */
            nb_pulse = 8;
            nbpos[0] = 4;
            nbpos[1] = 8;
            nbpos[2] = 8;
            break;
        case 44:                               /* 44 bits, 10 pulses, 4 tracks */
            nbiter = 4;                          /* 4x26x16=1664 loop */
            alp = 4096;                          /* alp = 1.0 (Q12) */
            nb_pulse = 10;
            nbpos[0] = 4;
            nbpos[1] = 6;
            nbpos[2] = 8;
            nbpos[3] = 8;
            break;
        case 52:                               /* 52 bits, 12 pulses, 4 tracks */
            nbiter = 4;                          /* 4x26x16=1664 loop */
            alp = 4096;                          /* alp = 1.0 (Q12) */
            nb_pulse = 12;
            nbpos[0] = 4;
            nbpos[1] = 6;
            nbpos[2] = 8;
            nbpos[3] = 8;
            break;
        case 64:                               /* 64 bits, 16 pulses, 4 tracks */
            nbiter = 3;                          /* 3x36x16=1728 loop */
            alp = 3277;                          /* alp = 0.8 (Q12) */
            nb_pulse = 16;
            nbpos[0] = 4;
            nbpos[1] = 4;
            nbpos[2] = 6;
            nbpos[3] = 6;
            nbpos[4] = 8;
            nbpos[5] = 8;
            break;
        case 72:                               /* 72 bits, 18 pulses, 4 tracks */
            nbiter = 3;                          /* 3x35x16=1680 loop */
            alp = 3072;                          /* alp = 0.75 (Q12) */
            nb_pulse = 18;
            nbpos[0] = 2;
            nbpos[1] = 3;
            nbpos[2] = 4;
            nbpos[3] = 5;
            nbpos[4] = 6;
            nbpos[5] = 7;
            nbpos[6] = 8;
            break;
        case 88:                               /* 88 bits, 24 pulses, 4 tracks */
            if(ser_size > 462)
                nbiter = 1;
            else
                nbiter = 2;                    /* 2x53x16=1696 loop */

            alp = 2048;                          /* alp = 0.5 (Q12) */
            nb_pulse = 24;
            nbpos[0] = 2;
            nbpos[1] = 2;
            nbpos[2] = 3;
            nbpos[3] = 4;
            nbpos[4] = 5;
            nbpos[5] = 6;
            nbpos[6] = 7;
            nbpos[7] = 8;
            nbpos[8] = 8;
            nbpos[9] = 8;
            break;
        default:
            nbiter = 0;
            alp = 0;
            nb_pulse = 0;
    }

    for (i = 0; i < nb_pulse; i++)
    {
        codvec[i] = i;
    }

    /*----------------------------------------------------------------*
    * Find sign for each pulse position.                             *
    *----------------------------------------------------------------*/
    /* calculate energy for normalization of cn[] and dn[] */
    /* set k_cn = 32..32767 (ener_cn = 2^30..256-0) */
    s = Dot_product12(cn, cn, L_SUBFR, &exp);

    Isqrt_n(&s, &exp);
    s = L_shl(s, (exp + 5));
    k_cn = extract_h(L_add(s, 0x8000));

    /* set k_dn = 32..512 (ener_dn = 2^30..2^22) */
    s = Dot_product12(dn, dn, L_SUBFR, &exp);

    Isqrt_n(&s, &exp);
    k_dn = (L_shl(s, (exp + 5 + 3)) + 0x8000) >> 16;    /* k_dn = 256..4096 */
    k_dn = vo_mult_r(alp, k_dn);              /* alp in Q12 */

    /* mix normalized cn[] and dn[] */
    p0 = cn;
    p1 = dn;
    p2 = dn2;
    ptr_end = p0 + L_SUBFR;

    __asm__ volatile (
        ".set push                                      \n\t"
        ".set noreorder                                 \n\t"

      "1:                                               \n\t"
        "lh             %[Temp1],   0(%[p0])            \n\t"
        "lh             %[Temp2],   2(%[p0])            \n\t"
        "lh             %[Temp3],   4(%[p0])            \n\t"
        "lh             %[Temp4],   6(%[p0])            \n\t"
        "lh             %[Temp5],   0(%[p1])            \n\t"
        "lh             %[Temp6],   2(%[p1])            \n\t"
        "lh             %[Temp7],   4(%[p1])            \n\t"
        "lh             %[Temp8],   6(%[p1])            \n\t"
        "mult           $ac0,       %[Temp1],   %[k_cn] \n\t"
        "mult           $ac1,       %[Temp2],   %[k_cn] \n\t"
        "mult           $ac2,       %[Temp3],   %[k_cn] \n\t"
        "mult           $ac3,       %[Temp4],   %[k_cn] \n\t"
        "madd           $ac0,       %[Temp5],   %[k_dn] \n\t"
        "madd           $ac1,       %[Temp6],   %[k_dn] \n\t"
        "madd           $ac2,       %[Temp7],   %[k_dn] \n\t"
        "madd           $ac3,       %[Temp8],   %[k_dn] \n\t"
        "extr.w         %[Temp1],   $ac0,       7       \n\t"
        "extr.w         %[Temp2],   $ac1,       7       \n\t"
        "extr.w         %[Temp3],   $ac2,       7       \n\t"
        "extr.w         %[Temp4],   $ac3,       7       \n\t"
        "addiu          %[p0],      %[p0],      8       \n\t"
        "addiu          %[p1],      %[p1],      8       \n\t"
        "sh             %[Temp1],   0(%[p2])            \n\t"
        "sh             %[Temp2],   2(%[p2])            \n\t"
        "sh             %[Temp3],   4(%[p2])            \n\t"
        "sh             %[Temp4],   6(%[p2])            \n\t"
        "bne            %[p0],      %[ptr_end], 1b      \n\t"
        " addiu         %[p2],      %[p2],      8       \n\t"

        ".set pop                                       \n\t"

        : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
          [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
          [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
          [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
          [p0]    "+r"  (p0),    [p1]    "+r"  (p1),
          [p2]    "+r"  (p2)
        : [ptr_end] "r" (ptr_end), [k_cn] "r" (k_cn),
          [k_dn]    "r" (k_dn)
        : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi",
          "$ac2lo", "$ac3hi", "$ac3lo",
          "memory"
    );

    /* set sign according to dn2[] = k_cn*cn[] + k_dn*dn[]    */
    for(i = 0; i < L_SUBFR; i++)
    {
        val = dn[i];
        ps = dn2[i];
        if (ps >= 0)
        {
            sign[i] = 32767;             /* sign = +1 (Q12) */
            vec[i] = -32768;
        } else
        {
            sign[i] = -32768;            /* sign = -1 (Q12) */
            vec[i] = 32767;
            dn[i] = -val;
            dn2[i] = -ps;
        }
    }
    /*----------------------------------------------------------------*
    * Select NB_MAX position per track according to max of dn2[].    *
    *----------------------------------------------------------------*/
    pos = 0;
    for (i = 0; i < NB_TRACK; i++)
    {
        for (k = 0; k < NB_MAX; k++)
        {
            ps = -1;
            for (j = i; j < L_SUBFR; j += STEP)
            {
                if(dn2[j] > ps)
                {
                    ps = dn2[j];
                    pos = j;
                }
            }
            dn2[pos] = (k - NB_MAX);     /* dn2 < 0 when position is selected */
            if (k == 0)
            {
                pos_max[i] = pos;
            }
        }
    }

    /*--------------------------------------------------------------*
    * Scale h[] to avoid overflow and to get maximum of precision  *
    * on correlation.                                              *
    *                                                              *
    * Maximum of h[] (h[0]) is fixed to 2048 (MAX16 / 16).         *
    *  ==> This allow addition of 16 pulses without saturation.    *
    *                                                              *
    * Energy worst case (on resonant impulse response),            *
    * - energy of h[] is approximately MAX/16.                     *
    * - During search, the energy is divided by 8 to avoid         *
    *   overflow on "alp". (energy of h[] = MAX/128).              *
    *  ==> "alp" worst case detected is 22854 on sinusoidal wave.  *
    *--------------------------------------------------------------*/

    /* impulse response buffer for fast computation */

    h = h_buf;
    h_inv = h_buf + (2 * L_SUBFR);
    memset (h,      0, L_SUBFR * sizeof(Word16));
    memset (h_inv,  0, L_SUBFR * sizeof(Word16));
    h     += L_SUBFR;
    h_inv += L_SUBFR;

    ptr = H;
    ptr_end = ptr + L_SUBFR;

    __asm__ volatile (
        ".set push                                          \n\t"
        ".set noreorder                                     \n\t"

        "mult           $ac0,       $0,         $0          \n\t"
      "1:                                                   \n\t"
        "lh             %[Temp1],   0(%[ptr])               \n\t"
        "lh             %[Temp2],   2(%[ptr])               \n\t"
        "lh             %[Temp3],   4(%[ptr])               \n\t"
        "lh             %[Temp4],   6(%[ptr])               \n\t"
        "maq_s.w.phr    $ac0,       %[Temp1],   %[Temp1]    \n\t"
        "maq_s.w.phr    $ac0,       %[Temp2],   %[Temp2]    \n\t"
        "maq_s.w.phr    $ac0,       %[Temp3],   %[Temp3]    \n\t"
        "maq_s.w.phr    $ac0,       %[Temp4],   %[Temp4]    \n\t"
        "lh             %[Temp1],   8(%[ptr])               \n\t"
        "lh             %[Temp2],   10(%[ptr])              \n\t"
        "lh             %[Temp3],   12(%[ptr])              \n\t"
        "lh             %[Temp4],   14(%[ptr])              \n\t"
        "addiu          %[ptr],     %[ptr],     16          \n\t"
        "maq_s.w.phr    $ac0,       %[Temp1],   %[Temp1]    \n\t"
        "maq_s.w.phr    $ac0,       %[Temp2],   %[Temp2]    \n\t"
        "maq_s.w.phr    $ac0,       %[Temp3],   %[Temp3]    \n\t"
        "bne            %[ptr],     %[ptr_end], 1b          \n\t"
        " maq_s.w.phr   $ac0,       %[Temp4],   %[Temp4]    \n\t"
        "extr.w         %[val],     $ac0,       16          \n\t"

        ".set pop                                           \n\t"

        : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
          [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
          [ptr]   "+r"  (ptr),   [val]   "=r"  (val)
        : [ptr_end] "r" (ptr_end)
        : "hi", "lo", "memory"
    );

    h_shift = 0;

    if ((nb_pulse >= 12) && (val > 1024))
    {
        h_shift = 1;
    }
    p0 = H;
    p1 = h;
    p2 = h_inv;
    ptr_end = p0 + L_SUBFR;

    if (h_shift > 0) {
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

          "1:                                                   \n\t"
            "ulw        %[Temp1],       0(%[p0])                \n\t"
            "ulw        %[Temp2],       4(%[p0])                \n\t"
            "ulw        %[Temp3],       8(%[p0])                \n\t"
            "ulw        %[Temp4],       12(%[p0])               \n\t"
            "addiu      %[p0],          %[p0],      16          \n\t"
            "shrav.ph   %[Temp1],       %[Temp1],   %[h_shift]  \n\t"
            "shrav.ph   %[Temp2],       %[Temp2],   %[h_shift]  \n\t"
            "shrav.ph   %[Temp3],       %[Temp3],   %[h_shift]  \n\t"
            "shrav.ph   %[Temp4],       %[Temp4],   %[h_shift]  \n\t"
            "subq.ph    %[Temp5],       $0,         %[Temp1]    \n\t"
            "subq.ph    %[Temp6],       $0,         %[Temp2]    \n\t"
            "subq.ph    %[Temp7],       $0,         %[Temp3]    \n\t"
            "subq.ph    %[Temp8],       $0,         %[Temp4]    \n\t"
            "usw        %[Temp1],       0(%[p1])                \n\t"
            "usw        %[Temp2],       4(%[p1])                \n\t"
            "usw        %[Temp3],       8(%[p1])                \n\t"
            "usw        %[Temp4],       12(%[p1])               \n\t"
            "usw        %[Temp5],       0(%[p2])                \n\t"
            "usw        %[Temp6],       4(%[p2])                \n\t"
            "usw        %[Temp7],       8(%[p2])                \n\t"
            "usw        %[Temp8],       12(%[p2])               \n\t"
            "addiu      %[p1],          %[p1],      16          \n\t"
            "bne        %[p0],          %[ptr_end], 1b          \n\t"
            " addiu     %[p2],          %[p2],      16          \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [p0]    "+r"  (p0),    [p1]    "+r"  (p1),
              [p2]    "+r"  (p2)
            : [ptr_end] "r" (ptr_end), [h_shift] "r" (h_shift)
            : "memory"
        );
    } else {
        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

          "1:                                                   \n\t"
            "ulw        %[Temp1],       0(%[p0])                \n\t"
            "ulw        %[Temp2],       4(%[p0])                \n\t"
            "ulw        %[Temp3],       8(%[p0])                \n\t"
            "ulw        %[Temp4],       12(%[p0])               \n\t"
            "addiu      %[p0],          %[p0],      16          \n\t"
            "subq.ph    %[Temp5],       $0,         %[Temp1]    \n\t"
            "subq.ph    %[Temp6],       $0,         %[Temp2]    \n\t"
            "subq.ph    %[Temp7],       $0,         %[Temp3]    \n\t"
            "subq.ph    %[Temp8],       $0,         %[Temp4]    \n\t"
            "usw        %[Temp1],       0(%[p1])                \n\t"
            "usw        %[Temp2],       4(%[p1])                \n\t"
            "usw        %[Temp3],       8(%[p1])                \n\t"
            "usw        %[Temp4],       12(%[p1])               \n\t"
            "usw        %[Temp5],       0(%[p2])                \n\t"
            "usw        %[Temp6],       4(%[p2])                \n\t"
            "usw        %[Temp7],       8(%[p2])                \n\t"
            "usw        %[Temp8],       12(%[p2])               \n\t"
            "addiu      %[p1],          %[p1],      16          \n\t"
            "bne        %[p0],          %[ptr_end], 1b          \n\t"
            " addiu     %[p2],          %[p2],      16          \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
              [p0]    "+r"  (p0),    [p1]    "+r"  (p1),
              [p2]    "+r"  (p2)
            : [ptr_end] "r" (ptr_end), [h_shift] "r" (h_shift)
            : "memory"
        );
    }

    /*------------------------------------------------------------*
    * Compute rrixix[][] needed for the codebook search.         *
    * This algorithm compute impulse response energy of all      *
    * positions (16) in each track (4).       Total = 4x16 = 64. *
    *------------------------------------------------------------*/

    /* storage order --> i3i3, i2i2, i1i1, i0i0 */

    /* Init pointers to last position of rrixix[] */
    p0 = &rrixix[0][NB_POS - 4];
    p1 = &rrixix[1][NB_POS - 4];
    p2 = &rrixix[2][NB_POS - 4];
    p3 = &rrixix[3][NB_POS - 4];

    ptr_h1 = h;
    ptr_end = ptr_h1 + 4 * NB_POS;

    __asm__ volatile (
        ".set push                                          \n\t"
        ".set noreorder                                     \n\t"

        "li             %[cor],     0x8000                  \n\t"
      "1:                                                   \n\t"
        "lh             %[Temp1],   0(%[ptr_h1])            \n\t"
        "lh             %[Temp2],   2(%[ptr_h1])            \n\t"
        "lh             %[Temp3],   4(%[ptr_h1])            \n\t"
        "lh             %[Temp4],   6(%[ptr_h1])            \n\t"
        "muleq_s.w.phr  %[Temp1],   %[Temp1],   %[Temp1]    \n\t"
        "muleq_s.w.phr  %[Temp2],   %[Temp2],   %[Temp2]    \n\t"
        "muleq_s.w.phr  %[Temp3],   %[Temp3],   %[Temp3]    \n\t"
        "muleq_s.w.phr  %[Temp4],   %[Temp4],   %[Temp4]    \n\t"
        "lh             %[Temp5],   8(%[ptr_h1])            \n\t"
        "lh             %[Temp6],   10(%[ptr_h1])           \n\t"
        "lh             %[Temp7],   12(%[ptr_h1])           \n\t"
        "lh             %[Temp8],   14(%[ptr_h1])           \n\t"
        "muleq_s.w.phr  %[Temp5],   %[Temp5],   %[Temp5]    \n\t"
        "muleq_s.w.phr  %[Temp6],   %[Temp6],   %[Temp6]    \n\t"
        "muleq_s.w.phr  %[Temp7],   %[Temp7],   %[Temp7]    \n\t"
        "muleq_s.w.phr  %[Temp8],   %[Temp8],   %[Temp8]    \n\t"
        "addu           %[Temp1],   %[cor],     %[Temp1]    \n\t"
        "addu           %[Temp2],   %[Temp1],   %[Temp2]    \n\t"
        "addu           %[Temp3],   %[Temp2],   %[Temp3]    \n\t"
        "addu           %[Temp4],   %[Temp3],   %[Temp4]    \n\t"
        "addu           %[Temp5],   %[Temp4],   %[Temp5]    \n\t"
        "addu           %[Temp6],   %[Temp5],   %[Temp6]    \n\t"
        "addu           %[Temp7],   %[Temp6],   %[Temp7]    \n\t"
        "addu           %[cor],     %[Temp7],   %[Temp8]    \n\t"
        "precrq.ph.w    %[Temp1],   %[Temp1],   %[Temp5]    \n\t"
        "precrq.ph.w    %[Temp2],   %[Temp2],   %[Temp6]    \n\t"
        "precrq.ph.w    %[Temp3],   %[Temp3],   %[Temp7]    \n\t"
        "precrq.ph.w    %[Temp4],   %[Temp4],   %[cor]      \n\t"
        "usw            %[Temp1],   4(%[p3])                \n\t"
        "usw            %[Temp2],   4(%[p2])                \n\t"
        "usw            %[Temp3],   4(%[p1])                \n\t"
        "usw            %[Temp4],   4(%[p0])                \n\t"
        "addiu          %[ptr_h1],  %[ptr_h1],  16          \n\t"
        "addiu          %[p3],      %[p3],      -4          \n\t"
        "addiu          %[p2],      %[p2],      -4          \n\t"
        "addiu          %[p1],      %[p1],      -4          \n\t"
        "bne            %[ptr_h1],  %[ptr_end], 1b          \n\t"
        " addiu         %[p0],      %[p0],      -4          \n\t"

        ".set pop                                           \n\t"

        : [Temp1] "=&r" (Temp1), [Temp2]  "=&r" (Temp2),
          [Temp3] "=&r" (Temp3), [Temp4]  "=&r" (Temp4),
          [Temp5] "=&r" (Temp5), [Temp6]  "=&r" (Temp6),
          [Temp7] "=&r" (Temp7), [Temp8]  "=&r" (Temp8),
          [cor]   "=&r" (cor),   [p0]     "+r"  (p0),
          [p1]    "+r"  (p1),    [p2]     "+r"  (p2),
          [p3]    "+r"  (p3),    [ptr_h1] "+r"  (ptr_h1)
        : [ptr_end] "r" (ptr_end)
        : "hi", "lo", "memory"
    );

    /*------------------------------------------------------------*
    * Compute rrixiy[][] needed for the codebook search.         *
    * This algorithm compute correlation between 2 pulses        *
    * (2 impulses responses) in 4 possible adjacents tracks.     *
    * (track 0-1, 1-2, 2-3 and 3-0).     Total = 4x16x16 = 1024. *
    *------------------------------------------------------------*/

    /* storage order --> i2i3, i1i2, i0i1, i3i0 */

    pos = MSIZE - 1;
    ptr_hf = h + 1;

    for (k = 0; k < NB_POS; k+=2)
    {
        p3 = &rrixiy[2][pos];
        p2 = &rrixiy[1][pos];
        p1 = &rrixiy[0][pos];
        p0 = &rrixiy[3][pos - NB_POS];

        ptr_h1 = h;
        ptr_h2 = ptr_hf;
        ptr_end = ptr_h1 + 4 * (14-k);

      __asm__ volatile (
        ".set push                                          \n\t"
        ".set noreorder                                     \n\t"

        "li             %[cor],     0x8000                  \n\t"
        "beq            %[ptr_h1],  %[ptr_end], 2f          \n\t"
        " li            %[cor1],    0x8000                  \n\t"
      "1:                                                   \n\t"
        "lh             %[Temp1],   0(%[ptr_h1])            \n\t"
        "lh             %[Temp2],   0(%[ptr_h2])            \n\t"
        "lh             %[Temp3],   8(%[ptr_h2])            \n\t"
        "lh             %[Temp4],   2(%[ptr_h1])            \n\t"
        "muleq_s.w.phr  %[Temp2],   %[Temp1],   %[Temp2]    \n\t"
        "muleq_s.w.phr  %[Temp1],   %[Temp1],   %[Temp3]    \n\t"
        "lh             %[Temp5],   2(%[ptr_h2])            \n\t"
        "lh             %[Temp6],   10(%[ptr_h2])           \n\t"
        "lh             %[Temp7],   4(%[ptr_h1])            \n\t"
        "muleq_s.w.phr  %[Temp5],   %[Temp4],   %[Temp5]    \n\t"
        "muleq_s.w.phr  %[Temp4],   %[Temp4],   %[Temp6]    \n\t"
        "lh             %[Temp8],   4(%[ptr_h2])            \n\t"
        "lh             %[Temp9],   12(%[ptr_h2])           \n\t"
        "lh             %[Temp3],   6(%[ptr_h1])            \n\t"
        "muleq_s.w.phr  %[Temp8],   %[Temp7],   %[Temp8]    \n\t"
        "muleq_s.w.phr  %[Temp7],   %[Temp7],   %[Temp9]    \n\t"
        "lh             %[Temp6],   6(%[ptr_h2])            \n\t"
        "lh             %[Temp9],   14(%[ptr_h2])           \n\t"
        "addiu          %[ptr_h1],  %[ptr_h1],  8           \n\t"
        "muleq_s.w.phr  %[Temp6],   %[Temp3],   %[Temp6]    \n\t"
        "muleq_s.w.phr  %[Temp3],   %[Temp3],   %[Temp9]    \n\t"
        "addiu          %[ptr_h2],  %[ptr_h2],  8           \n\t"
        "addu           %[cor],     %[cor],     %[Temp2]    \n\t"
        "addu           %[cor1],    %[cor1],    %[Temp1]    \n\t"
        "packrl.ph      %[Temp2],   $0,         %[cor]      \n\t"
        "packrl.ph      %[Temp1],   $0,         %[cor1]     \n\t"
        "addu           %[cor],     %[cor],     %[Temp5]    \n\t"
        "addu           %[cor1],    %[cor1],    %[Temp4]    \n\t"
        "packrl.ph      %[Temp5],   $0,         %[cor]      \n\t"
        "packrl.ph      %[Temp4],   $0,         %[cor1]     \n\t"
        "addu           %[cor],     %[cor],     %[Temp8]    \n\t"
        "addu           %[cor1],    %[cor1],    %[Temp7]    \n\t"
        "packrl.ph      %[Temp8],   $0,         %[cor]      \n\t"
        "packrl.ph      %[Temp7],   $0,         %[cor1]     \n\t"
        "addu           %[cor],     %[cor],     %[Temp6]    \n\t"
        "addu           %[cor1],    %[cor1],    %[Temp3]    \n\t"
        "packrl.ph      %[Temp6],   $0,         %[cor]      \n\t"
        "packrl.ph      %[Temp3],   $0,         %[cor1]     \n\t"
        "sh             %[Temp2],   0(%[p3])                \n\t"
        "sh             %[Temp1],   -32(%[p3])              \n\t"
        "sh             %[Temp5],   0(%[p2])                \n\t"
        "sh             %[Temp4],   -32(%[p2])              \n\t"
        "sh             %[Temp8],   0(%[p1])                \n\t"
        "sh             %[Temp7],   -32(%[p1])              \n\t"
        "sh             %[Temp6],   0(%[p0])                \n\t"
        "sh             %[Temp3],   -32(%[p0])              \n\t"
        "addiu          %[p3],      %[p3],      -34         \n\t"
        "addiu          %[p2],      %[p2],      -34         \n\t"
        "addiu          %[p1],      %[p1],      -34         \n\t"
        "bne            %[ptr_h1],  %[ptr_end], 1b          \n\t"
        " addiu         %[p0],      %[p0],      -34         \n\t"
      "2:                                                   \n\t"
        "lh             %[Temp1],   0(%[ptr_h1])            \n\t"
        "lh             %[Temp2],   0(%[ptr_h2])            \n\t"
        "lh             %[Temp3],   8(%[ptr_h2])            \n\t"
        "lh             %[Temp4],   2(%[ptr_h1])            \n\t"
        "muleq_s.w.phr  %[Temp2],   %[Temp1],   %[Temp2]    \n\t"
        "muleq_s.w.phr  %[Temp1],   %[Temp1],   %[Temp3]    \n\t"
        "lh             %[Temp5],   2(%[ptr_h2])            \n\t"
        "lh             %[Temp6],   10(%[ptr_h2])           \n\t"
        "lh             %[Temp7],   4(%[ptr_h1])            \n\t"
        "muleq_s.w.phr  %[Temp5],   %[Temp4],   %[Temp5]    \n\t"
        "muleq_s.w.phr  %[Temp4],   %[Temp4],   %[Temp6]    \n\t"
        "lh             %[Temp8],   4(%[ptr_h2])            \n\t"
        "lh             %[Temp9],   12(%[ptr_h2])           \n\t"
        "lh             %[Temp3],   6(%[ptr_h1])            \n\t"
        "muleq_s.w.phr  %[Temp8],   %[Temp7],   %[Temp8]    \n\t"
        "muleq_s.w.phr  %[Temp7],   %[Temp7],   %[Temp9]    \n\t"
        "lh             %[Temp9],   6(%[ptr_h2])            \n\t"
        "addu           %[cor],     %[cor],     %[Temp2]    \n\t"
        "addu           %[cor1],    %[cor1],    %[Temp1]    \n\t"
        "muleq_s.w.phr  %[Temp3],   %[Temp3],   %[Temp9]    \n\t"
        "sra            %[Temp2],   %[cor],     16          \n\t"
        "sra            %[Temp1],   %[cor1],    16          \n\t"
        "sh             %[Temp2],   0(%[p3])                \n\t"
        "sh             %[Temp1],   -32(%[p3])              \n\t"
        "lh             %[Temp6],   8(%[ptr_h1])            \n\t"
        "addu           %[cor],     %[cor],     %[Temp5]    \n\t"
        "addu           %[cor1],    %[cor1],    %[Temp4]    \n\t"
        "lh             %[Temp1],   8(%[ptr_h2])            \n\t"
        "sra            %[Temp5],   %[cor],     16          \n\t"
        "sra            %[Temp4],   %[cor1],    16          \n\t"
        "muleq_s.w.phr  %[Temp1],   %[Temp1],   %[Temp6]    \n\t"
        "lh             %[Temp2],   10(%[ptr_h1])           \n\t"
        "lh             %[Temp6],   10(%[ptr_h2])           \n\t"
        "sh             %[Temp5],   0(%[p2])                \n\t"
        "sh             %[Temp4],   -32(%[p2])              \n\t"
        "muleq_s.w.phr  %[Temp2],   %[Temp2],   %[Temp6]    \n\t"
        "addu           %[cor],     %[cor],     %[Temp8]    \n\t"
        "addu           %[cor1],    %[cor1],    %[Temp7]    \n\t"
        "lh             %[Temp4],   12(%[ptr_h1])           \n\t"
        "lh             %[Temp6],   12(%[ptr_h2])           \n\t"
        "sra            %[Temp8],   %[cor],     16          \n\t"
        "sra            %[Temp7],   %[cor1],    16          \n\t"
        "muleq_s.w.phr  %[Temp4],   %[Temp4],   %[Temp6]    \n\t"
        "sh             %[Temp8],   0(%[p1])                \n\t"
        "sh             %[Temp7],   -32(%[p1])              \n\t"
        "addu           %[cor],     %[cor],     %[Temp3]    \n\t"
        "sra            %[Temp3],   %[cor],     16          \n\t"
        "sh             %[Temp3],   0(%[p0])                \n\t"
        "addu           %[cor],     %[cor],     %[Temp1]    \n\t"
        "sra            %[Temp1],   %[cor],     16          \n\t"
        "addu           %[cor],     %[cor],     %[Temp2]    \n\t"
        "sra            %[Temp2],   %[cor],     16          \n\t"
        "addu           %[cor],     %[cor],     %[Temp4]    \n\t"
        "sra            %[Temp4],   %[cor],     16          \n\t"
        "sh             %[Temp1],   -34(%[p3])              \n\t"
        "sh             %[Temp2],   -34(%[p2])              \n\t"
        "sh             %[Temp4],   -34(%[p1])              \n\t"

        ".set pop                                           \n\t"

        : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
          [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
          [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
          [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
          [Temp9]  "=&r" (Temp9),  [cor]    "=&r" (cor),
          [cor1]   "=&r" (cor1),   [ptr_h1] "+r"  (ptr_h1),
          [ptr_h2] "+r"  (ptr_h2), [p0]     "+r"  (p0),
          [p1]     "+r"  (p1),     [p2]     "+r"  (p2),
          [p3]     "+r"  (p3)
        : [ptr_end] "r" (ptr_end)
        : "hi", "lo", "memory"
      );

        pos -= 2*NB_POS;
        ptr_hf += 2 * STEP;
    }

    /* storage order --> i3i0, i2i3, i1i2, i0i1 */

    pos = MSIZE - 1;
    ptr_hf = h + 3;

    for (k = 0; k < NB_POS; k+=2)
    {
        p3 = &rrixiy[3][pos];
        p2 = &rrixiy[2][pos - 1];
        p1 = &rrixiy[1][pos - 1];
        p0 = &rrixiy[0][pos - 1];

        ptr_h1 = h;
        ptr_h2 = ptr_hf;
        ptr_end = ptr_h1 + 4*(14-k);

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "li             %[cor],     0x8000                  \n\t"
            "beq            %[ptr_h1],  %[ptr_end], 1f          \n\t"
            " li            %[cor1],    0x8000                  \n\t"
          "2:                                                   \n\t"
            "lh             %[Temp1],   0(%[ptr_h1])            \n\t"
            "lh             %[Temp2],   0(%[ptr_h2])            \n\t"
            "lh             %[Temp3],   8(%[ptr_h2])            \n\t"
            "lh             %[Temp4],   2(%[ptr_h1])            \n\t"
            "muleq_s.w.phr  %[Temp2],   %[Temp1],   %[Temp2]    \n\t"
            "muleq_s.w.phr  %[Temp1],   %[Temp1],   %[Temp3]    \n\t"
            "lh             %[Temp5],   2(%[ptr_h2])            \n\t"
            "lh             %[Temp6],   10(%[ptr_h2])           \n\t"
            "lh             %[Temp7],   4(%[ptr_h1])            \n\t"
            "muleq_s.w.phr  %[Temp5],   %[Temp4],   %[Temp5]    \n\t"
            "muleq_s.w.phr  %[Temp4],   %[Temp4],   %[Temp6]    \n\t"
            "lh             %[Temp8],   4(%[ptr_h2])            \n\t"
            "lh             %[Temp9],   12(%[ptr_h2])           \n\t"
            "lh             %[Temp3],   6(%[ptr_h1])            \n\t"
            "muleq_s.w.phr  %[Temp8],   %[Temp7],   %[Temp8]    \n\t"
            "muleq_s.w.phr  %[Temp7],   %[Temp7],   %[Temp9]    \n\t"
            "lh             %[Temp6],   6(%[ptr_h2])            \n\t"
            "lh             %[Temp9],   14(%[ptr_h2])           \n\t"
            "addiu          %[ptr_h1],  %[ptr_h1],  8           \n\t"
            "muleq_s.w.phr  %[Temp6],   %[Temp3],   %[Temp6]    \n\t"
            "muleq_s.w.phr  %[Temp3],   %[Temp3],   %[Temp9]    \n\t"
            "addiu          %[ptr_h2],  %[ptr_h2],  8           \n\t"
            "addu           %[Temp2],   %[cor],     %[Temp2]    \n\t"
            "addu           %[Temp1],   %[cor1],    %[Temp1]    \n\t"
            "addu           %[Temp5],   %[Temp5],   %[Temp2]    \n\t"
            "addu           %[Temp4],   %[Temp4],   %[Temp1]    \n\t"
            "addu           %[Temp8],   %[Temp8],   %[Temp5]    \n\t"
            "addu           %[Temp7],   %[Temp7],   %[Temp4]    \n\t"
            "addu           %[cor],     %[Temp8],   %[Temp6]    \n\t"
            "addu           %[cor1],    %[Temp7],   %[Temp3]    \n\t"
            "precrq.ph.w    %[Temp1],   %[Temp2],   %[Temp1]    \n\t"
            "precrq.ph.w    %[Temp2],   %[Temp5],   %[Temp4]    \n\t"
            "precrq.ph.w    %[Temp3],   %[Temp8],   %[Temp7]    \n\t"
            "precrq.ph.w    %[Temp4],   %[cor],     %[cor1]     \n\t"
            "usw            %[Temp1],   -2(%[p3])               \n\t"
            "usw            %[Temp2],   -2(%[p2])               \n\t"
            "usw            %[Temp3],   -2(%[p1])               \n\t"
            "usw            %[Temp4],   -2(%[p0])               \n\t"
            "addiu          %[p3],      %[p3],      -34         \n\t"
            "addiu          %[p2],      %[p2],      -34         \n\t"
            "addiu          %[p1],      %[p1],      -34         \n\t"
            "bne            %[ptr_h1],  %[ptr_end], 2b          \n\t"
            " addiu         %[p0],      %[p0],      -34         \n\t"
          "1:                                                   \n\t"
            "lh             %[Temp1],   0(%[ptr_h1])            \n\t"
            "lh             %[Temp2],   0(%[ptr_h2])            \n\t"
            "lh             %[Temp3],   8(%[ptr_h2])            \n\t"
            "lh             %[Temp4],   2(%[ptr_h1])            \n\t"
            "muleq_s.w.phr  %[Temp2],   %[Temp1],   %[Temp2]    \n\t"
            "muleq_s.w.phr  %[Temp1],   %[Temp1],   %[Temp3]    \n\t"
            "lh             %[Temp3],   2(%[ptr_h2])            \n\t"
            "lh             %[Temp5],   4(%[ptr_h1])            \n\t"
            "lh             %[Temp6],   4(%[ptr_h2])            \n\t"
            "muleq_s.w.phr  %[Temp3],   %[Temp3],   %[Temp4]    \n\t"
            "lh             %[Temp4],   6(%[ptr_h1])            \n\t"
            "muleq_s.w.phr  %[Temp5],   %[Temp5],   %[Temp6]    \n\t"
            "lh             %[Temp6],   6(%[ptr_h2])            \n\t"
            "lh             %[Temp7],   8(%[ptr_h1])            \n\t"
            "lh             %[Temp8],   8(%[ptr_h2])            \n\t"
            "muleq_s.w.phr  %[Temp4],   %[Temp4],   %[Temp6]    \n\t"
            "addu           %[cor],     %[cor],     %[Temp2]    \n\t"
            "muleq_s.w.phr  %[Temp6],   %[Temp7],   %[Temp8]    \n\t"
            "addu           %[cor1],    %[cor1],    %[Temp1]    \n\t"
            "precrq.ph.w    %[Temp1],   %[cor],     %[cor1]     \n\t"
            "usw            %[Temp1],   -2(%[p3])               \n\t"
            "addu           %[cor],     %[cor],     %[Temp3]    \n\t"
            "sra            %[Temp3],   %[cor],     16          \n\t"
            "addu           %[cor],     %[cor],     %[Temp5]    \n\t"
            "sra            %[Temp5],   %[cor],     16          \n\t"
            "addu           %[cor],     %[cor],     %[Temp4]    \n\t"
            "sra            %[Temp4],   %[cor],     16          \n\t"
            "addu           %[cor],     %[cor],     %[Temp6]    \n\t"
            "sra            %[Temp6],   %[cor],     16          \n\t"
            "sh             %[Temp3],   0(%[p2])                \n\t"
            "sh             %[Temp5],   0(%[p1])                \n\t"
            "sh             %[Temp4],   0(%[p0])                \n\t"
            "sh             %[Temp6],   -34(%[p3])              \n\t"

            ".set pop                                           \n\t"

            : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
              [Temp9]  "=&r" (Temp9),  [cor]    "=&r" (cor),
              [cor1]   "=&r" (cor1),   [ptr_h1] "+r"  (ptr_h1),
              [ptr_h2] "+r"  (ptr_h2), [p0]     "+r"  (p0),
              [p1]     "+r"  (p1),     [p2]     "+r"  (p2),
              [p3]     "+r"  (p3)
            : [ptr_end] "r" (ptr_end)
            : "hi", "lo", "memory"
        );

        pos -= 2;
        ptr_hf += 2*STEP;
    }

    /*------------------------------------------------------------*
    * Modification of rrixiy[][] to take signs into account.     *
    *------------------------------------------------------------*/

    p0 = &rrixiy[0][0];

    for (k = 0; k < NB_TRACK; k++)
    {
        j_temp = (k + 1)&0x03;
        for (i = k; i < L_SUBFR; i += STEP)
        {
            psign = sign;
            if (psign[i] < 0)
            {
                psign = vec;
            }
            j = j_temp;
            for (; j < L_SUBFR; j += STEP)
            {
                *p0 = vo_mult(*p0, psign[j]);
                p0++;
            }
        }
    }

    /*-------------------------------------------------------------------*
    *                       Deep first search                           *
    *-------------------------------------------------------------------*/

    psk = -1;
    alpk = 1;

    for (k = 0; k < nbiter; k++)
    {
        j_temp = k<<2;
        for (i = 0; i < nb_pulse; i++)
            ipos[i] = tipos[j_temp + i];

        if(nbbits == 20)
        {
            pos = 0;
            ps = 0;
            alp = 0;
            for (i = 0; i < L_SUBFR; i++)
            {
                vec[i] = 0;
            }
        } else if ((nbbits == 36) || (nbbits == 44))
        {
            /* first stage: fix 2 pulses */
            pos = 2;

            ix = ind[0] = pos_max[ipos[0]];
            iy = ind[1] = pos_max[ipos[1]];
            ps = dn[ix] + dn[iy];
            i = ix >> 2;                /* ix / STEP */
            j = iy >> 2;                /* iy / STEP */
            s = rrixix[ipos[0]][i] << 13;
            s += rrixix[ipos[1]][j] << 13;
            i = (i << 4) + j;         /* (ix/STEP)*NB_POS + (iy/STEP) */
            s += rrixiy[ipos[0]][i] << 14;
            alp = (s + 0x8000) >> 16;
            if (sign[ix] < 0)
                p0 = h_inv - ix;
            else
                p0 = h - ix;
            if (sign[iy] < 0)
                p1 = h_inv - iy;
            else
                p1 = h - iy;

            ptr = vec;
            ptr_end = p0 + L_SUBFR;

            __asm__ volatile (
                ".set push                                      \n\t"
                ".set noreorder                                 \n\t"

              "1:                                               \n\t"
                "ulw        %[Temp1],   0(%[p0])                \n\t"
                "ulw        %[Temp2],   0(%[p1])                \n\t"
                "ulw        %[Temp3],   4(%[p0])                \n\t"
                "ulw        %[Temp4],   4(%[p1])                \n\t"
                "ulw        %[Temp5],   8(%[p0])                \n\t"
                "ulw        %[Temp6],   8(%[p1])                \n\t"
                "ulw        %[Temp7],   12(%[p0])               \n\t"
                "ulw        %[Temp8],   12(%[p1])               \n\t"
                "addiu      %[p0],      %[p0],      16          \n\t"
                "addiu      %[p1],      %[p1],      16          \n\t"
                "addq.ph    %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
                "addq.ph    %[Temp3],   %[Temp3],   %[Temp4]    \n\t"
                "addq.ph    %[Temp5],   %[Temp5],   %[Temp6]    \n\t"
                "addq.ph    %[Temp7],   %[Temp7],   %[Temp8]    \n\t"
                "usw        %[Temp1],   0(%[ptr])               \n\t"
                "usw        %[Temp3],   4(%[ptr])               \n\t"
                "usw        %[Temp5],   8(%[ptr])               \n\t"
                "usw        %[Temp7],   12(%[ptr])              \n\t"
                "bne        %[p0],      %[ptr_end], 1b          \n\t"
                " addiu     %[ptr],     %[ptr],     16          \n\t"

                ".set pop                                       \n\t"

                : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
                  [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
                  [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
                  [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
                  [ptr]   "+r"  (ptr),   [p0]    "+r"  (p0),
                  [p1]    "+r"  (p1)
                : [ptr_end] "r" (ptr_end)
                : "memory"
            );

            if(nbbits == 44)
            {
                ipos[8] = 0;
                ipos[9] = 1;
            }
        } else
        {
            /* first stage: fix 4 pulses */
            pos = 4;

            ix = ind[0] = pos_max[ipos[0]];
            iy = ind[1] = pos_max[ipos[1]];
            i = ind[2] = pos_max[ipos[2]];
            j = ind[3] = pos_max[ipos[3]];
            ps = add1(add1(add1(dn[ix], dn[iy]), dn[i]), dn[j]);

            if (sign[ix] < 0)
                p0 = h_inv - ix;
            else
                p0 = h - ix;

            if (sign[iy] < 0)
                p1 = h_inv - iy;
            else
                p1 = h - iy;

            if (sign[i] < 0)
                p2 = h_inv - i;
            else
                p2 = h - i;

            if (sign[j] < 0)
                p3 = h_inv - j;
            else
                p3 = h - j;

            ptr = vec;
            ptr_end = p0 + L_SUBFR;

            __asm__ volatile (
                ".set push                                          \n\t"
                ".set noreorder                                     \n\t"

                "mult           $ac0,       $0,         $0          \n\t"
              "1:                                                   \n\t"
                "ulw            %[Temp1],   0(%[p0])                \n\t"
                "ulw            %[Temp2],   0(%[p1])                \n\t"
                "ulw            %[Temp3],   0(%[p2])                \n\t"
                "ulw            %[Temp4],   0(%[p3])                \n\t"
                "ulw            %[Temp5],   4(%[p0])                \n\t"
                "ulw            %[Temp6],   4(%[p1])                \n\t"
                "ulw            %[Temp7],   4(%[p2])                \n\t"
                "ulw            %[Temp8],   4(%[p3])                \n\t"
                "addiu          %[p0],      %[p0],      8           \n\t"
                "addiu          %[p1],      %[p1],      8           \n\t"
                "addiu          %[p2],      %[p2],      8           \n\t"
                "addq.ph        %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
                "addq.ph        %[Temp5],   %[Temp5],   %[Temp6]    \n\t"
                "addq.ph        %[Temp3],   %[Temp3],   %[Temp4]    \n\t"
                "addq.ph        %[Temp7],   %[Temp7],   %[Temp8]    \n\t"
                "addq.ph        %[Temp1],   %[Temp1],   %[Temp3]    \n\t"
                "addq.ph        %[Temp5],   %[Temp5],   %[Temp7]    \n\t"
                "usw            %[Temp1],   0(%[ptr])               \n\t"
                "usw            %[Temp5],   4(%[ptr])               \n\t"
                "addiu          %[ptr],     %[ptr],     8           \n\t"
                "maq_s.w.phr    $ac0,       %[Temp1],   %[Temp1]    \n\t"
                "maq_s.w.phl    $ac0,       %[Temp1],   %[Temp1]    \n\t"
                "maq_s.w.phr    $ac0,       %[Temp5],   %[Temp5]    \n\t"
                "maq_s.w.phl    $ac0,       %[Temp5],   %[Temp5]    \n\t"
                "bne            %[p0],      %[ptr_end], 1b          \n\t"
                " addiu         %[p3],      %[p3],      8           \n\t"
                "mflo           %[L_tmp],   $ac0                    \n\t"

                ".set pop                                           \n\t"

                : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
                  [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
                  [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
                  [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
                  [L_tmp] "=r"  (L_tmp), [ptr]   "+r"  (ptr),
                  [p0]    "+r"  (p0),    [p1]    "+r"  (p1),
                  [p2]    "+r"  (p2),    [p3]    "+r"  (p3)
                : [ptr_end] "r" (ptr_end)
                : "hi", "lo", "memory"
            );

            alp = ((L_tmp >> 3) + 0x8000) >> 16;

            if(nbbits == 72)
            {
                ipos[16] = 0;
                ipos[17] = 1;
            }
        }

        /* other stages of 2 pulses */

        for (j = pos, st = 0; j < nb_pulse; j += 2, st++)
        {
            /*--------------------------------------------------*
            * Calculate correlation of all possible positions  *
            * of the next 2 pulses with previous fixed pulses. *
            * Each pulse can have 16 possible positions.       *
            *--------------------------------------------------*/
            if(ipos[j] == 3)
            {
                cor_h_vec_30_mips(h, vec, ipos[j], sign, rrixix, cor_x, cor_y);
            }
            else
            {
                cor_h_vec_012_mips(h, vec, ipos[j], sign, rrixix, cor_x, cor_y);
            }
            /*--------------------------------------------------*
            * Find best positions of 2 pulses.                 *
            *--------------------------------------------------*/
            search_ixiy_mips(nbpos[st], ipos[j], ipos[j + 1], &ps, &alp,
                    &ix, &iy, dn, dn2, cor_x, cor_y, rrixiy);

            ind[j] = ix;
            ind[j + 1] = iy;

            if (sign[ix] < 0)
                p0 = h_inv - ix;
            else
                p0 = h - ix;
            if (sign[iy] < 0)
                p1 = h_inv - iy;
            else
                p1 = h - iy;

            ptr = vec;
            ptr_end = p0 + L_SUBFR;

            __asm__ volatile (
                ".set push                                      \n\t"
                ".set noreorder                                 \n\t"

              "1:                                               \n\t"
                "ulw        %[Temp1],   0(%[ptr])               \n\t"
                "ulw        %[Temp2],   0(%[p0])                \n\t"
                "ulw        %[Temp3],   0(%[p1])                \n\t"
                "ulw        %[Temp4],   4(%[ptr])               \n\t"
                "ulw        %[Temp5],   4(%[p0])                \n\t"
                "ulw        %[Temp6],   4(%[p1])                \n\t"
                "addq.ph    %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
                "addq.ph    %[Temp4],   %[Temp4],   %[Temp5]    \n\t"
                "addq.ph    %[Temp1],   %[Temp1],   %[Temp3]    \n\t"
                "addq.ph    %[Temp4],   %[Temp4],   %[Temp6]    \n\t"
                "usw        %[Temp1],   0(%[ptr])               \n\t"
                "usw        %[Temp4],   4(%[ptr])               \n\t"
                "ulw        %[Temp1],   8(%[ptr])               \n\t"
                "ulw        %[Temp2],   8(%[p0])                \n\t"
                "ulw        %[Temp3],   8(%[p1])                \n\t"
                "ulw        %[Temp4],   12(%[ptr])              \n\t"
                "ulw        %[Temp5],   12(%[p0])               \n\t"
                "ulw        %[Temp6],   12(%[p1])               \n\t"
                "addiu      %[p0],      %[p0],      16          \n\t"
                "addiu      %[p1],      %[p1],      16          \n\t"
                "addq.ph    %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
                "addq.ph    %[Temp4],   %[Temp4],   %[Temp5]    \n\t"
                "addq.ph    %[Temp1],   %[Temp1],   %[Temp3]    \n\t"
                "addq.ph    %[Temp4],   %[Temp4],   %[Temp6]    \n\t"
                "usw        %[Temp1],   8(%[ptr])               \n\t"
                "usw        %[Temp4],   12(%[ptr])              \n\t"
                "bne        %[p0],      %[ptr_end], 1b          \n\t"
                " addiu     %[ptr],     %[ptr],     16          \n\t"

                ".set pop                                       \n\t"

                : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
                  [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
                  [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
                  [ptr]   "+r"  (ptr),   [p0]    "+r"  (p0),
                  [p1]    "+r"  (p1)
                : [ptr_end] "r" (ptr_end)
                : "memory"
            );
        }
        /* memorise the best codevector */
        ps = vo_mult(ps, ps);
        s = vo_L_msu(vo_L_mult(alpk, ps), psk, alp);
        if (s > 0)
        {
            psk = ps;
            alpk = alp;
            memcpy (codvec, ind, sizeof(Word16) * nb_pulse);
            memcpy (y, vec, sizeof(Word16) * L_SUBFR);
        }
    }
    /*-------------------------------------------------------------------*
    * Build the codeword, the filtered codeword and index of codevector.*
    *-------------------------------------------------------------------*/
    memset (ind, -1, sizeof(Word16) * NPMAXPT * NB_TRACK);
    memset (code, 0, sizeof(Word16) * L_SUBFR);
    ptr = y;
    ptr_end = ptr + L_SUBFR;

    while (ptr < ptr_end)
    {
        __asm__ volatile (
            ".set push                                  \n\t"
            ".set noreorder                             \n\t"

            "ulw        %[Temp1],   0(%[ptr])           \n\t"
            "ulw        %[Temp2],   4(%[ptr])           \n\t"
            "ulw        %[Temp3],   8(%[ptr])           \n\t"
            "ulw        %[Temp4],   12(%[ptr])          \n\t"
            "shra_r.ph  %[Temp1],   %[Temp1],   3       \n\t"
            "shra_r.ph  %[Temp2],   %[Temp2],   3       \n\t"
            "shra_r.ph  %[Temp3],   %[Temp3],   3       \n\t"
            "shra_r.ph  %[Temp4],   %[Temp4],   3       \n\t"
            "usw        %[Temp1],   0(%[ptr])           \n\t"
            "usw        %[Temp2],   4(%[ptr])           \n\t"
            "usw        %[Temp3],   8(%[ptr])           \n\t"
            "usw        %[Temp4],   12(%[ptr])          \n\t"
            "addiu      %[ptr],     %[ptr],     16      \n\t"

            ".set pop                                   \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
              [ptr]   "+r"  (ptr)
            : [ptr_end] "r" (ptr_end)
            : "memory"
        );
    }
    val = (512 >> h_shift);               /* codeword in Q9 format */
    for (k = 0; k < nb_pulse; k++)
    {
        i = codvec[k];                       /* read pulse position */
        j = sign[i];                         /* read sign           */
        index = i >> 2;                 /* index = pos of pulse (0..15) */
        track = (Word16) (i & 0x03);         /* track = i % NB_TRACK (0..3)  */

        if (j > 0)
        {
            code[i] += val;
            codvec[k] += 128;
        } else
        {
            code[i] -= val;
            index += NB_POS;
        }

        i = (Word16)((vo_L_mult(track, NPMAXPT) >> 1));

        while (ind[i] >= 0)
        {
            i += 1;
        }
        ind[i] = index;
    }

    k = 0;
    /* Build index of codevector */
    if(nbbits == 20)
    {
        for (track = 0; track < NB_TRACK; track++)
        {
            _index[track] = (Word16)(quant_1p_N1(ind[k], 4));
            k += NPMAXPT;
        }
    } else if(nbbits == 36)
    {
        for (track = 0; track < NB_TRACK; track++)
        {
            _index[track] = (Word16)(quant_2p_2N1(ind[k], ind[k + 1], 4));
            k += NPMAXPT;
        }
    } else if(nbbits == 44)
    {
        for (track = 0; track < NB_TRACK - 2; track++)
        {
            _index[track] = (Word16)(quant_3p_3N1(ind[k], ind[k + 1], ind[k + 2], 4));
            k += NPMAXPT;
        }
        for (track = 2; track < NB_TRACK; track++)
        {
            _index[track] = (Word16)(quant_2p_2N1(ind[k], ind[k + 1], 4));
            k += NPMAXPT;
        }
    } else if(nbbits == 52)
    {
        for (track = 0; track < NB_TRACK; track++)
        {
            _index[track] = (Word16)(quant_3p_3N1(ind[k], ind[k + 1], ind[k + 2], 4));
            k += NPMAXPT;
        }
    } else if(nbbits == 64)
    {
        for (track = 0; track < NB_TRACK; track++)
        {
            L_index = quant_4p_4N(&ind[k], 4);
            _index[track] = (Word16)((L_index >> 14) & 3);
            _index[track + NB_TRACK] = (Word16)(L_index & 0x3FFF);
            k += NPMAXPT;
        }
    } else if(nbbits == 72)
    {
        for (track = 0; track < NB_TRACK - 2; track++)
        {
            L_index = quant_5p_5N(&ind[k], 4);
            _index[track] = (Word16)((L_index >> 10) & 0x03FF);
            _index[track + NB_TRACK] = (Word16)(L_index & 0x03FF);
            k += NPMAXPT;
        }
        for (track = 2; track < NB_TRACK; track++)
        {
            L_index = quant_4p_4N(&ind[k], 4);
            _index[track] = (Word16)((L_index >> 14) & 3);
            _index[track + NB_TRACK] = (Word16)(L_index & 0x3FFF);
            k += NPMAXPT;
        }
    } else if(nbbits == 88)
    {
        for (track = 0; track < NB_TRACK; track++)
        {
            L_index = quant_6p_6N_2(&ind[k], 4);
            _index[track] = (Word16)((L_index >> 11) & 0x07FF);
            _index[track + NB_TRACK] = (Word16)(L_index & 0x07FF);
            k += NPMAXPT;
        }
    }
    return;
}
#endif /* defined(MIPS_DSP_R1_LE) */

/*-------------------------------------------------------------------*
 * Function  cor_h_vec_30_mips()                                     *
 * ~~~~~~~~~~~~~~~~~~~~~                                             *
 * Compute correlations of h[] with vec[] for the specified track.   *
 *-------------------------------------------------------------------*/
void cor_h_vec_30_mips(
        Word16 h[],                           /* (i) scaled impulse response                  */
        Word16 vec[],                         /* (i) scaled vector (/8) to correlate with h[] */
        Word16 track,                         /* (i) track to use                             */
        Word16 sign[],                        /* (i) sign vector                              */
        Word16 rrixix[][NB_POS],              /* (i) correlation of h[x] with h[x]            */
        Word16 cor_1[],                       /* (o) result of correlation (NB_POS elements)  */
        Word16 cor_2[]                        /* (o) result of correlation (NB_POS elements)  */
        )
{
#if defined(MIPS_DSP_R1_LE)
    Word32 i, j, pos;
    Word16 *p0, *p1, *p2, *p3, *cor_x, *cor_y, *p4;
    Word32 L_sum1,L_sum2;
    Word32 L_sum3,L_sum4;
    Word32 p10, p12, p14, p1m4, p1m2;
    Word32 p2m3, p2m1, p20, p22, p24;
#else /* MIPS_DSP_R1_LE */
    Word32 i, j, pos;
    Word16 *p0, *p1, *p2,*p3,*cor_x,*cor_y, *p4;
    Word32 L_sum1,L_sum2;
    Word32 L_sum3,L_sum4;
    Word32 L_sum11, L_sum21;
    Word32 L_sum31, L_sum41;
    Word16 p10, p11, p12, p13, p14, p1m2;
    Word16 p2m3, p2m2, p2m1, p20, p21, p22, p23, p24;
    Word32 tmp32 = 32768;
#endif /* MIPS_DSP_R1_LE */
    cor_x = cor_1;
    cor_y = cor_2;
    p0 = rrixix[track];
    p3 = rrixix[0];
    pos = track;

    for (i = 0; i < NB_POS; i+=2)
    {
#if defined(MIPS_DSP_R1_LE)
        p1 = h;
        p2 = &vec[pos];
        j = L_SUBFR - pos - 5;
        __asm__ volatile (
            "ulw            %[p10],     0(%[p1])                \n\t"
            "ulw            %[p12],     4(%[p1])                \n\t"
            "lh             %[p14],     8(%[p1])                \n\t"
            "ulw            %[p2m3],    -6(%[p2])               \n\t"
            "ulw            %[p2m1],    -2(%[p2])               \n\t"
            "ulw            %[p20],     0(%[p2])                \n\t"
            "ulw            %[p22],     4(%[p2])                \n\t"
            "lh             %[p24],     8(%[p2])                \n\t"
            "mult           $ac0,       $zero,      $zero       \n\t"
            "mult           $ac1,       $zero,      $zero       \n\t"
            "mult           $ac2,       $zero,      $zero       \n\t"
            "mult           $ac3,       $zero,      $zero       \n\t"
            "dpaq_s.w.ph    $ac1,       %[p10],     %[p2m3]     \n\t"
            "dpaq_s.w.ph    $ac0,       %[p10],     %[p20]      \n\t"
            "dpaq_s.w.ph    $ac1,       %[p12],     %[p2m1]     \n\t"
            "sra            %[p20],     %[p20],     16          \n\t"
            "dpaq_s.w.ph    $ac0,       %[p12],     %[p22]      \n\t"
            "maq_s.w.phr    $ac0,       %[p14],     %[p24]      \n\t"
            "maq_s.w.phr    $ac1,       %[p14],     %[p20]      \n\t"
            "maq_s.w.phr    $ac2,       %[p10],     %[p24]      \n\t"
            "maq_s.w.phr    $ac3,       %[p10],     %[p20]      \n\t"
            "addiu          %[p1],      %[p1],      10          \n\t"
            "addiu          %[p2],      %[p2],      10          \n\t"
            "beqz           %[j],       ali_lp%=                \n\t"

          "vAa_lp%=:                                            \n\t"
            "ulw            %[p1m4],    -8(%[p1])               \n\t"
            "ulw            %[p1m2],    -4(%[p1])               \n\t"
            "ulw            %[p10],     0(%[p1])                \n\t"
            "ulw            %[p12],     4(%[p1])                \n\t"
            "ulw            %[p2m3],    -6(%[p2])               \n\t"
            "ulw            %[p2m1],    -2(%[p2])               \n\t"
            "ulw            %[p20],     0(%[p2])                \n\t"
            "ulw            %[p22],     4(%[p2])                \n\t"
            "addiu          %[p1],      %[p1],      8           \n\t"
            "addiu          %[p2],      %[p2],      8           \n\t"
            "dpaq_s.w.ph    $ac1,       %[p10],     %[p2m3]     \n\t"
            "dpaq_s.w.ph    $ac0,       %[p10],     %[p20]      \n\t"
            "dpaq_s.w.ph    $ac3,       %[p1m4],    %[p2m3]     \n\t"
            "dpaq_s.w.ph    $ac2,       %[p1m4],    %[p20]      \n\t"
            "dpaq_s.w.ph    $ac1,       %[p12],     %[p2m1]     \n\t"
            "dpaq_s.w.ph    $ac0,       %[p12],     %[p22]      \n\t"
            "dpaq_s.w.ph    $ac3,       %[p1m2],    %[p2m1]     \n\t"
            "dpaq_s.w.ph    $ac2,       %[p1m2],    %[p22]      \n\t"
            "addiu          %[j],       %[j],       -4          \n\t"
            "bgtz           %[j],       vAa_lp%=                \n\t"

          "ali_lp%=:                                            \n\t"
            "addiu          %[p2],      %[p2],      -6          \n\t"
            "ulw            %[p1m4],    -8(%[p1])               \n\t"
            "lh             %[p1m2],    -4(%[p1])               \n\t"
            "ulw            %[p10],     0(%[p1])                \n\t"
            "lh             %[p12],     4(%[p1])                \n\t"
            "ulw            %[p20],     0(%[p2])                \n\t"
            "lh             %[p22],     4(%[p2])                \n\t"
            "dpaq_s.w.ph    $ac3,       %[p1m4],    %[p20]      \n\t"
            "maq_s.w.phr    $ac3,       %[p1m2],    %[p22]      \n\t"
            "dpaq_s.w.ph    $ac1,       %[p10],     %[p20]      \n\t"
            "maq_s.w.phr    $ac1,       %[p12],     %[p22]      \n\t"
            "extr_r.w       %[L_sum3],  $ac2,       15          \n\t"
            "extr_r.w       %[L_sum1],  $ac0,       15          \n\t"
            "extr_r.w       %[L_sum4],  $ac3,       15          \n\t"
            "extr_r.w       %[L_sum2],  $ac1,       15          \n\t"

            : [p10] "=&r" (p10), [p12] "=&r" (p12), [p2m3] "=&r" (p2m3),
              [p2m1] "=&r" (p2m1), [p20] "=&r" (p20), [p22] "=&r" (p22),
              [p14] "=&r" (p14), [p1] "+r" (p1), [p2] "+r" (p2),
              [p24] "=&r" (p24), [p1m2] "=&r" (p1m2),
              [p1m4] "=&r" (p1m4), [j] "+r" (j),
              [L_sum2] "=r" (L_sum2), [L_sum1] "=r" (L_sum1),
              [L_sum4] "=r" (L_sum4), [L_sum3] "=r" (L_sum3)
            :
            : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
              "$ac3hi", "$ac3lo", "memory"
        );

        p4 = &sign[pos];

        __asm__ volatile (
            "lh             %[p10],     -6(%[p4])               \n\t"
            "lh             %[p1m2],    0(%[p4])                \n\t"
            "lh             %[p12],     2(%[p4])                \n\t"
            "lh             %[p14],     8(%[p4])                \n\t"
            "lh             %[p20],     0(%[p0])                \n\t"
            "lh             %[p2m1],    2(%[p0])                \n\t"
            "lh             %[p22],     0(%[p3])                \n\t"
            "lh             %[p2m3],    2(%[p3])                \n\t"
            "mul            %[p1m2],    %[L_sum1],  %[p1m2]     \n\t"
            "mul            %[p14],     %[L_sum3],  %[p14]      \n\t"
            "mul            %[p10],     %[L_sum2],  %[p10]      \n\t"
            "mul            %[p12],     %[L_sum4],  %[p12]      \n\t"
            "addiu          %[p0],      %[p0],      4           \n\t"
            "addiu          %[p3],      %[p3],      4           \n\t"
            "sra            %[p1m2],    %[p1m2],    15          \n\t"
            "sra            %[p14],     %[p14],     15          \n\t"
            "sra            %[p10],     %[p10],     15          \n\t"
            "sra            %[p12],     %[p12],     15          \n\t"
            "addu           %[p1m2],    %[p20],     %[p1m2]     \n\t"
            "addu           %[p14],     %[p2m1],    %[p14]      \n\t"
            "addu           %[p10],     %[p22],     %[p10]      \n\t"
            "addu           %[p12],     %[p2m3],    %[p12]      \n\t"
            "sh             %[p1m2],    0(%[cor_x])             \n\t"
            "sh             %[p14],     2(%[cor_x])             \n\t"
            "sh             %[p10],     0(%[cor_y])             \n\t"
            "sh             %[p12],     2(%[cor_y])             \n\t"
            "addiu          %[cor_x],   %[cor_x],   4           \n\t"
            "addiu          %[cor_y],   %[cor_y],   4           \n\t"
            "addiu          %[pos],     %[pos],     8           \n\t"
            : [p10] "=&r" (p10), [p1m2] "=&r" (p1m2), [p12] "=&r" (p12),
              [p14] "=&r" (p14), [p20] "=&r" (p20), [p2m1] "=&r" (p2m1),
              [p22] "=&r" (p22), [p2m3] "=&r" (p2m3), [p0] "+r" (p0),
              [p3] "+r" (p3), [cor_x] "+r" (cor_x), [cor_y] "+r" (cor_y),
              [pos] "+r" (pos)
            : [p4] "r" (p4), [L_sum1] "r" (L_sum1), [L_sum2] "r" (L_sum2),
              [L_sum3] "r" (L_sum3), [L_sum4] "r" (L_sum4)
            : "memory", "hi", "lo"
        );
#else /* MIPS_DSP_R1_LE */
        p1 = h;
        p2 = &vec[pos];

        __asm__ volatile (
            "lh         %[p10],     0(%[p1])                \n\t"
            "lh         %[p11],     2(%[p1])                \n\t"
            "lh         %[p12],     4(%[p1])                \n\t"
            "lh         %[p13],     6(%[p1])                \n\t"
            "lh         %[p14],     8(%[p1])                \n\t"
            "lh         %[p2m3],    -6(%[p2])               \n\t"
            "lh         %[p2m2],    -4(%[p2])               \n\t"
            "lh         %[p2m1],    -2(%[p2])               \n\t"
            "lh         %[p20],     0(%[p2])                \n\t"
            "lh         %[p21],     2(%[p2])                \n\t"
            "lh         %[p24],     8(%[p2])                \n\t"
            "mult       %[p10],     %[p2m3]                 \n\t"
            "madd       %[p11],     %[p2m2]                 \n\t"
            "madd       %[p12],     %[p2m1]                 \n\t"
            "madd       %[p13],     %[p20]                  \n\t"
            "madd       %[p14],     %[p21]                  \n\t"
            "mul        %[L_sum4],  %[p10],     %[p21]      \n\t"
            "mul        %[L_sum3],  %[p10],     %[p24]      \n\t"
            "lh         %[p22],     4(%[p2])                \n\t"
            "lh         %[p23],     6(%[p2])                \n\t"
            "mflo       %[L_sum2]                           \n\t"
            "mult       %[p10],     %[p20]                  \n\t"
            "madd       %[p14],     %[p24]                  \n\t"
            "madd       %[p11],     %[p21]                  \n\t"
            "madd       %[p12],     %[p22]                  \n\t"
            "madd       %[p13],     %[p23]                  \n\t"
            "addiu      %[p1],      %[p1],      10          \n\t"
            "addiu      %[p2],      %[p2],      10          \n\t"
            "mflo       %[L_sum1]                           \n\t"
            : [p10] "=&r" (p10), [p11] "=&r" (p11), [p12] "=&r" (p12),
              [p13] "=&r" (p13), [p14] "=&r" (p14), [p2m3] "=&r" (p2m3),
              [p2m2] "=&r" (p2m2), [p2m1] "=&r" (p2m1), [p20] "=&r" (p20),
              [p21] "=&r" (p21), [p22] "=&r" (p22), [p23] "=&r" (p23),
              [p24] "=&r" (p24), [L_sum2] "=&r" (L_sum2),
              [p1] "+r" (p1), [p2] "+r" (p2), [L_sum3] "=&r" (L_sum3),
              [L_sum4] "=&r" (L_sum4), [L_sum1] "=r" (L_sum1)
            :
            : "hi", "lo", "memory"
        );

        for (j=pos+5;j < L_SUBFR; j+=4)
        {
            __asm__ volatile (
                "lh         %[p2m3],    -6(%[p2])           \n\t"
                "lh         %[p2m2],    -4(%[p2])           \n\t"
                "lh         %[p2m1],    -2(%[p2])           \n\t"
                "lh         %[p20],     0(%[p2])            \n\t"
                "lh         %[p10],     0(%[p1])            \n\t"
                "lh         %[p11],     2(%[p1])            \n\t"
                "lh         %[p12],     4(%[p1])            \n\t"
                "lh         %[p13],     6(%[p1])            \n\t"
                "mult       %[p10],     %[p2m3]             \n\t"
                "madd       %[p11],     %[p2m2]             \n\t"
                "madd       %[p12],     %[p2m1]             \n\t"
                "madd       %[p13],     %[p20]              \n\t"
                "lh         %[p21],     2(%[p2])            \n\t"
                "lh         %[p22],     4(%[p2])            \n\t"
                "lh         %[p23],     6(%[p2])            \n\t"
                "mflo       %[L_sum21]                      \n\t"
                "mult       %[p10],     %[p20]              \n\t"
                "madd       %[p11],     %[p21]              \n\t"
                "madd       %[p12],     %[p22]              \n\t"
                "madd       %[p13],     %[p23]              \n\t"
                "lh         %[p10],     -8(%[p1])           \n\t"
                "lh         %[p11],     -6(%[p1])           \n\t"
                "lh         %[p12],     -4(%[p1])           \n\t"
                "lh         %[p13],     -2(%[p1])           \n\t"
                "mflo       %[L_sum11]                      \n\t"
                "mult       %[p10],     %[p2m3]             \n\t"
                "madd       %[p11],     %[p2m2]             \n\t"
                "madd       %[p12],     %[p2m1]             \n\t"
                "madd       %[p13],     %[p20]              \n\t"
                "mflo       %[L_sum41]                      \n\t"
                "mult       %[p10],     %[p20]              \n\t"
                "madd       %[p11],     %[p21]              \n\t"
                "madd       %[p12],     %[p22]              \n\t"
                "madd       %[p13],     %[p23]              \n\t"
                "addiu      %[p1],      %[p1],      8       \n\t"
                "addiu      %[p2],      %[p2],      8       \n\t"
                "mflo       %[L_sum31]                      \n\t"
                : [p10] "=&r" (p10), [p11] "=&r" (p11), [p12] "=&r" (p12),
                  [p13] "=&r" (p13), [p2m3] "=&r" (p2m3),
                  [p2m2] "=&r" (p2m2), [p2m1] "=&r" (p2m1), [p20] "=&r" (p20),
                  [p21] "=&r" (p21), [p22] "=&r" (p22), [p23] "=&r" (p23),
                  [L_sum21] "=&r" (L_sum21), [L_sum11] "=&r" (L_sum11),
                  [p1] "+r" (p1), [p2] "+r" (p2), [L_sum31] "=r" (L_sum31),
                  [L_sum41] "=&r" (L_sum41)
                :
                : "hi", "lo", "memory"
            );

            L_sum2 += L_sum21;
            L_sum1 += L_sum11;
            L_sum4 += L_sum41;
            L_sum3 += L_sum31;
        }

        p2-=3;
        __asm__ volatile (
            "lh         %[p20],     0(%[p2])                \n\t"
            "lh         %[p21],     2(%[p2])                \n\t"
            "lh         %[p22],     4(%[p2])                \n\t"
            "lh         %[p10],     0(%[p1])                \n\t"
            "lh         %[p11],     2(%[p1])                \n\t"
            "lh         %[p12],     4(%[p1])                \n\t"
            "mult       %[p10],     %[p20]                  \n\t"
            "madd       %[p11],     %[p21]                  \n\t"
            "madd       %[p12],     %[p22]                  \n\t"
            "lh         %[p10],     -8(%[p1])               \n\t"
            "lh         %[p11],     -6(%[p1])               \n\t"
            "lh         %[p12],     -4(%[p1])               \n\t"
            "mflo       %[L_sum21]                          \n\t"
            "mult       %[p10],     %[p20]                  \n\t"
            "madd       %[p11],     %[p21]                  \n\t"
            "madd       %[p12],     %[p22]                  \n\t"
            "sll        %[L_sum1],  %[L_sum1],  2           \n\t"
            "sll        %[L_sum3],  %[L_sum3],  2           \n\t"
            "addu       %[L_sum2],  %[L_sum21], %[L_sum2]   \n\t"
            "addu       %[L_sum1],  %[L_sum1],  %[tmp32]    \n\t"
            "mflo       %[L_sum41]                          \n\t"
            "sll        %[L_sum2],  %[L_sum2],  2           \n\t"
            "sra        %[L_sum1],  %[L_sum1],  16          \n\t"
            "addu       %[L_sum3],  %[L_sum3],  %[tmp32]    \n\t"
            "addu       %[L_sum2],  %[L_sum2],  %[tmp32]    \n\t"
            "sra        %[L_sum3],  %[L_sum3],  16          \n\t"
            "sra        %[L_sum2],  %[L_sum2],  16          \n\t"
            "addu       %[L_sum4],  %[L_sum41], %[L_sum4]   \n\t"
            "sll        %[L_sum4],  %[L_sum4],  2           \n\t"
            "addu       %[L_sum4],  %[L_sum4],  %[tmp32]    \n\t"
            "sra        %[L_sum4],  %[L_sum4],  16          \n\t"
            : [p10] "=&r" (p10), [p11] "=&r" (p11), [p12] "=&r" (p12),
              [p20] "=&r" (p20), [p21] "=&r" (p21), [p22] "=&r" (p22),
              [L_sum21] "=&r" (L_sum21), [L_sum41] "=&r" (L_sum41),
              [L_sum1] "+r" (L_sum1), [L_sum3] "+r" (L_sum3),
              [L_sum4] "+r" (L_sum4), [L_sum2] "+r" (L_sum2)
            : [p1] "r" (p1), [p2] "r" (p2), [tmp32] "r" (tmp32)
            : "hi", "lo", "memory"
        );

        p4 = &sign[pos];
        __asm__ volatile (
            "lh         %[p10],     -6(%[p4])               \n\t"
            "lh         %[p1m2],    0(%[p4])                \n\t"
            "lh         %[p12],     2(%[p4])                \n\t"
            "lh         %[p14],     8(%[p4])                \n\t"
            "lh         %[p20],     0(%[p0])                \n\t"
            "lh         %[p2m1],    2(%[p0])                \n\t"
            "lh         %[p22],     0(%[p3])                \n\t"
            "lh         %[p2m3],    2(%[p3])                \n\t"
            "mul        %[p1m2],    %[L_sum1],  %[p1m2]     \n\t"
            "mul        %[p14],     %[L_sum3],  %[p14]      \n\t"
            "mul        %[p10],     %[L_sum2],  %[p10]      \n\t"
            "mul        %[p12],     %[L_sum4],  %[p12]      \n\t"
            "addiu      %[p0],      %[p0],      4           \n\t"
            "addiu      %[p3],      %[p3],      4           \n\t"
            "sra        %[p1m2],    %[p1m2],    15          \n\t"
            "sra        %[p14],     %[p14],     15          \n\t"
            "sra        %[p10],     %[p10],     15          \n\t"
            "sra        %[p12],     %[p12],     15          \n\t"
            "addu       %[p1m2],    %[p20],     %[p1m2]     \n\t"
            "addu       %[p14],     %[p2m1],    %[p14]      \n\t"
            "addu       %[p10],     %[p22],     %[p10]      \n\t"
            "addu       %[p12],     %[p2m3],    %[p12]      \n\t"
            "sh         %[p1m2],    0(%[cor_x])             \n\t"
            "sh         %[p14],     2(%[cor_x])             \n\t"
            "sh         %[p10],     0(%[cor_y])             \n\t"
            "sh         %[p12],     2(%[cor_y])             \n\t"
            "addiu      %[cor_x],   %[cor_x],   4           \n\t"
            "addiu      %[cor_y],   %[cor_y],   4           \n\t"
            "addiu      %[pos],     %[pos],     8           \n\t"
            : [p10] "=&r" (p10), [p1m2] "=&r" (p1m2), [p12] "=&r" (p12),
              [p14] "=&r" (p14), [p20] "=&r" (p20), [p2m1] "=&r" (p2m1),
              [p22] "=&r" (p22), [p2m3] "=&r" (p2m3), [p0] "+r" (p0),
              [p3] "+r" (p3), [cor_x] "+r" (cor_x), [cor_y] "+r" (cor_y),
              [pos] "+r" (pos)
            : [p4] "r" (p4), [L_sum1] "r" (L_sum1), [L_sum2] "r" (L_sum2),
              [L_sum3] "r" (L_sum3), [L_sum4] "r" (L_sum4)
            : "hi", "lo", "memory"
        );
#endif /* MIPS_DSP_R1_LE */
    }

    return;
}

void cor_h_vec_012_mips(
        Word16 h[],                           /* (i) scaled impulse response                 */
        Word16 vec[],                         /* (i) scaled vector (/8) to correlate with h[] */
        Word16 track,                         /* (i) track to use                            */
        Word16 sign[],                        /* (i) sign vector                             */
        Word16 rrixix[][NB_POS],              /* (i) correlation of h[x] with h[x]      */
        Word16 cor_1[],                       /* (o) result of correlation (NB_POS elements) */
        Word16 cor_2[]                        /* (o) result of correlation (NB_POS elements) */
        )
{
    Word32 i, pos;
    Word16 *p0, *p1, *p2,*p3,*cor_x,*cor_y;
    Word16 *end1, *end2;
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5,
            Temp6, Temp7, Temp8, Temp9, Temp10;
#if defined(MIPS_DSP_R2_LE)
    Word32 Temp11;
#endif /* defined(MIPS_DSP_R2_LE) */
#if !defined(MIPS_DSP_R1_LE)
    Word32 Temp11, Temp12, Temp13;
    Word32 L_sum1, L_sum2;
#else
    Word32 cond;
    Word16 *c_x, *c_y, *s;
#endif /* !defined(MIPS_DSP_R1_LE) */
    cor_x = cor_1;
    cor_y = cor_2;
    p0 = rrixix[track];
    p3 = rrixix[track+1];
    pos = track;
#if defined(MIPS_DSP_R2_LE)
    for (i = 0; i < NB_POS; i += 2)
    {
        p1 = h;
        p2 = &vec[pos];
        end1 = p1 + (((59 - pos) >> 2) << 2);
        end2 = end1 + ((59 - pos) & 3);
        c_x = &cor_x[i];
        c_y = &cor_y[i];
        s   = &sign[pos];

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "mult           $ac0,       $0,         $0          \n\t"
            "mult           $ac1,       $0,         $0          \n\t"
            "mult           $ac2,       $0,         $0          \n\t"
            "slt            %[cond],    %[p1],      %[end1]     \n\t"
            "beqz           %[cond],    2f                      \n\t"
            " mult          $ac3,       $0,         $0          \n\t"
          "1:                                                   \n\t"
            "ulw            %[Temp1],   0(%[p1])                \n\t"
            "ulw            %[Temp2],   4(%[p1])                \n\t"
            "ulw            %[Temp3],   0(%[p2])                \n\t"
            "ulw            %[Temp4],   4(%[p2])                \n\t"
            "ulw            %[Temp5],   8(%[p2])                \n\t"
            "ulw            %[Temp6],   12(%[p2])               \n\t"
            "lh             %[Temp7],   16(%[p2])               \n\t"
            "addiu          %[p1],      %[p1],      8           \n\t"
            "packrl.ph      %[Temp8],   %[Temp4],   %[Temp3]    \n\t"
            "packrl.ph      %[Temp9],   %[Temp5],   %[Temp4]    \n\t"
            "packrl.ph      %[Temp10],  %[Temp6],   %[Temp5]    \n\t"
            "packrl.ph      %[Temp11],  %[Temp7],   %[Temp6]    \n\t"
            "dpa.w.ph       $ac0,       %[Temp1],   %[Temp3]    \n\t"
            "dpa.w.ph       $ac0,       %[Temp2],   %[Temp4]    \n\t"
            "dpa.w.ph       $ac1,       %[Temp1],   %[Temp8]    \n\t"
            "dpa.w.ph       $ac1,       %[Temp2],   %[Temp9]    \n\t"
            "dpa.w.ph       $ac2,       %[Temp1],   %[Temp5]    \n\t"
            "dpa.w.ph       $ac2,       %[Temp2],   %[Temp6]    \n\t"
            "dpa.w.ph       $ac3,       %[Temp1],   %[Temp10]   \n\t"
            "dpa.w.ph       $ac3,       %[Temp2],   %[Temp11]   \n\t"
            "slt            %[cond],    %[p1],      %[end1]     \n\t"
            "bnez           %[cond],    1b                      \n\t"
            " addiu         %[p2],      %[p2],      8           \n\t"
          "2:                                                   \n\t"
            "slt            %[cond],    %[p1],      %[end2]     \n\t"
            "beqz           %[cond],    4f                      \n\t"
          "3:                                                   \n\t"
            "lh             %[Temp1],   0(%[p1])                \n\t"
            "lh             %[Temp2],   0(%[p2])                \n\t"
            "lh             %[Temp3],   2(%[p2])                \n\t"
            "lh             %[Temp4],   8(%[p2])                \n\t"
            "lh             %[Temp5],   10(%[p2])               \n\t"
            "madd           $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "madd           $ac1,       %[Temp1],   %[Temp3]    \n\t"
            "madd           $ac2,       %[Temp1],   %[Temp4]    \n\t"
            "madd           $ac3,       %[Temp1],   %[Temp5]    \n\t"
            "addiu          %[p1],      %[p1],      2           \n\t"
            "slt            %[cond],    %[p1],      %[end2]     \n\t"
            "bnez           %[cond],    3b                      \n\t"
            " addiu         %[p2],      %[p2],      2           \n\t"
          "4:                                                   \n\t"
            "ulw            %[Temp1],   0(%[p1])                \n\t"
            "ulw            %[Temp2],   0(%[p2])                \n\t"
            "ulw            %[Temp3],   4(%[p1])                \n\t"
            "ulw            %[Temp4],   4(%[p2])                \n\t"
            "lh             %[Temp5],   8(%[p1])                \n\t"
            "lh             %[Temp6],   8(%[p2])                \n\t"
            "seh            %[Temp7],   %[Temp1]                \n\t"
            "packrl.ph      %[Temp8],   %[Temp4],   %[Temp2]    \n\t"
            "packrl.ph      %[Temp9],   %[Temp6],   %[Temp4]    \n\t"
            "dpa.w.ph       $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "dpa.w.ph       $ac0,       %[Temp3],   %[Temp4]    \n\t"
            "madd           $ac0,       %[Temp5],   %[Temp6]    \n\t"
            "dpa.w.ph       $ac1,       %[Temp1],   %[Temp8]    \n\t"
            "dpa.w.ph       $ac1,       %[Temp3],   %[Temp9]    \n\t"
            "madd           $ac2,       %[Temp6],   %[Temp7]    \n\t"
            "extr_rs.w      %[Temp1],   $ac0,       14          \n\t"
            "extr_rs.w      %[Temp2],   $ac1,       14          \n\t"
            "extr_rs.w      %[Temp3],   $ac2,       14          \n\t"
            "extr_rs.w      %[Temp4],   $ac3,       14          \n\t"
            "ulw            %[Temp5],   0(%[p0])                \n\t"
            "ulw            %[Temp6],   0(%[p3])                \n\t"
            "ulw            %[Temp7],   0(%[s])                 \n\t"
            "ulw            %[Temp8],   8(%[s])                 \n\t"
            "append         %[Temp3],   %[Temp1],   16          \n\t"
            "append         %[Temp4],   %[Temp2],   16          \n\t"
            "addiu          %[p0],      %[p0],      4           \n\t"
            "addiu          %[p3],      %[p3],      4           \n\t"
            "precrq.ph.w    %[Temp9],   %[Temp8],   %[Temp7]    \n\t"
            "append         %[Temp8],   %[Temp7],   16          \n\t"
            "mulq_s.ph      %[Temp3],   %[Temp3],   %[Temp8]    \n\t"
            "mulq_s.ph      %[Temp4],   %[Temp4],   %[Temp9]    \n\t"
            "addq.ph        %[Temp3],   %[Temp3],   %[Temp5]    \n\t"
            "addq.ph        %[Temp4],   %[Temp4],   %[Temp6]    \n\t"
            "usw            %[Temp3],   0(%[c_x])               \n\t"
            "usw            %[Temp4],   0(%[c_y])               \n\t"

            ".set pop                                           \n\t"

            : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
              [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
              [Temp11] "=&r" (Temp11), [cond]   "=&r" (cond),
              [p0]     "+r"  (p0),     [p1]     "+r"  (p1),
              [p2]     "+r"  (p2),     [p3]     "+r"  (p3)
            : [end1] "r" (end1), [end2] "r" (end2),
              [c_x]  "r" (c_x),  [c_y]  "r" (c_y),
              [s]    "r" (s)
            : "hi", "lo", "$ac1hi", "$ac1lo",
              "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo",
              "memory"
        );

        pos += 2 * STEP;
    }
#elif defined(MIPS_DSP_R1_LE)
    for (i = 0; i < NB_POS; i += 2)
    {
        p1 = h;
        p2 = &vec[pos];
        end1 = p1 + (((59 - pos) >> 2) << 2);
        end2 = end1 + ((59 - pos) & 3);
        c_x = &cor_x[i];
        c_y = &cor_y[i];
        s   = &sign[pos];

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "mult           $ac0,       $0,         $0          \n\t"
            "mult           $ac1,       $0,         $0          \n\t"
            "mult           $ac2,       $0,         $0          \n\t"
            "slt            %[cond],    %[p1],      %[end1]     \n\t"
            "beqz           %[cond],    2f                      \n\t"
            " mult          $ac3,       $0,         $0          \n\t"
          "1:                                                   \n\t"
            "lh             %[Temp1],   0(%[p1])                \n\t"
            "lh             %[Temp2],   2(%[p1])                \n\t"
            "lh             %[Temp3],   4(%[p1])                \n\t"
            "lh             %[Temp4],   6(%[p1])                \n\t"
            "lh             %[Temp5],   0(%[p2])                \n\t"
            "lh             %[Temp6],   2(%[p2])                \n\t"
            "lh             %[Temp7],   4(%[p2])                \n\t"
            "lh             %[Temp8],   6(%[p2])                \n\t"
            "madd           $ac0,       %[Temp1],   %[Temp5]    \n\t"
            "madd           $ac0,       %[Temp2],   %[Temp6]    \n\t"
            "madd           $ac0,       %[Temp3],   %[Temp7]    \n\t"
            "madd           $ac0,       %[Temp4],   %[Temp8]    \n\t"
            "lh             %[Temp5],   8(%[p2])                \n\t"
            "madd           $ac1,       %[Temp1],   %[Temp6]    \n\t"
            "madd           $ac1,       %[Temp2],   %[Temp7]    \n\t"
            "madd           $ac1,       %[Temp3],   %[Temp8]    \n\t"
            "madd           $ac1,       %[Temp4],   %[Temp5]    \n\t"
            "lh             %[Temp6],   10(%[p2])               \n\t"
            "lh             %[Temp7],   12(%[p2])               \n\t"
            "lh             %[Temp8],   14(%[p2])               \n\t"
            "madd           $ac2,       %[Temp1],   %[Temp5]    \n\t"
            "madd           $ac2,       %[Temp2],   %[Temp6]    \n\t"
            "madd           $ac2,       %[Temp3],   %[Temp7]    \n\t"
            "madd           $ac2,       %[Temp4],   %[Temp8]    \n\t"
            "lh             %[Temp5],   16(%[p2])               \n\t"
            "madd           $ac3,       %[Temp1],   %[Temp6]    \n\t"
            "madd           $ac3,       %[Temp2],   %[Temp7]    \n\t"
            "madd           $ac3,       %[Temp3],   %[Temp8]    \n\t"
            "madd           $ac3,       %[Temp4],   %[Temp5]    \n\t"
            "addiu          %[p1],      %[p1],      8           \n\t"
            "slt            %[cond],    %[p1],      %[end1]     \n\t"
            "bnez           %[cond],    1b                      \n\t"
            " addiu         %[p2],      %[p2],      8           \n\t"
          "2:                                                   \n\t"
            "slt            %[cond],    %[p1],      %[end2]     \n\t"
            "beqz           %[cond],    4f                      \n\t"
          "3:                                                   \n\t"
            "lh             %[Temp1],   0(%[p1])                \n\t"
            "lh             %[Temp2],   0(%[p2])                \n\t"
            "lh             %[Temp3],   2(%[p2])                \n\t"
            "lh             %[Temp4],   8(%[p2])                \n\t"
            "lh             %[Temp5],   10(%[p2])               \n\t"
            "madd           $ac0,       %[Temp1],   %[Temp2]    \n\t"
            "madd           $ac1,       %[Temp1],   %[Temp3]    \n\t"
            "madd           $ac2,       %[Temp1],   %[Temp4]    \n\t"
            "madd           $ac3,       %[Temp1],   %[Temp5]    \n\t"
            "addiu          %[p1],      %[p1],      2           \n\t"
            "slt            %[cond],    %[p1],      %[end2]     \n\t"
            "bnez           %[cond],    3b                      \n\t"
            " addiu         %[p2],      %[p2],      2           \n\t"
          "4:                                                   \n\t"
            "lh             %[Temp1],   0(%[p1])                \n\t"
            "lh             %[Temp2],   2(%[p1])                \n\t"
            "lh             %[Temp3],   4(%[p1])                \n\t"
            "lh             %[Temp4],   6(%[p1])                \n\t"
            "lh             %[Temp5],   8(%[p1])                \n\t"
            "lh             %[Temp6],   0(%[p2])                \n\t"
            "lh             %[Temp7],   2(%[p2])                \n\t"
            "lh             %[Temp8],   4(%[p2])                \n\t"
            "lh             %[Temp9],   6(%[p2])                \n\t"
            "lh             %[Temp10],  8(%[p2])                \n\t"
            "madd           $ac0,       %[Temp1],   %[Temp6]    \n\t"
            "madd           $ac0,       %[Temp2],   %[Temp7]    \n\t"
            "madd           $ac0,       %[Temp3],   %[Temp8]    \n\t"
            "madd           $ac0,       %[Temp4],   %[Temp9]    \n\t"
            "madd           $ac0,       %[Temp5],   %[Temp10]   \n\t"
            "madd           $ac1,       %[Temp1],   %[Temp7]    \n\t"
            "madd           $ac1,       %[Temp2],   %[Temp8]    \n\t"
            "madd           $ac1,       %[Temp3],   %[Temp9]    \n\t"
            "madd           $ac1,       %[Temp4],   %[Temp10]   \n\t"
            "madd           $ac2,       %[Temp1],   %[Temp10]   \n\t"
            "extr_rs.w      %[Temp1],   $ac0,       14          \n\t"
            "extr_rs.w      %[Temp2],   $ac1,       14          \n\t"
            "extr_rs.w      %[Temp3],   $ac2,       14          \n\t"
            "extr_rs.w      %[Temp4],   $ac3,       14          \n\t"
            "lh             %[Temp5],   0(%[s])                 \n\t"
            "lh             %[Temp6],   2(%[s])                 \n\t"
            "lh             %[Temp7],   8(%[s])                 \n\t"
            "lh             %[Temp8],   10(%[s])                \n\t"
            "ulw            %[Temp9],   0(%[p0])                \n\t"
            "ulw            %[Temp10],  0(%[p3])                \n\t"
            "muleq_s.w.phr  %[Temp1],   %[Temp1],   %[Temp5]    \n\t"
            "muleq_s.w.phr  %[Temp2],   %[Temp2],   %[Temp6]    \n\t"
            "muleq_s.w.phr  %[Temp3],   %[Temp3],   %[Temp7]    \n\t"
            "muleq_s.w.phr  %[Temp4],   %[Temp4],   %[Temp8]    \n\t"
            "addiu          %[p0],      %[p0],      4           \n\t"
            "addiu          %[p3],      %[p3],      4           \n\t"
            "precrq.ph.w    %[Temp3],   %[Temp3],   %[Temp1]    \n\t"
            "precrq.ph.w    %[Temp4],   %[Temp4],   %[Temp2]    \n\t"
            "addq.ph        %[Temp3],   %[Temp3],   %[Temp9]    \n\t"
            "addq.ph        %[Temp4],   %[Temp4],   %[Temp10]   \n\t"
            "usw            %[Temp3],   0(%[c_x])               \n\t"
            "usw            %[Temp4],   0(%[c_y])               \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1), [Temp2]  "=&r" (Temp2),
              [Temp3] "=&r" (Temp3), [Temp4]  "=&r" (Temp4),
              [Temp5] "=&r" (Temp5), [Temp6]  "=&r" (Temp6),
              [Temp7] "=&r" (Temp7), [Temp8]  "=&r" (Temp8),
              [Temp9] "=&r" (Temp9), [Temp10] "=&r" (Temp10),
              [cond]  "=&r" (cond),  [p1]     "+r"  (p1),
              [p2]    "+r"  (p2),    [p0]     "+r"  (p0),
              [p3]    "+r"  (p3)
            : [end1] "r" (end1), [end2] "r" (end2),
              [c_x]  "r" (c_x),  [c_y]  "r" (c_y),
              [s]    "r" (s)
            : "hi", "lo", "$ac1hi", "$ac1lo",
              "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo",
              "memory"
        );

        pos += 2 * STEP;
    }
#else
    for (i = 0; i < NB_POS; i++)
    {
        p1 = h;
        p2 = &vec[pos];
        end1 = p1 + (((63-pos) >> 2) << 2);
        end2 = end1 + ((63-pos)&3);

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "li             %[L_sum2],  0                       \n\t"
            "beq            %[p1],      %[end1],    1f          \n\t"
            " mult          $0,         $0                      \n\t"
          "2:                                                   \n\t"
            "lh             %[Temp1],   0(%[p1])                \n\t"
            "lh             %[Temp2],   0(%[p2])                \n\t"
            "lh             %[Temp3],   2(%[p1])                \n\t"
            "lh             %[Temp4],   2(%[p2])                \n\t"
            "lh             %[Temp5],   4(%[p1])                \n\t"
            "lh             %[Temp6],   4(%[p2])                \n\t"
            "lh             %[Temp7],   6(%[p1])                \n\t"
            "lh             %[Temp8],   6(%[p2])                \n\t"
            "lh             %[Temp9],   8(%[p2])                \n\t"
            "mul            %[Temp10],  %[Temp1],   %[Temp4]    \n\t"
            "mul            %[Temp11],  %[Temp3],   %[Temp6]    \n\t"
            "mul            %[Temp12],  %[Temp5],   %[Temp8]    \n\t"
            "mul            %[Temp13],  %[Temp7],   %[Temp9]    \n\t"
            "addiu          %[p1],      %[p1],      8           \n\t"
            "addiu          %[p2],      %[p2],      8           \n\t"
            "madd           %[Temp1],   %[Temp2]                \n\t"
            "madd           %[Temp3],   %[Temp4]                \n\t"
            "madd           %[Temp5],   %[Temp6]                \n\t"
            "madd           %[Temp7],   %[Temp8]                \n\t"
            "addu           %[L_sum2],  %[L_sum2],  %[Temp10]   \n\t"
            "addu           %[L_sum2],  %[L_sum2],  %[Temp11]   \n\t"
            "addu           %[L_sum2],  %[L_sum2],  %[Temp12]   \n\t"
            "bne            %[p1],      %[end1],    2b          \n\t"
            " addu          %[L_sum2],  %[L_sum2],  %[Temp13]   \n\t"
          "1:                                                   \n\t"
            "beq            %[p1],      %[end2],    3f          \n\t"
          "4:                                                   \n\t"
            " lh            %[Temp1],   0(%[p1])                \n\t"
            "lh             %[Temp2],   0(%[p2])                \n\t"
            "lh             %[Temp3],   2(%[p2])                \n\t"
            "mul            %[Temp4],   %[Temp1],   %[Temp3]    \n\t"
            "madd           %[Temp1],   %[Temp2]                \n\t"
            "addiu          %[p1],      %[p1],      2           \n\t"
            "addiu          %[p2],      %[p2],      2           \n\t"
            "bne            %[p1],      %[end2],    4b          \n\t"
            " addu          %[L_sum2],  %[L_sum2],  %[Temp4]    \n\t"
          "3:                                                   \n\t"
            "lh             %[Temp1],   0(%[p1])                \n\t"
            "lh             %[Temp2],   0(%[p2])                \n\t"
            "li             %[Temp3],   0x8000                  \n\t"
            "madd           %[Temp1],   %[Temp2]                \n\t"
            "sll            %[L_sum2],  %[L_sum2],  2           \n\t"
            "addu           %[L_sum2],  %[L_sum2],  %[Temp3]    \n\t"
            "sra            %[L_sum2],  %[L_sum2],  16          \n\t"
            "mflo           %[L_sum1]                           \n\t"
            "sll            %[L_sum1],  %[L_sum1],  2           \n\t"
            "addu           %[L_sum1],  %[L_sum1],  %[Temp3]    \n\t"
            "sra            %[L_sum1],  %[L_sum1],  16          \n\t"

            ".set pop                                           \n\t"

            : [Temp1]  "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3]  "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5]  "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [Temp7]  "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
              [Temp9]  "=&r" (Temp9),  [Temp10] "=&r" (Temp10),
              [Temp11] "=&r" (Temp11), [Temp12] "=&r" (Temp12),
              [Temp13] "=&r" (Temp13), [L_sum1] "=&r" (L_sum1),
              [L_sum2] "=&r" (L_sum2), [p1]     "+r"  (p1),
              [p2]     "+r"  (p2)
            : [end1] "r" (end1), [end2] "r" (end2)
            : "hi", "lo", "memory"
        );

        cor_x[i] = vo_mult(L_sum1, sign[pos    ]) + (*p0++);
        cor_y[i] = vo_mult(L_sum2, sign[pos + 1]) + (*p3++);

        pos += STEP;
    }
#endif /* defined(MIPS_DSP_R2_LE) */
    return;
}

/*-------------------------------------------------------------------*
 * Function  search_ixiy_mips()                                      *
 * ~~~~~~~~~~~~~~~~~~~~~~~                                           *
 * Find the best positions of 2 pulses in a subframe.                *
 *-------------------------------------------------------------------*/

void search_ixiy_mips(
        Word16 nb_pos_ix,                     /* (i) nb of pos for pulse 1 (1..8)       */
        Word16 track_x,                       /* (i) track of pulse 1                   */
        Word16 track_y,                       /* (i) track of pulse 2                   */
        Word16 * ps,                          /* (i/o) correlation of all fixed pulses  */
        Word16 * alp,                         /* (i/o) energy of all fixed pulses       */
        Word16 * ix,                          /* (o) position of pulse 1                */
        Word16 * iy,                          /* (o) position of pulse 2                */
        Word16 dn[],                          /* (i) corr. between target and h[]       */
        Word16 dn2[],                         /* (i) vector of selected positions       */
        Word16 cor_x[],                       /* (i) corr. of pulse 1 with fixed pulses */
        Word16 cor_y[],                       /* (i) corr. of pulse 2 with fixed pulses */
        Word16 rrixiy[][MSIZE]                /* (i) corr. of pulse 1 with pulse 2      */
        )
{
    Word32 x, y, pos, thres_ix;
    Word32 ps1, ps2, sqk;
    Word32 alp_16, alpk;
    Word16 *p0, *p1, *p2;
    Word32 s, alp0, alp1, alp2;
    Word32 ps21, ps22, ps23;
    Word32 alp21, alp22, alp23;
    Word32 alp_161, alp_162, alp_163;
    Word32 s1, s2, s3, s11, s31;
    Word32 ixt = *ix, iyt = *iy;
    Word32 a0, a1, a2;

    p0 = cor_x;
    p1 = cor_y;
    p2 = rrixiy[track_x];

    thres_ix = nb_pos_ix - NB_MAX;

    alp0 = L_deposit_h(*alp);
    alp0 = (alp0 + 0x00008000L);       /* for rounding */

    sqk = -1;
    alpk = 1;

#if defined(MIPS_DSP_R1_LE)
    for (x = track_x; x < L_SUBFR; x += STEP)
    {
        alp1 = alp0 + ((*p0++)<<13);

        if (dn2[x] < thres_ix)
        {
            pos = -1;
            ps1 = *ps + dn[x];

            for (y = track_y; y < L_SUBFR; y += 16)
            {
                __asm__ volatile (
                    "lh         %[alp2],        0(%[p1])                    \n\t"
                    "lh         %[alp21],       2(%[p1])                    \n\t"
                    "lh         %[alp22],       4(%[p1])                    \n\t"
                    "lh         %[alp23],       6(%[p1])                    \n\t"
                    "sll        %[alp2],        %[alp2],        13          \n\t"
                    "sll        %[alp21],       %[alp21],       13          \n\t"
                    "sll        %[alp22],       %[alp22],       13          \n\t"
                    "sll        %[alp23],       %[alp23],       13          \n\t"
                    "addu       %[alp2],        %[alp2],        %[alp1]     \n\t"
                    "addu       %[alp21],       %[alp21],       %[alp1]     \n\t"
                    "addu       %[alp22],       %[alp22],       %[alp1]     \n\t"
                    "addu       %[alp23],       %[alp23],       %[alp1]     \n\t"
                    "lh         %[alp_16],      0(%[p2])                    \n\t"
                    "lh         %[alp_161],     2(%[p2])                    \n\t"
                    "lh         %[alp_162],     4(%[p2])                    \n\t"
                    "lh         %[alp_163],     6(%[p2])                    \n\t"
                    "sll        %[alp_16],      %[alp_16],      14          \n\t"
                    "sll        %[alp_161],     %[alp_161],     14          \n\t"
                    "sll        %[alp_162],     %[alp_162],     14          \n\t"
                    "sll        %[alp_163],     %[alp_163],     14          \n\t"
                    "addu       %[alp_16],      %[alp_16],      %[alp2]     \n\t"
                    "addu       %[alp_161],     %[alp_161],     %[alp21]    \n\t"
                    "addu       %[alp_162],     %[alp_162],     %[alp22]    \n\t"
                    "addu       %[alp_163],     %[alp_163],     %[alp23]    \n\t"
                    "sra        %[alp_16],      %[alp_16],      16          \n\t"
                    "sra        %[alp_161],     %[alp_161],     16          \n\t"
                    "sra        %[alp_162],     %[alp_162],     16          \n\t"
                    "sra        %[alp_163],     %[alp_163],     16          \n\t"
                    "addiu      %[p1],          %[p1],          8           \n\t"
                    "addiu      %[p2],          %[p2],          8           \n\t"
                    : [alp2] "=&r" (alp2), [p1] "+r" (p1), [alp21] "=&r" (alp21),
                      [alp22] "=&r" (alp22), [alp23] "=&r" (alp23), [p2] "+r" (p2),
                      [alp_16] "=&r" (alp_16), [alp_161] "=&r" (alp_161),
                      [alp_162] "=&r" (alp_162), [alp_163] "=&r" (alp_163)
                    : [alp1] "r" (alp1)
                    : "memory"
                );

                __asm__ volatile (
                    "sll        %[alp2],        %[y],           1           \n\t"
                    "addiu      %[s],           %[alp2],        8           \n\t"
                    "addiu      %[s1],          %[alp2],        16          \n\t"
                    "addiu      %[s11],         %[alp2],        24          \n\t"
                    "lhx        %[ps2],         %[alp2](%[dn])              \n\t"
                    "lhx        %[ps21],        %[s](%[dn])                 \n\t"
                    "lhx        %[ps22],        %[s1](%[dn])                \n\t"
                    "lhx        %[ps23],        %[s11](%[dn])               \n\t"
                    "addu       %[ps2],         %[ps2],         %[ps1]      \n\t"
                    "addu       %[ps21],        %[ps21],        %[ps1]      \n\t"
                    "addu       %[ps22],        %[ps22],        %[ps1]      \n\t"
                    "addu       %[ps23],        %[ps23],        %[ps1]      \n\t"
                    "mul        %[ps2],         %[ps2],         %[ps2]      \n\t"
                    "mul        %[ps21],        %[ps21],        %[ps21]     \n\t"
                    "mul        %[ps22],        %[ps22],        %[ps22]     \n\t"
                    "mul        %[ps23],        %[ps23],        %[ps23]     \n\t"
                    "sra        %[ps2],         %[ps2],         15          \n\t"
                    "sra        %[ps21],        %[ps21],        15          \n\t"
                    "mult       $ac1,           %[ps2],         %[alpk]     \n\t"
                    "mult       $ac2,           %[ps21],        %[alpk]     \n\t"
                    "mult       $ac3,           %[ps21],        %[alp_16]   \n\t"
                    "msub       $ac1,           %[alp_16],      %[sqk]      \n\t"
                    "msub       $ac2,           %[alp_161],     %[sqk]      \n\t"
                    "msub       $ac3,           %[ps2],         %[alp_161]  \n\t"
                    "sra        %[ps22],        %[ps22],        15          \n\t"
                    "sra        %[ps23],        %[ps23],        15          \n\t"
                    "mflo       %[s],           $ac1                        \n\t"
                    "mflo       %[s1],          $ac2                        \n\t"
                    "mflo       %[s11],         $ac3                        \n\t"
                    : [alp2] "=&r" (alp2), [ps2] "=&r" (ps2), [ps21] "=&r" (ps21),
                      [ps22] "=&r" (ps22), [ps23] "=&r" (ps23),
                      [s] "=&r" (s), [s1] "=&r" (s1), [s11] "=&r" (s11)
                    : [y] "r" (y), [dn] "r" (dn), [ps1] "r" (ps1), [alpk] "r" (alpk),
                      [sqk] "r" (sqk), [alp_16] "r" (alp_16), [alp_161] "r" (alp_161)
                    : "hi", "lo", "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo",
                      "$ac3hi", "$ac3lo", "memory"
                );

                if (s > 0)
                {
                    sqk = ps2;
                    alpk = alp_16;
                    s1 = s11;
                    pos = y;
                }
                if (s1 > 0)
                {
                    sqk = ps21;
                    alpk = alp_161;
                    pos = y+4;
                }

                __asm__ volatile (
                    "mult       $ac1,       %[ps22],        %[alpk]         \n\t"
                    "msub       $ac1,       %[alp_162],     %[sqk]          \n\t"
                    "mult       $ac2,       %[ps23],        %[alpk]         \n\t"
                    "msub       $ac2,       %[alp_163],     %[sqk]          \n\t"
                    "mult       $ac3,       %[ps23],        %[alp_162]      \n\t"
                    "msub       $ac3,       %[alp_163],     %[ps22]         \n\t"
                    "mflo       %[s2],      $ac1                            \n\t"
                    "mflo       %[s3],      $ac2                            \n\t"
                    "mflo       %[s31],     $ac3                            \n\t"
                    : [s2] "=r" (s2), [s3] "=r" (s3), [s31] "=r" (s31)
                    : [ps22] "r" (ps22), [alpk] "r" (alpk), [sqk] "r" (sqk),
                      [alp_162] "r" (alp_162), [alp_163] "r" (alp_163), [ps23] "r" (ps23)
                    : "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo"
                );

                if (s2 > 0)
                {
                    sqk = ps22;
                    alpk = alp_162;
                    pos = y+8;
                    s3 = s31;
                }

                if (s3 > 0)
                {
                    sqk = ps23;
                    alpk = alp_163;
                    pos = y+12;
                }
            }
            p1 -= NB_POS;

            if (pos >= 0)
            {
                ixt = x;
                iyt = pos;
            }
        } else
        {
            p2 += NB_POS;
        }
    }

    __asm__ volatile (
        "lh         %[a2],      0(%[ps])                \n\t"
        "sll        %[a0],      %[ixt],         1       \n\t"
        "sll        %[a1],      %[iyt],         1       \n\t"
        "lhx        %[a0],      %[a0](%[dn])            \n\t"
        "lhx        %[a1],      %[a1](%[dn])            \n\t"
        "addu       %[a0],      %[a1],          %[a0]   \n\t"
        "addu       %[a0],      %[a2],          %[a0]   \n\t"
        "sh         %[ixt],     0(%[ix])                \n\t"
        "sh         %[iyt],     0(%[iy])                \n\t"
        "sh         %[alpk],    0(%[alp])               \n\t"
        "sh         %[a0],      0(%[ps])                \n\t"
        : [a0] "=&r" (a0), [a1] "=&r" (a1), [a2] "=&r" (a2)
        : [ps] "r" (ps), [ixt] "r" (ixt), [iyt] "r" (iyt), [dn] "r" (dn),
          [alp] "r" (alp),  [alpk] "r" (alpk), [ix] "r" (ix), [iy] "r" (iy)
        : "memory"
    );
#else /* MIPS_DSP_R1_LE */
    for (x = track_x; x < L_SUBFR; x += STEP)
    {
        alp1 = alp0 + ((*p0++)<<13);

        if (dn2[x] < thres_ix)
        {
            pos = -1;
            ps1 = *ps + dn[x];

            for (y = track_y; y < L_SUBFR; y += 16)
            {
                __asm__ volatile (
                    "lh         %[alp2],        0(%[p1])                    \n\t"
                    "lh         %[alp21],       2(%[p1])                    \n\t"
                    "lh         %[alp22],       4(%[p1])                    \n\t"
                    "lh         %[alp23],       6(%[p1])                    \n\t"
                    "sll        %[alp2],        %[alp2],        13          \n\t"
                    "sll        %[alp21],       %[alp21],       13          \n\t"
                    "sll        %[alp22],       %[alp22],       13          \n\t"
                    "sll        %[alp23],       %[alp23],       13          \n\t"
                    "addu       %[alp2],        %[alp2],        %[alp1]     \n\t"
                    "addu       %[alp21],       %[alp21],       %[alp1]     \n\t"
                    "addu       %[alp22],       %[alp22],       %[alp1]     \n\t"
                    "addu       %[alp23],       %[alp23],       %[alp1]     \n\t"
                    "lh         %[alp_16],      0(%[p2])                    \n\t"
                    "lh         %[alp_161],     2(%[p2])                    \n\t"
                    "lh         %[alp_162],     4(%[p2])                    \n\t"
                    "lh         %[alp_163],     6(%[p2])                    \n\t"
                    "sll        %[alp_16],      %[alp_16],      14          \n\t"
                    "sll        %[alp_161],     %[alp_161],     14          \n\t"
                    "sll        %[alp_162],     %[alp_162],     14          \n\t"
                    "sll        %[alp_163],     %[alp_163],     14          \n\t"
                    "addu       %[alp_16],      %[alp_16],      %[alp2]     \n\t"
                    "addu       %[alp_161],     %[alp_161],     %[alp21]    \n\t"
                    "addu       %[alp_162],     %[alp_162],     %[alp22]    \n\t"
                    "addu       %[alp_163],     %[alp_163],     %[alp23]    \n\t"
                    "sra        %[alp_16],      %[alp_16],      16          \n\t"
                    "sra        %[alp_161],     %[alp_161],     16          \n\t"
                    "sra        %[alp_162],     %[alp_162],     16          \n\t"
                    "sra        %[alp_163],     %[alp_163],     16          \n\t"
                    "addiu      %[p1],          %[p1],          8           \n\t"
                    "addiu      %[p2],          %[p2],          8           \n\t"
                    : [alp2] "=&r" (alp2), [p1] "+r" (p1), [alp21] "=&r" (alp21),
                      [alp22] "=&r" (alp22), [alp23] "=&r" (alp23), [p2] "+r" (p2),
                      [alp_16] "=&r" (alp_16), [alp_161] "=&r" (alp_161),
                      [alp_162] "=&r" (alp_162), [alp_163] "=&r" (alp_163)
                    : [alp1] "r" (alp1)
                    : "memory"
                );

                __asm__ volatile (
                    "sll        %[alp2],    %[y],               1           \n\t"
                    "addiu      %[s],       %[alp2],            8           \n\t"
                    "addiu      %[s1],      %[alp2],            16          \n\t"
                    "addiu      %[s11],     %[alp2],            24          \n\t"
                    "addu       %[alp2],    %[alp2],            %[dn]       \n\t"
                    "addu       %[s],       %[s],               %[dn]       \n\t"
                    "addu       %[s1],      %[s1],              %[dn]       \n\t"
                    "addu       %[s11],     %[s11],             %[dn]       \n\t"
                    "lh         %[ps2],     0(%[alp2])                      \n\t"
                    "lh         %[ps21],    0(%[s])                         \n\t"
                    "lh         %[ps22],    0(%[s1])                        \n\t"
                    "lh         %[ps23],    0(%[s11])                       \n\t"
                    "addu       %[ps2],     %[ps2],             %[ps1]      \n\t"
                    "addu       %[ps21],    %[ps21],            %[ps1]      \n\t"
                    "addu       %[ps22],    %[ps22],            %[ps1]      \n\t"
                    "addu       %[ps23],    %[ps23],            %[ps1]      \n\t"
                    "mul        %[ps2],     %[ps2],             %[ps2]      \n\t"
                    "mul        %[ps21],    %[ps21],            %[ps21]     \n\t"
                    "mul        %[ps22],    %[ps22],            %[ps22]     \n\t"
                    "mul        %[ps23],    %[ps23],            %[ps23]     \n\t"
                    "sra        %[ps2],     %[ps2],             15          \n\t"
                    "sra        %[ps21],    %[ps21],            15          \n\t"
                    "mul        %[alp2],    %[ps2],             %[alpk]     \n\t"
                    "mul        %[s],       %[alp_16],          %[sqk]      \n\t"
                    "mul        %[a0],      %[ps21],            %[alpk]     \n\t"
                    "mul        %[s1],      %[alp_161],         %[sqk]      \n\t"
                    "mult       %[ps21],    %[alp_16]                       \n\t"
                    "msub       %[ps2],     %[alp_161]                      \n\t"
                    "sra        %[ps22],    %[ps22],            15          \n\t"
                    "sra        %[ps23],    %[ps23],            15          \n\t"
                    "subu       %[s],       %[alp2],            %[s]        \n\t"
                    "subu       %[s1],      %[a0],              %[s1]       \n\t"
                    "mflo       %[s11]                                      \n\t"
                    : [alp2] "=&r" (alp2), [ps2] "=&r" (ps2), [ps21] "=&r" (ps21),
                      [ps22] "=&r" (ps22), [ps23] "=&r" (ps23), [a0] "=&r" (a0),
                      [s] "=&r" (s), [s1] "=&r" (s1), [s11] "=&r" (s11)
                    : [dn] "r" (dn), [ps1] "r" (ps1), [alpk] "r" (alpk), [y] "r" (y),
                      [sqk] "r" (sqk), [alp_16] "r" (alp_16), [alp_161] "r" (alp_161)
                    : "hi", "lo", "memory"
                );

                if (s > 0)
                {
                    sqk = ps2;
                    alpk = alp_16;
                    s1 = s11;
                    pos = y;
                }

                if (s1 > 0)
                {
                    sqk = ps21;
                    alpk = alp_161;
                    pos = y+4;
                }

                __asm__ volatile (
                    "mul        %[ps21],    %[ps22],            %[alpk]         \n\t"
                    "mul        %[s2],      %[alp_162],         %[sqk]          \n\t"
                    "mul        %[alp2],    %[ps23],            %[alpk]         \n\t"
                    "mul        %[s3],      %[alp_163],         %[sqk]          \n\t"
                    "mul        %[s],       %[ps23],            %[alp_162]      \n\t"
                    "mul        %[s31],     %[alp_163],         %[ps22]         \n\t"
                    "subu       %[s2],      %[ps21],            %[s2]           \n\t"
                    "subu       %[s3],      %[alp2],            %[s3]           \n\t"
                    "subu       %[s31],     %[s],               %[s31]          \n\t"
                    : [s2] "=&r" (s2), [s3] "=&r" (s3), [s31] "=&r" (s31), [ps21] "=&r" (ps21),
                      [alp2] "=&r" (alp2), [s] "=&r" (s)
                    : [ps22] "r" (ps22), [alpk] "r" (alpk), [sqk] "r" (sqk), [alp_162] "r" (alp_162),
                      [alp_163] "r" (alp_163), [ps23] "r" (ps23)
                    : "hi", "lo"
                );

                if (s2 > 0)
                {
                    sqk = ps22;
                    alpk = alp_162;
                    pos = y+8;
                    s3 = s31;
                }

                if (s3 > 0)
                {
                    sqk = ps23;
                    alpk = alp_163;
                    pos = y+12;
                }
            }
            p1 -= NB_POS;

            if (pos >= 0)
            {
                ixt = x;
                iyt = pos;
            }
        } else
        {
            p2 += NB_POS;
        }
    }

    __asm__ volatile (
        "lh         %[a2],      0(%[ps])                \n\t"
        "sll        %[a0],      %[ixt],     1           \n\t"
        "sll        %[a1],      %[iyt],     1           \n\t"
        "addu       %[a0],      %[dn],      %[a0]       \n\t"
        "addu       %[a1],      %[dn],      %[a1]       \n\t"
        "lh         %[a0],      0(%[a0])                \n\t"
        "lh         %[a1],      0(%[a1])                \n\t"
        "addu       %[a0],      %[a1],      %[a0]       \n\t"
        "addu       %[a0],      %[a2],      %[a0]       \n\t"
        "sh         %[ixt],     0(%[ix])                \n\t"
        "sh         %[iyt],     0(%[iy])                \n\t"
        "sh         %[alpk],    0(%[alp])               \n\t"
        "sh         %[a0],      0(%[ps])                \n\t"
        : [a0] "=&r" (a0), [a1] "=&r" (a1), [a2] "=&r" (a2)
        : [ps] "r" (ps), [ixt] "r" (ixt), [iyt] "r" (iyt), [dn] "r" (dn),
          [alp] "r" (alp),  [alpk] "r" (alpk), [ix] "r" (ix), [iy] "r" (iy)
        : "memory"
    );
#endif /* MIPS_DSP_R1_LE */

    return;
}




