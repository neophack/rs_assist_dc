#include <iostream>
#include <opencv2/core/core.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <Eigen/Core>
#include <Eigen/Geometry>

#include "param.h"
#include "tag_detector.h"
#include "MultiCamCalib.h"

using namespace cv;
using namespace std;
using namespace boost::filesystem;  // NOLINT
using namespace Eigen;
using namespace MultiCamCalib;

DEFINE_string(calib_yaml_1, "/home/xiaopeng/new-sale/120-fov/4st-intrinsic-param/calib0_4st.yaml",
              "load calib file");
DEFINE_string(calib_yaml_2, "/home/xiaopeng/new-sale/120-fov/4st-intrinsic-param/calib2_4st.yaml",
              "load calib file");
DEFINE_string(calib_yaml_3, "/home/xiaopeng/new-sale/120-fov/4st-intrinsic-param/calib3_4st.yaml",
              "load calib file");
DEFINE_string(calib_yaml_4, "/home/xiaopeng/new-sale/120-fov/4st-intrinsic-param/calib4_4st.yaml",
              "load calib file");
DEFINE_string(calib_yaml_5, "/home/xiaopeng/new-sale/120-fov/4st-intrinsic-param/calib5_4st.yaml",
              "load calib file");
DEFINE_string(calib_yaml_6, "/home/xiaopeng/new-sale/120-fov/4st-intrinsic-param/calib6_4st.yaml",
              "load calib file");
DEFINE_string(calib_yaml_7, "/home/xiaopeng/new-sale/120-fov/4st-intrinsic-param/calib7_4st.yaml",
              "load calib file");
DEFINE_string(calib_yaml_8, "/home/xiaopeng/new-sale/120-fov/4st-intrinsic-param/calib8_4st.yaml",
              "load calib file");
DEFINE_string(calib_yaml_9, "/home/xiaopeng/new-sale/120-fov/4st-intrinsic-param/calib9_4st.yaml",
              "load calib file");
DEFINE_string(calib_yaml_10, "/home/xiaopeng/new-sale/120-fov/4st-intrinsic-param/calib10_4st.yaml",
              "load calib file");
DEFINE_string(calib_yaml_11, "/home/xiaopeng/new-sale/120-fov/4st-intrinsic-param/calib11_4st.yaml",
              "load calib file");

DEFINE_string(folder_path, "/home/xiaopeng/new-sale/huojia/img", "folder containing image. "
              "should be 0/cam0 0/cam1 1/cam0 1/cam1");
DEFINE_string(output_folder, "./", "output folder containing parameters. "
              "transLeftToRight.txt, transsTargetToWorld.txt etc.");
DEFINE_double(gap_square_ratio, 0.25, "length of the gap between tags");  // april tag pattern
DEFINE_bool(show_det, false, "whether or not show image with tag detection");
DEFINE_bool(show_reproj_det, false, "whether or not show image with reprojection detection error");
DEFINE_bool(verbose, false, "whether or not print more message");
DEFINE_int32(display_timeout, 500, "how many millisec does a window display. 0 is inf");
DEFINE_bool(has_intrinsic, false, "whether camera has calibrate before or not");
DEFINE_double(square_size, 0.033, "length of the side of one tag 0.227/0.033");
DEFINE_bool(small_board, true,
            "big or small calibration board is used, small board is for shelf calibration 5*7 tags"
            "big board is for ceiling camera calibration 3*5 tags");
DEFINE_bool(stereo, false, "stereo or monocular calibration");
DEFINE_int32(num_camera, 3, "how many camera need to calibrate");
DEFINE_int32(width, 640, "image width");
DEFINE_int32(height, 480, "image height");

// [NOTE] Assume 5x7 april tag is used.
//const int kFullCorners = 5 * 7 * 4;

void preset_window() {
    cv::namedWindow("GetFocus", CV_WINDOW_NORMAL);
    cv::Mat img = cv::Mat::zeros(100, 100, CV_8UC3);
    cv::imshow("GetFocus", img);
    cv::setWindowProperty("GetFocus", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
    cv::waitKey(1);
    cv::setWindowProperty("GetFocus", CV_WND_PROP_FULLSCREEN, CV_WINDOW_NORMAL);
    cv::destroyWindow("GetFocus");
}


cv::Point3f id2coordinate(const int det_id) {
    // compute 3d position
    // tag corner ids are
    // 1 2
    // 4 3
    // board is 5x7
    // Arrangement of tag ids is
    // 00 01 02 03 04 05 06
    // 24 25 26 27 28 29 30
    // 48 49 50...
    // 72 ..
    // 96 ..             102
    cv::Point3f coordinate;
    const int tag_id = det_id / 10;
    const int col_id = tag_id % 24;
    const int row_id = tag_id / 24;
    CHECK_LT(col_id, 7) << "tag_id " << tag_id << " does not belong to the tag board";
    const int corner_id = det_id % 10;
    // std::cout << tag_id << ":" << corner_id << " -> " << corner_positions[i] << std::endl;
    const float x_upper_left =
        row_id * (FLAGS_square_size + FLAGS_square_size * FLAGS_gap_square_ratio);
    const float y_upper_left =
        col_id * (FLAGS_square_size + FLAGS_square_size * FLAGS_gap_square_ratio);
    coordinate.x = x_upper_left;
    coordinate.y = y_upper_left;
    coordinate.z = 0;
    CHECK_GE(corner_id, 1);
    CHECK_LE(corner_id, 4);

    if (corner_id == 2 || corner_id == 3) {
        coordinate.y += FLAGS_square_size;
    }

    if (corner_id == 3 || corner_id == 4) {
        coordinate.x += FLAGS_square_size;
    }
    return coordinate;
}

// [NOTE] Assume 3x5 april tag is used.
//const int kFullCorners = 3 * 5 * 4;
cv::Point3f id2coordinate2(const int det_id) {
    // compute 3d position
    // tag corner ids are
    // 1 2
    // 4 3
    // board is 3x5
    // Arrangement of tag ids is
    // 02 03 04 05 06
    // 26 27 28 29 30
    // 50 51 52 53 54
    cv::Point3f coordinate;
    const int tag_id = det_id / 10;
    const int col_id = tag_id % 24;
    const int row_id = tag_id / 24;
    CHECK_LT(col_id, 7) << "tag_id " << tag_id << " does not belong to the tag board";
    const int corner_id = det_id % 10;
    // std::cout << tag_id << ":" << corner_id << " -> " << corner_positions[i] << std::endl;
    const float x_upper_left =
        row_id * (FLAGS_square_size + FLAGS_square_size * FLAGS_gap_square_ratio);
    const float y_upper_left =
        (col_id - 2) * (FLAGS_square_size + FLAGS_square_size * FLAGS_gap_square_ratio);
    coordinate.x = x_upper_left;
    coordinate.y = y_upper_left;
    coordinate.z = 0;
    CHECK_GE(corner_id, 1);
    CHECK_LE(corner_id, 4);

    if (corner_id == 2 || corner_id == 3) {
        coordinate.y += FLAGS_square_size;
    }

    if (corner_id == 3 || corner_id == 4) {
        coordinate.x += FLAGS_square_size;
    }

    return coordinate;
}

bool saveIntrinsic(const char* fileName, std::vector<Intrinsic> intrinsic) {
    FILE* fp = fopen(fileName, "w");

    if (!fp) {
        return false;
    }

    const int N = static_cast<int>(intrinsic.size());
    fprintf(fp, "%d\n", N);

    for (int i = 0; i < N; ++i) {
        fprintf(fp, "%le %le\n", intrinsic[i].m_focal(0, 0), intrinsic[i].m_focal(1, 0));
        fprintf(fp, "%le %le\n", intrinsic[i].m_center(0, 0), intrinsic[i].m_center(1, 0));
        fprintf(fp, "%le %le\n", intrinsic[i].m_distortion[0], intrinsic[i].m_distortion[1]);
    }

    fclose(fp);
    cout << "Save file " << fileName << endl;
    return true;
}

// print before optimazation error
void printError(vector<cv::Point3f> boards_3d, cv::Mat rvec, cv::Mat tvec, cv::Matx33f K,
                cv::Mat D, vector<cv::Point2f> corners_2d, int lr, int i, int LR, vector<std::string> lr_folder) {
    if (boards_3d.size() == 0) {
        return;
    }

    vector<cv::Point2f> corner_reproj;
    cv::projectPoints(boards_3d,
                      rvec,
                      tvec,
                      K,
                      D,
                      corner_reproj);

    float reproj_error2_this_img = 0;
    int reproj_count_this_img = 0;

    for (size_t it_corner = 0; it_corner < corner_reproj.size(); it_corner++) {
        float x_diff = corners_2d[it_corner].x - corner_reproj[it_corner].x;
        float y_diff = corners_2d[it_corner].y - corner_reproj[it_corner].y;
        float diff = x_diff * x_diff + y_diff * y_diff;
        reproj_error2_this_img += diff;
        reproj_count_this_img++;
    }

    cout << "Before optimization, Left(Right) :" << LR << ", Camera: " << lr << " targets: " << i <<
         " reproj corners num: "
         << reproj_count_this_img
         << " , error: " << sqrt(reproj_error2_this_img / reproj_count_this_img) << endl;

    boost::filesystem::path folder_path(FLAGS_folder_path + "/" + lr_folder[lr]);

    if (!is_directory(folder_path)) {
        LOG(ERROR) << folder_path << " is not a folder";
        return;
    }

    vector<path> file_names_vec;
    copy(directory_iterator(folder_path), directory_iterator(), std::back_inserter(file_names_vec));
    std::sort(file_names_vec.begin(), file_names_vec.end());

    if (FLAGS_verbose) {
        Mat _img = imread(file_names_vec[i].string(), IMREAD_COLOR);

        for (size_t it_corner = 0; it_corner < corners_2d.size(); ++it_corner) {
            cv::circle(_img, corners_2d[it_corner], 2, cv::Scalar(0, 0xff, 0));
            cv::line(_img, corners_2d[it_corner],
                     corner_reproj[it_corner], cv::Scalar(0, 0, 0xff));
        }

        cv::imshow("Before_optimization_reproj", _img);
        std::string fp = "reproj_LR_" + std::to_string(LR) + "Cam_" + std::to_string(lr) + "Target_" + std::to_string(
                i) + ".png";
        boost::filesystem::path folder_path(FLAGS_output_folder + '/' + fp);
        cv::imwrite(folder_path.c_str(), _img);
        cv::waitKey(-1);
    }
}

int main(int argc, char** argv) {
    google::InitGoogleLogging(argv[0]);
    google::ParseCommandLineFlags(&argc, &argv, false);
    google::InstallFailureSignalHandler();
    vector<vector<XP::DuoCalibParam>> duo_calib_param(1);

    if (FLAGS_stereo) {
        duo_calib_param.resize(2);
    }

    if (FLAGS_has_intrinsic) {
        for (int i = 0; i < FLAGS_num_camera; i++) {
            if (FLAGS_stereo) {
                duo_calib_param[1].push_back(XP::DuoCalibParam());
            }

            duo_calib_param[0].push_back(XP::DuoCalibParam());
        }

        if (!duo_calib_param[0][0].LoadCamCalibFromYaml(FLAGS_calib_yaml_1) ||
                !duo_calib_param[0][1].LoadCamCalibFromYaml(FLAGS_calib_yaml_2) ||
                !duo_calib_param[0][2].LoadCamCalibFromYaml(FLAGS_calib_yaml_3) ||
                !duo_calib_param[0][3].LoadCamCalibFromYaml(FLAGS_calib_yaml_4) ||
                !duo_calib_param[0][4].LoadCamCalibFromYaml(FLAGS_calib_yaml_5) ||
                !duo_calib_param[0][5].LoadCamCalibFromYaml(FLAGS_calib_yaml_6)
               /* !duo_calib_param[0][6].LoadCamCalibFromYaml(FLAGS_calib_yaml_7) ||
                !duo_calib_param[0][7].LoadCamCalibFromYaml(FLAGS_calib_yaml_8) ||
                !duo_calib_param[0][8].LoadCamCalibFromYaml(FLAGS_calib_yaml_9) ||
                !duo_calib_param[0][9].LoadCamCalibFromYaml(FLAGS_calib_yaml_10) ||
                !duo_calib_param[0][10].LoadCamCalibFromYaml(FLAGS_calib_yaml_11)*/) {
            LOG(ERROR) << "Load calib param file error";
            return -1;
        }

        // 双目摄像头的矫正文件载入方式
//        if (!duo_calib_param[0][0].LoadCamCalibFromYaml(FLAGS_calib_yaml_1) ||
//            !duo_calib_param[0][1].LoadCamCalibFromYaml(FLAGS_calib_yaml_2) ||
//            !duo_calib_param[0][2].LoadCamCalibFromYaml(FLAGS_calib_yaml_3) ||
//            !duo_calib_param[1][0].LoadCamCalibFromYaml(FLAGS_calib_yaml_4) ||
//            !duo_calib_param[1][1].LoadCamCalibFromYaml(FLAGS_calib_yaml_5) ||
//            !duo_calib_param[1][2].LoadCamCalibFromYaml(FLAGS_calib_yaml_6)) {
//            LOG(ERROR) << "Load calib param file error";
//            return -1;
//        }

    }

    vector<int> g_tag_ids;

    // 5*7的完整板子, 用于shelf标定
    if (FLAGS_small_board) {
        vector<int> tmp {00, 01, 02, 03, 04, 05, 06,
                         24, 25, 26, 27, 28, 29, 30,
                         48, 49, 50, 51, 52, 53, 54,
                         72, 73, 74, 75, 76, 77, 78,
                         96, 97, 98, 99, 100, 101, 102
                        };
        g_tag_ids = tmp;
    } else { // 3*5的板子,用于天花板的人体追踪摄像头标定
        vector<int> tmp {02, 03, 04, 05, 06,
                         26, 27, 28, 29, 30,
                         50, 51, 52, 53, 54
                        };
        g_tag_ids = tmp;
    }

    vector<int> g_corner_ids;

    for (int i = 0; i < g_tag_ids.size(); i++) {
        g_corner_ids.push_back(10 * g_tag_ids[i] + 1);
        g_corner_ids.push_back(10 * g_tag_ids[i] + 2);
        g_corner_ids.push_back(10 * g_tag_ids[i] + 3);
        g_corner_ids.push_back(10 * g_tag_ids[i] + 4);
    }

    std::vector<Vector3> g_corners3D;
    std::vector<Frame> g_frames;
    int g_num_targets = 0;
    vector<Intrinsic> g_camera_intrinsic;
    vector<Intrinsic> g_camera_intrinsic_r;
    XP::AprilTagDetector april_tag_detector;

    typedef vector<cv::Point2f> DetectedCorners;
    typedef vector<cv::Point3f> ObjectCorners;
    vector<vector<vector<DetectedCorners>>> detected_corners_all_lr(2);
    detected_corners_all_lr[0].resize(FLAGS_num_camera);
    vector<vector<vector<ObjectCorners>>> corresponding_board_coordinates_all_lr(2);
    corresponding_board_coordinates_all_lr[0].resize(FLAGS_num_camera);
    vector<vector<vector<vector<int>>>> corner_ids_all_lr(2);
    corner_ids_all_lr[0].resize(FLAGS_num_camera);
    // 图像保存的文件夹位置
    vector<vector<std::string>> lr_folders(2);
    lr_folders[0].resize(FLAGS_num_camera);

    int camera_rig = 1;

    if (FLAGS_stereo) {
        detected_corners_all_lr[1].resize(FLAGS_num_camera);
        corresponding_board_coordinates_all_lr[1].resize(FLAGS_num_camera);
        corner_ids_all_lr[1].resize(FLAGS_num_camera);
        lr_folders[1].resize(FLAGS_num_camera);
        camera_rig = 2;
    }

    vector<Transformation> transLeftToRight;

    // 图片存放的格式,
    // 单目摄像头 0/cam0 1/cam0 2/cam0 3/cam0 4/cam0 5/cam0
    // 双目摄像头 0/cam0 0/cam1 1/cam0 1/cam1 2/cam0 2/cam1
    for (int l = 0; l < camera_rig; l++) {
        for (int i = 0; i < FLAGS_num_camera; i++) {
            lr_folders[l][i] = std::to_string(i) + "/cam" + std::to_string(l);
        }
    }

    // 矫正板对应的所有3D点
    for (int i = 0; i < g_corner_ids.size(); i++) {
        cv::Point3f p;
        if (FLAGS_small_board)
            p = id2coordinate(g_corner_ids[i]);
        else
            p = id2coordinate2(g_corner_ids[i]);
        Vector3 vec;
        vec << p.x, p.y, p.z;
        g_corners3D.push_back(vec);
    }

    for (int lr = 0; lr < FLAGS_num_camera; ++lr) {
        for (int k = 0; k < camera_rig; k++) {
            boost::filesystem::path folder_path(FLAGS_folder_path + "/" + lr_folders[k][lr]);

            if (!is_directory(folder_path)) {
                LOG(ERROR) << folder_path << " is not a folder";
                return -1;
            }

            vector<DetectedCorners>& detected_corners_all = detected_corners_all_lr[k][lr];
            vector<ObjectCorners>& corresponding_board_coordinates_all =
                corresponding_board_coordinates_all_lr[k][lr];

            detected_corners_all.reserve(200);
            corresponding_board_coordinates_all.reserve(200);
            corner_ids_all_lr[k][lr].reserve(200);
            const cv::TermCriteria pixel_refinement_criteria
                = cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001);
            int img_counter = 0;
            vector<path> file_names_vec;
            copy(directory_iterator(folder_path), directory_iterator(), std::back_inserter(file_names_vec));
            std::sort(file_names_vec.begin(), file_names_vec.end());

            // 第几个target
            g_num_targets = 0;

            if (FLAGS_show_det) {
                preset_window();
            }
            for (auto it_path : file_names_vec) {
                cv::Mat img_gray_original = cv::imread(it_path.string(), CV_LOAD_IMAGE_GRAYSCALE);
                cv::Mat img_gray = img_gray_original;

                if (img_gray.rows == 0) {
                    LOG(ERROR) << it_path << " cannot be read by opencv";
                }

                std::vector<cv::KeyPoint> kps;
                int det_num = april_tag_detector.detect(img_gray, &kps);
                std::vector<cv::Point2f> corner_positions(kps.size());
                std::vector<int> corner_ids(kps.size());

                for (size_t i = 0; i < kps.size(); ++i) {
                    corner_positions[i] = kps[i].pt;
                    corner_ids[i] = kps[i].class_id;
                }

                corner_ids_all_lr[k][lr].push_back(corner_ids);
                /**
                std::cout << "detected " << det_num << " tags from "
                          << it_path.filename() << " size " << img_size << std::endl;
                **/

                vector<cv::Point3f> corresponding_board_coordinates(det_num * 4);

                if (det_num == 0) {
                    ++img_counter;
                    g_num_targets++;
                    detected_corners_all.push_back(corner_positions);
                    corresponding_board_coordinates_all.push_back(corresponding_board_coordinates);
                    continue;
                }

                ++img_counter;

                CHECK_EQ(det_num * 4, corner_ids.size());

                for (size_t i = 0; i < corner_ids.size(); i++) {
                  if (FLAGS_small_board)
                    corresponding_board_coordinates[i] = id2coordinate(corner_ids[i]);
                  else
                    corresponding_board_coordinates[i] = id2coordinate2(corner_ids[i]);
                    // shift detection result
                    corner_positions[i].x -= 0.5;
                    corner_positions[i].y -= 0.5;
                }

                const auto corner_pos_before_refinement = corner_positions;
                cv::cornerSubPix(img_gray, corner_positions,
                                 cv::Size(2, 2),  // Half of the side length of the search window
                                 cv::Size(-1, -1),  // zeroZone
                                 pixel_refinement_criteria);
                detected_corners_all.push_back(corner_positions);
                corresponding_board_coordinates_all.push_back(corresponding_board_coordinates);

                if (FLAGS_show_det) {
                    cv::Mat detection_img;
                    cv::cvtColor(img_gray, detection_img, CV_GRAY2RGB);

                    for (size_t it_corner = 0; it_corner < corner_positions.size(); ++it_corner) {
                        cv::circle(detection_img, corner_positions[it_corner], 2, cv::Scalar(0, 0xff, 0));
                    }
                    cv::namedWindow("detection");
                    cv::imshow("detection", detection_img);
                    std::string fp = "LR_" + std::to_string(k) + "Target_" + std::to_string(g_num_targets) + "Cam_" + \
                                std::to_string(lr) + ".png";
                    boost::filesystem::path folder_path(FLAGS_output_folder + '/' + fp);
                    cv::imwrite(folder_path.c_str(),
                                detection_img);
                    cv::waitKey(-1);
                }

                g_num_targets++;
            }
        }

        // 内参已经矫正的,提供的检测点是normalize plane上的检测点,就是undistort的点
        if (FLAGS_has_intrinsic) {
            if (FLAGS_stereo) {
                Transformation t;
                Eigen::Matrix4f T = duo_calib_param[0][lr].Camera.D_T_C_lr[1];
                Eigen::Matrix3f r1 = T.topLeftCorner(3, 3).transpose();
                Eigen::Vector3f t1 = -r1 * T.topRightCorner(3, 1);
                t.m_rotation << r1(0, 0), r1(0, 1), r1(0, 2),
                             r1(1, 0), r1(1, 1), r1(1, 2),
                             r1(2, 0), r1(2, 1), r1(2, 2);
                t.m_translation << t1(0, 0), t1(1, 0), t1(2, 0);
                transLeftToRight.push_back(t);
            }

            for (int i = 0; i < detected_corners_all_lr[0][lr].size(); i++) {
                if (detected_corners_all_lr[0][lr][i].size() == 0) {
                    continue;
                }

                if (FLAGS_stereo && detected_corners_all_lr[1][lr][i].size() == 0) {
                    continue;
                }

                DetectedCorners corners_2d = detected_corners_all_lr[0][lr][i];
                ObjectCorners boards_3d = corresponding_board_coordinates_all_lr[0][lr][i];
                vector<cv::Point2f> undistorted_corners;

                Frame frame;
                frame.m_iTarget = i;
                frame.m_iCam = lr;
                cv::undistortPoints(corners_2d, undistorted_corners,
                                    duo_calib_param[0][lr].Camera.cv_camK_lr[0],
                                    duo_calib_param[0][lr].Camera.cv_dist_coeff_lr[0]);

                vector<int> corner_ids = corner_ids_all_lr[0][lr][i];

                for (int j = 0; j < g_corner_ids.size(); ++j) {
                    Vector2 c;
                    c << 0, 0;
                    std::vector<int>::iterator it = std::find(corner_ids.begin(), corner_ids.end(), g_corner_ids[j]);

                    if (it != corner_ids.end()) {
                        c << undistorted_corners[it - corner_ids.begin()].x,
                        undistorted_corners[it - corner_ids.begin()].y;
                    } else {  // 如果点没有检测到,提供给优化的点要设置为无效坐标
                        c.invalidate();
                    }

                    frame.m_corners2D.push_back(c);
                }

                cv::Mat rvec;
                cv::Mat tvec;
                cv::solvePnP(boards_3d,
                             corners_2d,
                             duo_calib_param[0][lr].Camera.cv_camK_lr[0],
                             duo_calib_param[0][lr].Camera.cv_dist_coeff_lr[0],
                             rvec,
                             tvec);

                if (FLAGS_verbose) {
                    // before optimzation error
                    printError(boards_3d, rvec, tvec, duo_calib_param[0][lr].Camera.cv_camK_lr[0],
                               duo_calib_param[0][lr].Camera.cv_dist_coeff_lr[0], corners_2d, lr, i, 0, lr_folders[0]);
                }

                cv::Mat rotation;
                Rodrigues(rvec, rotation);
                Transformation transform;
                transform.m_rotation << rotation.at<double>(0, 0), rotation.at<double>(0, 1), rotation.at<double>(0,
                                     2),
                                     rotation.at<double>(1, 0), rotation.at<double>(1, 1), rotation.at<double>(1, 2),
                                     rotation.at<double>(2, 0), rotation.at<double>(2, 1), rotation.at<double>(2, 2);
                transform.m_translation << tvec.at<double>(0, 0), tvec.at<double>(1, 0), tvec.at<double>(2, 0);
                frame.m_transTargetToCam = transform;

                if (FLAGS_stereo) {
                    corners_2d = detected_corners_all_lr[1][lr][i];
                    boards_3d = corresponding_board_coordinates_all_lr[1][lr][i];
                    undistorted_corners.clear();
                    cv::undistortPoints(corners_2d, undistorted_corners,
                                        duo_calib_param[1][lr].Camera.cv_camK_lr[1],
                                        duo_calib_param[1][lr].Camera.cv_dist_coeff_lr[1]);

                    vector<int> corner_ids_r = corner_ids_all_lr[1][lr][i];

                    for (int j = 0; j < g_corner_ids.size(); ++j) {
                        Vector2 c;
                        c << 0, 0;
                        std::vector<int>::iterator it = std::find(corner_ids_r.begin(), corner_ids_r.end(),
                                                        g_corner_ids[j]);

                        if (it != corner_ids_r.end()) {
                            c << undistorted_corners[it - corner_ids_r.begin()].x,
                            undistorted_corners[it - corner_ids_r.begin()].y;
                        } else {
                            c.invalidate();
                        }

                        frame.m_corners2DRight.push_back(c);
                    }

                    cv::solvePnP(boards_3d,
                                 corners_2d,
                                 duo_calib_param[1][lr].Camera.cv_camK_lr[1],
                                 duo_calib_param[1][lr].Camera.cv_dist_coeff_lr[1],
                                 rvec, tvec);

                    if (FLAGS_verbose) {
                        // before optimzation, pnp error
                        printError(boards_3d, rvec, tvec, duo_calib_param[1][lr].Camera.cv_camK_lr[1],
                                   duo_calib_param[1][lr].Camera.cv_dist_coeff_lr[1], corners_2d, lr, i, 1, lr_folders[1]);
                    }
                }

                g_frames.push_back(frame);
            }
        } else {  // 没有内参数,提供的是原始的检测点
            cv::Mat camera_K = (cv::Mat_<float>(3, 3) << 430, 0, 320, 0, 430, 240, 0, 0, 1);
            cv::Mat camera_D;
            Mat camera_K_r = (cv::Mat_<float>(3, 3) << 430, 0, 320, 0, 430, 240, 0, 0, 1);
            Mat camera_D_r;

            vector<cv::Mat> rvecs;
            vector<cv::Mat> tvecs;
            Intrinsic camera_intrinsic;
            Intrinsic camera_intrinsic_r;

            vector<ObjectCorners> objects_3d;
            vector<DetectedCorners> images_2d;

            for (int i = 0; i < corresponding_board_coordinates_all_lr[0][lr].size(); i++) {
                if (corresponding_board_coordinates_all_lr[0][lr][i].size() > 16) {
                    objects_3d.push_back(corresponding_board_coordinates_all_lr[0][lr][i]);
                    images_2d.push_back(detected_corners_all_lr[0][lr][i]);
                }
            }

            float reproj = cv::calibrateCamera(objects_3d,
                                               images_2d,
                                               cv::Size(FLAGS_width, FLAGS_height),
                                               camera_K,
                                               camera_D,
                                               rvecs,
                                               tvecs,
                                               CV_CALIB_USE_INTRINSIC_GUESS |
                                               CV_CALIB_ZERO_TANGENT_DIST |
                                                           CV_CALIB_FIX_K3 |
                                                           CV_CALIB_FIX_K4 |
                                                           CV_CALIB_FIX_K5 |
                                                           CV_CALIB_FIX_K6);

            if (FLAGS_stereo) {
                vector<ObjectCorners> objects_3d_r;
                vector<DetectedCorners> images_2d_r;

                objects_3d.clear();
                images_2d.clear();
                images_2d_r.clear();

                for (int i = 0; i < corresponding_board_coordinates_all_lr[0][lr].size(); i++) {
                    if (corresponding_board_coordinates_all_lr[0][lr][i].size() > 16
                            && corresponding_board_coordinates_all_lr[1][lr][i].size() > 16) {
                        vector<int> corner_id_l = corner_ids_all_lr[0][lr][i];
                        vector<int> corner_id_r = corner_ids_all_lr[1][lr][i];
                        ObjectCorners obj_3d;
                        DetectedCorners img_2d_l;
                        DetectedCorners img_2d_r;

                        for (int j = 0; j < corner_id_l.size(); j++) {
                            std::vector<int>::iterator it = std::find(corner_id_r.begin(), corner_id_r.end(), corner_id_l[j]);

                            if (it != corner_id_r.end()) {
                                obj_3d.push_back(corresponding_board_coordinates_all_lr[0][lr][i][j]);
                                img_2d_l.push_back(detected_corners_all_lr[0][lr][i][j]);
                                img_2d_r.push_back(detected_corners_all_lr[1][lr][i][it - corner_id_r.begin()]);
                            }
                        }

                        if (obj_3d.size() == 0) {
                            continue;
                        }

                        objects_3d.push_back(obj_3d);
                        images_2d.push_back(img_2d_l);
                        images_2d_r.push_back(img_2d_r);
                    }
                }

                Mat R;
                Mat T;
                Mat E;
                Mat F;
                float reproj = cv::stereoCalibrate(objects_3d,
                                                   images_2d,
                                                   images_2d_r,
                                                   camera_K,
                                                   camera_D,
                                                   camera_K_r,
                                                   camera_D_r,
                                                   cv::Size(FLAGS_width, FLAGS_height),
                                                   R,
                                                   T,
                                                   E,
                                                   F,
                                                   CV_CALIB_USE_INTRINSIC_GUESS |
                                                   CV_CALIB_ZERO_TANGENT_DIST |
                                                   CV_CALIB_FIX_K3 |
                                                   CV_CALIB_FIX_K4 |
                                                   CV_CALIB_FIX_K5 |
                                                   CV_CALIB_FIX_K6);

                Transformation _t;
                _t.m_rotation << R.at<double>(0, 0), R.at<double>(0, 1), R.at<double>(0, 2),
                              R.at<double>(1, 0), R.at<double>(1, 1), R.at<double>(1, 2),
                              R.at<double>(2, 0), R.at<double>(2, 1), R.at<double>(2, 2);
                _t.m_translation << T.at<double>(0, 0), T.at<double>(1, 0), T.at<double>(2, 0);
                transLeftToRight.push_back(_t);

                camera_intrinsic_r.m_center = Vector2d(camera_K_r.at<double>(0, 2), camera_K_r.at<double>(1, 2));
                camera_intrinsic_r.m_focal = Vector2d(camera_K_r.at<double>(0, 0), camera_K_r.at<double>(1, 1));
                camera_intrinsic_r.m_distortion[0] = camera_D_r.at<double>(0, 0);
                camera_intrinsic_r.m_distortion[1] = camera_D_r.at<double>(0, 1);
                g_camera_intrinsic_r.push_back(camera_intrinsic_r);
            }

            camera_intrinsic.m_center = Vector2d(camera_K.at<double>(0, 2), camera_K.at<double>(1, 2));
            camera_intrinsic.m_focal = Vector2d(camera_K.at<double>(0, 0), camera_K.at<double>(1, 1));
            camera_intrinsic.m_distortion[0] = camera_D.at<double>(0, 0);
            camera_intrinsic.m_distortion[1] = camera_D.at<double>(0, 1);
            g_camera_intrinsic.push_back(camera_intrinsic);

            for (int i = 0; i < detected_corners_all_lr[0][lr].size(); i++) {
                Frame frame;
                frame.m_iTarget = i;
                frame.m_iCam = lr;

                if (detected_corners_all_lr[0][lr][i].size() == 0) {
                    continue;
                }

                if (FLAGS_stereo && detected_corners_all_lr[1][lr][i].size() == 0) {
                    continue;
                }

                vector<int> corner_ids = corner_ids_all_lr[0][lr][i];

                for (int j = 0; j < g_corner_ids.size(); ++j) {
                    Vector2 c;
                    c << 0, 0;
                    std::vector<int>::iterator it = std::find(corner_ids.begin(), corner_ids.end(), g_corner_ids[j]);

                    if (it != corner_ids.end()) {
                        c << detected_corners_all_lr[0][lr][i][it - corner_ids.begin()].x,
                        detected_corners_all_lr[0][lr][i][it - corner_ids.begin()].y;
                    } else {
                        c.invalidate();
                    }

                    frame.m_corners2D.push_back(c);
                }

                Mat rvec;
                Mat tvec;
                cv::solvePnP(corresponding_board_coordinates_all_lr[0][lr][i],
                             detected_corners_all_lr[0][lr][i],
                             camera_K,
                             camera_D,
                             rvec,
                             tvec);

                cv::Mat rotation;
                Rodrigues(rvec, rotation);
                Transformation transform;
                transform.m_rotation << rotation.at<double>(0, 0), rotation.at<double>(0, 1), rotation.at<double>(0,
                                     2),
                                     rotation.at<double>(1, 0), rotation.at<double>(1, 1), rotation.at<double>(1, 2),
                                     rotation.at<double>(2, 0), rotation.at<double>(2, 1), rotation.at<double>(2, 2);
                transform.m_translation << tvec.at<double>(0, 0), tvec.at<double>(1, 0), tvec.at<double>(2, 0);
                frame.m_transTargetToCam = transform;
#if 0
                {
                    // before optimzation, pnp error
                    printError(corresponding_board_coordinates_all_lr[0][lr][i], rvec, tvec, camera_K,
                               camera_D, detected_corners_all_lr[0][lr][i], lr, i, 0, lr_folders[0]);
                }
#endif

                if (FLAGS_stereo) {
                    vector<int> corner_ids_r = corner_ids_all_lr[1][lr][i];

                    for (int j = 0; j < g_corner_ids.size(); ++j) {
                        Vector2 c;
                        c << 0, 0;
                        std::vector<int>::iterator it = std::find(corner_ids_r.begin(), corner_ids_r.end(),
                                                        g_corner_ids[j]);

                        if (it != corner_ids_r.end()) {
                            c << detected_corners_all_lr[1][lr][i][it - corner_ids_r.begin()].x,
                            detected_corners_all_lr[1][lr][i][it - corner_ids_r.begin()].y;
                        } else {
                            c.invalidate();
                        }

                        frame.m_corners2DRight.push_back(c);
                    }

#if 0
                    {
                        cv::Mat rvec_r, tvec_r;
                        cv::solvePnP(corresponding_board_coordinates_all_lr[1][lr][i],
                                     detected_corners_all_lr[1][lr][i],
                                     camera_K_r,
                                     camera_D_r,
                                     rvec_r,
                                     tvec_r);
                        // before optimzation, pnp error
                        printError(corresponding_board_coordinates_all_lr[1][lr][i], rvec_r, tvec_r, camera_K_r,
                                   camera_D_r, detected_corners_all_lr[1][lr][i], lr, i, 1, lr_folders[1]);
                    }
#endif

                }

                g_frames.push_back(frame);
            }
        }
    }

    std::vector<MultiCamCalib::Transformation> transsTargetToWorld;
    std::vector<MultiCamCalib::Transformation> transsWorldToCam;
    std::vector<Intrinsic> refine_intrinsics;
    std::vector<Intrinsic> refine_intrinsics_r;
    std::vector<Transformation> refine_transsLeftToRight;

    cout << "  ------ Before optimziation, reproj error -----------" << endl;

    if (FLAGS_has_intrinsic) {
        cout << "use -verbose to print before optimzation detection reproject error" << endl;
    } else if (FLAGS_stereo) {
        MultiCamCalib::print_error(g_corners3D, g_frames, &g_camera_intrinsic, &g_camera_intrinsic_r,
                                   &transLeftToRight);
    } else {
        MultiCamCalib::print_error(g_corners3D, g_frames, &g_camera_intrinsic, NULL, NULL);
    }
    boost::filesystem::path folder_path0(FLAGS_output_folder + "/" + "data.txt");

    if (FLAGS_has_intrinsic && !FLAGS_stereo) {
        save(folder_path0.c_str(), g_num_targets, FLAGS_num_camera, g_corners3D, g_frames, refine_intrinsics,
             refine_intrinsics_r, refine_transsLeftToRight);
        MultiCamCalib::run(g_num_targets, FLAGS_num_camera, g_corners3D, g_frames,
                           &transsTargetToWorld, &transsWorldToCam, NULL, NULL, NULL);
    } else if (!FLAGS_has_intrinsic && !FLAGS_stereo) {
        save(folder_path0.c_str(), g_num_targets, FLAGS_num_camera, g_corners3D, g_frames, g_camera_intrinsic,
             refine_intrinsics_r, refine_transsLeftToRight);
        MultiCamCalib::run(g_num_targets, FLAGS_num_camera, g_corners3D, g_frames,
                           &transsTargetToWorld, &transsWorldToCam, &g_camera_intrinsic, NULL, NULL);
    } else if (FLAGS_has_intrinsic && FLAGS_stereo) {
        save(folder_path0.c_str(), g_num_targets, FLAGS_num_camera, g_corners3D, g_frames, refine_intrinsics,
             refine_intrinsics_r, transLeftToRight);
        MultiCamCalib::run(g_num_targets, FLAGS_num_camera, g_corners3D, g_frames,
                           &transsTargetToWorld, &transsWorldToCam, NULL, NULL, &transLeftToRight);
    } else { // no intrinsic, stereo
        save(folder_path0.c_str(), g_num_targets, FLAGS_num_camera, g_corners3D, g_frames, g_camera_intrinsic,
             g_camera_intrinsic_r, transLeftToRight);
        MultiCamCalib::run(g_num_targets, FLAGS_num_camera, g_corners3D, g_frames,
                           &transsTargetToWorld, &transsWorldToCam, &g_camera_intrinsic, &g_camera_intrinsic_r,
                           &transLeftToRight);
    }

    cout << "  ------ After optimziation, reproj error -----------" << endl;

    if (FLAGS_has_intrinsic) {
        cout << "  use -show_reproj_det to print after optimzation reproject error" << endl;
    } else if (FLAGS_stereo) {
        MultiCamCalib::print_error(g_corners3D, g_frames, transsTargetToWorld, transsWorldToCam,
                                   &g_camera_intrinsic, &g_camera_intrinsic_r, &transLeftToRight);
    } else {
        MultiCamCalib::print_error(g_corners3D, g_frames, transsTargetToWorld, transsWorldToCam,
                                   &g_camera_intrinsic, NULL, NULL);
    }


    boost::filesystem::path folder_path(FLAGS_output_folder);

    if (!is_directory(folder_path)) {
        LOG(ERROR) << folder_path << " is not a folder";
        return 1;
    }
    boost::filesystem::path folder_path1(FLAGS_output_folder + "/" + "transsTargetToWorld.txt");
    save(folder_path1.c_str(), transsTargetToWorld);
    boost::filesystem::path folder_path2(FLAGS_output_folder + "/" + "transsWorldToCam.txt");
    save(folder_path2.c_str(), transsWorldToCam);
    boost::filesystem::path folder_path3(FLAGS_output_folder + "/" + "transLeftToRight.txt");
    save(folder_path3.c_str(), transLeftToRight);
    boost::filesystem::path folder_path4(FLAGS_output_folder + "/" + "leftIntrinsic.txt");
    saveIntrinsic(folder_path4.c_str(), g_camera_intrinsic);
    boost::filesystem::path folder_path5(FLAGS_output_folder + "/" + "rightIntrinsic.txt");
    saveIntrinsic(folder_path5.c_str(), g_camera_intrinsic_r);


    // 重投影误差
    if (FLAGS_show_reproj_det) {
        preset_window();
        for (int k = 0; k < camera_rig; k++) {
            for (int lr = 0; lr < FLAGS_num_camera; lr++) {
                boost::filesystem::path folder_path(FLAGS_folder_path + "/" + lr_folders[k][lr]);
                vector<path> file_names_vec;
                copy(directory_iterator(folder_path), directory_iterator(),
                     std::back_inserter(file_names_vec));
                std::sort(file_names_vec.begin(), file_names_vec.end());

                for (int j = 0; j < g_num_targets; j++) {
                    Transformation transform;

                    if (k == 0) {
                        transform = transsWorldToCam[lr] * transsTargetToWorld[j];
                    } else {
                        transform = transLeftToRight[lr] * transsWorldToCam[lr] * transsTargetToWorld[j];
                    }

                    Rotation eigen_r = transform.m_rotation;
                    Vector3 eigen_t = transform.m_translation;
                    cv::Mat cv_r;
                    cv_r.create(3, 3, CV_64F);
                    cv_r.at<double>(0, 0) = eigen_r(0, 0), cv_r.at<double>(0, 1) = eigen_r(0, 1), cv_r.at<double>(0,
                                            2) =
                                                eigen_r(0, 2);
                    cv_r.at<double>(1, 0) = eigen_r(1, 0), cv_r.at<double>(1, 1) = eigen_r(1, 1), cv_r.at<double>(1,
                                            2) =
                                                eigen_r(1, 2);
                    cv_r.at<double>(2, 0) = eigen_r(2, 0), cv_r.at<double>(2, 1) = eigen_r(2, 1), cv_r.at<double>(2,
                                            2) =
                                                eigen_r(2, 2);
                    cv::Mat tvec;
                    tvec.create(3, 1, CV_64F);
                    tvec.at<double>(0, 0) = eigen_t[0], tvec.at<double>(1, 0) = eigen_t[1], tvec.at<double>(2,
                                            0) = eigen_t[2];
                    Mat rvec;
                    Rodrigues(cv_r, rvec);

                    vector<cv::Point2f> corner_reproj;

                    if (corresponding_board_coordinates_all_lr[k][lr][j].size() > 0) {
                        if (FLAGS_has_intrinsic) {
                            cv::projectPoints(corresponding_board_coordinates_all_lr[k][lr][j],
                                              rvec,
                                              tvec,
                                              duo_calib_param[k][lr].Camera.cv_camK_lr[k],
                                              duo_calib_param[k][lr].Camera.cv_dist_coeff_lr[k],
                                              corner_reproj);
                        } else {
                            cv::Mat camera_k;
                            cv::Mat camera_d;

                            if (k == 0) {
                                camera_k = (Mat_<double>(3, 3) << g_camera_intrinsic[lr].m_focal(0, 0), 0,
                                            g_camera_intrinsic[lr].m_center(0, 0),
                                            0., g_camera_intrinsic[lr].m_focal(1, 0), g_camera_intrinsic[lr].m_center(1, 0),
                                            0., 0., 1.);
                                camera_d =
                                    (Mat_<double>(4, 1) << g_camera_intrinsic[lr].m_distortion[0],
                                     g_camera_intrinsic[lr].m_distortion[1], 0., 0.);
                            } else {
                                camera_k = (Mat_<double>(3, 3) << g_camera_intrinsic_r[lr].m_focal(0, 0), 0,
                                            g_camera_intrinsic_r[lr].m_center(0, 0),
                                            0., g_camera_intrinsic_r[lr].m_focal(1, 0), g_camera_intrinsic_r[lr].m_center(1, 0),
                                            0., 0., 1.);
                                camera_d =
                                    (Mat_<double>(4, 1) << g_camera_intrinsic_r[lr].m_distortion[0],
                                     g_camera_intrinsic_r[lr].m_distortion[1], 0., 0.);

                            }

                            cv::projectPoints(corresponding_board_coordinates_all_lr[k][lr][j],
                                              rvec,
                                              tvec,
                                              camera_k,
                                              camera_d,
                                              corner_reproj);
                        }

                        vector<Point2f> corner_positions = detected_corners_all_lr[k][lr][j];
                        float reproj_error2_this_img = 0;
                        int reproj_count_this_img = 0;

                        for (size_t it_corner = 0; it_corner < corner_reproj.size(); it_corner++) {
                            float x_diff = corner_positions[it_corner].x - corner_reproj[it_corner].x;
                            float y_diff = corner_positions[it_corner].y - corner_reproj[it_corner].y;
                            float diff = x_diff * x_diff + y_diff * y_diff;
                            reproj_error2_this_img += diff;
                            reproj_count_this_img++;
                        }

                        cout << "Optimization error, LR: " << k << " Camera: " << lr << " targets: " << j
                             << " reproj corners num: "
                             << reproj_count_this_img
                             << " , error: " << sqrt(reproj_error2_this_img / reproj_count_this_img) << endl;
                        Mat _img = imread(file_names_vec[j].string(), IMREAD_COLOR);

                        for (size_t it_corner = 0; it_corner < corner_positions.size(); ++it_corner) {
                            cv::circle(_img, corner_positions[it_corner], 2, cv::Scalar(0, 0xff, 0));
                            cv::line(_img, corner_positions[it_corner],
                                     corner_reproj[it_corner], cv::Scalar(0, 0, 0xff));
                        }
                        cv::namedWindow("refine");
                        cv::imshow("refine", _img);
                        std::string fp = "refine_LR_" + std::to_string(k) + "Cam_" + std::to_string(
                                        lr) + "Target_" + std::to_string(j) + ".png";
                        boost::filesystem::path folder_path(FLAGS_output_folder + '/' + fp);

                        cv::imwrite(folder_path.c_str(), _img);

                        cv::waitKey(-1);
                    }
                }
            }
        }
    }

#if 1
    int num = 0;

    for (auto t : transsWorldToCam) {
        Transformation _t = t.get_inverse();
        cout << "camera " << num << " position: " << _t.m_translation[0] << "," << _t.m_translation[1] <<
             "," << _t.m_translation[2] << endl;
        num++;
    }

    num = 0;

    for (auto t : transsTargetToWorld) {
//        Transformation _t = t.get_inverse();
        Vector3 normal;
        normal = t * Vector3(Vector3d(0, 0, 1)) - t * Vector3(Vector3d(0, 0, 0));
        cout << "targets: " << num << " normal angle: " << acos(normal[2] / sqrt(
                    1 * normal.norm())) * 180 / 3.1415926
             << ", z: " << t.m_translation[2] << endl;
        num++;
    }

#endif

    return 0;
}
