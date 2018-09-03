#include "MultiCamCalib.h"

#ifdef CFG_DEBUG
//#define DEBUG_REMOVE_USELESS_FRAME
#define DEBUG_CHECK_JACOBIAN
#endif

//#define SFM_INCREMENTAL           0
#define SFM_INCREMENTAL             1
//#define SFM_INCREMENTAL_MIN_CORNERS 10
#define SFM_INCREMENTAL_MIN_CORNERS 20
//#define SFM_INCREMENTAL_MIN_CORNERS 140
#define BA_ITERATIONS               10
#define BA_CONVERGE_ERROR           1.0e-2
#define BA_DEFAULT_FOCAL            500.0

namespace MultiCamCalib {

int CountValidCorners2D(const std::vector<Vector2>& corners2D) {
    int cnt = 0;
    const int N = static_cast<int>(corners2D.size());

    for (int i = 0; i < N; ++i) {
        if (corners2D[i].valid()) {
            ++cnt;
        }
    }

    return cnt;
}

bool Initialize(const std::vector<Frame>& frms,
                std::vector<Transformation>* transsTargetToWorld,
                std::vector<Transformation>* transsWorldToCam) {
    const int nTargets = static_cast<int>(transsTargetToWorld->size());
    const int nCams = static_cast<int>(transsWorldToCam->size());
    std::vector<bool> targetMarks1(nTargets, false), camMarks1(nCams, false);
    std::vector<bool> targetMarks2(nTargets, false), camMarks2(nCams, false);
    transsTargetToWorld->at(0).set_identity();
    targetMarks1[0] = targetMarks2[0] = true;
    int targetCntTotal = 1, camCntTotal = 0;

    while (1) {
        int targetCnt = 0;
        int camCnt = 0;
        const int nFrms = static_cast<int>(frms.size());

        for (int iFrm = 0; iFrm < nFrms; ++iFrm) {
            const Frame& frm = frms[iFrm];

            if (frm.m_transTargetToCam.invalid() ||
                    CountValidCorners2D(frm.m_corners2D) < SFM_INCREMENTAL_MIN_CORNERS) {
                continue;
            } else if (targetMarks1[frm.m_iTarget] && !camMarks2[frm.m_iCam]) {
                transsWorldToCam->at(frm.m_iCam) = frm.m_transTargetToCam *
                                                   transsTargetToWorld->at(frm.m_iTarget).get_inverse();
                camMarks2[frm.m_iCam] = true;
                ++camCnt;
            } else if (camMarks1[frm.m_iCam] && !targetMarks2[frm.m_iTarget]) {
                transsTargetToWorld->at(frm.m_iTarget) = transsWorldToCam->at(frm.m_iCam).get_inverse() *
                        frm.m_transTargetToCam;
                targetMarks2[frm.m_iTarget] = true;
                ++targetCnt;
            }
        }

        if (targetCnt == 0 && camCnt == 0) {
            break;
        }

        targetMarks1 = targetMarks2;
        camMarks1 = camMarks2;
        targetCntTotal += targetCnt;
        camCntTotal += camCnt;
    }

    const bool scc = targetCntTotal == nTargets && camCntTotal == nCams;
#ifdef CFG_VERBOSE

    if (!scc) {
        printf("Isolated targets: {");

        for (int iTarget = 0; iTarget < nTargets; ++iTarget) {
            if (!targetMarks1[iTarget]) {
                printf(" %d", iTarget);
            }
        }

        printf(" }\n");
        printf("Isolated cameras: {");

        for (int iCam = 0; iCam < nCams; ++iCam) {
            if (!camMarks1[iCam]) {
                printf(" %d", iCam);
            }
        }

        printf(" }\n");
    }

#endif
    return scc;
}

Vector2 Vector3::project(const Intrinsic* K) const {
    Vector2 x;
    const double zI = 1.0 / this->z();
    x.x() = this->x() * zI;
    x.y() = this->y() * zI;

    if (K) {
        const double r2 = x.squaredNorm(), r4 = r2 * r2;
        const double s = 1 + K->m_distortion[0] * r2 + K->m_distortion[1] * r4;
        Vector2 sx;
        sx = s * x;
        x.x() = K->m_focal.x() * sx.x() + K->m_center.x();
        x.y() = K->m_focal.y() * sx.y() + K->m_center.y();
    }

    return x;
}

bool Vector3::project(Vector2* x, Matrix2x3* Jx, const Intrinsic* K, Matrix2x6* Jk) const {
    //if (this->z() <= 0.0) {
    //  return false;
    //}
    if (fabs(this->z()) < DBL_EPSILON) {
        return false;
    }

    const double zI = 1.0 / this->z();
    Vector2& _x = *x;
    _x.x() = this->x() * zI;
    _x.y() = this->y() * zI;
    Matrix2x3& _Jx = *Jx;
    _Jx(0, 0) = zI;
    _Jx(0, 1) = 0.0;
    _Jx(0, 2) = -_x.x() * zI;
    _Jx(1, 0) = 0.0;
    _Jx(1, 1) = zI;
    _Jx(1, 2) = -_x.y() * zI;

    if (K) {
        const double fx = K->m_focal.x(), fy = K->m_focal.y();
        const double k1 = K->m_distortion[0], k2 = K->m_distortion[1];
        const double r2 = _x.squaredNorm(), r4 = r2 * r2;
        const double s = 1 + k1 * r2 + k2 * r4;
        const double Jsr2 = k1 + (k2 * 2 * r2);
        Vector2 Jsx;
        Jsx = Jsr2 * 2 * _x;
        Matrix2x2 Jix;
        Jix(0, 0) = fx * (Jsx.x() * _x.x() + s);
        Jix(0, 1) = fx * Jsx.y() * _x.x();
        Jix(1, 0) = fy * Jsx.x() * _x.y();
        Jix(1, 1) = fy * (Jsx.y() * _x.y() + s);
        _Jx = Jix * _Jx;
        Vector2 sx;
        sx = s * _x;

        if (Jk) {
            Matrix2x6& _Jk = *Jk;
            _Jk(0, 0) = sx.x();
            _Jk(0, 1) = 0.0;
            _Jk(0, 2) = 1.0;
            _Jk(0, 3) = 0.0;
            _Jk(1, 0) = 0.0;
            _Jk(1, 1) = sx.y();
            _Jk(1, 2) = 0.0;
            _Jk(1, 3) = 1.0;
            _Jk(0, 4) = fx * r2 * _x.x();
            _Jk(0, 5) = _Jk(0, 4) * r2;
            _Jk(1, 4) = fy * r2 * _x.y();
            _Jk(1, 5) = _Jk(1, 4) * r2;
        }

        _x.x() = fx * sx.x() + K->m_center.x();
        _x.y() = fy * sx.y() + K->m_center.y();
    }

    return true;
}

class Linearization {
public:
    inline bool Set(const Transformation& Twt, const Transformation& Tcw, const Transformation& Tct,
                    const Vector3& Xt, const Vector2& x, const Intrinsic* K = NULL,
                    const bool k = false, const Transformation* Trl = NULL, const bool r = false) {
        const Vector3 Xc = Tct * Xt;
        Matrix2x3 Jx;

        if (!Xc.project(&m_e, &Jx, K, k ? &m_Jk : NULL)) {
            return false;
        }

        Matrix2x3 Jtr;
        Jtr = Jx * Tct.m_rotation * Xt.GetSkewSymmetricMatrix();
        Matrix2x3 Jtt;
        Jtt = Jx * Tcw.m_rotation;
        Matrix2x3 Jcr;
        Jcr = Jtt * Vector3(Twt * Xt).GetSkewSymmetricMatrix();
        Matrix2x3 Jct;
        Jct = Trl ? Matrix2x3(Jx * Trl->m_rotation) : Jx;
        m_Jt.set(Jtr, Jtt);
        m_Jc.set(Jcr, Jct);
        m_e = x - m_e;

        if (r) {
#ifdef CFG_DEBUG
            assert(K != NULL && Trl != NULL);
#endif
            Matrix2x3 Jrr;
            Jrr = Jx * Vector3(Xc - Trl->m_translation).GetSkewSymmetricMatrix() *
                  Trl->m_rotation;
            const Matrix2x3 Jrt = Jx;
            m_Jr.set(Jrr, Jrt);
        }

        return true;
    }
    inline bool Check(const Transformation& Twt, const Transformation& Tcw, const Vector3& Xt,
                      const Intrinsic* K = NULL, const bool k = false,
                      const Transformation* Trl = NULL, const bool r = false) const {
        const double xtrMax = 0.0;
        //const double xtrMax = 1.0;
        const double xttMax = 0.0;
        //const double xttMax = 0.01;
        const double xcrMax = 0.0;
        //const double xcrMax = 1.0;
        const double xctMax = 0.0;
        //const double xctMax = 0.01;
        //const double xkfMax = 0.0;
        const double xkfMax = 10.0;
        //const double xkcMax = 0.0;
        const double xkcMax = 10.0;
        //const double xkdMax = 0.0;
        const double xkdMax = 0.1;
        const double xrrMax = 0.0;
        //const double xrrMax = 1.0;
        const double xrtMax = 0.0;
        //const double xrtMax = 0.01;
        const Vector6 xt = Vector6::random(xtrMax * M_PI / 180.0, xttMax);
        const Vector6 xc = Vector6::random(xcrMax * M_PI / 180.0, xctMax);
        const Vector6 xk = k ? Vector6::random(xkfMax, xkcMax, xkdMax) : Vector6::zero();
        const Vector6 xr = r ? Vector6::random(xrrMax * M_PI / 180.0, xrtMax) : Vector6::zero();
        const Transformation Twt1 = Twt + xt;
        const Transformation Tcw1 = Tcw + xc;
        const Intrinsic _K = K ? *K : Intrinsic::identity();
        const Intrinsic K1 = _K + xk;
        const Transformation _Trl = Trl ? *Trl : Transformation::identity();
        const Transformation Trl1 = _Trl + xr;
        const Vector2 x = Vector3(_Trl * Tcw * Twt * Xt).project(&_K);
        const Vector2 x1 = Vector3(Trl1 * Tcw1 * Twt1 * Xt).project(&K1);
        Vector2 e1;
        Vector2 e2;
        e1 = x1 - x, e2 = e1 - m_Jt * xt - m_Jc * xc - m_Jk * xk - m_Jr * xr;
        const bool scc = e2.norm() <= e1.norm();

        if (!scc) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            printf("    %f (%f, %f)\n", e1.norm(), e1.x(), e1.y());
            printf("--> %f (%f, %f)\n", e2.norm(), e2.x(), e2.y());
            printf("xt = (%f, %f)\n", Vector3(xt.block<3, 1>(0, 0)).norm() * 180.0 / M_PI,
                   Vector3(xt.block<3, 1>(3, 0)).norm());
            printf("xc = (%f, %f)\n", Vector3(xc.block<3, 1>(0, 0)).norm() * 180.0 / M_PI,
                   Vector3(xc.block<3, 1>(3, 0)).norm());
            printf("xk = (%f, %f, %f, %f, %f, %f)\n", xk(0, 0), xk(1, 0), xk(2, 0),
                   xk(3, 0), xk(4, 0), xk(5, 0));
            printf("xr = (%f, %f)\n", Vector3(xc.block<3, 1>(0, 0)).norm() * 180.0 / M_PI,
                   Vector3(xc.block<3, 1>(3, 0)).norm());
        }

        return scc;
    }
public:
    Matrix2x6 m_Jt;
    Matrix2x6 m_Jc;
    Matrix2x6 m_Jk;
    Matrix2x6 m_Jr;
    Vector2 m_e;
};

class Factor {
public:
    inline void Initialize() {
        m_Att.setZero();
        m_Atc.setZero();
        m_Atk.setZero();
        m_Atr.setZero();
        m_bt.setZero();
        m_Acc.setZero();
        m_Ack.setZero();
        m_Acr.setZero();
        m_bc.setZero();
        m_Akk.setZero();
        m_Akr.setZero();
        m_bk.setZero();
        m_Arr.setZero();
        m_br.setZero();
    }
    inline void Accumulate(const Linearization& L, const bool k = false, const bool r = false) {
        Matrix6x2 JtT;
        JtT = L.m_Jt.transpose();
        Matrix6x2 JcT;
        JcT = L.m_Jc.transpose();
        m_Att += JtT * L.m_Jt;
        m_Atc += JtT * L.m_Jc;
        m_Acc += JcT * L.m_Jc;
        m_bt += JtT * L.m_e;
        m_bc += JcT * L.m_e;

        if (k) {
            Matrix6x2 JkT;
            JkT = L.m_Jk.transpose();
            m_Atk += JtT * L.m_Jk;
            m_Ack += JcT * L.m_Jk;
            m_Akk += JkT * L.m_Jk;

            if (r) {
                m_Akr += JkT * L.m_Jr;
            }

            m_bk += JkT * L.m_e;
        }

        if (r) {
            Matrix6x2 JrT;
            JrT = L.m_Jr.transpose();
            m_Atr += JtT * L.m_Jr;
            m_Acr += JcT * L.m_Jr;
            m_Arr += JrT * L.m_Jr;
            m_br += JrT * L.m_e;
        }
    }
    inline void Accumulate(const int ipt, const int ipc, MatrixX* A, VectorX* b,
                           const int ipk = -1, const int ipr = -1) const {
        if (ipt >= 0) {
            A->block<6, 6>(ipt, ipt) += m_Att;
            A->block<6, 6>(ipt, ipc) += m_Atc;
            b->block<6, 1>(ipt, 0) += m_bt;
        }

        A->block<6, 6>(ipc, ipc) += m_Acc;
        b->block<6, 1>(ipc, 0) += m_bc;

        if (ipk >= 0) {
            if (ipt >= 0) {
                A->block<6, 6>(ipt, ipk) += m_Atk;
            }

            A->block<6, 6>(ipc, ipk) += m_Ack;
            A->block<6, 6>(ipk, ipk) += m_Akk;
            b->block<6, 1>(ipk, 0) += m_bk;
        }

        if (ipr >= 0) {
            if (ipt >= 0) {
                A->block<6, 6>(ipt, ipr) += m_Atr;
            }

            A->block<6, 6>(ipc, ipr) += m_Acr;

            if (ipk >= 0) {
                A->block<6, 6>(ipk, ipr) += m_Akr;
            }

            A->block<6, 6>(ipr, ipr) += m_Arr;
            b->block<6, 1>(ipr, 0) += m_br;
        }
    }
public:
    Matrix6x6 m_Att;
    Matrix6x6 m_Atc;
    Matrix6x6 m_Atk;
    Matrix6x6 m_Atr;
    Matrix6x6 m_Acc;
    Matrix6x6 m_Ack;
    Matrix6x6 m_Acr;
    Matrix6x6 m_Akk;
    Matrix6x6 m_Akr;
    Matrix6x6 m_Arr;
    Vector6 m_bt;
    Vector6 m_bc;
    Vector6 m_bk;
    Vector6 m_br;
};

static void Rollback(const std::vector<Transformation>& TwtsPre,
                     const std::vector<Transformation>& TcwsPre,
                     const std::vector<Intrinsic>& KsPre,
                     const std::vector<Intrinsic>& KrsPre,
                     const std::vector<Transformation>& TrlsPre,
                     std::vector<Transformation>* TwtsCur,
                     std::vector<Transformation>* TcwsCur,
                     std::vector<Intrinsic>* KsCur = NULL,
                     std::vector<Intrinsic>* KrsCur = NULL,
                     std::vector<Transformation>* TrlsCur = NULL,
                     const bool k = false, const bool kr = false,
                     const std::vector<int>* iTargets = NULL,
                     const std::vector<int>* iCams = NULL) {
    const int nTargets = static_cast<int>(TwtsCur->size());

    for (int iTarget = 1; iTarget < nTargets; ++iTarget) {
        const int it = iTargets ? iTargets->at(iTarget) : iTarget;

        if (it != -1) {
            TwtsCur->at(iTarget) = TwtsPre[it];
        }
    }

    const int nCams = static_cast<int>(TcwsCur->size());

    for (int iCam = 0; iCam < nCams; ++iCam) {
        const int ic = iCams ? iCams->at(iCam) : iCam;

        if (ic != -1) {
            TcwsCur->at(iCam) = TcwsPre[ic];
        }
    }

    if (k) {
        for (int iCam = 0; iCam < nCams; ++iCam) {
            const int ic = iCams ? iCams->at(iCam) : iCam;

            if (ic != -1) {
                KsCur->at(iCam) = KsPre[ic];
            }
        }
    }

    if (kr) {
        for (int iCam = 0; iCam < nCams; ++iCam) {
            const int ic = iCams ? iCams->at(iCam) : iCam;

            if (ic == -1) {
                KrsCur->at(iCam) = KrsPre[ic];
                TrlsCur->at(iCam) = TrlsPre[ic];
            }
        }
    }
}

bool optimize(const std::vector<Vector3>& corners3D,
              const std::vector<Frame>& frms,
              std::vector<Transformation>* transsTargetToWorld,
              std::vector<Transformation>* transsWorldToCam,
              std::vector<Intrinsic>* intrinsics = NULL,
              std::vector<Intrinsic>* intrinsicsRight = NULL,
              std::vector<Transformation>* transsLeftToRight = NULL,
              const int targetCnt = -1, const int camCnt = -1,
              const std::vector<int>* iTargets = NULL,
              const std::vector<int>* iCams = NULL) {
    //#ifdef CFG_DEBUG
#if 0
    //if (camCnt < static_cast<int>(transsWorldToCam->size())) {
    //  return false;
    //}
    PrintResult(*transsTargetToWorld, *transsWorldToCam);
#endif
    const int nTargets = static_cast<int>(transsTargetToWorld->size());
    const int nCams = static_cast<int>(transsWorldToCam->size());
    const int Nt = targetCnt == -1 ? nTargets : targetCnt;
    const int Nc = camCnt == -1 ? nCams : camCnt;
    const int Npt = (Nt - 1) * 6, Npc = Nc * 6;
    //const bool k = intrinsics != NULL, kr = intrinsicsRight != NULL;
    const bool k = intrinsics &&  Nc == nCams/* && Nt == nTargets*/;
    //const bool k = false;
    const bool kr = k && intrinsicsRight;
    const int Npkl = k ? Nc * 6 : 0;
    const int Npkr = kr ? Nc * 6 : 0;
    const int Npk = Npkl + Npkr, Npr = Npkr;
    const int Np = Npt + Npc + Npk + Npr;
    MatrixX A;
    VectorX b, x;
    Factor F;
    Linearization L;
    A.resize(Np, Np);
    b.resize(Np, 1);
    const double focal = intrinsics ? 1.0 : BA_DEFAULT_FOCAL;
    double ePre = DBL_MAX;
    std::vector<Transformation> TwtsPre(Nt), TcwsPre(Nc), TrlsPre(kr ? Nc : 0);
    std::vector<Intrinsic> KsPre(k ? Nc : 0), KrsPre(kr ? Nc : 0);

    for (int iIter = 0; iIter < BA_ITERATIONS; ++iIter) {
        A.setZero();
        b.setZero();
        double e2 = 0.0;
        int cnt = 0;
        const int nFrms = static_cast<int>(frms.size());

        for (int iFrm = 0; iFrm < nFrms; ++iFrm) {
            const Frame& frm = frms[iFrm];

            if ((iTargets && iTargets->at(frm.m_iTarget) == -1) ||
                    (iCams && iCams->at(frm.m_iCam) == -1)) {
                continue;
            }

            //#ifdef CFG_DEBUG
#if 0
            double _e2 = 0.0;
            int _cnt = 0;
#endif
            const Transformation& Twt = transsTargetToWorld->at(frm.m_iTarget);
            const Transformation& Tlw = transsWorldToCam->at(frm.m_iCam);
            const Transformation Tlt = Tlw * Twt;
            const Intrinsic* K = intrinsics ? &intrinsics->at(frm.m_iCam) : NULL;
            F.Initialize();
            const int nCorners = static_cast<int>(corners3D.size());
#ifdef CFG_DEBUG
            assert(static_cast<int>(frm.m_corners2D.size()) == nCorners);
#endif

            for (int i = 0; i < nCorners; ++i) {
                if (frm.m_corners2D[i].invalid()) {
                    continue;
                }

                if (!L.Set(Twt, Tlw, Tlt, corners3D[i], frm.m_corners2D[i], K, k)) {
                    Rollback(TwtsPre, TcwsPre, KsPre, KrsPre, TrlsPre,
                             transsTargetToWorld, transsWorldToCam,
                             intrinsics, intrinsicsRight, transsLeftToRight,
                             k, kr, iTargets, iCams);
                    return false;
                }

#ifdef DEBUG_CHECK_JACOBIAN
                L.Check(Twt, Tlw, corners3D[i], K, k);
#endif
                F.Accumulate(L, k);
                e2 += L.m_e.squaredNorm();
                ++cnt;
                //#ifdef CFG_DEBUG
#if 0
                _e2 += L.m_e.squaredNorm();
                ++_cnt;
#endif
            }

            const int it = iTargets ? iTargets->at(frm.m_iTarget) : frm.m_iTarget;
            const int ic = iCams ? iCams->at(frm.m_iCam) : frm.m_iCam;
            const int ipt = it > 0 ? (it - 1) * 6 : -1, ipc = Npt + ic * 6;
            const int ipk = k ? (Npt + Npc + ic * 6) : -1;
            F.Accumulate(ipt, ipc, &A, &b, ipk);
            //#ifdef CFG_DEBUG
#if 0
            printf("Frame = %d, target = %d, camera = %d, error = %f\n",
                   iFrm, frm.m_iTarget, frm.m_iCam, std::sqrt(_e2 / _cnt) * focal);
#endif

            if (!transsLeftToRight) {
                continue;
            }

            const Transformation& Trl = transsLeftToRight->at(frm.m_iCam);
            const Transformation Trw = Trl * Tlw;
            const Transformation Trt = Trw * Twt;
            const Intrinsic* Kr = intrinsicsRight ? &intrinsicsRight->at(frm.m_iCam) : NULL;
            F.Initialize();
#ifdef CFG_DEBUG
            assert(static_cast<int>(frm.m_corners2DRight.size()) == nCorners);
#endif

            for (int i = 0; i < nCorners; ++i) {
                if (frm.m_corners2DRight[i].invalid()) {
                    continue;
                }

                if (!L.Set(Twt, Trw, Trt, corners3D[i], frm.m_corners2DRight[i], Kr, kr, &Trl, kr)) {
                    Rollback(TwtsPre, TcwsPre, KsPre, KrsPre, TrlsPre,
                             transsTargetToWorld, transsWorldToCam,
                             intrinsics, intrinsicsRight, transsLeftToRight,
                             k, kr, iTargets, iCams);
                    return false;
                }

#ifdef DEBUG_CHECK_JACOBIAN
                L.Check(Twt, Tlw, corners3D[i], Kr, kr, &Trl, kr);
#endif
                F.Accumulate(L, kr, kr);
                e2 += L.m_e.squaredNorm();
                ++cnt;
                //#ifdef CFG_DEBUG
#if 0
                _e2 += L.m_e.squaredNorm();
                ++_cnt;
#endif
            }

            //#ifdef CFG_DEBUG
#if 0
            printf("Frame = %d, target = %d, camera = %d, error = %f\n",
                   iFrm, frm.m_iTarget, frm.m_iCam, std::sqrt(_e2 / _cnt) * focal);
#endif
            const int ipkr = kr ? Npt + Npc + Npkl + ic * 6 : -1;
            const int ipr = kr ? Npt + Npc + Npk + ic * 6 : -1;
            F.Accumulate(ipt, ipc, &A, &b, ipkr, ipr);
        }

        const double eCur = std::sqrt(e2 / cnt) * focal;
#ifdef CFG_VERBOSE

        if (iIter == 0) {
            printf("-------------------------------------------\n");
        }

        printf("%d: error = %f\n", iIter, eCur);
#endif

        if (ePre - eCur < BA_CONVERGE_ERROR) {
            if (ePre < eCur) {
                Rollback(TwtsPre, TcwsPre, KsPre, KrsPre, TrlsPre,
                         transsTargetToWorld, transsWorldToCam,
                         intrinsics, intrinsicsRight, transsLeftToRight,
                         k, kr, iTargets, iCams);
            }

            break;
        }

        ePre = eCur;
        A.SetLowerFromUpper();
        //const int rank = Eigen::ColPivHouseholderQR<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic,
        //  Eigen::RowMajor> >(A).rank();
        //if (rank < Np) {
        //  return false;
        //}
        x = A.ldlt().solve(b);
#if 0

        for (int i = 0; i < Np; ++i) {
            for (int j = 0; j < Np; ++j) {
                printf("%e ", A(i, j));
            }

            printf("%e\n", b(i, 0));
        }

#endif

        for (int iTarget = 1; iTarget < nTargets; ++iTarget) {
            const int it = iTargets ? iTargets->at(iTarget) : iTarget;

            if (it == -1) {
                continue;
            }

            const int ipt = (it - 1) * 6;
            Vector6 xt;
            xt = x.block<6, 1>(ipt, 0);
            Transformation& Twt = transsTargetToWorld->at(iTarget);
            TwtsPre[it] = Twt;
            Twt += xt;
        }

        for (int iCam = 0; iCam < nCams; ++iCam) {
            const int ic = iCams ? iCams->at(iCam) : iCam;

            if (ic == -1) {
                continue;
            }

            const int ipc = Npt + ic * 6;
            Vector6 xc;
            xc = x.block<6, 1>(ipc, 0);
            Transformation& Tcw = transsWorldToCam->at(iCam);
            TcwsPre[ic] = Tcw;
            Tcw += xc;
        }

        if (k) {
            for (int iCam = 0; iCam < nCams; ++iCam) {
                const int ic = iCams ? iCams->at(iCam) : iCam;

                if (ic == -1) {
                    continue;
                }

                const int ipk = Npt + Npc + ic * 6;
                Vector6 xk;
                xk = x.block<6, 1>(ipk, 0);
                Intrinsic& K = intrinsics->at(iCam);
                KsPre[ic] = K;
                K += xk;
            }
        }

        if (kr) {
            for (int iCam = 0; iCam < nCams; ++iCam) {
                const int ic = iCams ? iCams->at(iCam) : iCam;

                if (ic == -1) {
                    continue;
                }

                const int ipk = Npt + Npc + Npkl + ic * 6;
                Vector6 xk;
                xk = x.block<6, 1>(ipk, 0);
                Intrinsic& K = intrinsicsRight->at(iCam);
                KrsPre[ic] = K;
                K += xk;
                const int ipr = Npt + Npc + Npk + ic * 6;
                Vector6 xr;
                xr = x.block<6, 1>(ipr, 0);
                Transformation& Trl = transsLeftToRight->at(iCam);
                TrlsPre[ic] = Trl;
                Trl += xr;
            }
        }
    }

    return true;
}

bool run(const int nTargets, const int nCams, const std::vector<Vector3>& corners3D,
         const std::vector<Frame>& frms, std::vector<Transformation>* transsTargetToWorld,
         std::vector<Transformation>* transsWorldToCam, std::vector<Intrinsic>* intrinsics,
         std::vector<Intrinsic>* intrinsicsRight,
         std::vector<Transformation>* transsLeftToRight) {
#ifdef CFG_DEBUG

    if (!intrinsics) {
        assert(intrinsicsRight == NULL);
    }

    if (intrinsicsRight) {
        assert(transsLeftToRight != NULL);
    }

    const int nFrms = static_cast<int>(frms.size()), nCorners = static_cast<int>(corners3D.size());

    for (int iFrm = 0; iFrm < nFrms; ++iFrm) {
        const Frame& frm = frms[iFrm];
        assert(static_cast<int>(frm.m_corners2D.size()) == nCorners);

        if (transsLeftToRight) {
            assert(static_cast<int>(frm.m_corners2DRight.size()) == nCorners);
        } else {
            assert(frm.m_corners2DRight.empty());
        }
    }

#endif
    transsTargetToWorld->resize(nTargets);
    transsWorldToCam->resize(nCams);
#if SFM_INCREMENTAL == 0
    //std::vector<int> iTargets(nTargets, -1), iCams(nCams, -1);
    //for (int iTarget = 0; iTarget < nTargets; ++iTarget) {
    //  iTargets[iTarget] = iTarget;
    //}
    //for (int iCam = 0; iCam < nCams; ++iCam) {
    //  iCams[iCam] = iCam;
    //}
    //const int targetCnt = nTargets;
    //const int camCnt = nCams;
    //iTargets[0] = 0;
    //iCams[0] = 0;
    //iCams[1] = 1;
    //const int targetCnt = 1;
    //const int camCnt = 2;
    return Initialize(frms, transsTargetToWorld, transsWorldToCam) &&
           Optimize(corners3D, frms, transsTargetToWorld, transsWorldToCam,
                    intrinsics, intrinsicsRight, transsLeftToRight/*, targetCnt, camCnt, &iTargets, &iCams*/);
#else
    std::vector<bool> targetMarks1(nTargets, false), camMarks1(nCams, false);
    std::vector<bool> targetMarks2(nTargets, false), camMarks2(nCams, false);
    std::vector<int> iTargets(nTargets, -1), iCams(nCams, -1);
    transsTargetToWorld->at(0).set_identity();
    targetMarks1[0] = targetMarks2[0] = true;
    iTargets[0] = 0;
    int targetCnt = 1;
    int camCnt = 0;

    while (1) {
#ifdef CFG_VERBOSE
        printf("-------------------------------------------\n");
#endif
        const int targetCntPre = targetCnt, camCntPre = camCnt;
        const int nFrms = static_cast<int>(frms.size());

        for (int iFrm = 0; iFrm < nFrms; ++iFrm) {
            const Frame& frm = frms[iFrm];

            if (frm.m_transTargetToCam.invalid() ||
                    CountValidCorners2D(frm.m_corners2D) < SFM_INCREMENTAL_MIN_CORNERS) {
                continue;
            } else if (targetMarks1[frm.m_iTarget] && !camMarks2[frm.m_iCam]) {
                transsWorldToCam->at(frm.m_iCam) = frm.m_transTargetToCam *
                                                   transsTargetToWorld->at(frm.m_iTarget).get_inverse();
                camMarks2[frm.m_iCam] = true;
                iCams[frm.m_iCam] = camCnt++;
#ifdef CFG_VERBOSE
                printf("Frame %d: target %d --> camera %d\n", iFrm, frm.m_iTarget, frm.m_iCam);
#endif
            } else if (camMarks1[frm.m_iCam] && !targetMarks2[frm.m_iTarget]) {
                transsTargetToWorld->at(frm.m_iTarget) = transsWorldToCam->at(frm.m_iCam).get_inverse() *
                        frm.m_transTargetToCam;
                targetMarks2[frm.m_iTarget] = true;
                iTargets[frm.m_iTarget] = targetCnt++;
#ifdef CFG_VERBOSE
                printf("Frame %d: target %d <-- camera %d\n", iFrm, frm.m_iTarget, frm.m_iCam);
#endif
            }
        }

        if (targetCnt == targetCntPre && camCnt == camCntPre) {
            break;
        }

        targetMarks1 = targetMarks2;
        camMarks1 = camMarks2;
        optimize(corners3D, frms, transsTargetToWorld, transsWorldToCam,
                 intrinsics, intrinsicsRight, transsLeftToRight,
                 targetCnt, camCnt, &iTargets, &iCams);
    }

    const bool scc = targetCnt == nTargets && camCnt == nCams;

    if (!scc) {
        for (int iTarget = 0; iTarget < nTargets; ++iTarget) {
            if (!targetMarks1[iTarget]) {
                transsTargetToWorld->at(iTarget).invalidate();
            }
        }

        for (int iCam = 0; iCam < nCams; ++iCam) {
            if (!camMarks1[iCam]) {
                transsWorldToCam->at(iCam).invalidate();
            }
        }

#ifdef CFG_VERBOSE
        printf("-------------------------------------------\n");
        printf("Isolated targets: {");

        for (int iTarget = 0; iTarget < nTargets; ++iTarget) {
            if (!targetMarks1[iTarget]) {
                printf(" %d", iTarget);
            }
        }

        printf(" }\n");
        printf("Isolated cameras: {");

        for (int iCam = 0; iCam < nCams; ++iCam) {
            if (!camMarks1[iCam]) {
                printf(" %d", iCam);
            }
        }

        printf(" }\n");
#endif
    }

    return scc;
#endif
}

void print_error(const std::vector<Vector3>& corners3D, const std::vector<Vector2>& corners2D,
                 const int iFrm, const int iTarget, const int iCam,
                 const Transformation& transTargetToCam, const Intrinsic* intrinsic = NULL,
                 const Transformation* transsLeftToRight = NULL) {
    const int nCorners = static_cast<int>(corners3D.size());
#ifdef CFG_DEBUG
    assert(static_cast<int>(corners2D.size()) == nCorners);
#endif
    double e2 = 0.0;
    int cnt = 0;
    const Transformation Tct = transsLeftToRight ? *transsLeftToRight * transTargetToCam
                               : transTargetToCam;

    for (int i = 0; i < nCorners; ++i) {
        if (corners2D[i].invalid()) {
            continue;
        }

        const Vector3 X = Tct * corners3D[i];
        const Vector2 x = X.project(intrinsic);
        Vector2 e;
        e = x - corners2D[i];
        e2 += e.squaredNorm();
        ++cnt;
    }

    if (cnt == 0) {
        return;
    }

    const double focal = intrinsic ? 1.0 : BA_DEFAULT_FOCAL;
    const double e = std::sqrt(e2 / cnt) * focal;
    printf("Frame = %d, target = %d, camera = %d, corners = %d, error = %f",
           iFrm, iTarget, iCam, cnt, e);

    if (transsLeftToRight) {
        printf(", right");
    }

    printf("\n");
}

void print_error(const std::vector<Vector3>& corners3D,
                 const std::vector<Frame>& frms,
                 const std::vector<Intrinsic>* intrinsics,
                 const std::vector<Intrinsic>* intrinsicsRight,
                 const std::vector<Transformation>* transsLeftToRight) {
    printf("-------------------------------------------\n");
    const int nFrms = static_cast<int>(frms.size());

    for (int iFrm = 0; iFrm < nFrms; ++iFrm) {
        const Frame& frm = frms[iFrm];
        print_error(corners3D, frm.m_corners2D, iFrm, frm.m_iTarget, frm.m_iCam,
                    frm.m_transTargetToCam, intrinsics ? &intrinsics->at(frm.m_iCam) : NULL);

        if (!transsLeftToRight) {
            continue;
        }

        print_error(corners3D, frm.m_corners2DRight, iFrm, frm.m_iTarget, frm.m_iCam,
                    frm.m_transTargetToCam, intrinsicsRight ? &intrinsicsRight->at(frm.m_iCam) : NULL,
                    transsLeftToRight ? &transsLeftToRight->at(frm.m_iCam) : NULL);
    }
}

void print_error(const std::vector<Vector3>& corners3D,
                 const std::vector<Frame>& frms,
                 const std::vector<Transformation>& transsTargetToWorld,
                 const std::vector<Transformation>& transsWorldToCam,
                 const std::vector<Intrinsic>* intrinsics,
                 const std::vector<Intrinsic>* intrinsicsRight,
                 const std::vector<Transformation>* transsLeftToRight) {
    printf("-------------------------------------------\n");
    const int nFrms = static_cast<int>(frms.size());

    for (int iFrm = 0; iFrm < nFrms; ++iFrm) {
        const Frame& frm = frms[iFrm];
        const Transformation& Twt = transsTargetToWorld[frm.m_iTarget];
        const Transformation& Tcw = transsWorldToCam[frm.m_iCam];
        const Transformation Tct = Tcw * Twt;
        print_error(corners3D, frm.m_corners2D, iFrm, frm.m_iTarget, frm.m_iCam, Tct,
                    intrinsics ? &intrinsics->at(frm.m_iCam) : NULL);

        if (!transsLeftToRight) {
            continue;
        }

        print_error(corners3D, frm.m_corners2DRight, iFrm, frm.m_iTarget, frm.m_iCam, Tct,
                    intrinsicsRight ? &intrinsicsRight->at(frm.m_iCam) : NULL,
                    &transsLeftToRight->at(frm.m_iCam));
    }
}

void print_result(const std::vector<Transformation>& transsTargetToWorld,
                  const std::vector<Transformation>& transsWorldToCam) {
    printf("-------------------------------------------\n");
    const int nTargets = static_cast<int>(transsTargetToWorld.size());

    for (int iTarget = 0; iTarget < nTargets; ++iTarget) {
        const Transformation& T = transsTargetToWorld[iTarget];

        if (T.invalid()) {
            continue;
        }

        //printf("Target %d\n", iTarget);
        //T.Print();
        Vector3 rz;
        rz = T.m_rotation.block<3, 1>(0, 2);
        const Vector3& p = T.m_translation;
        printf("Target %d\tp = (%f %f %f)\trz = (%f %f %f)\n", iTarget, p.x(), p.y(), p.z(), rz.x(), rz.y(),
               rz.z());
    }

    printf("-------------------------------------------\n");
    const int nCams = static_cast<int>(transsWorldToCam.size());

    for (int iCam = 0; iCam < nCams; ++iCam) {
        const Transformation& T = transsWorldToCam[iCam];

        if (T.invalid()) {
            continue;
        }

        //printf("Camera %d\n", iCam);
        //T.Print();
        Vector3 p;
        p = -T.m_rotation.transpose() * T.m_translation;
        printf("Camera %d\tp = (%f %f %f)\n", iCam, p.x(), p.y(), p.z());
    }
}

void print_result(const std::vector<Intrinsic>* intrinsics,
                  const std::vector<Intrinsic>* intrinsicsRight,
                  const std::vector<Transformation>* transsLeftToRight) {
    if (!intrinsics && !intrinsicsRight && !transsLeftToRight) {
        return;
    }

    printf("-------------------------------------------\n");
    const int nCams = intrinsics ? static_cast<int>(intrinsics->size())
                      : (intrinsicsRight ? static_cast<int>(intrinsicsRight->size())
                         : static_cast<int>(transsLeftToRight->size()));

    for (int iCam = 0; iCam < nCams; ++iCam) {
        printf("Camera %d\n", iCam);

        if (intrinsicsRight || transsLeftToRight) {
            printf("\n");
        } else {
            printf("\t");
        }

        if (intrinsics) {
            intrinsics->at(iCam).print();
        }

        if (intrinsicsRight) {
            intrinsicsRight->at(iCam).print();
        }

        if (transsLeftToRight) {
            transsLeftToRight->at(iCam).print();
        }
    }
}

void save(FILE* fp, const std::vector<Vector2>& xs) {
    const int N = static_cast<int>(xs.size());
    fprintf(fp, "%d\n", N);

    for (int i = 0; i < N; ++i) {
        const Vector2& x = xs[i];
        fprintf(fp, "%le %le\n", x.x(), x.y());
    }
}
int load(FILE* fp, std::vector<Vector2>* xs) {
    int N = 0;
    int cnt = 0;
    fscanf(fp, "%d", &N);
    xs->resize(N);
    const double xMax = 1.0e10;

    for (int i = 0; i < N; ++i) {
        Vector2& x = xs->at(i);
        fscanf(fp, "%le %le", &x.x(), &x.y());

        if (x.x() > xMax) {
            x.invalidate();
        } else {
            ++cnt;
        }
    }

    return cnt;
}

void save(FILE* fp, const std::vector<Intrinsic>& Ks) {
    const int N = static_cast<int>(Ks.size());
    fprintf(fp, "%d\n", N);

    for (int i = 0; i < N; ++i) {
        Ks[i].save(fp);
    }
}
void load(FILE* fp, std::vector<Intrinsic>* Ks) {
    int N = 0;
    fscanf(fp, "%d", &N);
    Ks->resize(N);

    for (int i = 0; i < N; ++i) {
        Ks->at(i).load(fp);
    }
}

void save(FILE* fp, const std::vector<Transformation>& Ts) {
    const int N = static_cast<int>(Ts.size());
    fprintf(fp, "%d\n", N);

    for (int i = 0; i < N; ++i) {
        Ts[i].save(fp);
    }
}
void load(FILE* fp, std::vector<Transformation>* Ts) {
    int N = 0;
    fscanf(fp, "%d", &N);
    Ts->resize(N);

    for (int i = 0; i < N; ++i) {
        Ts->at(i).load(fp);
    }
}

bool save(const char* fileName, const int nTargets, const int nCams,
          const std::vector<Vector3>& corners3D,
          const std::vector<Frame>& frms,
          const std::vector<Intrinsic>& intrinsics,
          const std::vector<Intrinsic>& intrinsicsRight,
          const std::vector<Transformation>& transsLeftToRight) {
    FILE* fp = fopen(fileName, "w");

    if (!fp) {
        return false;
    }

    fprintf(fp, "%d %d\n", nTargets, nCams);
    const int nCorners = static_cast<int>(corners3D.size());
    fprintf(fp, "%d\n", nCorners);

    for (int i = 0; i < nCorners; ++i) {
        const Vector3& X = corners3D[i];
        fprintf(fp, "%le %le %le\n", X.x(), X.y(), X.z());
    }

    const int nFrms = static_cast<int>(frms.size());
    fprintf(fp, "%d\n", nFrms);

    for (int iFrm = 0; iFrm < nFrms; ++iFrm) {
        const Frame& frm = frms[iFrm];
        fprintf(fp, "%d %d\n", frm.m_iTarget, frm.m_iCam);
        frm.m_transTargetToCam.save(fp);
#ifdef CFG_DEBUG
        assert(static_cast<int>(frm.m_corners2D.size()) == nCorners);
#endif
        save(fp, frm.m_corners2D);
#ifdef CFG_DEBUG
        assert(frm.m_corners2DRight.empty() || static_cast<int>(frm.m_corners2DRight.size()) == nCorners);
#endif
        save(fp, frm.m_corners2DRight);
    }

    save(fp, intrinsics);
    save(fp, intrinsicsRight);
    save(fp, transsLeftToRight);
    fclose(fp);
#ifdef CFG_VERBOSE
    printf("Saved \'%s\'\n", fileName);
#endif
    return true;
}


bool load(const char* fileName, int* nTargets, int* nCams,
          std::vector<Vector3>* corners3D,
          std::vector<Frame>* frms,
          std::vector<Intrinsic>* intrinsics,
          std::vector<Intrinsic>* intrinsicsRight,
          std::vector<Transformation>* transsLeftToRight) {
    FILE* fp = fopen(fileName, "r");

    if (!fp) {
        return false;
    }

    fscanf(fp, "%d %d", nTargets, nCams);
    int nCorners = 0;
    fscanf(fp, "%d", &nCorners);
    corners3D->resize(nCorners);

    for (int i = 0; i < nCorners; ++i) {
        Vector3& X = corners3D->at(i);
        fscanf(fp, "%le %le %le", &X.x(), &X.y(), &X.z());
    }

    int nFrms = 0;
    fscanf(fp, "%d", &nFrms);
    frms->resize(nFrms);

    for (int iFrm = 0; iFrm < nFrms; ++iFrm) {
        Frame& frm = frms->at(iFrm);
        fscanf(fp, "%d %d", &frm.m_iTarget, &frm.m_iCam);
        frm.m_transTargetToCam.load(fp);
        //frm.m_transTargetToCam = frm.m_transTargetToCam.GetInverse();
        frm.m_corners2D.resize(nCorners);
        const int nCorners2D = load(fp, &frm.m_corners2D);
        const int nCorners2DRight = load(fp, &frm.m_corners2DRight);
#ifdef DEBUG_REMOVE_USELESS_FRAME

        if (nCorners2D + nCorners2DRight == 0) {
            frm.m_corners2D.resize(0);
            frm.m_corners2DRight.resize(0);
        }

#endif
    }

#ifdef DEBUG_REMOVE_USELESS_FRAME
    int iFrm, jFrm;

    for (iFrm = jFrm = 0; iFrm < nFrms; ++iFrm) {
        const Frame& frm = frms->at(iFrm);

        if (frm.m_corners2D.empty() && frm.m_corners2DRight.empty()) {
            continue;
        } else if (jFrm != iFrm) {
            frms->at(jFrm) = frm;
        }

        ++jFrm;
    }

    frms->resize(jFrm);
#endif
    load(fp, intrinsics);
    load(fp, intrinsicsRight);
    load(fp, transsLeftToRight);
    fclose(fp);
#ifdef CFG_VERBOSE
    printf("Loaded \'%s\'\n", fileName);
#endif
    return true;
}

bool save(const char* fileName, const std::vector<Transformation>& transs) {
    FILE* fp = fopen(fileName, "w");

    if (!fp) {
        return false;
    }

    save(fp, transs);
    fclose(fp);
    //#ifdef CFG_VERBOSE
    printf("Saved \'%s\'\n", fileName);
    //#endif
    return true;
}

bool load(const char* fileName, std::vector<Transformation>* transs) {
    FILE* fp = fopen(fileName, "r");

    if (!fp) {
        return false;
    }

    load(fp, transs);
    fclose(fp);
#ifdef CFG_VERBOSE
    printf("Loaded \'%s\'\n", fileName);
#endif
    return true;
}
}


