#pragma once

#include <QString>
#include <cmath>

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



class WGS84Point {
public:
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
