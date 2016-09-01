/* Copyright (c) 2016, Julian Straub <jstraub@csail.mit.edu> Licensed
 * under the MIT license. See the license file LICENSE.
 */

#include <tdp/cuda.h>
#include <tdp/nvidia/cuda_global.h>
#include <tdp/eigen/dense.h>

namespace tdp {

template<uint32_t K, uint32_t BLK_SIZE>
__global__ void KernelVectorSum(Image<Vector3fda> x,
    Image<uint32_t> z, uint32_t k0, int N_PER_T,
    Image<Vector4fda> SSs) 
{
  // sufficient statistics for whole blocksize
  // 3 (sum) + 1 (count) 
  __shared__ Vector4fda xSSs[BLK_SIZE*K];

  //const int tid = threadIdx.x;
  const int tid = threadIdx.x;
  const int idx = threadIdx.x + blockDim.x * blockIdx.x;

  // caching 
#pragma unroll
  for(int s=0; s<K; ++s) {
    // this is almost certainly bad ordering
    xSSs[tid*K+s](0) = 0.0f;
    xSSs[tid*K+s](1) = 0.0f;
    xSSs[tid*K+s](2) = 0.0f;
    xSSs[tid*K+s](3) = 0.0f;
  }
  __syncthreads(); // make sure that ys have been cached

  for(int id=idx*N_PER_T; id<min(x.Area(),(idx+1)*N_PER_T); ++id)
  {
    Vector3fda xi = x[id];
    int32_t k = z[id]-k0;
    if(0 <= k && k < K && IsValidData(xi))
    {
      // input sufficient statistics
      xSSs[tid*K+k].topRows(3) += xi;
      xSSs[tid*K+k](3) += 1.0f;
    }
  }

  // old reduction.....
  __syncthreads(); //sync the threads
#pragma unroll
  for(int s=(BLK_SIZE)/2; s>1; s>>=1) {
    if(tid < s)
    {
      const uint32_t si = s*K;
      const uint32_t tidk = tid*K;
#pragma unroll
      for( int k=0; k<K; ++k) {
        xSSs[tidk+k] += xSSs[si+tidk+k];
      }
    }
    __syncthreads();
  }
  if(tid < K) {
#pragma unroll
    for(int i=0; i<4; ++i) {
      // sum the last two remaining matrixes directly into global memory
      atomicAdd_<float>(&SSs[tid](i),xSSs[tid](i)+xSSs[tid+K](i));
    }
  }
}

void VectorSum(Image<Vector3fda> cuX,
    Image<uint32_t> cuZ, uint32_t k0, uint32_t K,
    Image<Vector4fda> cuSSs) 
{
  const int N_PER_T = 16;
  dim3 threads, blocks;
  ComputeKernelParamsForArray(blocks,threads,cuX.Area(),256,N_PER_T);
  if(K == 1){
    KernelVectorSum<1,256><<<blocks,threads>>>(cuX,cuZ,k0,N_PER_T,cuSSs);
  }else if(K==2){
    KernelVectorSum<2,256><<<blocks,threads>>>(cuX,cuZ,k0,N_PER_T,cuSSs);
  }else if(K==3){
    KernelVectorSum<3,256><<<blocks,threads>>>(cuX,cuZ,k0,N_PER_T,cuSSs);
  }else if(K==4){
    KernelVectorSum<4,256><<<blocks,threads>>>(cuX,cuZ,k0,N_PER_T,cuSSs);
  }else if(K==5){
    KernelVectorSum<5,256><<<blocks,threads>>>(cuX,cuZ,k0,N_PER_T,cuSSs);
  }else if(K==6){
    KernelVectorSum<6,256><<<blocks,threads>>>(cuX,cuZ,k0,N_PER_T,cuSSs);
  }else{
    assert(false);
  }
  checkCudaErrors(cudaDeviceSynchronize());
}

}

