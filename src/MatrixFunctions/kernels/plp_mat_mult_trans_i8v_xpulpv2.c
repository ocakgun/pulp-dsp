/* =====================================================================
 * Project:      PULP DSP Library
 * Title:        plp_mat_mult_i8v_xpulpv2.c
 * Description:  8-bit integer matrix multiplication for XPULPV2
 *
 * $Date:        22. December 2019
 * $Revision:    V0
 *
 * Target Processor: PULP cores
 * ===================================================================== */
/*
 * Copyright (C) 2019 ETH Zurich and Ubiversity of Bologna. All rights reserved.
 *
 * Author: Tom Kuchler, ETH Zurich
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

#include "plp_math.h"


/**
  @ingroup BasicMatMultTrans
 */

/**
  @defgroup BasicMatMultTransKernels Matrix Multiplication Kernels
  Computes the product of two matrices, the second of which is transposed.

  The Matrix Matrix Multiplication computes the product of two matrices with dimensions MxN and NxO, the second one is stored transposed in memory.
  The first matrix is accessed row wise, the second column wise, all values form the first are multiplied with the values of the second and then sum of the result gives the value for the result matrix.
  <pre>
      pDst[i,k] = pSrcA[i*M]*pSrcB[k*N] + pSrcA[i*M+1]*pSrcB[k*N+1] + ... + pSrcA[i*M+N-1]*pSrcB[k*N+N-1]
  </pre>
  There are separate functions int8, int16, and int32 data types. For lower precision integers (int8, int16), functions exploiting SIMD instructions are provided.

  The naming of the functions follows the following pattern (for example plp_dot_prod_i32s_rv32im):
  <pre>
      \<pulp\> _ \<function name\> _ \<data type\>\<precision\>\<method\>_\<isa extension\>, with

      data type = {f, i, q} respectively for floats, integers, fixed points

      precision = {32, 16, 8} bits

      method = {s, v, p} meaning single (or scalar, i.e. not using packed SIMD), vectorized (i.e. using SIMD instructions), and parallel (for multicore parallel computing), respectively.

      isa extension = rv32im, xpulpv2, etc. of which rv32im is the most general one.

  </pre>


 */


/**
  @addtogroup BasicMatMultTransKernels
  @{
 */

/**
  @brief Matrix multiplication of 8-bit integer matrices kernel for XPULPV2 extension.
  @param[in]  pSrcA     points to the first input matrix
  @param[in]  pSrcB     points to the second input matrix
  @param[in]  M         height of the first input matrix
  @param[in]  N         width of the first input matrix and hight of the second
  @param[in]  O         width of the second input matrix
  @param[out] pDstC     points to the output matrix
  @return        none
 */

// define BASIC_VERSION // if used don't forget to also use the undefine at end of file

#ifdef BASIC_VERSION

void plp_mat_mult_trans_i8v_xpulpv2(
                              const int8_t * __restrict__ pSrcA,
                              const int8_t * __restrict__ pSrcB,
                              uint32_t M,
                              uint32_t N,
                              uint32_t O,
                              int32_t * __restrict__ pDstC) {
        
        uint32_t i; // loop counter
        uint32_t j; // loop counter
        uint32_t k; // loop counter

        for(i=0; i < M; i++){
          for(k=0; k < O; k++){
            int32_t sum = 0;
            for(j=0; j<N; j++){
              sum = sum + pSrcA[i*N + j]*pSrcB[k*N+j];
            }
            pDstC[i*O +k] = sum;
          }
        }
}

#else 

void plp_mat_mult_trans_i8v_xpulpv2(
                              const int8_t * __restrict__ pSrcA,
                              const int8_t * __restrict__ pSrcB,
                              uint32_t M,
                              uint32_t N,
                              uint32_t O,
                              int32_t * __restrict__ pDstC) {
        
        uint32_t i; // loop counter
        uint32_t j; // loop counter
        uint32_t k; // loop counter

        uint32_t mod = N & 0x7;

        if(mod == 7){  
          for(i=0; i < M; i++){
            for(k=0; k < O; k++){
              int32_t sum1 = 0;
              int32_t sum2 = 0;
              for(j=0; j<N/8; j++){
                v4s aVec1 = *((v4s*)&(pSrcA[i*N + j*8]));
                v4s bVec1 = *((v4s*)&(pSrcB[k*N + j*8]));
                v4s aVec2 = *((v4s*)&(pSrcA[i*N + j*8+4]));
                v4s bVec2 = *((v4s*)&(pSrcB[k*N + j*8+4]));
                
                sum1 = __SUMDOTP4(aVec1, bVec1, sum1);
                sum2 = __SUMDOTP4(aVec2, bVec2, sum2);
              }
              v4s aRem = *((v4s*)&(pSrcA[i*N + j*8]));
              v4s bRem = *((v4s*)&(pSrcB[k*N + j*8]));
              sum1 = __SUMDOTP4(aRem,bRem,sum1);
              sum1 += pSrcA[i*N+j*8+4]*pSrcB[k*N+j*8+4];
              sum1 += pSrcA[i*N+j*8+5]*pSrcB[k*N+j*8+5];
              sum1 += pSrcA[i*N+j*8+6]*pSrcB[k*N+j*8+6];
              pDstC[i*O +k] = sum1+sum2;
            }
          }
        } else if(mod == 6){  
          for(i=0; i < M; i++){
            for(k=0; k < O; k++){
              int32_t sum1 = 0;
              int32_t sum2 = 0;
              for(j=0; j<N/8; j++){
                v4s aVec1 = *((v4s*)&(pSrcA[i*N + j*8]));
                v4s bVec1 = *((v4s*)&(pSrcB[k*N + j*8]));
                v4s aVec2 = *((v4s*)&(pSrcA[i*N + j*8+4]));
                v4s bVec2 = *((v4s*)&(pSrcB[k*N + j*8+4]));
                
                sum1 = __SUMDOTP4(aVec1, bVec1, sum1);
                sum2 = __SUMDOTP4(aVec2, bVec2, sum2);
              }
              v4s aRem = *((v4s*)&(pSrcA[i*N + j*8]));
              v4s bRem = *((v4s*)&(pSrcB[k*N + j*8]));
              sum1 = __SUMDOTP4(aRem,bRem,sum1);
              sum1 += pSrcA[i*N+j*8+4]*pSrcB[k*N+j*8+4];
              sum1 += pSrcA[i*N+j*8+5]*pSrcB[k*N+j*8+5];
              pDstC[i*O +k] = sum1+sum2;
            }
          }
        } else if(mod == 5){  
          for(i=0; i < M; i++){
            for(k=0; k < O; k++){
              int32_t sum1 = 0;
              int32_t sum2 = 0;
              for(j=0; j<N/8; j++){
                v4s aVec1 = *((v4s*)&(pSrcA[i*N + j*8]));
                v4s bVec1 = *((v4s*)&(pSrcB[k*N + j*8]));
                v4s aVec2 = *((v4s*)&(pSrcA[i*N + j*8+4]));
                v4s bVec2 = *((v4s*)&(pSrcB[k*N + j*8+4]));
                
                sum1 = __SUMDOTP4(aVec1, bVec1, sum1);
                sum2 = __SUMDOTP4(aVec2, bVec2, sum2);
              }
              v4s aRem = *((v4s*)&(pSrcA[i*N + j*8]));
              v4s bRem = *((v4s*)&(pSrcB[k*N + j*8]));
              sum1 = __SUMDOTP4(aRem,bRem,sum1);
              sum1 += pSrcA[i*N+j*8+4]*pSrcB[k*N+j*8+4];
              pDstC[i*O +k] = sum1+sum2;
            }
          }
        } else if(mod == 4){  
          for(i=0; i < M; i++){
            for(k=0; k < O; k++){
              int32_t sum1 = 0;
              int32_t sum2 = 0;
              for(j=0; j<N/8; j++){
                v4s aVec1 = *((v4s*)&(pSrcA[i*N + j*8]));
                v4s bVec1 = *((v4s*)&(pSrcB[k*N + j*8]));
                v4s aVec2 = *((v4s*)&(pSrcA[i*N + j*8+4]));
                v4s bVec2 = *((v4s*)&(pSrcB[k*N + j*8+4]));
                
                sum1 = __SUMDOTP4(aVec1, bVec1, sum1);
                sum2 = __SUMDOTP4(aVec2, bVec2, sum2);
              }
              v4s aRem = *((v4s*)&(pSrcA[i*N + j*8]));
              v4s bRem = *((v4s*)&(pSrcB[k*N + j*8]));
              sum1 = __SUMDOTP4(aRem,bRem,sum1);
              pDstC[i*O +k] = sum1+sum2;
            }
          }
        } else if(mod == 3){  
          for(i=0; i < M; i++){
            for(k=0; k < O; k++){
              int32_t sum1 = 0;
              int32_t sum2 = 0;
              for(j=0; j<N/8; j++){
                v4s aVec1 = *((v4s*)&(pSrcA[i*N + j*8]));
                v4s bVec1 = *((v4s*)&(pSrcB[k*N + j*8]));
                v4s aVec2 = *((v4s*)&(pSrcA[i*N + j*8+4]));
                v4s bVec2 = *((v4s*)&(pSrcB[k*N + j*8+4]));
                
                sum1 = __SUMDOTP4(aVec1, bVec1, sum1);
                sum2 = __SUMDOTP4(aVec2, bVec2, sum2);
              }
              sum1 += pSrcA[i*N+j*8]*pSrcB[k*N+j*8];
              sum1 += pSrcA[i*N+j*8+1]*pSrcB[k*N+j*8+1];
              sum1 += pSrcA[i*N+j*8+2]*pSrcB[k*N+j*8+2];
              pDstC[i*O +k] = sum1+sum2;
            }
          }
        } else if(mod == 2){  
          for(i=0; i < M; i++){
            for(k=0; k < O; k++){
              int32_t sum1 = 0;
              int32_t sum2 = 0;
              for(j=0; j<N/8; j++){
                v4s aVec1 = *((v4s*)&(pSrcA[i*N + j*8]));
                v4s bVec1 = *((v4s*)&(pSrcB[k*N + j*8]));
                v4s aVec2 = *((v4s*)&(pSrcA[i*N + j*8+4]));
                v4s bVec2 = *((v4s*)&(pSrcB[k*N + j*8+4]));
                
                sum1 = __SUMDOTP4(aVec1, bVec1, sum1);
                sum2 = __SUMDOTP4(aVec2, bVec2, sum2);
              }
              sum1 += pSrcA[i*N+j*8]*pSrcB[k*N+j*8];
              sum1 += pSrcA[i*N+j*8+1]*pSrcB[k*N+j*8+1];
              pDstC[i*O +k] = sum1+sum2;
            }
          }
        } else if(mod == 1){  
          for(i=0; i < M; i++){
            for(k=0; k < O; k++){
              int32_t sum1 = 0;
              int32_t sum2 = 0;
              for(j=0; j<N/8; j++){
                v4s aVec1 = *((v4s*)&(pSrcA[i*N + j*8]));
                v4s bVec1 = *((v4s*)&(pSrcB[k*N + j*8]));
                v4s aVec2 = *((v4s*)&(pSrcA[i*N + j*8+4]));
                v4s bVec2 = *((v4s*)&(pSrcB[k*N + j*8+4]));
                
                sum1 = __SUMDOTP4(aVec1, bVec1, sum1);
                sum2 = __SUMDOTP4(aVec2, bVec2, sum2);
              }
              sum1 += pSrcA[i*N+j*8]*pSrcB[k*N+j*8];
              pDstC[i*O +k] = sum1+sum2;
            }
          }
        } else {  
          for(i=0; i < M; i++){
            for(k=0; k < O; k++){
              int32_t sum1 = 0;
              int32_t sum2 = 0;
              for(j=0; j<N/8; j++){
                v4s aVec1 = *((v4s*)&(pSrcA[i*N + j*8]));
                v4s bVec1 = *((v4s*)&(pSrcB[k*N + j*8]));
                v4s aVec2 = *((v4s*)&(pSrcA[i*N + j*8+4]));
                v4s bVec2 = *((v4s*)&(pSrcB[k*N + j*8+4]));
                
                sum1 = __SUMDOTP4(aVec1, bVec1, sum1);
                sum2 = __SUMDOTP4(aVec2, bVec2, sum2);
              }
              pDstC[i*O +k] = sum1+sum2;
            }
          }
        }
}

#endif

// undefine BASIC_VERSION
/**
   @} end of BasicMatMultTransKernels group
*/
