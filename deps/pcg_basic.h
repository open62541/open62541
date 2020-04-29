/*
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *     http://www.pcg-random.org
 */

#ifndef PCG_BASIC_H_
#define PCG_BASIC_H_

#include <open62541/config.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pcg_state_setseq_64 {
    uint64_t state;  /* RNG state.  All values are possible. */
    uint64_t inc;    /* Controls which RNG sequence (stream) is selected. Must
                      * *always* be odd. */
} pcg32_random_t;

#define PCG32_INITIALIZER { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL }

void pcg32_srandom_r(pcg32_random_t* rng, uint64_t initial_state, uint64_t initseq);
uint32_t pcg32_random_r(pcg32_random_t* rng);

#ifdef __cplusplus
}
#endif

#endif /* PCG_BASIC_H_ */
