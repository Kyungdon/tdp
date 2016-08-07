
#include <cuda.h>
#include <stdio.h>
#include <Eigen/Dense>
#include <tdp/normals.h>

namespace tdp {

template<typename T>
__device__
T* RowPtr(Image<T>& I, size_t row) {
  return (T*)((uint8_t*)I.ptr+I.pitch*row);
}

__global__ void KernelSurfaceNormals(Image<float> d,
    Image<float> ddu, Image<float> ddv,
    Image<Eigen::Vector3f> n, float f, float uc, float vc) {

  //const int tid = threadIdx.x;
  const int tid = threadIdx.x + blockDim.x * threadIdx.y;
  const int idx = threadIdx.x + blockDim.x * blockIdx.x;
  const int idy = threadIdx.y + blockDim.y * blockIdx.y;

  if (idx < n.w && idy < n.h) {
    const float di = RowPtr<float>(d,idy)[idx];
    float* ni = (float*)(&RowPtr<Eigen::Vector3f>(n,idy)[idx]);
    if (di > 0) {
      const float ddui = RowPtr<float>(ddu,idy)[idx];
      const float ddvi = RowPtr<float>(ddv,idy)[idx];
      ni[0] = -ddui*f;
      ni[1] = -ddvi*f;
      ni[2] = ((idx-uc)*ddui + (idy-vc)*ddvi + di);
      const float norm = sqrtf(ni[0]*ni[0] + ni[1]*ni[1] + ni[2]*ni[2]);
      ni[0] /= norm;
      ni[1] /= norm;
      ni[2] /= norm;
    } else {
      ni[0] = 0.;
      ni[1] = 0.;
      ni[2] = 0.;
    }
  }
}


void ComputeNormals(
    const Image<float>& d,
    const Image<float>& ddu,
    const Image<float>& ddv,
    const Image<Eigen::Vector3f>& n,
    float f, float uc, float vc) {
  
  size_t w = d.w;
  size_t h = d.h;
  dim3 threads(32,32,1);
  dim3 blocks(w/32+(w%32>0?1:0), h/32+(h%32>0?1:0),1);
  KernelSurfaceNormals<<<blocks,threads>>>(d,ddu,ddv,n,f,uc,vc);

}

}
