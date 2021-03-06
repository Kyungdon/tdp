/* Copyright (c) 2016, Julian Straub <jstraub@csail.mit.edu> Licensed
 * under the MIT license. See the license file LICENSE.
 */
#pragma once
#include <assert.h>
#include <iostream>
#include <stddef.h>
#include <algorithm>
#include <tdp/data/image.h>
#ifdef CUDA_FOUND
#include <tdp/cuda/cuda.h>
#endif
#include <tdp/data/storage.h>

namespace tdp {

#ifdef CUDA_FOUND
  // TODO: no idea why tempalted header + explicit instantiation does
  // not work
//template <typename T>
//void PyrDown(
//    const Image<T>& Iin,
//    Image<T>& Iout
//    );

void PyrDown(
    const Image<Vector3fda>& Iin,
    Image<Vector3fda>& Iout
    );
void PyrDown(
    const Image<Vector2fda>& Iin,
    Image<Vector2fda>& Iout
    );
void PyrDown(
    const Image<float>& Iin,
    Image<float>& Iout
    );
void PyrDown(
    const Image<uint8_t>& Iin,
    Image<uint8_t>& Iout
    );

void PyrDownBlur(
    const Image<Vector2fda>& Iin,
    Image<Vector2fda>& Iout,
    float sigma_in
    );
void PyrDownBlur(
    const Image<float>& Iin,
    Image<float>& Iout,
    float sigma_in
    );
void PyrDownBlur(
    const Image<uint8_t>& Iin,
    Image<uint8_t>& Iout,
    float sigma_in
    );
void PyrDownBlur9(
    const Image<Vector2fda>& Iin,
    Image<Vector2fda>& Iout,
    float sigma_in
    );
void PyrDownBlur9(
    const Image<float>& Iin,
    Image<float>& Iout,
    float sigma_in
    );
void PyrDownBlur9(
    const Image<uint8_t>& Iin,
    Image<uint8_t>& Iout,
    float sigma_in
    );

#endif

Vector2fda ConvertLevel(const Vector2fda& uv, int lvlFrom, int lvlTo);
Vector2fda ConvertLevel(const Vector2ida& uv, int lvlFrom, int lvlTo);

template<typename T, int LEVELS>
class Pyramid {
 public:
  Pyramid()
    : w_(0), h_(0), ptr_(nullptr), storage_(Storage::Unknown)
  {}
  Pyramid(size_t w, size_t h, T* ptr, enum Storage storage = Storage::Unknown)
    : w_(w), h_(h), ptr_(ptr), storage_(storage)
  {}

  TDP_HOST_DEVICE
  const T& operator()(size_t lvl, size_t u, size_t v) const {
    return *(ImgPtr(lvl)+v*Width(lvl)+u);
  }

  TDP_HOST_DEVICE
  T& operator()(size_t lvl, size_t u, size_t v) {
    return *(ImgPtr(lvl)+v*Width(lvl)+u);
  }

  TDP_HOST_DEVICE
  const T& operator[](size_t i) const {
    return *(ptr_+i);
  }

  TDP_HOST_DEVICE
  T& operator[](size_t i) {
    return *(ptr_+i);
  }

  TDP_HOST_DEVICE
  T* ImgPtr(size_t lvl) const {
    return ptr_+NumElemsToLvl(lvl);
  }

  const Image<T> GetConstImage(int lvl) const {
    if (lvl < LEVELS) {
      return Image<T>(Width(lvl),Height(lvl),ImgPtr(lvl),storage_);
    }
    assert(false);
    return Image<T>(0,0,nullptr);
  }

  Image<T> GetImage(int lvl) {
    if (lvl < LEVELS) {
      return Image<T>(Width(lvl),Height(lvl),ImgPtr(lvl),storage_);
    }
    assert(false);
    return Image<T>(0,0,nullptr);
  }

  size_t Width(int lvl) const { return w_ >> lvl; };
  size_t Height(int lvl) const { return h_ >> lvl; };
  size_t Lvls() const { return LEVELS; }

  size_t SizeBytes() const { return NumElemsToLvl(w_,h_,LEVELS)*sizeof(T); }

  size_t NumElemsToLvl(int lvl) const { return NumElemsToLvl(w_,h_,lvl); }
  size_t NumElems() const { return NumElemsToLvl(LEVELS); }

  static size_t NumElemsToLvl(size_t w, size_t h, int lvl) { 
    return w*h*((1<<lvl)-1)/(1<<(std::max(0,lvl-1))); 
  }

  void Fill(T value) { 
    for (size_t i=0; i<NumElemsToLvl(w_,h_,LEVELS); ++i) 
      ptr_[i] = value; 
  }

  std::string Description() const {
    std::stringstream ss;
    ss << w_ << "x" << h_ << " lvls: " << LEVELS
      << " " << SizeBytes() << "bytes " 
      << " ptr: " << ptr_ << " storage: " << storage_;
    return ss.str();
  }

#ifdef CUDA_FOUND
  /// Perform copy from the given src pyramid to this pyramid.
  /// Use type to specify from which memory to which memory to copy.
  void CopyFrom(const Pyramid<T,LEVELS>& src, cudaMemcpyKind type,
      int lvl0=0, int lvlE=LEVELS) {
    if (src.SizeBytes() == SizeBytes()) {
      int offset = NumElemsToLvl(w_, h_, lvl0);
      int sizeBytes = (NumElemsToLvl(w_, h_, lvlE)-offset)*sizeof(T);
      checkCudaErrors(cudaMemcpy(&ptr_[offset], 
            &src.ptr_[offset], sizeBytes, type));
//      checkCudaErrors(cudaMemcpy(ptr_, 
//            src.ptr_, src.SizeBytes(), type));
    } else {
      std::cerr << "ERROR: not copying pyramid since sizes dont match" 
        << std::endl << Description() 
        << std::endl << src.Description() 
        << std::endl;
    }
  }
  void CopyFrom(const Pyramid<T,LEVELS>& src, int lvl0=0, int
      lvlE=LEVELS) {
    CopyFrom(src, CopyKindFromTo(src.storage_, storage_),
          lvl0, lvlE);
  }
#endif

  size_t w_;
  size_t h_;
  T* ptr_;
  enum Storage storage_;
 private:
};

#ifdef CUDA_FOUND
template<typename T, int LEVELS>
void ConstructPyramidFromImage(const Image<T>& I, Pyramid<T,LEVELS>& P) {
  P.GetImage(0).CopyFrom(I);
  CompletePyramid(P);
}

/// Complete pyramid from first level using pyrdown without blurr.
template<typename T, int LEVELS>
void CompletePyramid(Pyramid<T,LEVELS>& P) {
  if (P.storage_ == Storage::Gpu) {
    // P is on GPU so perform downsampling on GPU
    for (int lvl=1; lvl<LEVELS; ++lvl) {
      Image<T> Isrc = P.GetImage(lvl-1);
      Image<T> Idst = P.GetImage(lvl);
      PyrDown(Isrc, Idst);
    }
  } else {
    // P is on CPU so perform downsampling there as well
    for (int lvl=1; lvl<LEVELS; ++lvl) {
      Image<T> Isrc = P.GetImage(lvl-1);
      Image<T> Idst = P.GetImage(lvl);
      for (size_t v=0; v<Idst.h_; ++v) {
        T* dst = Idst.RowPtr(v);
        T* src0 = Idst.RowPtr(v*2);
        T* src1 = Idst.RowPtr(v*2+1);
        for (size_t u=0; u<Idst.w_; ++u) {
          dst[u] = 0.25*(src0[u*2] + src0[u*2+1] + src1[u*2] + src1[u*2+1]);
        }
      }
    }
  }
}

template<typename T, int LEVELS>
void ConstructPyramidFromImage(const Image<T>& I, Pyramid<T,LEVELS>& P, float sigma) {
  P.GetImage(0).CopyFrom(I);
  CompletePyramidBlur(P, sigma);
}

/// Use PyrDown with small Gaussian Blur.
/// @param sigma whats the expected std on the first level - to only
/// smooth over pixels that are within 3 sigma of the center pixel
template<typename T, int LEVELS>
void CompletePyramidBlur(Pyramid<T,LEVELS>& P, float sigma,
    int lvlE=LEVELS) {
  if (P.storage_ == Storage::Gpu) {
    // P is on GPU so perform downsampling on GPU
    for (int lvl=1; lvl<lvlE; ++lvl) {
      Image<T> Isrc = P.GetImage(lvl-1);
      Image<T> Idst = P.GetImage(lvl);
      PyrDownBlur(Isrc, Idst,sigma);
    }
  } else {
    assert(false);
  }
}

/// Use PyrDown with larger Gaussian Blur.
/// @param sigma whats the expected std on the first level - to only
/// smooth over pixels that are within 3 sigma of the center pixel
template<typename T, int LEVELS>
void CompletePyramidBlur9(Pyramid<T,LEVELS>& P, float sigma,
    int lvlE=LEVELS) {
  if (P.storage_ == Storage::Gpu) {
    // P is on GPU so perform downsampling on GPU
    for (int lvl=1; lvl<lvlE; ++lvl) {
      Image<T> Isrc = P.GetImage(lvl-1);
      Image<T> Idst = P.GetImage(lvl);
      PyrDownBlur9(Isrc, Idst,sigma);
    }
  } else {
    assert(false);
  }
}

/// Construct a image from a pyramid by pasting levels into a single
/// image.
template<typename T, int LEVELS>
void PyramidToImage(Pyramid<T,LEVELS>& P, Image<T>& I) {
  Image<T> IlvlSrc = P.GetImage(0);
  Image<T> Ilvl(P.Width(0), P.Height(0), I.pitch_, I.ptr_, I.storage_);
  Ilvl.CopyFrom(IlvlSrc);
  int v0 = 0;
  for (int lvl=1; lvl<LEVELS; ++lvl) {
    IlvlSrc = P.GetImage(lvl);
    Image<T> IlvlDst(P.Width(lvl), P.Height(lvl), I.pitch_, 
        &I(P.Width(0),v0),I.storage_);
    IlvlDst.CopyFrom(IlvlSrc);
    v0 += P.Height(lvl);
  }
}
#endif

}
