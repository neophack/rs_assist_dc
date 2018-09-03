
#ifndef _MULTI_CAM_CALIB_H_
#define _MULTI_CAM_CALIB_H_

#undef CFG_DEBUG
#undef CFG_VERBOSE

#include <vector>
#include <Eigen/Eigen>
#include <float.h>

namespace MultiCamCalib {

class Matrix2x2 : public Eigen::Matrix<double, 2, 2, Eigen::RowMajor> {
public:
    inline Matrix2x2() : Eigen::Matrix<double, 2, 2, Eigen::RowMajor>() {}
    inline Matrix2x2(const Eigen::Matrix<double, 2, 2, Eigen::RowMajor> &M) :
        Eigen::Matrix<double, 2, 2, Eigen::RowMajor>(M) {}
    inline void operator=(const Eigen::Matrix<double, 2, 2, Eigen::RowMajor> &M) {
        *((Eigen::Matrix<double, 2, 2, Eigen::RowMajor> *) this) = M;
    }
};

class Matrix2x3 : public Eigen::Matrix<double, 2, 3, Eigen::RowMajor> {
public:
    inline Matrix2x3() : Eigen::Matrix<double, 2, 3, Eigen::RowMajor>() {}
    inline Matrix2x3(const Eigen::Matrix<double, 2, 3, Eigen::RowMajor> &M) :
        Eigen::Matrix<double, 2, 3, Eigen::RowMajor>(M) {}
    inline void operator=(const Eigen::Matrix<double, 2, 3, Eigen::RowMajor> &M) {
        *((Eigen::Matrix<double, 2, 3, Eigen::RowMajor> *) this) = M;
    }
};

class Matrix2x6 : public Eigen::Matrix<double, 2, 6, Eigen::RowMajor> {
public:
    inline Matrix2x6() : Eigen::Matrix<double, 2, 6, Eigen::RowMajor>() {}
    inline Matrix2x6(const Eigen::Matrix<double, 2, 6, Eigen::RowMajor> &M) :
        Eigen::Matrix<double, 2, 6, Eigen::RowMajor>(M) {}
    inline void operator=(const Eigen::Matrix<double, 2, 6, Eigen::RowMajor> &M) {
        *((Eigen::Matrix<double, 2, 6, Eigen::RowMajor> *) this) = M;
    }
    inline void set(const Matrix2x3 &M0, const Matrix2x3 &M1) {
        this->block<2, 3>(0, 0) = M0;
        this->block<2, 3>(0, 3) = M1;
    }
};

class Matrix3x3 : public Eigen::Matrix3d {
public:
    inline Matrix3x3() : Eigen::Matrix3d() {}
    inline Matrix3x3(const Eigen::Matrix3d &M) : Eigen::Matrix3d(M) {}
    inline void operator=(const Eigen::Matrix3d &M) {
        *((Eigen::Matrix3d *) this) = M;
    }
};

class Matrix6x2 : public Eigen::Matrix<double, 6, 2, Eigen::RowMajor> {
public:
    inline Matrix6x2() : Eigen::Matrix<double, 6, 2, Eigen::RowMajor>() {}
    inline Matrix6x2(const Eigen::Matrix<double, 6, 2, Eigen::RowMajor> &M) :
        Eigen::Matrix<double, 6, 2, Eigen::RowMajor>(M) {}
    inline void operator=(const Eigen::Matrix<double, 6, 2, Eigen::RowMajor> &M) {
        *((Eigen::Matrix<double, 6, 2, Eigen::RowMajor> *) this) = M;
    }
};

class Matrix6x6 : public Eigen::Matrix<double, 6, 6, Eigen::RowMajor> {
public:
    inline Matrix6x6() : Eigen::Matrix<double, 6, 6, Eigen::RowMajor>() {}
    inline Matrix6x6(const Eigen::Matrix<double, 6, 6, Eigen::RowMajor> &M) :
        Eigen::Matrix<double, 6, 6, Eigen::RowMajor>(M) {}
    inline void operator=(const Eigen::Matrix<double, 6, 6, Eigen::RowMajor> &M) {
        *((Eigen::Matrix<double, 6, 6, Eigen::RowMajor> *) this) = M;
    }
};

class MatrixX : public Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> {
public:
    inline void SetLowerFromUpper() {
        MatrixX &m = *this;
        const int Nr = static_cast<int>(this->rows());
        const int Nc = static_cast<int>(this->cols());
        const int Nm = std::min(Nr, Nc);
        for (int i = 0; i < Nm; ++i) {
            for (int j = 0; j < i; ++j) {
                m(i, j) = m(j, i);
            }
        }
    }
};

class Vector2 : public Eigen::Vector2d {
public:
    inline Vector2() : Eigen::Vector2d() {}
    inline Vector2(const Eigen::Vector2d &v) : Eigen::Vector2d(v) {}
    inline void operator=(const Eigen::Vector2d &v) {
        *((Eigen::Vector2d *) this) = v;
    }
    inline void invalidate() {
        this->x() = DBL_MAX;
    }
    inline bool invalid() const {
        return this->x() == DBL_MAX;
    }
    inline bool valid() const {
        return this->x() != DBL_MAX;
    }
    inline void print() const {
        printf("%f %f\n", x(), y());
    }
    static inline Vector2 random(const double vMax) {
        Vector2 v;
        v.setRandom();
        v *= vMax;
        return v;
    }
};

class Intrinsic;

class Vector3 : public Eigen::Vector3d {
public:
    inline Vector3() : Eigen::Vector3d() {}
    inline Vector3(const Eigen::Vector3d &v) : Eigen::Vector3d(v) {}
    inline void operator=(const Eigen::Vector3d &v) {
        *((Eigen::Vector3d *) this) = v;
    }
    Vector2 project(const Intrinsic *K = NULL) const;
    bool project(Vector2 *x, Matrix2x3 *Jx, const Intrinsic *K = NULL, Matrix2x6 *Jk = NULL) const;
    inline Matrix3x3 GetSkewSymmetricMatrix() const {
        Matrix3x3 s;
        s(0, 0) = 0.0;
        s(0, 1) = -this->z();
        s(0, 2) = this->y();
        s(1, 0) = this->z();
        s(1, 1) = 0.0;
        s(1, 2) = -this->x();
        s(2, 0) = -this->y();
        s(2, 1) = this->x();
        s(2, 2) = 0.0;
        return s;
    }
    static inline Vector3 random(const double vMax) {
        Vector3 v;
        v.setRandom();
        v *= vMax;
        return v;
    }
};

class Vector6 : public Eigen::Matrix<double, 6, 1> {
public:
    inline Vector6() : Eigen::Matrix<double, 6, 1>() {}
    inline Vector6(const Eigen::Matrix<double, 6, 1> &M) :
        Eigen::Matrix<double, 6, 1>(M) {}
    inline void operator=(const Eigen::Matrix<double, 6, 1> &M) {
        *((Eigen::Matrix<double, 6, 1> *) this) = M;
    }
    static inline Vector6 random(const double vMax012, const double vMax345) {
        Vector6 v;
        v.block<3, 1>(0, 0) = Vector3::Random(vMax012);
        v.block<3, 1>(3, 0) = Vector3::Random(vMax345);
        return v;
    }
    static inline Vector6 zero() {
        Vector6 v;
        v.setZero();
        return v;
    }
    static inline Vector6 random(const double vMax01, const double vMax23,
                                 const double vMax45) {
        Vector6 v;
        v.block<2, 1>(0, 0) = Vector2::Random(vMax01);
        v.block<2, 1>(2, 0) = Vector2::Random(vMax23);
        v.block<2, 1>(4, 0) = Vector2::Random(vMax45);
        return v;
    }
};

class VectorX : public Eigen::Matrix<double, Eigen::Dynamic, 1> {
public:
    inline VectorX() : Eigen::Matrix<double, Eigen::Dynamic, 1>() {}
    inline VectorX(const Eigen::Matrix<double, Eigen::Dynamic, 1> &M) :
        Eigen::Matrix<double, Eigen::Dynamic, 1>(M) {}
    inline void operator=(const Eigen::Matrix<double, Eigen::Dynamic, 1> &M) {
        *((Eigen::Matrix<double, Eigen::Dynamic, 1> *) this) = M;
    }
};

class Rotation : public Matrix3x3 {
public:
    inline Rotation() : Matrix3x3() {}
    inline Rotation(const Matrix3x3 &M) : Matrix3x3(M) {}
    inline Rotation(const Eigen::Matrix3d &M) : Matrix3x3(M) {}
    inline Rotation(const Vector3 &w) {
        set_rodrigues(w);
    }
    inline void operator=(const Eigen::Matrix3d &M) {
        *((Matrix3x3 *) this) = M;
    }
    inline void set_quaternion(const Eigen::Quaterniond &q) {
        const double qxx = q.x() * q.x(), qxy = q.x() * q.y(), qxz = q.x() * q.z();
        const double qxw = q.x() * q.w(), qyy = q.y() * q.y(), qyz = q.y() * q.z();
        const double qyw = q.y() * q.w(), qzz = q.z() * q.z(), qzw = q.z() * q.w();
        Rotation &r = *this;
        r(0, 0) = qyy + qzz;
        r(0, 1) = qxy + qzw;
        r(0, 2) = qxz - qyw;
        r(1, 0) = qxy - qzw;
        r(1, 1) = qxx + qzz;
        r(1, 2) = qyz + qxw;
        r(2, 0) = qxz + qyw;
        r(2, 1) = qyz - qxw;
        r(2, 2) = qxx + qyy;
        r += r;
        r(0, 0) = -r(0, 0) + 1.0;
        r(1, 1) = -r(1, 1) + 1.0;
        r(2, 2) = -r(2, 2) + 1.0;
    }
    inline void set_rodrigues(const Vector3 &w) {
        Vector3 w2;
        w2 = w.cwiseProduct(w);
        const double th2 = w2.sum(), th = std::sqrt(th2);
        if (th2 < DBL_EPSILON) {
            const double s = 1.0 / std::sqrt(th2 + 4.0);
            Eigen::Quaterniond q;
            q.x() = w.x() * s;
            q.y() = w.y() * s;
            q.z() = w.z() * s;
            q.w() = s + s;
            *this = q.toRotationMatrix().transpose();
            //SetQuaternion(q);
            return;
        }
        const double t1 = std::sin(th) / th, t2 = (1.0 - std::cos(th)) / th2, t3 = 1.0 - t2 * th2;
        Vector3 t1w;
        t1w = w * t1;
        Vector3 t2w2;
        t2w2 = w2 * t2;
        const double t2wx = t2 * w.x();
        const double t2wxy = t2wx * w.y();
        const double t2wxz = t2wx * w.z();
        const double t2wyz = t2 * w.y() * w.z();
        Rotation &r = *this;
        r(0, 0) = t3 + t2w2.x();
        r(0, 1) = t2wxy + t1w.z();
        r(0, 2) = t2wxz - t1w.y();
        r(1, 0) = t2wxy - t1w.z();
        r(1, 1) = t3 + t2w2.y();
        r(1, 2) = t2wyz + t1w.x();
        r(2, 0) = t2wxz + t1w.y();
        r(2, 1) = t2wyz - t1w.x();
        r(2, 2) = t3 + t2w2.z();
    }
};

class Transformation {
public:
    inline void invalidate() {
        m_rotation(0, 0) = DBL_MAX;
    }
    inline bool invalid() const {
        return m_rotation(0, 0) == DBL_MAX;
    }
    inline bool valid() const {
        return m_rotation(0, 0) != DBL_MAX;
    }
    inline void set_identity() {
        m_rotation.setIdentity();
        m_translation.setZero();
    }
    inline Transformation get_inverse() const {
        Transformation trans_inv;
        trans_inv.m_rotation = m_rotation.transpose();
        trans_inv.m_translation = -trans_inv.m_rotation * m_translation;
        return trans_inv;
    }
    inline Transformation operator*(const Transformation &other) const {
        Transformation res;
        res.m_rotation = m_rotation * other.m_rotation;
        res.m_translation = m_rotation * other.m_translation + m_translation;
        return res;
    }
    inline Vector3 operator*(const Vector3 &position) const {
        Vector3 res;
        res = m_rotation * position;
        res += m_translation;
        return res;
    }
    inline void operator+=(const Vector6 &x) {
        const Vector3 xr(x.block<3, 1>(0, 0));
        const Vector3 xt(x.block<3, 1>(3, 0));
        m_rotation = m_rotation * Rotation(xr);
        m_translation += xt;
    }
    inline Transformation operator+(const Vector6 &x) const {
        Transformation t = *this;
        t += x;
        return t;
    }
    inline void print() const {
        printf("%f %f %f %f\n", m_rotation(0, 0), m_rotation(0, 1), m_rotation(0, 2),
               m_translation.x());
        printf("%f %f %f %f\n", m_rotation(1, 0), m_rotation(1, 1), m_rotation(1, 2),
               m_translation.y());
        printf("%f %f %f %f\n", m_rotation(2, 0), m_rotation(2, 1), m_rotation(2, 2),
               m_translation.z());
    }
    inline void save(FILE *fp) const {
        fprintf(fp, "%le %le %le %le\n", m_rotation(0, 0), m_rotation(0, 1), m_rotation(0, 2),
                m_translation.x());
        fprintf(fp, "%le %le %le %le\n", m_rotation(1, 0), m_rotation(1, 1), m_rotation(1, 2),
                m_translation.y());
        fprintf(fp, "%le %le %le %le\n", m_rotation(2, 0), m_rotation(2, 1), m_rotation(2, 2),
                m_translation.z());
    }
    inline void load(FILE *fp) {
        fscanf(fp, "%le %le %le %le", &m_rotation(0, 0), &m_rotation(0, 1), &m_rotation(0, 2),
               &m_translation.x());
        fscanf(fp, "%le %le %le %le", &m_rotation(1, 0), &m_rotation(1, 1), &m_rotation(1, 2),
               &m_translation.y());
        fscanf(fp, "%le %le %le %le", &m_rotation(2, 0), &m_rotation(2, 1), &m_rotation(2, 2),
               &m_translation.z());
    }
    static inline Transformation identity() {
        Transformation t;
        t.set_identity();
        return t;
    }
public:
    Rotation m_rotation;
    Vector3 m_translation;
};

class Intrinsic {
public:
    inline void operator+=(const Vector6 &x) {
        const Vector2 xf(x.block<2, 1>(0, 0));
        const Vector2 xc(x.block<2, 1>(2, 0));
        m_focal += xf;
        m_center += xc;
        m_distortion[0] += x(4, 0);
        m_distortion[1] += x(5, 0);
    }
    inline Intrinsic operator+(const Vector6 &x) const {
        Intrinsic k = *this;
        k += x;
        return k;
    }
    inline void print() const {
        printf("%f %f %f %f %f %f\n", m_focal.x(), m_focal.y(), m_center.x(), m_center.y(),
               m_distortion[0], m_distortion[1]);
    }
    inline void save(FILE *fp) const {
        fprintf(fp, "%le %le %le %le %le %le\n", m_focal.x(), m_focal.y(),
                m_center.x(), m_center.y(),
                m_distortion[0], m_distortion[1]);
    }
    inline void load(FILE *fp) {
        fscanf(fp, "%le %le %le %le %le %le", &m_focal.x(), &m_focal.y(),
               &m_center.x(), &m_center.y(),
               &m_distortion[0], &m_distortion[1]);
    }
    static inline Intrinsic identity() {
        Intrinsic k;
        k.m_focal.x() = k.m_focal.y() = 1.0;
        k.m_center.x() = k.m_center.y() = 0.0;
        k.m_distortion[0] = k.m_distortion[1] = 0.0;
        return k;
    }
public:
    Vector2 m_focal, m_center;
    double m_distortion[2];
};

class Frame {
public:
    int m_iTarget, m_iCam;
    Transformation m_transTargetToCam;      // transformation from target to left camera coordinate
    std::vector<Vector2> m_corners2D;       // 2D location in left image plane
    std::vector<Vector2> m_corners2DRight;  // 2D location in right image plane
};

bool run(const int nTargets, const int nCams,
         const std::vector<Vector3> &corners3D,     // 3D position in target coordinate
         const std::vector<Frame> &frms,
         std::vector<Transformation> *transsTargetToWorld,
         std::vector<Transformation> *transsWorldToCam,
         std::vector<Intrinsic> *intrinsics = NULL,
         // If intrinsic of left cameras are pre-calibrated, set intrinsics to NULL and normalize
         // m_corners2D, else input initial value for intrinsics and m_corners2D in origin image plane
         std::vector<Intrinsic> *intrinsicsRight = NULL,
         std::vector<Transformation> *transsLeftToRight = NULL
                 // For monocular camera, set intrinsicsRight and transsLeftToRight to NULL and
                 // m_corners2DRight is ignored. For stereo camera, if intrinsics of right cameras
                 // and extrinsics of left-right cameras are pre-calibrated, set intrinsicsRight to
                 // NULL, set transsLeftToRight to the known extrinsics and normalize m_corners2DRight,
                 // else input initial value for intrinsics, extrinsics and m_corners2D in origin image
                 // plane
        );

void print_error(const std::vector<Vector3> &corners3D,
                const std::vector<Frame> &frms,
                const std::vector<Intrinsic> *intrinsics = NULL,
                const std::vector<Intrinsic> *intrinsicsRight = NULL,
                const std::vector<Transformation> *transsLeftToRight = NULL);
void print_error(const std::vector<Vector3> &corners3D,
                const std::vector<Frame> &frms,
                const std::vector<Transformation> &transsTargetToWorld,
                const std::vector<Transformation> &transsWorldToCam,
                const std::vector<Intrinsic> *intrinsics = NULL,
                const std::vector<Intrinsic> *intrinsicsRight = NULL,
                const std::vector<Transformation> *transsLeftToRight = NULL);
void print_result(const std::vector<Transformation> &transsTargetToWorld,
                 const std::vector<Transformation> &transsWorldToCam);
void print_result(const std::vector<Intrinsic> *intrinsics = NULL,
                 const std::vector<Intrinsic> *intrinsicsRight = NULL,
                 const std::vector<Transformation> *transsLeftToRight = NULL);

bool save(const char *fileName, const int nTargets, const int nCams,
          const std::vector<Vector3> &corners3D,
          const std::vector<Frame> &frms,
          const std::vector<Intrinsic> &intrinsics,
          const std::vector<Intrinsic> &intrinsicsRight,
          const std::vector<Transformation> &transsLeftToRight);
bool load(const char *fileName, int *nTargets, int *nCams,
          std::vector<Vector3> *corners3D,
          std::vector<Frame> *frms,
          std::vector<Intrinsic> *intrinsics,
          std::vector<Intrinsic> *intrinsicsRight,
          std::vector<Transformation> *transsLeftToRight);
bool save(const char *fileName, const std::vector<Transformation> &transs);
bool load(const char *fileName, std::vector<Transformation> *transs);

}

#endif