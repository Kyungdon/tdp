/* Copyright (c) 2015, Julian Straub <jstraub@csail.mit.edu> Licensed
 * under the MIT license. See the license file LICENSE.
 */
#pragma once

#include <vector>
#include <Eigen/Dense>
#include <tdp/eigen/dense.h>
#include <tdp/eigen/std_vector.h>
#include <tdp/distributions/vmf.h>
#include <tdp/clustering/dpvmfmeans.hpp>
#include <tdp/manifold/SO3.h>
#include <tdp/data/managed_image.h>

namespace tdp {

void MAPLabelAssignvMFMM( 
    const Image<Vector3fda>& cuN,
    const Image<Vector3fda>& cuTauMu,
    const SO3fda& R_nvmf,
    const Image<float>& cuLogPi,
    Image<uint16_t>& cuZ,
    bool filterHalfSphere);

void MAPLabelAssignvMFMM( 
    std::vector<vMF<float,3>>& vmfs,
    const SO3fda& R_nvmf,
    const Image<Vector3fda>& cuN,
    Image<uint16_t>& cuZ,
    bool filterHalfSphere);

template<typename T>
bool ComputevMFMM(
    const Image<Vector3fda>& n,
    const Image<Vector3fda>& cuN,
    DPvMFmeans& dpvmfmeans,
    size_t maxIt, 
    float minNchangePerc,
    Image<uint16_t>& cuZ,
    std::vector<vMF<T,3>>& vmfs) {
  vmfs.clear();
  // Run the clustering algorithm.
  dpvmfmeans.Compute(n, cuN, cuZ, maxIt, minNchangePerc);
//  eigen_vector<Vector3fda>& centers = dpvmfmeans.centers_;
  std::vector<size_t> Ns = dpvmfmeans.Ns_;
  uint32_t K = dpvmfmeans.K_;

//  for(size_t it=0; it < 10; ++it) {
//    std::cout << "@" << it << std::endl;
    vmfs.clear();
    Eigen::Matrix<float,4,Eigen::Dynamic> xSums =
      tdp::SufficientStats1stOrder(cuN, cuZ, K);
    float W = xSums.bottomRows<1>().sum();
    for(uint32_t k=0; k<K; ++k) {
      Eigen::Vector3f xSum = xSums.block<3,1>(0,k);
      float w = xSums(3,k);
      if (w > 5) {
        T pi = w/W;
        T tau = vMF<double,3>::MLEstimateTau(xSum.cast<double>(),
            xSum.normalized().cast<double>(),w);
        vmfs.push_back(vMF<T,3>(xSum.normalized(),tau,pi));
        vmfs.back().Print();
      }
    }
//    K = vmfs.size();
//    tdp::MAPLabelAssignvMFMM(vmfs, SO3f(), cuN,  cuZ, true);
//  }
  return true;
}


}
