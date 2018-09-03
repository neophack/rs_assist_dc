//
// Created by xiaopeng on 18-5-8.
//

#include "multiview.h"
#include <iostream>

void load_intrinsic(const std::string fileName, std::vector<Intrinsic>& Ks) {
    FILE* fp = fopen(fileName.c_str(), "r");
    int n = 0;
    fscanf(fp, "%d", &n);
    Ks.resize(n);

    for (int i = 0; i < n; ++i) {
        fscanf(fp, "%le %le", &Ks[i].focal[0], &Ks[i].focal[1]);
        fscanf(fp, "%le %le", &Ks[i].center[0], &Ks[i].center[1]);
        fscanf(fp, "%le %le", &Ks[i].distort[0], &Ks[i].distort[1]);
    }
}

void load_extrisic(const std::string fileName, std::vector<Transformation>& Ts) {
    FILE* fp = fopen(fileName.c_str(), "r");
    int n = 0;
    fscanf(fp, "%d", &n);
    Ts.resize(n);

    for (int i = 0; i < n; ++i) {
        fscanf(fp, "%le %le %le %le", &Ts[i].rotation(0, 0), &Ts[i].rotation(0, 1),
               &Ts[i].rotation(0, 2), &Ts[i].translation.x());
        fscanf(fp, "%le %le %le %le", &Ts[i].rotation(1, 0), &Ts[i].rotation(1, 1),
               &Ts[i].rotation(1, 2), &Ts[i].translation.y());
        fscanf(fp, "%le %le %le %le", &Ts[i].rotation(2, 0), &Ts[i].rotation(2, 1),
               &Ts[i].rotation(2, 2), &Ts[i].translation.z());
    }
}

void get_pose(cv::Point2f& point2D,
              const int cam_num,
              std::vector<Intrinsic>& cam_intrinsic,
              std::vector<Transformation>& Tcw,
              const double& length,
              const double& width,
              const double& height,
              cv::Point3f& pt_world) {
    std::vector<cv::Point2f> corner;
    corner.push_back(point2D);
    std::vector<cv::Point2f> undistort_corner;

    cv::Mat camera_k = (cv::Mat_<double>(3, 3) << cam_intrinsic[cam_num].focal[0], 0.0,
                        cam_intrinsic[cam_num].center[0],
                        0.0, cam_intrinsic[cam_num].focal[1], cam_intrinsic[cam_num].center[1],
                        0.0, 0.0, 1.0);
    cv::Mat camera_d = (cv::Mat_<double>(4, 1) << cam_intrinsic[cam_num].distort[0],
                        cam_intrinsic[cam_num].distort[1], 0, 0);

    cv::undistortPoints(corner, undistort_corner, camera_k, camera_d);
    double x = undistort_corner[0].x;
    double y = undistort_corner[0].y;
    Eigen::Matrix3d rcw = Tcw[cam_num].rotation;
    Eigen::Matrix3d rwc = rcw.transpose();
    Eigen::Vector3d twc;
    Eigen::Vector3d tcw;
    tcw = Tcw[cam_num].translation;
    twc = -rwc * tcw;
    double scale = (height - twc(2, 0)) / (rwc(2, 0) * x + rwc(2, 1) * y + rwc(2, 2));
    Eigen::Vector3d pt_cam(scale * x, scale * y, scale);
    Eigen::Vector3d pt = rwc * pt_cam + twc;

    Eigen::Matrix3d transform;
    transform = Eigen::AngleAxisd(0.5 * M_PI, Eigen::Vector3d::UnitZ()) *
                Eigen::AngleAxisd(0.0 * M_PI, Eigen::Vector3d::UnitY()) *
                Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitX());

    pt = transform * pt;
    pt_world = cv::Point3f(pt(0, 0), pt(1, 0), pt(2, 0));
}

void reproj_2d_to_2d(cv::Point2f& point2D_src,
                     const int cam_id_src,
                     std::vector<Intrinsic>& cam_intrinsic,
                     std::vector<Transformation>& Tcw,
                     cv::Point2f& point2D_dst_0,
                     cv::Point2f& point2D_dst_1,
                     const int cam_id_dst) {

}

void proj_3d_to_2d(cv::Point3f& pt_world,
                   std::vector<Intrinsic>& cam_intrinsic,
                   std::vector<Transformation>& Tcw,
                   cv::Point2f& point2D_dst,
                   const int cam_id_dst) {
    cv::Mat camera_k =
        (cv::Mat_<double>(3, 3) << cam_intrinsic[cam_id_dst].focal[0], 0.0,
         cam_intrinsic[cam_id_dst].center[0],
         0.0, cam_intrinsic[cam_id_dst].focal[1], cam_intrinsic[cam_id_dst].center[1],
         0.0, 0.0, 1.0);
    cv::Mat camera_d = (cv::Mat_<double>(4, 1) << cam_intrinsic[cam_id_dst].distort[0],
                        cam_intrinsic[cam_id_dst].distort[1], 0, 0);

    Eigen::Matrix3d rcw = Tcw[cam_id_dst].rotation;
    Eigen::Vector3d tcw = Tcw[cam_id_dst].translation;

    cv::Mat rvec;
    cv::Mat cv_rcw = (cv::Mat_<double>(3, 3) << rcw(0, 0), rcw(0, 1), rcw(0, 2),
                      rcw(1, 0), rcw(1, 1), rcw(1, 2),
                      rcw(2, 0), rcw(2, 1), rcw(2, 2));
    Rodrigues(cv_rcw, rvec);
    cv::Mat tvec = (cv::Mat_<double>(3, 1) << tcw(0, 0), tcw(1, 0), tcw(2, 0));

    std::vector<cv::Point3f> corresponding_3d_coordinates;
    corresponding_3d_coordinates.push_back(cv::Point3f(pt_world.x, pt_world.y, pt_world.z));
    std::vector<cv::Point2f> corner_reproj;
    cv::projectPoints(corresponding_3d_coordinates,
                      rvec,
                      tvec,
                      camera_k, camera_d,
                      corner_reproj);
    point2D_dst = corner_reproj[0];
}


//void getEpiplorLine(cv::Point2f &point2D,
//                    const std::string& intricFile,
//                    const std::string& extricFile,
//                    const int in_cam,
//                    const int out_cam,
//                    const double &z1,
//                    const double &z2,
//                    std::vector<cv::Point2f> &corner_reproj){
//
//    std::vector<cv::Point2f> corner;
//    corner.push_back(point2D);
//    std::vector<cv::Point2f> undistort_corner;
//
//    std::vector<Intrinsic> cam_intrinsic;
//    std::vector<Transformation> Tcw;
//
//    loadIntrinsic(intricFile, cam_intrinsic);
//    loadExtrisic(extricFile, Tcw);
//
//    Mat cv_k_0 = (Mat_<double>(3,3) << cam_intrinsic[in_cam].focal[0], 0, cam_intrinsic[in_cam].center[0],
//                                        0, cam_intrinsic[in_cam].focal[1], cam_intrinsic[in_cam].center[1],
//                                        0., 0., 1.);
//    Mat cv_d_0 = (Mat_<double>(4,1) << cam_intrinsic[in_cam].distort[0], cam_intrinsic[in_cam].distort[1], 0, 0);
//
//    cv::undistortPoints(corner, undistort_corner, cv_k_0, cv_d_0);
//    double x = undistort_corner[0].x, y = undistort_corner[0].y;
//    double scale1, scale2;
//    Matrix3d Rc0w = Tcw[in_cam].rotation;
//    Matrix3d Rwc0 = Rc0w.transpose();
//    Vector3d twc0, tc0w;
//    tc0w = Tcw[in_cam].translation;
//    twc0 = -Rwc0*tc0w;
//
//    scale1 = (z1 - twc0(2,0))/(Rwc0(2,0)*x + Rwc0(2,1)*y + Rwc0(2,2));
//    scale2 = (z2 - twc0(2,0))/(Rwc0(2,0)*x + Rwc0(2,1)*y + Rwc0(2,2));
//
//    Vector3d pc1(scale1 *x, scale1*y, scale1);
//    Vector3d pc2(scale2 *x, scale2*y, scale2);
//    Vector3d pw1;
//    pw1 = Rwc0*pc1 + twc0;
//    Vector3d pw2;
//    pw2 = Rwc0*pc2 + twc0;
//
//    std::vector<cv::Point3f> corresponding_3d_coordinates;
//    corresponding_3d_coordinates.push_back(Point3f(pw1(0), pw1(1), pw1(2)));
//    corresponding_3d_coordinates.push_back(Point3f(pw2(0), pw2(1), pw2(2)));
//
//    Mat cv_k_1 = (Mat_<double>(3,3) << cam_intrinsic[out_cam].focal[0], 0, cam_intrinsic[out_cam].center[0],
//                                        0, cam_intrinsic[out_cam].focal[1], cam_intrinsic[out_cam].center[1],
//                                            0, 0., 1.);
//    Mat cv_d_1 = (Mat_<double>(4,1) << cam_intrinsic[out_cam].distort[0], cam_intrinsic[out_cam].distort[1], 0, 0);
//    Mat rvec;
//    Mat cv_rcw = (Mat_<double>(3,3) << Tcw[out_cam].rotation(0, 0), Tcw[out_cam].rotation(0, 1), Tcw[out_cam].rotation(0, 2),
//        Tcw[out_cam].rotation(1, 0), Tcw[out_cam].rotation(1, 1), Tcw[out_cam].rotation(1, 2),
//        Tcw[out_cam].rotation(2, 0), Tcw[out_cam].rotation(2, 1), Tcw[out_cam].rotation(2, 2));
//    Rodrigues(cv_rcw, rvec);
//    Mat tvec = (Mat_<double>(3,1)  << Tcw[out_cam].translation(0, 0), Tcw[out_cam].translation(1, 0), Tcw[out_cam].translation(2, 0));
//    cv::projectPoints(corresponding_3d_coordinates,
//                      rvec,
//                      tvec,
//                      cv_k_1, cv_d_1,
//                      corner_reproj);
//}
