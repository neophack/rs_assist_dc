#include <iostream>
#include <opencv2/opencv.hpp>
#include <glog/logging.h>
#include <Eigen/Dense>
#include <fstream>
#include <vector>
#include <boost/filesystem.hpp>

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


inline cv::Vec3b depth16S2color(float disparity16S, int maxVal) {
    if (disparity16S <= 0) {
        return cv::Vec3b(0, 0, 0);
    }
    // 255 / 16 ~= 16
    // opencv disparity result in 16S is multiplied by 16
    //  constexpr int max_disp_pixel = 96;
    int val = disparity16S * 255 / maxVal / 16;
    if (val > 255) {
        val = 255;
    }

    uchar r = 0, g = 0, b = 0;
    if (val > 127) {
        r = (val - 128) * 2;
        b = (255 - val) * 2;
    } else {
        b = (val) * 2;
        g = (127 - val) * 2;
    }
    return cv::Vec3b(b, g, r);
}

int cnt = 0;
template<typename T>
void showDisp(const cv::Mat &img, std::string windowname, int maxVal) {
    cv::Mat depth_canvas = cv::Mat::zeros(img.size(), CV_8UC3);
    cv::Mat img1 = cv::Mat::zeros(img.size(), CV_16U);
    for (int i = 0; i < img.rows; ++i) {
        for (int j = 0; j < img.cols; ++j) {
            depth_canvas.at<cv::Vec3b>(i, j) = depth16S2color(img.at<T>(i, j), maxVal);
        }
    }
    cv::imshow(windowname, depth_canvas);
    //  imwrite("disp_" + std::to_string(cnt++) + ".png", depth_canvas);
}

void get2dRegion(const cv::Mat &depth, const cv::Mat &mask, const std::vector<Intrinsic>& v_intrinsic,
        const std::vector<Eigen::Matrix4f> &v_tlw,
        const int cam_from,  const int cam_to,
        std::vector<cv::Point2f> &region){
    LOG(INFO) << "rgb: " << cam_from << '\t' << cam_to;

    cv::Point2f p_2d;
    std::vector<cv::Point3f> pts_3d;
    float fx = v_intrinsic[cam_from].focal[0], fy = v_intrinsic[cam_from].focal[1];
    float cx = v_intrinsic[cam_from].center[0], cy = v_intrinsic[cam_from].center[1];

    Eigen::Matrix3f r = v_tlw[cam_from].topLeftCorner(3,3).transpose();
    Eigen::Vector3f t = -r * v_tlw[cam_from].topRightCorner(3,1);

    for(int i=0; i<mask.cols; i++){
        for(int j=0; j<mask.rows; j++){
            if(mask.at<uchar>(j,i) == 0)
                continue;

            std::vector<cv::Point2f> corner;
            corner.push_back(cv::Point2f(i,j));
            std::vector<cv::Point2f> undistort_corner;
            cv::Mat camera_k = (cv::Mat_<double>(3, 3) << v_intrinsic[cam_from].focal[0], 0.0,
                    v_intrinsic[cam_from].center[0],
                    0.0, v_intrinsic[cam_from].focal[1], v_intrinsic[cam_from].center[1],
                    0.0, 0.0, 1.0);
            cv::Mat camera_d = (cv::Mat_<double>(4, 1) << v_intrinsic[cam_from].distort[0],
                    v_intrinsic[cam_from].distort[1], 0, 0);
            cv::undistortPoints(corner, undistort_corner, camera_k, camera_d);

            float z = depth.at<ushort>(j,i);
            float x = undistort_corner[0].x * z;
            float y = undistort_corner[0].y * z;
            Eigen::Vector3f pc(x,y,z);
            Eigen::Vector3f pw = 0.001*r*pc + t;
            pts_3d.push_back(cv::Point3f(pw(0,0), pw(1,0), pw(2,0)));
        }
    }

    cv::Mat camera_k = (cv::Mat_<double>(3, 3) << v_intrinsic[cam_to].focal[0], 0.0, v_intrinsic[cam_to].center[0],
            0.0, v_intrinsic[cam_to].focal[1], v_intrinsic[cam_to].center[1],
            0.0, 0.0, 1.0);
    cv::Mat camera_d = (cv::Mat_<double>(4, 1) << v_intrinsic[cam_to].distort[0], v_intrinsic[cam_to].distort[1], 0, 0);
    Eigen::Matrix3f rcw = v_tlw[cam_to].topLeftCorner(3, 3);
    Eigen::Vector3f tcw = v_tlw[cam_to].topRightCorner(3, 1);

    cv::Mat rvec;
    cv::Mat cv_rcw = (cv::Mat_<double>(3, 3) << rcw(0, 0), rcw(0, 1), rcw(0, 2),
            rcw(1, 0), rcw(1, 1), rcw(1, 2),
            rcw(2, 0), rcw(2, 1), rcw(2, 2));
    Rodrigues(cv_rcw, rvec);
    cv::Mat tvec = (cv::Mat_<double>(3, 1) << tcw(0, 0), tcw(1, 0), tcw(2, 0));

    if (pts_3d.size() > 0) {
        cv::projectPoints(pts_3d,
            rvec,
            tvec,
            camera_k, camera_d,
            region);
    }

}

DEFINE_string(extrinsic_fp, "./data/transsWorldToCam.txt", "extrinsic file");
DEFINE_string(intrinsic_fp, "./data/leftIntrinsic.txt", "intrinsic file");
DEFINE_string(rgb_path0, "./data/test_data/cam0/0001531476158412.jpg", "dir contins rgb images");
DEFINE_string(rgb_path1, "./data/test_data/cam0/0001531476172966.jpg", "dir contins rgb images");
DEFINE_string(depth_path0, "./data/test_data/cam2/13-0_Depth.raw", "dir contains depth images");
DEFINE_string(depth_path1, "./data/test_data/cam2/14-0_Depth.raw", "dir contains depth images");
DEFINE_string(result_fp, "./result.txt", "result file");
DEFINE_int32(depth_camid, 2, "cam id for depth images");
DEFINE_int32(rgb_camid, 0, "cam id for rgb images");
DEFINE_bool(show_result, false, "show result or not");

void preset_window() {
    cv::namedWindow("GetFocus", CV_WINDOW_NORMAL);
    cv::Mat img = cv::Mat::zeros(100, 100, CV_8UC3);
    cv::imshow("GetFocus", img);
    cv::setWindowProperty("GetFocus", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
    cv::waitKey(1);
    cv::setWindowProperty("GetFocus", CV_WND_PROP_FULLSCREEN, CV_WINDOW_NORMAL);
    cv::destroyWindow("GetFocus");
}

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    google::ParseCommandLineFlags(&argc, &argv, false);
    google::InstallFailureSignalHandler();
    FLAGS_logtostderr = 1;

    std::vector<Eigen::Matrix4f> v_tlw;
    loadExtricParam(FLAGS_extrinsic_fp, v_tlw);

    std::vector<Intrinsic> v_intrinsic;
    loadIntrinsic(FLAGS_intrinsic_fp, v_intrinsic);

    std::vector<std::string> rgb_files;
    boost::filesystem::path rgb_path0(FLAGS_rgb_path0);
    rgb_files.push_back(rgb_path0.string());
    boost::filesystem::path rgb_path1(FLAGS_rgb_path1);
    rgb_files.push_back(rgb_path1.string());

    std::vector<std::string> depth_files;
    boost::filesystem::path depth_path0(FLAGS_depth_path0);
    boost::filesystem::path depth_path1(FLAGS_depth_path1);

    depth_files.push_back(depth_path0.string());
    depth_files.push_back(depth_path1.string());

    int src_cam = FLAGS_depth_camid;
    int dst_cam = FLAGS_rgb_camid;

    // std::ofstream fobj(FLAGS_result_fp.c_str(), std::ios::out);
    // std::ofstream fobj = std::cout;
    for (std::vector<std::string>::size_type i = 1; i < depth_files.size(); i++) {
        cv::Mat rgb_img = cv::imread(rgb_files[i]);
        std::ifstream depth_file1(depth_files[i - 1]);
        std::ifstream depth_file2(depth_files[i]);

        LOG(INFO) << "rgb: " << rgb_files[i];
        LOG(INFO) << "d1: " << depth_files[i - 1];
        LOG(INFO) << "d2: " << depth_files[i];

        std::vector<std::uint16_t> img_buffer1, img_buffer2;

        std::uint16_t n;
        while (depth_file1.read(reinterpret_cast<char*>(&n), 2)) {
            img_buffer1.push_back(n);
        }
        while (depth_file2.read(reinterpret_cast<char*>(&n), 2)) {
            img_buffer2.push_back(n);
        }

        cv::Mat depth_img_pre(480, 640,  CV_16UC1, img_buffer1.data());
        cv::Mat depth_img(480, 640, CV_16UC1, img_buffer2.data());


        cv::Mat depth_diff;
        cv::absdiff(depth_img_pre, depth_img, depth_diff);
        depth_diff.convertTo(depth_diff, CV_8U);

        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(5, 5));
        cv::erode(depth_diff, depth_diff, kernel);
        cv::threshold(depth_diff, depth_diff, 10, 255, cv::THRESH_BINARY);

        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(depth_diff.clone(), contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

        std::vector<std::vector<cv::Point> > polys(contours.size());
        for (int i = 0; i < contours.size(); i++) {
            cv::approxPolyDP(contours[i], polys[i], 5, true);
        }

        cv::Mat mask = cv::Mat::zeros(depth_img.size(), CV_8UC1) * 255;
        int ind_largest = 0;
        double area_largest = cv::contourArea(polys[0], false);
        for (int i = 1; i < polys.size(); i++) {
            double area = cv::contourArea(polys[i], false);
            if (area > area_largest) {
                ind_largest = i;
                area_largest = area;
            }
        }
        if (area_largest > 2000) {
            cv::drawContours(mask, polys, ind_largest, 255, -1);
        }
        // LOG(INFO) << 'mask:' << mask;
        // mask to label
        cv::Mat region_diff = cv::Mat::zeros(depth_img_pre.size(), CV_32F);
        depth_img_pre.copyTo(region_diff, mask);

        cv::Mat region_diff1 = cv::Mat::zeros(depth_img.size(), CV_32F);
        depth_img.copyTo(region_diff1, mask);
        // cv::Mat diff = region_diff1 - region_diff;
        double sum = cv::sum(region_diff1)[0] - cv::sum(region_diff)[0];

        std::vector<cv::Point2f> region;
        get2dRegion(depth_img, mask, v_intrinsic, v_tlw, src_cam, dst_cam, region);

        LOG(INFO) << rgb_files[i-1] << "\t" << rgb_files[i] << "\t" << depth_files[i-1] << "\t"  \
            << depth_files[i] << "\t" << FLAGS_extrinsic_fp << "\t" << FLAGS_intrinsic_fp << "\t"  \
            << src_cam << "\t" << dst_cam;

        std::cout << std::endl;
        std::cout << "sum:\t" << sum << '\t';
        for(int i=0; i< region.size(); i++){
            cv::circle(rgb_img, region[i], 2, cv::Scalar(0,0,255) );
            std::cout << region[i].x << "," << region[i].y<<':';
        }
        std::cout << std::endl;
        if (FLAGS_show_result == true) {
            preset_window();
            cv::namedWindow("mask");
            cv::imshow("mask", mask);
            for(int i=0; i< region.size(); i++){
                cv::circle(rgb_img, region[i], 2, cv::Scalar(0,0,255) );
            }
            cv::namedWindow("ShowImg");
            cv::imshow("ShowImg", rgb_img);
            cv::waitKey(0);
        }


    }

    return 0;
}
