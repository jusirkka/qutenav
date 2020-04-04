#include "orthocam.h"

OrthoCam::OrthoCam(float /*wmm*/, float /*hmm*/) {}
void OrthoCam::pan(QVector2D /*dragStart*/, QVector2D /*dragAmount*/) {}
void OrthoCam::rotateEye(Angle /*tilt*/) {}
void OrthoCam::setScale(quint32 /*scale*/) {}
void OrthoCam::resize(float /*wmm*/, float /*hmm*/) {}
void OrthoCam::reset() {}
void OrthoCam::reset(WGS84Point /*eye*/, Angle /*tilt*/) {}
WGS84Point OrthoCam::eye() const {return WGS84Point();}
Angle OrthoCam::northAngle() const {return Angle();}
quint32 OrthoCam::maxScale() const {return 0;}
quint32 OrthoCam::minScale() const {return 0;}

/*

    float z = .7;
    m_projection.ortho(-a*z, a*z, -1*z, 1*z, 0, 2);
  }

*/
