/*
 * Copyright (C) 2010-2018 Arm Limited or its affiliates. All rights reserved.
 * Modifications Copyright (C) 2018 University of Bologna
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* ----------------------------------------------------------------------
 * Project:      CMSIS NN Library - Mixed Precision INT-Q
 * Title:        arm_nn_mat_mult_kernel_reordered_u2_int16_u4_PACT_CH_thr.c
 * Description:  Matrix-Multiplication function for
 *               u2 x int16_t convolution with reordered columns.
 *               Output is then quantized to u4 using thr
 *               config.folding technique.
 *
 * Target Processor:  Cortex-M cores
 * 
 * Modification: Mixed-Precision INT-Q extension
 *
 * $Date:        3 September 2019
 * $Revision:    V.1.2.0
 *
 * $Authors:     Alessandro Capotondi - alessandro.capotondi@unibo.it
 *               Marco Fariselli - marco.fariselli2@unibo.it 
 *               Manuele Rusci - manuele.rusci@unibo.it
 *               
 * -------------------------------------------------------------------- */

#include "arm_nnfunctions.h"
#include "arm_math.h"

  /**
   * @brief Matrix-Multiplication function for u2 x int16_t convolution with reordered columns.
   *        Output is then quantized to u4 using thr config.folding technique.
   * @param[in]       pA          pointer to operand A
   * @param[in]       pInBuffer   pointer to operand B, always consists of 2 vectors
   * @param[in]       ch_im_out   numRow of A
   * @param[in]       numCol_A    numCol of A
   * @param[in]       bias        the bias
   * @param[in,out]   pOut        pointer to output
   * @param[in]       z_a         A operand offset
   * @param[in]       *z_a        pointer to A operand offsets (per-output channel)
   * @param[in]       thresholds  pointer to thresholds for quantization
   * @return     The function returns the incremented output pointer
   *
   * @details
   *
   * This function assumes that data in pInBuffer are reordered
   */

uint8_t
*arm_nn_mat_mult_kernel_reordered_u2_int16_u4_PACT_CH_thr(const uint8_t * pA,
                            const int16_t * pInBuffer,
                            const uint16_t ch_im_out,
                            const uint16_t numCol_A,
                            const int32_t * bias,
                            uint8_t * pOut,
                            const uint8_t *z_a,
                            const int16_t * thresholds)
{

#if defined (ARM_MATH_DSP)
    /* set up the second output pointers */
    uint8_t *pOut2 = pOut + (ch_im_out>>1); // config.out_data_t: u4 (2CHs per-Bytes)
    int     i;
    const int16_t *pB = pInBuffer;
    const int16_t *pB2 = pB + numCol_A;



    /* this loop over rows in A */
    for (i = 0; i < ch_im_out; i += 2)
    {
        /* setup pointers for B */
        pB = pInBuffer;
        pB2 = pB + numCol_A;

        /* align the second pointer for A */
        const uint8_t *pA2 = pA + (numCol_A>>2); // config.wt_data_t: u2 (8Cols per-Byte)

        int16_t VzA[2] = {z_a[i],z_a[i]};
        const int16_t *pzA = VzA;
        int32_t inzA = *__SIMD32(pzA);

        int16_t VzA2[2] = {z_a[i+1],z_a[i+1]};
        const int16_t *pzA2 = VzA2;
        int32_t inzA2 = *__SIMD32(pzA2);

        int32_t     sum =  bias[i];
        int32_t     sum2 = bias[i];
        int32_t     sum3 = bias[i + 1];
        int32_t     sum4 = bias[i + 1];

        uint16_t  colCnt = numCol_A >> 4; // config.wt_data_t: u2 (16x uint2_t)

        /* accumulate over the vector */
        while (colCnt)
        {
            int32_t inA11, inA12, inA21, inA22;
            int32_t inA13, inA14, inA23, inA24;
            int32_t inA15, inA16, inA25, inA26;
            int32_t inA17, inA18, inA27, inA28;

            int32_t inB1 = *__SIMD32(pB)++;
            int32_t inB2 = *__SIMD32(pB2)++;

            pA = (uint8_t *) read_and_pad_reordered_u2((void *)pA, &inA11, &inA12, &inA13, &inA14, &inA15, &inA16, &inA17, &inA18);
            pA2 = (uint8_t *) read_and_pad_reordered_u2((void *)pA2, &inA21, &inA22, &inA23, &inA24, &inA25, &inA26, &inA27, &inA28);
            inA11 = __SSUB16(inA11, inzA);
            inA12 = __SSUB16(inA12, inzA);
            inA21 = __SSUB16(inA21, inzA2);
            inA22 = __SSUB16(inA22, inzA2);
            inA13 = __SSUB16(inA13, inzA);
            inA14 = __SSUB16(inA14, inzA);
            inA23 = __SSUB16(inA23, inzA2);
            inA24 = __SSUB16(inA24, inzA2);
            inA15 = __SSUB16(inA15, inzA);
            inA16 = __SSUB16(inA16, inzA);
            inA25 = __SSUB16(inA25, inzA2);
            inA26 = __SSUB16(inA26, inzA2);
            inA17 = __SSUB16(inA17, inzA);
            inA18 = __SSUB16(inA18, inzA);
            inA27 = __SSUB16(inA27, inzA2);
            inA28 = __SSUB16(inA28, inzA2);

            sum = __SMLAD(inA11, inB1, sum);
            sum2 = __SMLAD(inA11, inB2, sum2);
            sum3 = __SMLAD(inA21, inB1, sum3);
            sum4 = __SMLAD(inA21, inB2, sum4);

            inB1 = *__SIMD32(pB)++;
            inB2 = *__SIMD32(pB2)++;

            sum = __SMLAD(inA12, inB1, sum);
            sum2 = __SMLAD(inA12, inB2, sum2);
            sum3 = __SMLAD(inA22, inB1, sum3);
            sum4 = __SMLAD(inA22, inB2, sum4);

            inB1 = *__SIMD32(pB)++;
            inB2 = *__SIMD32(pB2)++;

            sum = __SMLAD(inA13, inB1, sum);
            sum2 = __SMLAD(inA13, inB2, sum2);
            sum3 = __SMLAD(inA23, inB1, sum3);
            sum4 = __SMLAD(inA23, inB2, sum4);

            inB1 = *__SIMD32(pB)++;
            inB2 = *__SIMD32(pB2)++;

            sum = __SMLAD(inA14, inB1, sum);
            sum2 = __SMLAD(inA14, inB2, sum2);
            sum3 = __SMLAD(inA24, inB1, sum3);
            sum4 = __SMLAD(inA24, inB2, sum4);
            
            inB1 = *__SIMD32(pB)++;
            inB2 = *__SIMD32(pB2)++;

            sum = __SMLAD(inA15, inB1, sum);
            sum2 = __SMLAD(inA15, inB2, sum2);
            sum3 = __SMLAD(inA25, inB1, sum3);
            sum4 = __SMLAD(inA25, inB2, sum4);

            inB1 = *__SIMD32(pB)++;
            inB2 = *__SIMD32(pB2)++;

            sum = __SMLAD(inA16, inB1, sum);
            sum2 = __SMLAD(inA16, inB2, sum2);
            sum3 = __SMLAD(inA26, inB1, sum3);
            sum4 = __SMLAD(inA26, inB2, sum4);

            inB1 = *__SIMD32(pB)++;
            inB2 = *__SIMD32(pB2)++;

            sum = __SMLAD(inA17, inB1, sum);
            sum2 = __SMLAD(inA17, inB2, sum2);
            sum3 = __SMLAD(inA27, inB1, sum3);
            sum4 = __SMLAD(inA27, inB2, sum4);

            inB1 = *__SIMD32(pB)++;
            inB2 = *__SIMD32(pB2)++;

            sum = __SMLAD(inA18, inB1, sum);
            sum2 = __SMLAD(inA18, inB2, sum2);
            sum3 = __SMLAD(inA28, inB1, sum3);
            sum4 = __SMLAD(inA28, inB2, sum4);
            colCnt--;
        } /* while over colCnt */

#if 0
        colCnt = numCol_A & 0xf; // config.wt_data_t: u2 (16x uint2_t)

        int wt_per_byte = 4;
        while (colCnt)
        {
            uint8_t inB1 = (uint8_t) *pB++;
            uint8_t inA1;
            switch(wt_per_byte)
            {
                case 4:
                    inA1 = (uint8_t) __USAT(*pA, 2);
                    break;
                case 3:
                    inA1 = (uint8_t) __USAT(__ROR(*pA, 2), 2);
                    break;
                case 2:
                    inA1 = (uint8_t) __USAT(__ROR(*pA, 2), 4);
                    break;
                case 1:
                    inA1 = (uint8_t) __USAT(__ROR(*pA, 2), 6);
                    pA++;
                break;
            }
            inA1 -= z_a[i];
            sum += inA1 * inB1;
            colCnt--;
        }
#endif

        /* Normalize by Thresholds (u4 output) */
        sum = __int16_to_u4((int16_t) sum, &thresholds[i<<4]);
        sum2 = __int16_to_u4((int16_t) sum2, &thresholds[i<<4]);
        sum3 = __int16_to_u4((int16_t) sum3, &thresholds[(i+1)<<4]);
        sum4 = __int16_to_u4((int16_t) sum4, &thresholds[(i+1)<<4]);

        /* Store Outputs (u4 output) */
        *pOut++  = ( __USAT(sum,4) | ((__USAT(sum3,4) << 4 ) & 0xF0 ));
        *pOut2++ = ( __USAT(sum2,4) | ((__USAT(sum4,4) << 4 ) & 0xF0 ));

        /* skip the row computed with A2 */
        pA += numCol_A>>2; // config.wt_data_t: u2 (4cols per-Bytes)
    } /* for over ch_im_out */
    
    pOut += ch_im_out>>1; // config.out_data_t: u4 (2CH per-Bytes)
#else
    #error "Cortex-M0 and Cortex-M3 not supported"
    /* Run the following code as reference implementation for Cortex-M0 and Cortex-M3 */
#endif /* ARM_MATH_DSP */

    /* return the new output pointer with offset */
    return pOut;
}
