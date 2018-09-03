#include <string>
#include <Eigen/Dense>
#include <glog/logging.h>
#include <opencv2/core.hpp>
#include <vector>
#include <cv.hpp>

struct Intrinsic {
  double focal[2];
  double center[2];
  double distort[2];
};

void loadExtricParam(const std::string &extric_fp, std::vector<Eigen::Matrix4f> &v_tlw) {
  FILE *fp = fopen(extric_fp.c_str(), "r");
  if (!fp) {
    LOG(ERROR) << "load file error " << extric_fp;
    return;
  }
  int ncam;
  fscanf(fp, "%d", &ncam);
  // left camera world extrinsic
  for (int i = 0; i < ncam; i++) {
    Eigen::Matrix4f tlw = Eigen::Matrix4f::Identity();
    fscanf(fp, "%f %f %f %f", &tlw(0, 0), &tlw(0, 1), &tlw(0, 2), &tlw(0, 3));
    fscanf(fp, "%f %f %f %f", &tlw(1, 0), &tlw(1, 1), &tlw(1, 2), &tlw(1, 3));
    fscanf(fp, "%f %f %f %f", &tlw(2, 0), &tlw(2, 1), &tlw(2, 2), &tlw(2, 3));
    v_tlw.push_back(tlw);
  }
}

void loadIntrinsic(const std::string fileName, std::vector<Intrinsic>& v_intrinsic) {
  FILE* fp = fopen(fileName.c_str(), "r");
  int n = 0;
  fscanf(fp, "%d", &n);
  v_intrinsic.resize(n);

  for (int i = 0; i < n; ++i) {
    fscanf(fp, "%le %le", &v_intrinsic[i].focal[0], &v_intrinsic[i].focal[1]);
    fscanf(fp, "%le %le", &v_intrinsic[i].center[0], &v_intrinsic[i].center[1]);
    fscanf(fp, "%le %le", &v_intrinsic[i].distort[0], &v_intrinsic[i].distort[1]);
  }
}

void get2dRegion(const cv::Mat &depth, const cv::Mat &mask, const std::vector<Intrinsic>& v_intrinsic,
                 const std::vector<Eigen::Matrix4f> &v_tlw,
                 const int cam_num,
                 std::vector<cv::Point2f> &region){

  
  cv::Point2f p_2d;
  std::vector<cv::Point3f> 3d_pts;
  3d_pts.push_back(cv::Point3f(p_3d.x, p_3d.y, p_3d.z));

  cv::Mat camera_k = (cv::Mat_<double>(3, 3) << v_intrinsic[cam_num].focal[0], 0.0, v_intrinsic[cam_num].center[0],
      0.0, v_intrinsic[cam_num].focal[1], v_intrinsic[cam_num].center[1],
      0.0, 0.0, 1.0);
  cv::Mat camera_d = (cv::Mat_<double>(4, 1) << v_intrinsic[cam_num].distort[0], v_intrinsic[cam_num].distort[1], 0, 0);
  Eigen::Matrix3f rcw = v_tlw[cam_num].topLeftCorner(3, 3);
  Eigen::Vector3f tcw = v_tlw[cam_num].topRightCorner(3, 1);

  cv::Mat rvec;
  cv::Mat cv_rcw = (cv::Mat_<double>(3, 3) << rcw(0, 0), rcw(0, 1), rcw(0, 2),
      rcw(1, 0), rcw(1, 1), rcw(1, 2),
      rcw(2, 0), rcw(2, 1), rcw(2, 2));
  Rodrigues(cv_rcw, rvec);
  cv::Mat tvec = (cv::Mat_<double>(3, 1) << tcw(0, 0), tcw(1, 0), tcw(2, 0));

  cv::projectPoints(3d_pts,
                    rvec,
                    tvec,
                    camera_k, camera_d,
                    region
  );

}
