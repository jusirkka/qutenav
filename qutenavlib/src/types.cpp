/* -*- coding: utf-8-unix -*-
 *
 * File: src/types.cpp
 *
 * Copyright (C) 2021 Jukka Sirkka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "types.h"
#include <cmath>
#include <QRegularExpression>
#include "Geodesic.hpp"
#include <QDebug>
#include <algorithm>

double Angle::degrees() const {
  return radians * DEGS_PER_RAD;
}

Angle Angle::fromRadians(double r) {
  return Angle(r);
}

Angle Angle::fromDegrees(double d) {
  return Angle(d * RADS_PER_DEG);
}

Angle Angle::ASin(double s) {
  return Angle(asin(s));
}

Angle Angle::ATan2(double y, double x) {
  return Angle(atan2(y, x));
}

Angle operator+ (const Angle& a, const Angle& b) {
  return Angle(a.radians + b.radians);
}

Angle operator- (const Angle& a, const Angle& b) {
  return Angle(a.radians - b.radians);
}

Angle operator- (const Angle& a) {
  return Angle(- a.radians);
}

Angle::Angle(double r): m_valid(true) {
  // normalize - valid implies normalized
  while (r > PI) {
    r -= 2 * PI;
  }
  while (r < -PI) {
    r += 2 * PI;
  }
  radians = r;
}



QString Angle::printAsLongitude() const {
  const double v = std::abs(degrees());
  quint32 a = static_cast<int>(v);
  const double b = v - a;

  if (a > 0) {
    // manipulate b -> b' (int)
    quint32 b1 = std::round(60 * b);
    if (b1 == 60) {
      b1 = 0;
      a += 1;
    }
    // print a + b'
    const QString r = QString::number(a) + "°%1'" + (radians >= 0 ? "E" : "W");
    return r.arg(b1, 2, 10, QChar('0'));
  }
  // manipulate b -> b' (int)
  quint32 b1 = std::round(60 * b);
  QString r = QString("%1'") +  (radians >= 0 ? "E" : "W");
  if (b1 == 60) {
    b1 = 0;
    r = "1°" + r;
  }
  // print b'
  return r.arg(b1, 2, 10, QChar('0'));
}

WGS84Point WGS84Point::fromLL(double lng, double lat) {
  return WGS84Point(lng, lat);
}

WGS84Point WGS84Point::fromLLRadians(double lng, double lat) {
  return WGS84Point(lng * DEGS_PER_RAD, lat * DEGS_PER_RAD);
}

WGS84Point WGS84Point::parseISO6709(const QString& loc) {
  // +27.5916+086.5640+8850CRSWGS_84/
  QRegularExpression coord("([+-]\\d+(?:\\.\\d+)?)(.*)");
  QVector<double> cs;
  QRegularExpressionMatch m;
  QString rest;
  for (m = coord.match(loc); m.hasMatch(); m = coord.match(m.captured(2))) {
    cs.append(m.captured(1).toDouble());
    rest = m.captured(2);
  }
  if (cs.length() < 2) {
    return WGS84Point();
  }

  double lat = cs[0];
  double lng = cs[1];

  QString datum("WGS_84");
  QRegularExpression datumRe("CRS([^/]+)(/.*)");
  m = datumRe.match(rest);
  if (m.hasMatch()) {
    datum = m.captured(1);
    rest = m.captured(2);
  }

  if (rest != "/" || datum != "WGS_84") {
    return WGS84Point();
  }

  return WGS84Point(lng, lat);
}

static double sexas(double r) {
    r = std::abs(r);
    return  60. * (r - std::floor(r));
}


QString WGS84Point::print(Units units, quint8 prec) const {
  //% "N/A"
  if (!m_Valid) return QT_TRID_NOOP("qutenav-not-applicable");

  const QChar z('0');

  switch (units) {
  case Units::DegMinSec: {
    // 50°40'46“N 024°48'26“E
    const QString s("%1°%2‘%3“%4");
    QString r;
    r += s.arg(degreesLat(), 2, 10, z).arg(minutesLat(), 2, 10, z)
            .arg(secondsLat(), 2, 10, z).arg(northern() ? "N" : "S");
    r += " ";
    r += s.arg(degreesLng(), 3, 10, z).arg(minutesLng(), 2, 10, z)
         .arg(secondsLng(), 2, 10, z).arg(eastern() ? "E" : "W");

    return r;
  }
  case Units::DegMin: {
    // 50°40.7667'N 024°48.4333'E
    const QString s("%1°%2‘%3");
    QString r;
    const int w = prec == 0 ? 2 : 3 + prec;
    r += s.arg(degreesLat(), 2, 10, z).arg(sexas(m_Latitude), w, 'f', prec, z)
        .arg(northern() ? "N" : "S");
    r += " ";
    r += s.arg(degreesLng(), 3, 10, z).arg(sexas(m_Longitude), w, 'f', prec, z)
        .arg(eastern() ? "E" : "W");

    return r;
  }
  case Units::Deg: {
    // 50.679445° 024.807222°
    const QString s("%1%2°");
    QString r;
    r += s.arg(northern() ? "" : "-").arg(std::abs(m_Latitude), 5 + prec, 'f', 2 + prec, z);
    r += " ";
    r += s.arg(eastern() ? "" : "-").arg(std::abs(m_Longitude), 6 + prec, 'f', 2 + prec, z);

    return r;
  }
  default:
    //% "N/A"
    return QT_TRID_NOOP("qutenav-not-applicable");
  }

}

QString WGS84Point::toISO6709() const {
  //% "N/A"
  if (!m_Valid) return QT_TRID_NOOP("qutenav-not-applicable");
  // +27.5916+086.5640CRSWGS_84/
  const QChar z('0');
  const QString s("%1%2%3%4CRSWGS_84/"); // sign lat, abs lat, sign lng, abs lng
  return s.arg(m_Latitude < 0 ? '-' : '+').arg(std::abs(m_Latitude), 0, 'f', 10, z)
      .arg(m_Longitude < 0 ? '-' : '+').arg(std::abs(m_Longitude), 0, 'f', 10, z);
}

double WGS84Point::radiansLng() const {
  return RADS_PER_DEG * m_Longitude;
}

double WGS84Point::radiansLat() const {
  return RADS_PER_DEG * m_Latitude;
}


ushort WGS84Point::degreesLat() const {
  return std::abs(m_Latitude);
}

ushort WGS84Point::degreesLng() const {
  return std::abs(m_Longitude);
}

ushort WGS84Point::minutesLat() const {
  return sexas(m_Latitude);
}

ushort WGS84Point::minutesLng() const {
  return sexas(m_Longitude);
}

ushort WGS84Point::secondsLat() const {
  return sexas(sexas(m_Latitude));
}

ushort WGS84Point::secondsLng() const {
  return sexas(sexas(m_Longitude));
}

double WGS84Point::normalizeAngle(double a) {
  while (a > 180) {
    a -= 360;
  }
  while (a < -180) {
    a += 360;
  }
  return a;
}

WGS84Point::WGS84Point(double lng, double lat)
  : m_Valid(!std::isnan(lng) && !std::isnan(lat)) {

  if (!m_Valid) return;

  // normalize angles here - valid implies normalized
  lng = normalizeAngle(lng);
  lat = normalizeAngle(lat);
  if (lat > 90) {
    lat = 90 - lat;
  } else if (lat < - 90) {
    lat = - 90 - lat;
  }
  m_Longitude = lng;
  m_Latitude = lat;
}

double WGS84Point::lng(const WGS84Point& ref) const {
  if (m_Longitude < ref.lng()) {
    return m_Longitude + 360.;
  }
  return m_Longitude;
}

bool WGS84Point::containedBy(const WGS84Point &sw, const WGS84Point &ne) const {
  Q_ASSERT(ne.lat() >= sw.lat());
  if (m_Latitude < sw.lat() || m_Latitude > ne.lat()) return false;

  double a = ne.lng() - sw.lng();
  while (a < 0) a += 360.;

  double b = m_Longitude - sw.lng();
  while (b < 0) b += 360.;

  return b <= a;
}


bool operator!= (const WGS84Point& a, const WGS84Point& b) {
  if (a.m_Valid != b.m_Valid) return true;
  if (!a.m_Valid) return false; // both invalid

  const WGS84Point d(a.m_Longitude - b.m_Longitude, a.m_Latitude - b.m_Latitude);

  if (d.m_Longitude > .1 || d.m_Latitude > .1) return true;

  const double c = cos(a.radiansLat());
  const double dlat2 = d.radiansLat() * d.radiansLat();
  const double dlng2 = d.radiansLng() * d.radiansLng();

  // within 20 meters?
  return sqrt(dlat2 + c * c * dlng2) * WGS84Point::semimajor_axis > WGS84Point::equality_radius;
}

bool operator== (const WGS84Point& a, const WGS84Point& b) {
  return !(a != b);
}

WGS84Bearing WGS84Bearing::fromMeters(double m, const Angle& a) {
  return WGS84Bearing(m, a);
}

WGS84Bearing WGS84Bearing::fromNM(double nm, const Angle& a) {
  return WGS84Bearing(nm * 1852., a);
}

WGS84Bearing::WGS84Bearing(double m, const Angle& a)
  : m_meters(m)
  , m_radians(a.radians)
  , m_valid(a.valid() && m > 0.)
{
  if (m_radians < 0.) m_radians += M_PI * 2;
}


WGS84Bearing operator- (const WGS84Bearing& b) {
  return WGS84Bearing(b.meters(), Angle::fromRadians(b.radians() + M_PI));
}

WGS84Point operator- (const WGS84Point& a, const WGS84Bearing& b) {
  return a + (-b);
}

WGS84Point operator+ (const WGS84Point& a, const WGS84Bearing& b) {
  double lng2;
  double lat2;
  GeographicLib::Geodesic::WGS84().Direct(a.lat(), a.lng(), b.degrees(), b.meters(),
                                          lat2, lng2);
  return WGS84Point::fromLL(lng2, lat2);
}

WGS84Bearing operator- (const WGS84Point& p2, const WGS84Point& p1) {
  double s12;
  double azi1;
  double azi2;
  GeographicLib::Geodesic::WGS84().Inverse(p1.lat(), p1.lng(), p2.lat(), p2.lng(),
                                           s12, azi1, azi2);
  return WGS84Bearing::fromMeters(s12, Angle::fromDegrees(azi1));
}

WGS84Bearing operator* (double s, const WGS84Bearing& b) {
  if (s < 0){
    return WGS84Bearing::fromMeters( - s * b.meters(), Angle::fromRadians(b.radians() + M_PI));
  }
  return WGS84Bearing::fromMeters(s * b.meters(), Angle::fromRadians(b.radians()));
}

QVector<QPointF> areaPath(const QSize& cs, qreal lw) {
  const QPointF ul(lw / 2, lw / 2);
  const QPointF lr(PickIconSize - lw / 2, PickIconSize - lw / 2);
  const QRectF r(ul, lr);

  QVector<QPointF> vs {
    r.topLeft(), r.topRight(),
    r.topRight(), r.bottomRight(),
    r.bottomRight(), r.bottomLeft(),
    r.bottomLeft(), r.topLeft()
  };

  const QPointF dp = .5 * QPointF(cs.width() - r.width(), cs.height() - r.height()) - r.topLeft();

  std::for_each(vs.begin(), vs.end(), [&dp] (QPointF& v) {
    v += dp;
  });

  return vs;
}

QVector<QPointF> linePath(const QSize& cs, qreal lw) {
  const QPointF ul(lw / 2, lw / 2);
  const QPointF lr(PickIconSize - lw / 2, PickIconSize - lw / 2);
  const QRectF r(ul, lr);

  //  const QPointF p1(r.left() + .5 * r.width(), r.top() + .15 * r.height());
  //  const QPointF p2(r.left() + .5 * r.width(), r.top() + .85 * r.height());

  QVector<QPointF> vs {
    r.topLeft(), r.bottomRight()
  };

  const QPointF dp = .5 * QPointF(cs.width() - r.width(), cs.height() - r.height()) - r.topLeft();

  std::for_each(vs.begin(), vs.end(), [&dp] (QPointF& v) {
    v += dp;
  });

  return vs;
}

