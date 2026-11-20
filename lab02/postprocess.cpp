// ORIGINAL: iou(), cmp() and nms() copied verbatim from tensorrtx
//   https://github.com/wang-xinyu/tensorrtx/blob/master/yolov5/src/postprocess.cpp
// The OpenCV drawing / mask functions of that file are not needed and were left out.
#include <algorithm>
#include <cstring>
#include <map>
#include <vector>
#include <ddtr_library>
#include "trtx_types.h"
#include "allocators.hpp"

#ifndef sel_dt_dets
#define sel_dt_dets vector
#endif
#ifndef sel_alloc_dets
#define sel_alloc_dets ddtr::malloc_allocator
#endif
using Dets = ddtr::sel_dt_dets<Detection, sel_alloc_dets>;
using Flags = ddtr::vector<unsigned char, sel_alloc_dets>;

static float iou(float lbox[4], float rbox[4]) {
  float interBox[] = {
    (std::max)(lbox[0] - lbox[2] / 2.f , rbox[0] - rbox[2] / 2.f), //left
    (std::min)(lbox[0] + lbox[2] / 2.f , rbox[0] + rbox[2] / 2.f), //right
    (std::max)(lbox[1] - lbox[3] / 2.f , rbox[1] - rbox[3] / 2.f), //top
    (std::min)(lbox[1] + lbox[3] / 2.f , rbox[1] + rbox[3] / 2.f), //bottom
  };

  if (interBox[2] > interBox[3] || interBox[0] > interBox[1])
    return 0.0f;

  float interBoxS = (interBox[1] - interBox[0])*(interBox[3] - interBox[2]);
  return interBoxS / (lbox[2] * lbox[3] + rbox[2] * rbox[3] - interBoxS);
}

static bool cmp(const Detection& a, const Detection& b) {
  return a.conf > b.conf;
}

bool operator<(const Detection& a, const Detection& b) {
  return a.conf > b.conf;
}

void nms(std::vector<Detection>& res, float* output, float conf_thresh, float nms_thresh) {
  int det_size = sizeof(Detection) / sizeof(float);
  std::map<float, Dets> m;
  for (int i = 0; i < output[0] && i < kMaxNumOutputBbox; i++) {
    if (output[1 + det_size * i + 4] <= conf_thresh) continue;
    Detection det;
    memcpy(&det, &output[1 + det_size * i], det_size * sizeof(float));
    if (m.count(det.class_id) == 0) m.emplace(det.class_id, Dets());
    m[det.class_id].push_back(det);
  }
  for (auto it = m.begin(); it != m.end(); it++) {
    auto& dets = it->second;
    dets.sort();
#ifdef NMS_FLAG
    // New version: rejected boxes are not erased, they are marked with a flag
    // (1 = rejected, 0 = kept). The flags vector uses the same allocator as Dets.
    Flags flags;
    for (auto x = dets.begin(); x != dets.end(); ++x) flags.push_back(0);
    std::size_t i = 0;
    for (auto mi = dets.begin(); mi != dets.end(); ++mi, ++i) {
      if (flags[i]) continue;            // already rejected by a better box
      auto& item = *mi;
      res.push_back(item);
      auto ni = mi;
      ++ni;
      std::size_t j = i + 1;
      for (; ni != dets.end(); ++ni, ++j) {
        if (!flags[j] && iou(item.bbox, (*ni).bbox) > nms_thresh) {
          flags[j] = 1;                  // mark instead of erase
        }
      }
    }
#else
    for (auto mi = dets.begin(); mi != dets.end(); ++mi) {
      auto& item = *mi;
      res.push_back(item);
      auto ni = mi;
      ++ni;
      while (ni != dets.end()) {
        if (iou(item.bbox, (*ni).bbox) > nms_thresh) {
          ni = dets.erase(ni);
        } else {
          ++ni;
        }
      }
    }
#endif
  }
}