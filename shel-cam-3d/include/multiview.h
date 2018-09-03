//
// Created by xiaopeng on 18-5-8.
//

#ifndef MULTICAMERA_CALIBRATION_MULTIVIEW_H
#define MULTICAMERA_CALIBRATION_MULTIVIEW_H

#include <opencv2/core.hpp>
#include <vector>
#include <Eigen/Dense>

struct Intrinsic {
    double focal[2];
    double center[2];
    double distort[2];
};

struct Transformation {
    Eigen::Matrix3d rotation;
    Eigen::Vector3d translation;
};

//void getEpiplorLine(cv::Point2f &point2D,
//                    const std::string& intricFile,
//                    const std::string& extricFile,
//                    const int in_cam,
//                    const int out_cam,
//                    const double &z1,
//                    const double &z2,
//                    std::vector<cv::Point2f> &corner_reproj);

void load_intrinsic(const std::string fileName, std::vector<Intrinsic>& Ks);

void load_extrisic(const std::string fileName, std::vector<Transformation>& Ts);

void get_pose(cv::Point2f& point2D,
              const int cam_num,
              std::vector<Intrinsic>& cam_intrinsic,
              std::vector<Transformation>& Tcw,
              const double& length,
              const double& width,
              const double& height,
              cv::Point3f& pt_world);

void reproj_2d_to_2d(cv::Point2f& point2D_src,
                     const int cam_id_src,
                     std::vector<Intrinsic>& cam_intrinsic,
                     std::vector<Transformation>& Tcw,
                     cv::Point2f& point2D_dst_0,
                     cv::Point2f& point2D_dst_1,
                     const int cam_id_dst);

void proj_3d_to_2d(cv::Point3f& pt_world,
                   std::vector<Intrinsic>& cam_intrinsic,
                   std::vector<Transformation>& Tcw,
                   cv::Point2f& point2D_dst,
                   const int cam_id_dst);

#endif //MULTICAMERA_CALIBRATION_MULTIVIEW_H
