/******************************************************************************
 * Copyright 2017 Baidu Robotic Vision Authors. All Rights Reserved.
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

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <xp_utilities.h>
#include <getopt.h>
#include <image_u8.h>
#include <pnm.h>
#include <tag25h7.h>
#include <tag25h9.h>
#include <tag36artoolkit.h>
#include <tag36h10.h>
#include <tag36h11.h>
#include <zarray.h>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace MARKER {

cv::Matx31d generate_C_om_O_from_markers(
    const cv::Mat& in_mat,
    const cv::Matx<double, 5, 1>& dist_coeffs,
    const cv::Matx33d& camera_matrix,
    const int width,
    const int height) {
  std::vector<cv::Point3f> marker_pos_3d =
      calc_corner_positions_from_opencv_mat(in_mat, width, height);
  std::vector<cv::Point2f> marker_pos_2d;
  std::vector<int> marker_id;
  generate_tag_results_from_opencv_mat(in_mat, &marker_pos_2d, &marker_id);

  // C: camera, B: body (imu), G: global earth (z up), O: object,
  // om: omega, t: translation, T: 4x4 transformation
  // camera = C_T_O * object;
  cv::Matx31d C_om_O, C_t_O;
  cv::solvePnP(marker_pos_3d, marker_pos_2d, camera_matrix, dist_coeffs, C_om_O, C_t_O);

  return C_om_O;
}

// currently we are using width = 7, height = 5
std::vector<cv::Point3f> calc_corner_positions_from_opencv_mat(
    const cv::Mat& mat,
    const int width,
    const int height) {
  const float square_size = 1.0;
  float size4x = 4 * square_size;
  float size5x = 5 * square_size;

  std::vector<cv::Point3f> positions;
  cv::Point3f top_left, top_right, bottom_right, bottom_left;

  // MARKER corner calculation: row by row
  // in each row: top_left -> top_right -> bottom_right -> bottom_left
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      top_left.x = j * size5x;
      top_left.y = i * size5x;
      top_left.z = 0;
      top_right.x = j * size5x + size4x;
      top_right.y = i * size5x;
      top_right.z = 0;
      bottom_right.x = j * size5x + size4x;
      bottom_right.y = i * size5x + size4x;
      bottom_right.z = 0;
      bottom_left.x = j * size5x;
      bottom_left.y = i * size5x + size4x;
      bottom_left.z = 0;
      positions.push_back(top_left);
      positions.push_back(top_right);
      positions.push_back(bottom_right);
      positions.push_back(bottom_left);
    }
  }

  return positions;
}

void generate_tag_results_from_opencv_mat(
    const cv::Mat& mat,
    std::vector<cv::Point2f>* corner_positions,
    std::vector<int>* corner_ids) {
  // TODO(XP): remove this by calling pnm_create_from_opencv_mat
  std::string testString = "/tmp/tmp_pnm.pnm";
  std::cout << "the string: " << testString << std::endl;
  cv::imwrite(testString, mat);
  image_u8_t *im = image_u8_create_from_pnm(testString.c_str());

  if (im == NULL) {
    printf("im is NULL\n");
  }

  /*
    Usage: ./april_tag [options] <input files>
    -h | --help           [ true ]       Show this help
    -d | --debug          [ false ]      Enable debugging output (slow)
    -q | --quiet          [ false ]      Reduce output
    -f | --family         [ tag36h11 ]   Tag family to use
    --border         [ 1 ]          Set tag family border size
    -i | --iters          [ 1 ]          Repeat processing on input set this many times
    -t | --threads        [ 4 ]          Use this many CPU threads
    -x | --decimate       [ 1.0 ]        Decimate input image by this factor
    -b | --blur           [ 0.0 ]        Apply low-pass blur to input
    -0 | --refine-edges   [ true ]       Spend more time trying to align edges of tags
    -1 | --refine-decode  [ false ]      Spend more time trying to decode tags
    -2 | --refine-pose    [ false ]      Spend more time trying to precisely localize tags
  */
  // every parameter below is hard-coded by design with the default values
  apriltag_family_t *tf = tag36h11_create();
  tf->black_border = 1;
  apriltag_detector_t *td = apriltag_detector_create();
  apriltag_detector_add_family(td, tf);
  td->quad_decimate = 1.0;
  td->quad_sigma = 0.0;
  td->nthreads = 4;
  td->debug = false;
  td->refine_edges = true;
  td->refine_decode = false;
  td->refine_pose = false;
  bool quiet = true;
  // so far one iteration can always succeed so we never try multiple iterations
  // int maxiters = 1;
  // const int hamm_hist_max = 10;

  zarray_t *detections = apriltag_detector_detect(td, im);
  // std::vector<at_res> corners;
  // std::vector<cv::Point2f> corner_positions;
  corner_positions->clear();
  corner_positions->reserve(zarray_size(detections) * 4);
  corner_ids->clear();
  corner_ids->reserve(zarray_size(detections) * 4);

  for (int i = 0; i < zarray_size(detections); i++) {
    apriltag_detection_t *det;
    zarray_get(detections, i, &det);

    if (!quiet) {
      printf("detection %3d: id (%2dx%2d)-%-4d, hamming %d, goodness %8.3f, margin %8.3f\n",
             i, det->family->d * det->family->d, det->family->h, det->id,
             det->hamming, det->goodness, det->decision_margin);
      printf("XP: centerX %8.3f, centerY %8.3f\n", det->c[0], det->c[1]);
      printf("XP: corner0X %8.3f, corner0Y %8.3f, corner1X %8.3f, corner1Y %8.3f, corner2X %8.3f,"
             " corner2Y %8.3f, corner3X %8.3f, corner3Y %8.3f\n",
             det->p[0][0], det->p[0][1], det->p[1][0], det->p[1][1],
             det->p[2][0], det->p[2][1], det->p[3][0], det->p[3][1]);
    }

    for (int i = 0; i < 4; i++) {
      cv::Point2f tmpCorner(det->p[i][0], det->p[i][1]);
      corner_positions->push_back(tmpCorner);
      corner_ids->push_back(det->id);
    }
  }

  apriltag_detections_destroy(detections);
  if (quiet) {
    printf("%12.3f", timeprofile_total_utime(td->tp) / 1.0E3);
  }
  delete im->buf;
  apriltag_detector_destroy(td);
  tag36h11_destroy(tf);

  // return corner_positions;
}
apriltag_detector_t* create_apriltag_detector_t() {
  apriltag_family_t *tf = tag36h11_create();
  tf->black_border = 1;
  apriltag_detector_t* td = apriltag_detector_create();
  apriltag_detector_add_family(td, tf);
  td->quad_decimate = 1.0;
  td->quad_sigma = 0.0;
  td->nthreads = 4;
  td->debug = false;
  td->refine_edges = true;
  td->refine_decode = false;
  td->refine_pose = false;
  return td;
}

int det_tag(
    apriltag_detector_t* td_ptr,
    const cv::Mat& mat,
    std::vector<cv::Point2f>* corner_positions,
    std::vector<int>* corner_ids) {
  // first convert mat to single channel
  // april tag requires stride size to be multiple of 96
  // int img_single_channel_cols = mat.cols % 96 == 0 ? mat.cols : (mat.cols / 96 + 1) * 96;
  cv::Mat_<uchar> img_single_channel(mat.rows, (mat.cols / 96 + 1) * 96);
  img_single_channel.setTo(0x00);
  if (mat.channels() == 1) {
    // Note this has to be copyTo to make android work
    mat.copyTo(img_single_channel.colRange(0, mat.cols));
  } else {
    cv::cvtColor(mat, img_single_channel.colRange(0, mat.cols), CV_BGR2GRAY);
  }
  image_u8_t im = {
      .width = img_single_channel.cols,
      .height = img_single_channel.rows,
      .stride = img_single_channel.cols,
      .buf = img_single_channel.data };

  zarray_t *detections = apriltag_detector_detect(td_ptr, &im);
  int det_num = zarray_size(detections);
  corner_positions->clear();
  corner_positions->reserve(det_num * 4);
  corner_ids->clear();
  corner_ids->reserve(det_num * 4);

  for (int i = 0; i < det_num; i++) {
    apriltag_detection_t *det;
    zarray_get(detections, i, &det);
    for (int j = 0; j < 4; j++) {
      cv::Point2f tmpCorner(det->p[j][0], det->p[j][1]);
      corner_positions->push_back(tmpCorner);
      corner_ids->push_back(det->id * 10 + j + 1);  // corner id starts from 1
    }
  }

  apriltag_detections_destroy(detections);
  // do not destroy im because it does not own any data
  return det_num;
}

/*
// TODO(XP): not working yet
image_u8_t *image_u8_t_create_from_opencv_mat(const cv::Mat& mat) {
  pnm_t *pnm = (pnm_t *)calloc(1, sizeof(pnm_t));
  pnm->format = PNM_FORMAT_GRAY;
  pnm->width = mat.cols;
  pnm->height = mat.rows;
  pnm->buflen = pnm->width * pnm->height;
  pnm->buf = (uint8_t *)malloc(pnm->buflen);
  pnm->buf = (uint8_t *)mat.data;

  size_t len = fread(pnm->buf, 1, pnm->buflen, f);
  if (len != pnm->buflen)
    goto error;
  fclose(f);

  image_u8_t *im = NULL;

  im = image_u8_create_alignment(pnm->width, pnm->height, 96);
  for (int y = 0; y < im->height; y++) {
    memcpy(&im->buf[y*im->stride], &pnm->buf[y*im->width], im->width);
  }
  pnm_destroy(pnm);

  return im;
}
*/

};  // namespace MARKER
