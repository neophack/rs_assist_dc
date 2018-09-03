/******************************************************************************
 * Copyright 2017-2018 Baidu Robotic Vision Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include "tag_detector.h"
#include "apriltag-2015-03-18/include/tag36h11.h"  // For tag36h11
#include <glog/logging.h>

namespace XP {
AprilTagDetector::AprilTagDetector() {
  tf_ = tag36h11_create();
  tf_->black_border = 1;
  td_ = apriltag_detector_create();
  apriltag_detector_add_family(td_, tf_);
  td_->quad_decimate = 1.0;
  td_->quad_sigma = 0.0;
  td_->nthreads = 2;
  td_->debug = false;
  td_->refine_edges = true;
  td_->refine_decode = false;
  td_->refine_pose = false;
}
AprilTagDetector::~AprilTagDetector() {
  apriltag_detector_destroy(td_);
  tag36h11_destroy(tf_);
}
int AprilTagDetector::detect(const cv::Mat& img_in_raw,
                       std::vector<cv::KeyPoint>* key_pnts_ptr,
                       cv::Mat* orb_feat_ptr) {
  CHECK_NOTNULL(key_pnts_ptr);
  std::vector<cv::Point2f> corner_positions;
  std::vector<int> corner_ids;
  int det_num = MARKER::det_tag(td_, img_in_raw, &corner_positions, &corner_ids);
  key_pnts_ptr->resize(corner_ids.size());
  CHECK_EQ(corner_ids.size(), corner_positions.size());
  for (size_t i = 0; i < corner_positions.size(); ++i) {
    key_pnts_ptr->at(i).class_id = corner_ids[i];
    key_pnts_ptr->at(i).pt = corner_positions[i];
  }
  if (orb_feat_ptr != nullptr) {
    // TODO(bao) encode april tag as orb
    LOG(FATAL) << "TODO encode april tag as orb";
  }
  return det_num;
}

}  // namespace XP
