/* 
 * A number of functions to generate LCG random sequences
 * Reference:
 * https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
 * https://github.com/ospray/ospray/blob/66fa8108485a8a92ff31ad2e06081bbaf391bc26/modules/cpu/math/random.ih
 */
#ifndef MSL_COMMON_RANDOM_H
#define MSL_COMMON_RANDOM_H
#include <metal_stdlib>

#include "types.h"

uint murmurhash3_mix(uint hash, uint k)
{
  const uint c1 = 0xcc9e2d51;
  const uint c2 = 0x1b873593;
  const uint r1 = 15;
  const uint r2 = 13;
  const uint m = 5;
  const uint n = 0xe6546b64;

  k *= c1;
  k = (k << r1) | (k >> (32 - r1));
  k *= c2;

  hash ^= k;
  hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;

  return hash;
}

uint murmurhash3_finalize(uint hash)
{
  hash ^= hash >> 16;
  hash *= 0x85ebca6b;
  hash ^= hash >> 13;
  hash *= 0xc2b2ae35;
  hash ^= hash >> 16;

  return hash;
}

RNG make_random(uint pixel_id, uint frame_no) {
    RNG rng;
    rng.state = murmurhash3_mix(0, pixel_id);
    rng.state = murmurhash3_mix(rng.state, frame_no);
    rng.state = murmurhash3_finalize(rng.state);

    return rng;
}

uint next_random(thread RNG& rng) {
    rng.state = 1664525u * rng.state + 1013904223u;
    return rng.state;
}

float next_randomf(thread RNG& rng) {
    uint r = next_random(rng);
    return as_type<float>((r & 0x007FFFFFu) | 0x3F800000u) - 1.0;
}

float3 next_random3f(thread RNG& rng) {
    return float3(next_randomf(rng), next_randomf(rng), next_randomf(rng));
}

float2 next_random2f(thread RNG& rng) {
    return float2(next_randomf(rng), next_randomf(rng));
}

#endif