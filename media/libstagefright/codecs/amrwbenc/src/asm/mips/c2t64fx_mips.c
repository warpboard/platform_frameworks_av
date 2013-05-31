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

/************************************************************************
*      File: c2t64fx_mips.c                                             *
*                                                                       *
*      Description:Performs algebraic codebook search for 6.60kbits mode*
*                                                                       *
*************************************************************************
*                                                                       *
*      Optimized for MIPS architecture                                  *
*                                                                       *
************************************************************************/

#include "typedef.h"
#include "basic_op.h"
#include "math_op.h"
#include "acelp.h"
#include "cnst.h"

#define NB_TRACK  2
#define STEP      2
#define NB_POS    32
#define MSIZE     1024

/*************************************************************************
* Function:  ACELP_2t64_fx_mips()                                        *
*                                                                        *
* 12 bits algebraic codebook.                                            *
* 2 tracks x 32 positions per track = 64 samples.                        *
*                                                                        *
* 12 bits --> 2 pulses in a frame of 64 samples.                         *
*                                                                        *
* All pulses can have two (2) possible amplitudes: +1 or -1.             *
* Each pulse can have 32 possible positions.                             *
**************************************************************************/

void ACELP_2t64_fx_mips(
        Word16 dn[],                          /* (i) <12b : correlation between target x[] and H[]      */
        Word16 cn[],                          /* (i) <12b : residual after long term prediction         */
        Word16 H[],                           /* (i) Q12: impulse response of weighted synthesis filter */
        Word16 code[],                        /* (o) Q9 : algebraic (fixed) codebook excitation         */
        Word16 y[],                           /* (o) Q9 : filtered fixed codebook excitation            */
        Word16 * index                        /* (o) : index (12): 5+1+5+1 = 11 bits.                   */
        )
{
    Word32 i, j, k, i0, i1, ix, iy, pos, pos2;
    Word16 ps, psk, ps1, ps2, alpk, alp1, alp2, sq;
    Word16 alp, val, exp, k_cn, k_dn;
    Word16 *p0, *p1, *p2, *psign;
    Word16 *h, *h_inv, *ptr_h1, *ptr_h2, *ptr_hf;
    Word16 sign[L_SUBFR], vec[L_SUBFR], dn2[L_SUBFR];
    Word16 h_buf[4 * L_SUBFR] = {0};
    Word16 rrixix[NB_TRACK][NB_POS];
    Word16 rrixiy[MSIZE];
    Word32 s, cor, cor1;
    Word32 Temp1, Temp2, Temp3, Temp4, Temp5, Temp6, Temp7, Temp8;
    Word16 *ptr, *ptr_end;

    /*----------------------------------------------------------------*
    * Find sign for each pulse position.                             *
    *----------------------------------------------------------------*/
    alp = 8192;                              /* alp = 2.0 (Q12) */

    /* calculate energy for normalization of cn[] and dn[] */
    /* set k_cn = 32..32767 (ener_cn = 2^30..256-0) */
    s = Dot_product12(cn, cn, L_SUBFR, &exp);

    Isqrt_n(&s, &exp);
    s = L_shl(s, add1(exp, 5));
    k_cn = vo_round(s);

    /* set k_dn = 32..512 (ener_dn = 2^30..2^22) */
    s = Dot_product12(dn, dn, L_SUBFR, &exp);

    Isqrt_n(&s, &exp);
    k_dn = vo_round(L_shl(s, (exp + 8)));    /* k_dn = 256..4096 */
    k_dn = vo_mult_r(alp, k_dn);              /* alp in Q12 */

    /* mix normalized cn[] and dn[] */
    p0 = cn;
    p1 = dn;
    p2 = dn2;
    ptr_end = p0 + L_SUBFR;

    __asm__ volatile (
        ".set push                                          \n\t"
        ".set noreorder                                     \n\t"

      "1:                                                   \n\t"
        "lh             %[Temp1],   0(%[p0])                \n\t"
        "lh             %[Temp2],   2(%[p0])                \n\t"
        "lh             %[Temp3],   4(%[p0])                \n\t"
        "lh             %[Temp4],   6(%[p0])                \n\t"
        "lh             %[Temp5],   0(%[p1])                \n\t"
        "lh             %[Temp6],   2(%[p1])                \n\t"
        "lh             %[Temp7],   4(%[p1])                \n\t"
        "lh             %[Temp8],   6(%[p1])                \n\t"
        "addiu          %[p0],      %[p0],      8           \n\t"
        "addiu          %[p1],      %[p1],      8           \n\t"
        "mult           $ac0,       %[Temp1],   %[k_cn]     \n\t"
        "mult           $ac1,       %[Temp2],   %[k_cn]     \n\t"
        "mult           $ac2,       %[Temp3],   %[k_cn]     \n\t"
        "mult           $ac3,       %[Temp4],   %[k_cn]     \n\t"
        "madd           $ac0,       %[Temp5],   %[k_dn]     \n\t"
        "madd           $ac1,       %[Temp6],   %[k_dn]     \n\t"
        "madd           $ac2,       %[Temp7],   %[k_dn]     \n\t"
        "madd           $ac3,       %[Temp8],   %[k_dn]     \n\t"
        "extr.w         %[Temp1],   $ac0,       7           \n\t"
        "extr.w         %[Temp2],   $ac1,       7           \n\t"
        "extr.w         %[Temp3],   $ac2,       7           \n\t"
        "extr.w         %[Temp4],   $ac3,       7           \n\t"
        "sh             %[Temp1],   0(%[p2])                \n\t"
        "sh             %[Temp2],   2(%[p2])                \n\t"
        "sh             %[Temp3],   4(%[p2])                \n\t"
        "sh             %[Temp4],   6(%[p2])                \n\t"
        "bne            %[p0],      %[ptr_end], 1b          \n\t"
        " addiu         %[p2],      %[p2],      8           \n\t"

        ".set pop                                           \n\t"

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
    for (i = 0; i < L_SUBFR; i ++)
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
        }
    }
    /*------------------------------------------------------------*
    * Compute h_inv[i].                                          *
    *------------------------------------------------------------*/
    /* impulse response buffer for fast computation */
    h = h_buf + L_SUBFR;
    h_inv = h + (L_SUBFR<<1);

    p0 = h;
    p1 = h_inv;
    p2 = H;
    ptr_end = p2 + L_SUBFR;

    __asm__ volatile (
        ".set push                                              \n\t"
        ".set noreorder                                         \n\t"

      "1:                                                       \n\t"
        "ulw            %[Temp1],   0(%[p2])                    \n\t"
        "ulw            %[Temp2],   4(%[p2])                    \n\t"
        "ulw            %[Temp3],   8(%[p2])                    \n\t"
        "ulw            %[Temp4],   12(%[p2])                   \n\t"
        "addiu          %[p2],      %[p2],      16              \n\t"
        "subq.ph        %[Temp5],   $0,         %[Temp1]        \n\t"
        "subq.ph        %[Temp6],   $0,         %[Temp2]        \n\t"
        "subq.ph        %[Temp7],   $0,         %[Temp3]        \n\t"
        "subq.ph        %[Temp8],   $0,         %[Temp4]        \n\t"
        "usw            %[Temp1],   0(%[p0])                    \n\t"
        "usw            %[Temp5],   0(%[p1])                    \n\t"
        "usw            %[Temp2],   4(%[p0])                    \n\t"
        "usw            %[Temp6],   4(%[p1])                    \n\t"
        "usw            %[Temp3],   8(%[p0])                    \n\t"
        "usw            %[Temp7],   8(%[p1])                    \n\t"
        "usw            %[Temp4],   12(%[p0])                   \n\t"
        "usw            %[Temp8],   12(%[p1])                   \n\t"
        "addiu          %[p1],      %[p1],      16              \n\t"
        "bne            %[p2],      %[ptr_end], 1b              \n\t"
        " addiu         %[p0],      %[p0],      16              \n\t"

        ".set pop                                               \n\t"

        : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
          [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
          [Temp5] "=&r" (Temp5), [Temp6] "=&r" (Temp6),
          [Temp7] "=&r" (Temp7), [Temp8] "=&r" (Temp8),
          [p0]    "+r"  (p0),    [p1]    "+r"  (p1),
          [p2]    "+r"  (p2)
        : [ptr_end] "r" (ptr_end)
        : "memory"
    );

    /*------------------------------------------------------------*
    * Compute rrixix[][] needed for the codebook search.         *
    * Result is multiplied by 0.5                                *
    *------------------------------------------------------------*/
    /* Init pointers to last position of rrixix[] */
    p0 = &rrixix[0][NB_POS - 1];
    p1 = &rrixix[1][NB_POS - 1];

    ptr_h1 = h;
    ptr_end = ptr_h1 + NB_POS * 2;

    __asm__ volatile (
        ".set push                                              \n\t"
        ".set noreorder                                         \n\t"

        "lui            %[cor],     0x1                         \n\t"
      "1:                                                       \n\t"
        "lh             %[Temp1],   0(%[ptr_h1])                \n\t"
        "lh             %[Temp2],   2(%[ptr_h1])                \n\t"
        "lh             %[Temp3],   4(%[ptr_h1])                \n\t"
        "lh             %[Temp4],   6(%[ptr_h1])                \n\t"
        "muleq_s.w.phr  %[Temp1],   %[Temp1],   %[Temp1]        \n\t"
        "muleq_s.w.phr  %[Temp2],   %[Temp2],   %[Temp2]        \n\t"
        "muleq_s.w.phr  %[Temp3],   %[Temp3],   %[Temp3]        \n\t"
        "muleq_s.w.phr  %[Temp4],   %[Temp4],   %[Temp4]        \n\t"
        "lh             %[Temp5],   8(%[ptr_h1])                \n\t"
        "lh             %[Temp6],   10(%[ptr_h1])               \n\t"
        "lh             %[Temp7],   12(%[ptr_h1])               \n\t"
        "lh             %[Temp8],   14(%[ptr_h1])               \n\t"
        "muleq_s.w.phr  %[Temp5],   %[Temp5],   %[Temp5]        \n\t"
        "muleq_s.w.phr  %[Temp6],   %[Temp6],   %[Temp6]        \n\t"
        "muleq_s.w.phr  %[Temp7],   %[Temp7],   %[Temp7]        \n\t"
        "muleq_s.w.phr  %[Temp8],   %[Temp8],   %[Temp8]        \n\t"
        "addu           %[Temp1],   %[cor],     %[Temp1]        \n\t"
        "addu           %[Temp2],   %[Temp2],   %[Temp1]        \n\t"
        "addu           %[Temp3],   %[Temp3],   %[Temp2]        \n\t"
        "addu           %[Temp4],   %[Temp4],   %[Temp3]        \n\t"
        "addu           %[Temp5],   %[Temp5],   %[Temp4]        \n\t"
        "addu           %[Temp6],   %[Temp6],   %[Temp5]        \n\t"
        "addu           %[Temp7],   %[Temp7],   %[Temp6]        \n\t"
        "addu           %[cor],     %[Temp7],   %[Temp8]        \n\t"
        "precrq.ph.w    %[Temp1],   %[Temp1],   %[Temp3]        \n\t"
        "precrq.ph.w    %[Temp2],   %[Temp2],   %[Temp4]        \n\t"
        "precrq.ph.w    %[Temp3],   %[Temp5],   %[Temp7]        \n\t"
        "precrq.ph.w    %[Temp4],   %[Temp6],   %[cor]          \n\t"
        "shra.ph        %[Temp1],   %[Temp1],   1               \n\t"
        "shra.ph        %[Temp2],   %[Temp2],   1               \n\t"
        "shra.ph        %[Temp3],   %[Temp3],   1               \n\t"
        "shra.ph        %[Temp4],   %[Temp4],   1               \n\t"
        "usw            %[Temp1],   -2(%[p1])                   \n\t"
        "usw            %[Temp2],   -2(%[p0])                   \n\t"
        "usw            %[Temp3],   -6(%[p1])                   \n\t"
        "usw            %[Temp4],   -6(%[p0])                   \n\t"
        "addiu          %[ptr_h1],  %[ptr_h1],  16              \n\t"
        "addiu          %[p1],      %[p1],      -8              \n\t"
        "bne            %[ptr_h1],  %[ptr_end], 1b              \n\t"
        " addiu         %[p0],      %[p0],      -8              \n\t"

        ".set pop                                               \n\t"

        : [Temp1] "=&r" (Temp1), [Temp2]  "=&r" (Temp2),
          [Temp3] "=&r" (Temp3), [Temp4]  "=&r" (Temp4),
          [Temp5] "=&r" (Temp5), [Temp6]  "=&r" (Temp6),
          [Temp7] "=&r" (Temp7), [Temp8]  "=&r" (Temp8),
          [cor]   "=&r" (cor),   [ptr_h1] "+r"  (ptr_h1),
          [p0]    "+r"  (p0),    [p1]     "+r"  (p1)
        : [ptr_end] "r" (ptr_end)
        : "hi", "lo", "memory"
    );

    /*------------------------------------------------------------*
    * Compute rrixiy[][] needed for the codebook search.         *
    *------------------------------------------------------------*/
    pos = MSIZE - 1;
    pos2 = MSIZE - 2;
    ptr_hf = h + 1;

    for (k = 0; k < NB_POS; k+=2)
    {
        p1 = &rrixiy[pos];
        p0 = &rrixiy[pos2];
        ptr_h1 = h;
        ptr_h2 = ptr_hf;
        ptr_end = ptr_h1 + 2*(NB_POS-2-k);

        __asm__ volatile (
            ".set push                                          \n\t"
            ".set noreorder                                     \n\t"

            "li             %[cor],     0x8000                  \n\t"
            "beq            %[ptr_h1],  %[ptr_end], 2f          \n\t"
            " li            %[cor1],    0x8000                  \n\t"
          "1:                                                   \n\t"
            "lh             %[Temp1],   0(%[ptr_h1])            \n\t"
            "lh             %[Temp2],   0(%[ptr_h2])            \n\t"
            "lh             %[Temp3],   4(%[ptr_h2])            \n\t"
            "lh             %[Temp4],   2(%[ptr_h1])            \n\t"
            "muleq_s.w.phr  %[Temp2],   %[Temp1],   %[Temp2]    \n\t"
            "muleq_s.w.phr  %[Temp1],   %[Temp1],   %[Temp3]    \n\t"
            "lh             %[Temp5],   2(%[ptr_h2])            \n\t"
            "lh             %[Temp6],   6(%[ptr_h2])            \n\t"
            "addiu          %[ptr_h1],  %[ptr_h1],  4           \n\t"
            "muleq_s.w.phr  %[Temp5],   %[Temp4],   %[Temp5]    \n\t"
            "muleq_s.w.phr  %[Temp4],   %[Temp4],   %[Temp6]    \n\t"
            "addiu          %[ptr_h2],  %[ptr_h2],  4           \n\t"
            "addu           %[cor],     %[cor],     %[Temp2]    \n\t"
            "addu           %[cor1],    %[cor1],    %[Temp1]    \n\t"
            "sra            %[Temp2],   %[cor],     16          \n\t"
            "sra            %[Temp1],   %[cor1],    16          \n\t"
            "addu           %[cor],     %[cor],     %[Temp5]    \n\t"
            "addu           %[cor1],    %[cor1],    %[Temp4]    \n\t"
            "sra            %[Temp5],   %[cor],     16          \n\t"
            "sra            %[Temp4],   %[cor1],    16          \n\t"
            "sh             %[Temp2],   0(%[p1])                \n\t"
            "sh             %[Temp1],   -64(%[p1])              \n\t"
            "sh             %[Temp5],   0(%[p0])                \n\t"
            "sh             %[Temp4],   -2(%[p0])               \n\t"
            "addiu          %[p1],      %[p1],      -66         \n\t"
            "bne            %[ptr_h1],  %[ptr_end], 1b          \n\t"
            " addiu         %[p0],      %[p0],      -66         \n\t"
          "2:                                                   \n\t"
            "lh             %[Temp1],   0(%[ptr_h1])            \n\t"
            "lh             %[Temp2],   0(%[ptr_h2])            \n\t"
            "lh             %[Temp3],   4(%[ptr_h2])            \n\t"
            "lh             %[Temp4],   2(%[ptr_h1])            \n\t"
            "muleq_s.w.phr  %[Temp2],   %[Temp1],   %[Temp2]    \n\t"
            "muleq_s.w.phr  %[Temp1],   %[Temp1],   %[Temp3]    \n\t"
            "lh             %[Temp5],   2(%[ptr_h2])            \n\t"
            "lh             %[Temp6],   4(%[ptr_h1])            \n\t"
            "muleq_s.w.phr  %[Temp4],   %[Temp4],   %[Temp5]    \n\t"
            "muleq_s.w.phr  %[Temp3],   %[Temp3],   %[Temp6]    \n\t"
            "addu           %[cor],     %[cor],     %[Temp2]    \n\t"
            "addu           %[cor1],    %[cor1],    %[Temp1]    \n\t"
            "sra            %[Temp2],   %[cor],     16          \n\t"
            "sra            %[Temp1],   %[cor1],    16          \n\t"
            "sh             %[Temp2],   0(%[p1])                \n\t"
            "sh             %[Temp1],   -64(%[p1])              \n\t"
            "addu           %[cor],     %[cor],     %[Temp4]    \n\t"
            "sra            %[Temp4],   %[cor],     16          \n\t"
            "addu           %[cor],     %[cor],     %[Temp3]    \n\t"
            "sra            %[Temp3],   %[cor],     16          \n\t"
            "sh             %[Temp4],   0(%[p0])                \n\t"
            "sh             %[Temp3],   -66(%[p1])              \n\t"

            ".set pop                                           \n\t"

            : [Temp1] "=&r" (Temp1),  [Temp2]  "=&r" (Temp2),
              [Temp3] "=&r" (Temp3),  [Temp4]  "=&r" (Temp4),
              [Temp5] "=&r" (Temp5),  [Temp6]  "=&r" (Temp6),
              [Temp7] "=&r" (Temp7),  [Temp8]  "=&r" (Temp8),
              [cor]   "=&r" (cor),    [cor1]   "=&r" (cor1),
              [ptr_h1]"+r"  (ptr_h1), [ptr_h2] "+r"  (ptr_h2),
              [p0]    "+r"  (p0),     [p1]     "+r"  (p1)
            : [ptr_end] "r" (ptr_end)
            : "hi", "lo", "memory"
        );

        pos -= 2*NB_POS;
        pos2 -= 2;
        ptr_hf += 2*STEP;
    }

    /*------------------------------------------------------------*
    * Modification of rrixiy[][] to take signs into account.     *
    *------------------------------------------------------------*/
    p0 = rrixiy;
    for (i = 0; i < L_SUBFR; i += STEP)
    {
        psign = sign;
        if (psign[i] < 0)
        {
            psign = vec;
        }
        for (j = 1; j < L_SUBFR; j += STEP)
        {
            *p0 = vo_mult(*p0, psign[j]);
            p0++;
        }
    }
    /*-------------------------------------------------------------------*
    * search 2 pulses:                                                  *
    * ~@~~~~~~~~~~~~~~                                                  *
    * 32 pos x 32 pos = 1024 tests (all combinaisons is tested)         *
    *-------------------------------------------------------------------*/
    p0 = rrixix[0];
    p1 = rrixix[1];
    p2 = rrixiy;

    psk = -1;
    alpk = 1;
    ix = 0;
    iy = 1;

    for (i0 = 0; i0 < L_SUBFR; i0 += STEP)
    {
        ps1 = dn[i0];
        alp1 = (*p0++);
        pos = -1;
        for (i1 = 1; i1 < L_SUBFR; i1 += STEP)
        {
            ps2 = add1(ps1, dn[i1]);
            alp2 = add1(alp1, add1(*p1++, *p2++));
            sq = vo_mult(ps2, ps2);
            s = vo_L_mult(alpk, sq) - ((psk * alp2)<<1);
            if (s > 0)
            {
                psk = sq;
                alpk = alp2;
                pos = i1;
            }
        }
        p1 -= NB_POS;
        if (pos >= 0)
        {
            ix = i0;
            iy = pos;
        }
    }
    /*-------------------------------------------------------------------*
    * Build the codeword, the filtered codeword and index of codevector.*
    *-------------------------------------------------------------------*/

    memset (code, 0, sizeof(Word16) * L_SUBFR);

    i0 = (ix >> 1);                       /* pos of pulse 1 (0..31) */
    i1 = (iy >> 1);                       /* pos of pulse 2 (0..31) */
    if (sign[ix] > 0)
    {
        code[ix] = 512;                     /* codeword in Q9 format */
        p0 = h - ix;
    } else
    {
        code[ix] = -512;
        i0 += NB_POS;
        p0 = h_inv - ix;
    }
    if (sign[iy] > 0)
    {
        code[iy] = 512;
        p1 = h - iy;
    } else
    {
        code[iy] = -512;
        i1 += NB_POS;
        p1 = h_inv - iy;
    }
    *index = add1((i0 << 6), i1);

    ptr = y;
    ptr_end = p0 + L_SUBFR;

    __asm__ volatile (
        ".set push                                              \n\t"
        ".set noreorder                                         \n\t"

      "1:                                                       \n\t"
        "ulw                %[Temp1],   0(%[p0])                \n\t"
        "ulw                %[Temp2],   0(%[p1])                \n\t"
        "ulw                %[Temp3],   4(%[p0])                \n\t"
        "ulw                %[Temp4],   4(%[p1])                \n\t"
        "addq.ph            %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
        "addq.ph            %[Temp3],   %[Temp3],   %[Temp4]    \n\t"
        "shra_r.ph          %[Temp1],   %[Temp1],   3           \n\t"
        "shra_r.ph          %[Temp3],   %[Temp3],   3           \n\t"
        "usw                %[Temp1],   0(%[ptr])               \n\t"
        "usw                %[Temp3],   4(%[ptr])               \n\t"
        "ulw                %[Temp1],   8(%[p0])                \n\t"
        "ulw                %[Temp2],   8(%[p1])                \n\t"
        "ulw                %[Temp3],   12(%[p0])               \n\t"
        "ulw                %[Temp4],   12(%[p1])               \n\t"
        "addiu              %[p0],      %[p0],      16          \n\t"
        "addiu              %[p1],      %[p1],      16          \n\t"
        "addq.ph            %[Temp1],   %[Temp1],   %[Temp2]    \n\t"
        "addq.ph            %[Temp3],   %[Temp3],   %[Temp4]    \n\t"
        "shra_r.ph          %[Temp1],   %[Temp1],   3           \n\t"
        "shra_r.ph          %[Temp3],   %[Temp3],   3           \n\t"
        "usw                %[Temp1],   8(%[ptr])               \n\t"
        "usw                %[Temp3],   12(%[ptr])              \n\t"
        "bne                %[p0],      %[ptr_end], 1b          \n\t"
        " addiu             %[ptr],     %[ptr],     16          \n\t"

        ".set pop                                               \n\t"

        : [Temp1] "=&r" (Temp1), [Temp2] "=&r" (Temp2),
          [Temp3] "=&r" (Temp3), [Temp4] "=&r" (Temp4),
          [ptr]   "+r"  (ptr),   [p0]    "+r"  (p0),
          [p1]    "+r"  (p1)
        : [ptr_end] "r" (ptr_end)
        : "memory"
    );

    return;
}



