#pragma once
#include <tdp/data/image.h>
#include <tdp/gl/render.h>
#include <tdp/camera/camera_base.h>

namespace tdp {

template<int D, typename Derived>
class ProjectiveAssociation {
 public: 
  ProjectiveAssociation(const CameraBase<float,D,Derived>& cam, 
      size_t w, size_t h)
    : w_(w), h_(h), cam_(cam), tex_(w, h, GL_RGBA), 
     render_(w, h, GL_DEPTH_COMPONENT), fbo_(tex_, render_)
  { }
  ~ProjectiveAssociation() {}


  
  void Associate(pangolin::GlBuffer& vbo,
      SE3f T_cw, float dMin, float dMax, uint32_t numElems) {
    fbo_.Bind();

//    glViewport(0,0,w_,h_);
//    glMatrixMode(GL_PROJECTION);
//    glLoadIdentity();
//    glOrtho(-0.5, w_-0.5, -0.5, h_-0.5, -1, 1);
//    glMatrixMode(GL_MODELVIEW);
//    glLoadIdentity();

    glPushAttrib(GL_VIEWPORT_BIT);
    glPointSize(1);
    glViewport(0, 0, w_, h_);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    tdp::RenderVboIds(vbo, T_cw, cam_, w_, h_, dMin, dMax, numElems);
    fbo_.Unbind();
    glPopAttrib();
    glFinish();
  }
  void GetAssoc(tdp::Image<uint32_t>& z) {
    tex_.Download(z.ptr_, GL_RGBA, GL_UNSIGNED_BYTE);
    for (size_t i=0; i<z.Area(); ++i) {
      z[i] &= 0x00FFFFFF; // alpha channel is 255 always
    }
  }
  void GetAssoc(tdp::Image<uint32_t>& z, tdp::Image<uint8_t>& mask) {
    GetAssoc(z);
    for (size_t i=0; i<z.Area(); ++i) {
      mask[i] = z[i]>0? 1: 0; 
    }
  }

  void Associate(const Image<Vector3fda>& pc_w,
      SE3f T_cw, float dMin, float dMax, uint32_t numElems) {
    if (vbo_.num_elements != pc_w.Area()) {
      vbo_.Reinitialise(pangolin::GlArrayBuffer,pc_w.Area(),GL_FLOAT,3,
        GL_DYNAMIC_DRAW);
    }
    vbo_.Upload(pc_w.ptr_, pc_w.SizeBytes(), 0);
    return Associate(vbo_, T_cw, dMin, dMax, numElems);
  };

  void Associate(pangolin::GlBuffer& vbo,
      SE3f T_cw, float dMin, float dMax) {
    return Associate(vbo, T_cw, dMin, dMax, vbo.num_elements);
  }

  void Associate(const Image<Vector3fda>& pc_w,
      SE3f T_cw, float dMin, float dMax) {
    return Associate(pc_w, T_cw, dMin, dMax, pc_w.Area());
  }
 
  size_t w_, h_;
  CameraBase<float,D,Derived> cam_;
  pangolin::GlTexture tex_;
 private:
  pangolin::GlBuffer vbo_;
  pangolin::GlRenderBuffer render_;
  pangolin::GlFramebuffer fbo_;

};



}
