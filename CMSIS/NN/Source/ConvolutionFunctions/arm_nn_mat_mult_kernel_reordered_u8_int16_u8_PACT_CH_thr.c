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
 * Title:        arm_nn_mat_mult_kernel_reordered_u8_int16_u8_PACT_CH_thr.c
 * Description:  Matrix-Multiplication function for
 *               u8 x int16_t convolution with reordered columns.
 *               Output is then quantized to u8 using thr
 *               config.folding technique.
 *
 * $Date:        March 2019
 * $Authors:     Alessandro Capotondi - alessandro.capotondi@unibo.it
 *               Manuele Rusci - manuele.rusci@unibo.it
 *
 * Target Processor:  Cortex-M cores
 * -------------------------------------------------------------------- */

#include "arm_nnfunctions.h"
#include "arm_math.h"

  /**
   * @brief Matrix-Multiplication function for u8 x int16_t convolution with reordered columns.
   *        Output is then quantized to u8 using thr config.folding technique.
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

uint8_t *arm_nn_mat_mult_kernel_reordered_u8_int16_u8_PACT_CH_thr(const uint8_t * pA,
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
    uint8_t *pOut2 = pOut + ch_im_out;
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
        const uint8_t *pA2 = pA + numCol_A;

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

        uint16_t  colCnt = numCol_A >> 2;

        /* accumulate over the vector */
        while (colCnt)
        {
        	int32_t inA11, inA12, inA21, inA22;

        	int32_t inB1 = *__SIMD32(pB)++;
        	int32_t inB2 = *__SIMD32(pB2)++;

            pA = (uint8_t *) read_and_pad_reordered_u8((void *)pA, &inA11, &inA12);
            pA2 = (uint8_t *) read_and_pad_reordered_u8((void *)pA2, &inA21, &inA22);
			inA11 = __SSUB16(inA11, inzA);
			inA12 = __SSUB16(inA12, inzA);
			inA21 = __SSUB16(inA21, inzA2);
			inA22 = __SSUB16(inA22, inzA2);

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

            colCnt--;
        } /* while over colCnt */

#if 0
        //FIXME Lefover cols is missing
#endif

        /* Thresholds (u8 output)*/



        /* skip the row computed with A2 */
        pA += numCol_A;

    } /* for over ch_im_out */

    pOut += ch_im_out;
#else
	#error "Cortex-M0 and Cortex-M3 not supported"
    /* Run the following code as reference implementation for Cortex-M0 and Cortex-M3 */
#endif /* ARM_MATH_DSP */

    /* return the new output pointer with offset */
    return pOut;
}
