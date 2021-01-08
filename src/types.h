#pragma once

#include <QString>
#include <cmath>
#include <QVector>
#include <QOpenGLFunctions>
#include <glm/vec2.hpp>
#include "platform.h"

class NotImplementedError {
public:
  NotImplementedError(QString msg): m_detail(std::move(msg)) {}
  const QString msg() const {return m_detail;}
private:
  QString m_detail;
};

class ChartFileError {
public:
  ChartFileError(QString msg): m_detail(std::move(msg)) {}
  const QString msg() const {return m_detail;}
private:
  QString m_detail;
};



class Angle {
public:

  friend Angle operator+ (const Angle& a, const Angle& b);
  friend Angle operator- (const Angle& a, const Angle& b);
  friend Angle operator- (const Angle& a);

  double degrees() const;
  static Angle fromRadians(double);
  static Angle fromDegrees(double);
  static Angle ASin(double);
  static Angle ATan2(double, double);

  bool valid() const {return m_valid;}
  bool isZero() const {return std::abs(radians) < 1.e-10;}
  double cos() const {return std::cos(radians);}
  double sin() const {return std::sin(radians);}

  Angle(const Angle& a): radians(a.radians), m_valid(a.m_valid) {}
  Angle& operator=(const Angle& a) {radians = a.radians; m_valid = a.m_valid; return *this;}
  Angle(): radians(0), m_valid(false) {}

  double radians;

private:

  constexpr static double DEGS_PER_RAD = 57.29577951308232087680;
  constexpr static double RADS_PER_DEG =  0.01745329251994329577;
  constexpr static double PI = 3.14159265358979323846;


  Angle(double r);

  bool m_valid;

};


Angle operator+ (const Angle& a, const Angle& b);
Angle operator- (const Angle& a, const Angle& b);
Angle operator- (const Angle& a);



class WGS84Bearing;

class WGS84Point {
public:

  enum class Units {Deg, DegMin, DegMinSec};

  static constexpr double semimajor_axis = 6378137.0;
  // closer than this points are considered equal
  static constexpr double equality_radius = 20.;

  friend WGS84Point operator+ (const WGS84Point& a, const WGS84Bearing& b);
  friend bool operator!= (const WGS84Point& a, const WGS84Point& b);

  static WGS84Point fromLL(double lng, double lat);
  static WGS84Point fromLLRadians(double lng, double lat);
  static WGS84Point parseISO6709(const QString& loc);

  WGS84Point(const WGS84Point& a): m_Longitude(a.m_Longitude), m_Latitude(a.m_Latitude), m_Valid(a.m_Valid) {}
  WGS84Point& operator=(const WGS84Point& a) {m_Longitude = a.m_Longitude; m_Latitude = a.m_Latitude; m_Valid = a.m_Valid; return *this;}
  WGS84Point(): m_Longitude(0), m_Latitude(0), m_Valid(false) {}

  QString print(Units units = Units::DegMin) const;
  QString toISO6709() const;
  double lng() const {return m_Longitude;}
  double lat() const {return m_Latitude;}

  double radiansLng() const;
  double radiansLat() const;

  bool northern() const {return m_Latitude >= 0;}
  bool eastern() const {return m_Longitude >= 0;}
  bool valid() const {return m_Valid;}

  ushort degreesLat() const;
  ushort degreesLng() const;
  ushort minutesLat() const;
  ushort minutesLng() const;
  ushort secondsLat() const;
  ushort secondsLng() const;

private:

  constexpr static double DEGS_PER_RAD = 57.29577951308232087680;
  constexpr static double RADS_PER_DEG =  0.01745329251994329577;

  static double normalizeAngle(double a);

  WGS84Point(double lng, double lat);

  double m_Longitude;
  double m_Latitude;
  bool m_Valid;

};

bool operator!= (const WGS84Point& a, const WGS84Point& b);
bool operator== (const WGS84Point& a, const WGS84Point& b);

inline uint qHash(const WGS84Point& a, uint seed) {
  return qHash(qMakePair(a.lng(), a.lat()), seed);
}


using WGS84PointVector = QVector<WGS84Point>;


class WGS84Bearing {
public:

  friend WGS84Bearing operator- (const WGS84Point& a, const WGS84Point& b);
  friend WGS84Bearing operator- (const WGS84Bearing& a);

  static WGS84Bearing fromMeters(double m, const Angle& a);
  static WGS84Bearing fromNM(double nm, const Angle& a);

  WGS84Bearing(const WGS84Bearing& a): m_meters(a.m_meters), m_radians(a.m_radians), m_valid(a.m_valid) {}
  WGS84Bearing& operator=(const WGS84Bearing& a) {m_meters = a.m_meters; m_radians = a.m_radians; m_valid = a.m_valid; return *this;}
  WGS84Bearing(): m_meters(0), m_radians(0), m_valid(false) {}

  double meters() const {return m_meters;}
  double radians() const {return m_radians;}
  double degrees() const {return m_radians * DEGS_PER_RAD;}

  bool valid() const {return m_valid;}

private:

  constexpr static double DEGS_PER_RAD = 57.29577951308232087680;

  WGS84Bearing(double m, const Angle& a);

  double m_meters;
  double m_radians;
  bool m_valid;

};

WGS84Point operator+ (const WGS84Point& a, const WGS84Bearing& b);
WGS84Point operator- (const WGS84Point& a, const WGS84Bearing& b);
WGS84Bearing operator- (const WGS84Point& a, const WGS84Point& b);
WGS84Bearing operator- (const WGS84Bearing& a);
WGS84Bearing operator* (double s, const WGS84Bearing& b);
WGS84Bearing operator* (const WGS84Bearing& b, double s);


class Extent {
public:

  Extent() = default;
  Extent(const Extent& e) = default;
  Extent& operator=(const Extent&) = default;

  Extent(const WGS84Point& sw,
         const WGS84Point& se,
         const WGS84Point& ne,
         const WGS84Point& nw)
    : m_wgs84Points()
  {
    m_wgs84Points << sw << se << ne << nw;
  }

  Extent(const WGS84Point& sw,
         const WGS84Point& ne)
    : m_wgs84Points()
  {
    WGS84Point se = WGS84Point::fromLL(ne.lng(), sw.lat());
    WGS84Point nw = WGS84Point::fromLL(sw.lng(), ne.lat());
    m_wgs84Points << sw << se << ne << nw;
  }

  const WGS84PointVector& corners() const {return m_wgs84Points;}
  WGS84Point sw() const {
    if (m_wgs84Points.size() < 1) return WGS84Point();
    return m_wgs84Points[0];
  }
  WGS84Point se() const {
    if (m_wgs84Points.size() < 2) return WGS84Point();
    return m_wgs84Points[1];
  }
  WGS84Point ne() const {
    if (m_wgs84Points.size() < 3) return WGS84Point();
    return m_wgs84Points[2];
  }
  WGS84Point nw() const {
    if (m_wgs84Points.size() < 4) return WGS84Point();
    return m_wgs84Points[3];
  }

private:

  WGS84PointVector m_wgs84Points;
};

template <typename Enumeration>
auto as_numeric(Enumeration const value) {
  return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

template <typename E, typename T = typename std::underlying_type<E>::type>
class OutOfRangeError {
public:
  OutOfRangeError(T v)
    : m_value(v) {}
  QString msg() const {return QString("Value %1 cannot be cast to %2")
        .arg(m_value).arg(typeid(E).name());}

private:
  T m_value;
};

template <typename E>
auto as_enum(typename std::underlying_type<E>::type value,
             const QVector<typename std::underlying_type<E>::type>& all_values) {
  if (!all_values.contains(value)) throw OutOfRangeError<E>(value);
  return static_cast<E>(value);
}

namespace TXT {

enum class HJust: quint8 {Centre = 1, Right = 2, Left = 3};
enum class VJust: quint8 {Bottom = 1, Centre = 2, Top = 3};
enum class Space: quint8 {Fit = 1, Standard = 2, Wrapped = 3};
enum class Weight: quint8 {Light = 4, Medium = 5, Bold = 6};

static const inline QVector<quint8> AllHjusts {1, 2, 3};
static const inline QVector<quint8> AllVjusts {1, 2, 3};
static const inline QVector<quint8> AllSpaces {1, 2, 3};
static const inline QVector<quint8> AllWeights {4, 5, 6};

}

namespace S52 {

enum class SymbolType: quint8 {Single = 1, LineStyle = 2, Pattern = 3};
static const inline QVector<quint8> AllSymbols {1, 2, 3};

enum class Alpha: quint8 {P0 = 0, P25 = 1, P50 = 2, P75 = 3, P100 = 4, Unset = 5};
static const inline QVector<quint8> AllAlphas {0, 1, 2, 3, 4, 5};

enum class LineType: uint {Solid = 0x3ffff, Dashed = 0x3ffc0, Dotted = 0x30c30};
static const inline QVector<uint> AllLineTypes {0x3ffff, 0x3ffc0, 0x30c30};

inline GLfloat LineWidthMM(GLfloat lw) {
  return lw * 0.3;
}

inline GLfloat LineWidthDots(GLfloat lw) {
  return lw * 0.3 * dots_per_mm_y;
}

struct Color {
  Color(quint32 i = 0, Alpha a = Alpha::Unset) : index(i), alpha(a) {}
  quint32 index;
  Alpha alpha;
};

using ColorVector = QVector<Color>;

inline bool operator== (const Color& k1, const Color& k2) {
  if (k1.index != k2.index) return false;
  return k1.alpha == k2.alpha;
}

inline uint qHash(const Color& key, uint seed) {
  return qHash(qMakePair(key.index, as_numeric(key.alpha)), seed);
}

static const inline double DefaultDepth = - 15.;

}

namespace QT {
using ColorVector = QVector<QColor>;
}

namespace GL {
using IndexVector = QVector<GLuint>;
using VertexVector = QVector<GLfloat>;

inline glm::vec2 Vec2(const QPointF& p) {
  return glm::vec2(p.x(), p.y());
}

}

struct SymbolKey {

  SymbolKey(quint32 idx, S52::SymbolType s)
    : index(idx)
    , type(s) {}

  SymbolKey() = default;

  quint32 index;
  S52::SymbolType type;
};

inline bool operator== (const SymbolKey& k1, const SymbolKey& k2) {
  if (k1.index != k2.index) return false;
  return k1.type == k2.type;
}

inline uint qHash(const SymbolKey& key, uint seed) {
  return qHash(qMakePair(key.index, as_numeric(key.type)), seed);
}

struct PatternAdvance {
  PatternAdvance(int x0, int y0, int x1)
    : x(x0), xy(x1, y0) {}

  PatternAdvance() = default;

  int x;
  QPoint xy;
};


