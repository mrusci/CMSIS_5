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
 * Title:        arm_convolve_HWC_u8_u8_u2_PACT_CH_thr.c
 * Description:  Mixed Precision Convolutional function that uses u8
 *               activations, u2 weights and produce u8
 *               output activations. Outputs are quantized using thr
 *               folding technique.
 *
 * $Date:        March 2019
 * $Authors:     Alessandro Capotondi - alessandro.capotondi@unibo.it
 *               Manuele Rusci - manuele.rusci@unibo.it
 *
 * Target Processor:  Cortex-M cores
 * -------------------------------------------------------------------- */
#include <assert.h>

#include "arm_math.h"
#include "arm_nnfunctions.h"

/**
 *  @ingroup groupNN
 */

/**
 * @addtogroup NNConv
 * @{
 */

  /**
   * @brief Mixed Precision Convolution thr (in: u8, out: u8, wt: u2)
   *
   * @param[in]       Im_in       pointer to input tensor
   * @param[in]       dim_im_in   input tensor dimension
   * @param[in]       ch_im_in    number of input tensor channels
   * @param[in]       wt          pointer to kernel weights
   * @param[in]       ch_im_out   number of filters, i.e., output tensor channels
   * @param[in]       dim_kernel  filter kernel size
   * @param[in]       left_pad    padding sizes
   * @param[in]       right_pad   padding sizes
   * @param[in]       top_pad     padding sizes
   * @param[in]       bottom_pad  padding sizes
   * @param[in]       stride      convolution stride
   * @param[in]       bias        pointer to bias
   * @param[in,out]   Im_out      pointer to output tensor
   * @param[in]       dim_im_out  output tensor dimension
   * @param[in]       z_in        input offset
   * @param[in]       *z_wt       weights offset, per-output channel
   * @param[in]       thresholds  pointer to thresholds
   * @param[in,out]   bufferA     pointer to buffer space for input
   * @param[in,out]   bufferB     pointer to buffer space for output
   * @return     The function returns either
   * <code>ARM_MATH_SIZE_MISMATCH</code> or <code>ARM_MATH_SUCCESS</code> based on the outcome of size checking.
   */
arm_status
arm_convolve_HWC_u8_u8_u2_PACT_CH_thr(const uint8_t * Im_in,
                         const uint16_t dim_im_in,
                         const uint16_t ch_im_in,
                         const uint8_t * wt,
                         const uint16_t ch_im_out,
                         const uint16_t dim_kernel,
                         const uint8_t left_padding,
                         const uint8_t right_padding,
                         const uint8_t top_padding,
                         const uint8_t bottom_padding,
                         const uint16_t stride,
                         const int32_t * bias,
                         uint8_t * Im_out,
                         const uint16_t dim_im_out,
                         const uint8_t z_in,
                         const uint8_t *z_wt,
                         const int16_t * thresholds,
                         int16_t * bufferA,
                         uint8_t * bufferB)
{

#if defined (ARM_MATH_DSP)
    /* Run the following code for Cortex-M4 and Cortex-M7 */

    int16_t   i_out_y, i_out_x, i_ker_y, i_ker_x;
    int16_t  *pBuffer = bufferA;
    uint8_t  *pOut = Im_out;

    if (ch_im_in % 4 != 0 || ch_im_out % 4 != 0)
    {
        /* check if the input dimension meets the constraints */
        return ARM_MATH_SIZE_MISMATCH;
    }

    /*
     *  Here we split the entire matrix into three regions depending on the padding situation
     *    Top: i_out_y from 0 to padding - 1
     * Middle: i_out_y from padding to dim_im_out-padding-1
     * Bottom: i_out_y from dim_im_out-padding to dim_im_out-1
     */

    /* top part */
    for (i_out_y = 0; i_out_y < top_padding; i_out_y++)
    {
        for (i_out_x = 0; i_out_x < dim_im_out; i_out_x++)
        {
            /* This part implements the im2col function */
            for (i_ker_y = i_out_y * stride - top_padding; i_ker_y < i_out_y * stride - top_padding + dim_kernel; i_ker_y++)
            {
                for (i_ker_x = i_out_x * stride - left_padding; i_ker_x < i_out_x * stride - left_padding + dim_kernel; i_ker_x++)
                {
                    if (i_ker_y < 0 || i_ker_y >= dim_im_in || i_ker_x < 0 || i_ker_x >= dim_im_in)
                    {
                        memset(pBuffer, 0, sizeof(int16_t)*ch_im_in);
                    }
                    else
                    {
                        arm_u8_to_int16_reordered (
                                Im_in + (i_ker_y * dim_im_in + i_ker_x) * ch_im_in,
                                pBuffer,
                                ch_im_in,
                                z_in);
                    }
                    pBuffer += ch_im_in;
                }
            }

            if (pBuffer == bufferA + 2 * ch_im_in * dim_kernel * dim_kernel)
            {
                pOut =
                    arm_nn_mat_mult_kernel_reordered_u2_int16_u8_PACT_CH_thr(wt,
                                                            bufferA,
                                                            ch_im_out,
                                                            ch_im_in*dim_kernel*dim_kernel,
                                                            bias,
                                                            pOut,
                                                            z_wt,
                                                            thresholds);
                /* counter reset */
                pBuffer = bufferA;
            }
        }
    }

    /* middle part, here we also divide the x into left, mid and right */
    for (; i_out_y < dim_im_out - bottom_padding; i_out_y++)
    {

        /* left part */
        for (i_out_x = 0; i_out_x < left_padding; i_out_x++)
        {
            /* This part implements the im2col function */
            for (i_ker_y = i_out_y * stride - top_padding; i_ker_y < i_out_y * stride - top_padding + dim_kernel; i_ker_y++)
            {
                for (i_ker_x = i_out_x * stride - left_padding; i_ker_x < i_out_x * stride - left_padding + dim_kernel; i_ker_x++)
                {
                    if (i_ker_x < 0 || i_ker_x >= dim_im_in)
                    {
                        memset(pBuffer, 0, sizeof(int16_t)*ch_im_in);
                    }
                    else
                    {
                        arm_u8_to_int16_reordered (
                                Im_in + (i_ker_y * dim_im_in + i_ker_x) * ch_im_in,
                                pBuffer,
                                ch_im_in,
                                z_in);
                    }
                    pBuffer += ch_im_in;
                }
            }

            if (pBuffer == bufferA + 2 * ch_im_in * dim_kernel * dim_kernel)
            {
                pOut =
                    arm_nn_mat_mult_kernel_reordered_u2_int16_u8_PACT_CH_thr(wt,
                                                            bufferA,
                                                            ch_im_out,
                                                            ch_im_in*dim_kernel*dim_kernel,
                                                            bias,
                                                            pOut,
                                                            z_wt,
                                                            thresholds);
                /* counter reset */
                pBuffer = bufferA;
            }
        }

        /* mid part */
        for (; i_out_x < dim_im_out - right_padding; i_out_x++)
        {
            /* This part implements the im2col function */
            for (i_ker_y = i_out_y * stride - top_padding; i_ker_y < i_out_y * stride - top_padding + dim_kernel; i_ker_y++)
            {
                arm_u8_to_int16_reordered (
                    Im_in + (i_ker_y * dim_im_in + i_out_x * stride - top_padding) * ch_im_in,
                    pBuffer,
                    ch_im_in * dim_kernel,
                    z_in);
                pBuffer += ch_im_in * dim_kernel;
            }

            if (pBuffer == bufferA + 2 * ch_im_in * dim_kernel * dim_kernel)
            {
                pOut =
                    arm_nn_mat_mult_kernel_reordered_u2_int16_u8_PACT_CH_thr(wt,
                                                            bufferA,
                                                            ch_im_out,
                                                            ch_im_in*dim_kernel*dim_kernel,
                                                            bias,
                                                            pOut,
                                                            z_wt,
                                                            thresholds);
                /* counter reset */
                pBuffer = bufferA;
            }
        }

        /* right part */
        for (; i_out_x < dim_im_out; i_out_x++)
        {
            /* This part implements the im2col function */
            for (i_ker_y = i_out_y * stride - top_padding; i_ker_y < i_out_y * stride - top_padding + dim_kernel; i_ker_y++)
            {
                for (i_ker_x = i_out_x * stride - left_padding; i_ker_x < i_out_x * stride - left_padding + dim_kernel; i_ker_x++)
                {
                    if (i_ker_x < 0 || i_ker_x >= dim_im_in)
                    {
                        memset(pBuffer, 0, sizeof(int16_t)*ch_im_in);
                    }
                    else
                    {
                        arm_u8_to_int16_reordered (
                                Im_in + (i_ker_y * dim_im_in + i_ker_x) * ch_im_in,
                                pBuffer,
                                ch_im_in,
                                z_in);
                    }
                    pBuffer += ch_im_in;
                }
            }

            if (pBuffer == bufferA + 2 * ch_im_in * dim_kernel * dim_kernel)
            {
                pOut =
                    arm_nn_mat_mult_kernel_reordered_u2_int16_u8_PACT_CH_thr(wt,
                                                            bufferA,
                                                            ch_im_out,
                                                            ch_im_in*dim_kernel*dim_kernel,
                                                            bias,
                                                            pOut,
                                                            z_wt,
                                                            thresholds);
                /* counter reset */
                pBuffer = bufferA;
            }
        }
    }

    for (; i_out_y < dim_im_out; i_out_y++)
    {
        for (i_out_x = 0; i_out_x < dim_im_out; i_out_x++)
        {
            /* This part implements the im2col function */
            for (i_ker_y = i_out_y * stride - top_padding; i_ker_y < i_out_y * stride - top_padding + dim_kernel; i_ker_y++)
            {
                for (i_ker_x = i_out_x * stride - left_padding; i_ker_x < i_out_x * stride - left_padding + dim_kernel; i_ker_x++)
                {
                    if (i_ker_y < 0 || i_ker_y >= dim_im_in || i_ker_x < 0 || i_ker_x >= dim_im_in)
                    {
                        memset(pBuffer, 0, sizeof(int16_t)*ch_im_in);
                    }
                    else
                    {
                        arm_u8_to_int16_reordered (
                                Im_in + (i_ker_y * dim_im_in + i_ker_x) * ch_im_in,
                                pBuffer,
                                ch_im_in,
                                z_in);
                    }
                    pBuffer += ch_im_in;
                }
            }

            if (pBuffer == bufferA + 2 * ch_im_in * dim_kernel * dim_kernel)
            {
                pOut =
                    arm_nn_mat_mult_kernel_reordered_u2_int16_u8_PACT_CH_thr(wt,
                                                            bufferA,
                                                            ch_im_out,
                                                            ch_im_in*dim_kernel*dim_kernel,
                                                            bias,
                                                            pOut,
                                                            z_wt,
                                                            thresholds);
                /* counter reset */
                pBuffer = bufferA;
            }
        }
    }

    /* check if there is left-over for compute */
    
     if (pBuffer != bufferA)
    {


        /* Weights Pointer */
        const uint8_t *pA = wt;
        int       i;


        for (i = 0; i < ch_im_out; i++)
        {
            /* Offset over Weights */
            int16_t Vz_wt[2] = {z_wt[ch_out_id], z_wt[ch_out_id]};
            const int16_t *pzA = VzA;
            int32_t inzA = *__SIMD32(pzA);
            int32_t sum = bias[i];
            int16_t *pB = bufferA;

            uint16_t  colCnt = ch_im_in * dim_kernel * dim_kernel >> 4; // config.wt_data_t: u2 (16x uint2_t)

            /* accumulate over the vector */
            while (colCnt)
            {
                int32_t inA1, inA2;
                int32_t inA3, inA4;
                int32_t inA5, inA6;
                int32_t inA7, inA8;
                int32_t inB1, inB2;

                pA = (uint8_t *) read_and_pad_reordered_uint2((void *)pA, &inA1, &inA2, &inA3, &inA4, &inA5, &inA6, &inA7, &inA8);

                inB1 = *__SIMD32(pB)++;
                inA1 = __SSUB16(inA1, inzA);
                inA2 = __SSUB16(inA2, inzA);
                sum = __SMLAD(inA1, inB1, sum);
                inB2 = *__SIMD32(pB)++;
                sum = __SMLAD(inA2, inB2, sum);
                inB1 = *__SIMD32(pB)++;
                inA3 = __SSUB16(inA3, inzA);
                inA4 = __SSUB16(inA4, inzA);
                sum = __SMLAD(inA3, inB1, sum);
                inB2 = *__SIMD32(pB)++;
                sum = __SMLAD(inA4, inB2, sum);
                inB1 = *__SIMD32(pB)++;
                inA3 = __SSUB16(inA3, inzA);
                inA4 = __SSUB16(inA4, inzA);
                sum = __SMLAD(inA3, inB1, sum);
                inB2 = *__SIMD32(pB)++;
                sum = __SMLAD(inA4, inB2, sum);

                inB1 = *__SIMD32(pB)++;
                inA5 = __SSUB16(inA5, inzA);
                inA6 = __SSUB16(inA6, inzA);
                sum = __SMLAD(inA5, inB1, sum);
                inB2 = *__SIMD32(pB)++;
                sum = __SMLAD(inA6, inB2, sum);
                
                inB1 = *__SIMD32(pB)++;
                inA7 = __SSUB16(inA7, inzA);
                inA8 = __SSUB16(inA8, inzA);
                sum = __SMLAD(inA7, inB1, sum);
                inB2 = *__SIMD32(pB)++;
                sum = __SMLAD(inA8, inB2, sum);
                colCnt--;
            }

            colCnt = ch_im_in * dim_kernel * dim_kernel & 0xf; // config.wt_data_t: u2 (16x uint2_t)

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
                inA1 -= z_wt[ch_out_id];
                sum += inA1 * inB1;
                colCnt--;
            }

            /* Normalize by Thresholds (u8 output) */
            #warning No threasholds available at u8

            /* Store Outputs (u8 output) */
            *pOut++ = (uint8_t) __USAT(sum, 8);
        }
    }

#else
    #error "Cortex-M0 and Cortex-M3 not supported"
    /* Run the following code as reference implementation for Cortex-M0 and Cortex-M3 */
#endif                          /* ARM_MATH_DSP */

    /* Return to application */
    return ARM_MATH_SUCCESS;
}

/**
 * @} end of NNConv group
 */
