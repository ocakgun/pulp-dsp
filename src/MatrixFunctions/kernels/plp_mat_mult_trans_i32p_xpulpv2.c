/* =====================================================================
 * Project:      PULP DSP Library
 * Title:        plp_mat_mult_i32p_xpulpv2.c
 * Description:  3parallel 2-bit integer matrix multiplication for XPULPV2
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
   @brief         Parallel matrix transposed matrix multiplication of a 32-bit integer matrices for RV32IM extension.
   @param[in]  args      pointer to plp_mat_mult_instance_i32 struct initialized by plp_mat_mult_i32_parallel
   @return        none
*/

// define BASIC_VERSION // if used don't forget to also use the undefine at end of file

#ifdef BASIC_VERSION

void plp_mat_mult_trans_i32p_xpulpv2( void* args) {
        plp_mat_mult_instance_i32* arguments = (plp_mat_mult_instance_i32*) args;
        const int32_t * __restrict__ pSrcA = arguments->pSrcA;
        const int32_t * __restrict__ pSrcB = arguments->pSrcB;
        uint32_t M = arguments->M;
        uint32_t N = arguments->N;
        uint32_t O = arguments->O;
        uint32_t nPE = arguments->nPE;
        int32_t * __restrict__ pDstC = arguments->pDstC;
        
        uint32_t i; // loop counter for M
        uint32_t j; // loop counter for N
        uint32_t k; // loop counter for O

        int core_id = rt_core_id();
        int step = (O-1+nPE)/nPE;
        uint32_t START = step*core_id;
        uint32_t END = (core_id != rt_nb_pe()-1) ? START+step : M;

        for(i=START; i < END; i++){
          for(k=0; k < M; k++){
            int32_t sum = 0;
            for(j=0; j < N; j++){
              sum = sum + pSrcA[i*N + j]*pSrcB[k*N + j];
            }
            pDstC[i*O + k] = sum;
          }
        }

        rt_team_barrier();
}

#else 

void plp_mat_mult_trans_i32p_xpulpv2( void* args) {
        plp_mat_mult_instance_i32* arguments = (plp_mat_mult_instance_i32*) args;
        const int32_t * __restrict__ pSrcA = arguments->pSrcA;
        const int32_t * __restrict__ pSrcB = arguments->pSrcB;
        uint32_t M = arguments->M;
        uint32_t N = arguments->N;
        uint32_t O = arguments->O;
        uint32_t nPE = arguments->nPE;
        int32_t * __restrict__ pDstC = arguments->pDstC;
        
        uint32_t i; // loop counter for M
        uint32_t j; // loop counter for N
        uint32_t k; // loop counter for O

        int core_id = rt_core_id();
        int step = (O-1+nPE)/nPE;
        uint32_t START = step*core_id;
        uint32_t END = (core_id != rt_nb_pe()-1) ? START+step : M;

        if(N & 0x1){
          for(i=START; i < END; i++){
            for(k=0; k < O; k++){
              int32_t sum1 = 0;
              int32_t sum2 = 0;
              for(j=0; j<N/2; j++){
                sum1 = sum1 + pSrcA[i*N + j*2]*pSrcB[k*N+j*2];
                sum2 = sum2 + pSrcA[i*N + j*2+1]*pSrcB[k*N+j*2+1];
              }
              pDstC[i*O +k] = sum1 + sum2 + pSrcA[i*N + j*2]*pSrcB[k*N+j*2];
            }
          }
        } else {
          for(i=START; i < END; i++){
            for(k=0; k < O; k++){
              int32_t sum1 = 0;
              int32_t sum2 = 0;
              for(j=0; j<N/2; j++){
                sum1 = sum1 + pSrcA[i*N + j*2]*pSrcB[k*N+j*2];
                sum2 = sum2 + pSrcA[i*N + j*2+1]*pSrcB[k*N+j*2+1];
              }
              pDstC[i*O +k] = sum1 + sum2;
            }
          }
        }

        rt_team_barrier();
}

#endif

// undefine BASIC_VERSION
/**
   @} end of BasicMatMultTransKernels group
*/
