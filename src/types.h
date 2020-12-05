#pragma once

#include <QString>
#include <cmath>
#include <QVector2D>
#include <QVector>

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

  static constexpr double semimajor_axis = 6378137.0;

  friend WGS84Point operator+ (const WGS84Point& a, const WGS84Bearing& b);

  static WGS84Point fromLL(double lng, double lat);
  static WGS84Point fromLLRadians(double lng, double lat);
  static WGS84Point parseISO6709(const QString& loc);

  WGS84Point(const WGS84Point& a): m_Longitude(a.m_Longitude), m_Latitude(a.m_Latitude), m_Valid(a.m_Valid) {}
  WGS84Point& operator=(const WGS84Point& a) {m_Longitude = a.m_Longitude; m_Latitude = a.m_Latitude; m_Valid = a.m_Valid; return *this;}
  WGS84Point(): m_Longitude(0), m_Latitude(0), m_Valid(false) {}

  QString print();
  QString toISO6709();
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

}

