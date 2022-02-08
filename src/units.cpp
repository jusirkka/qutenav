/* -*- coding: utf-8-unix -*-
 *
 * File: src/units.cpp
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
#include "units.h"
#include <QDebug>
#include "platform.h"
#include "settings.h"

using namespace Units;

static quint8 numdigits(quint32 v) {
  quint8 num = 0;
  do {
    num++;
    v /= 10;
  } while (v != 0);
  return num;
}


QString Converter::display(float v, int prec) const {
  Q_ASSERT(v >= 0.f);

  quint32 a = static_cast<int>(v);
  const float b = v - a;

  if (a > 0) {
    const quint8 d = numdigits(a);
    if (prec > d) {
      // manipulate b -> b' (int)
      const quint32 m = std::pow(10, prec - d);
      quint32 b1 = std::round(m * b);
      if (b1 == m) {
        b1 = 0;
        a += 1;
      }
      // print a + b'
      const QString r = QString::number(a) + ".%1 " + m_symbol;
      return r.arg(b1, prec - d, 10, QChar('0'));
    }
    // print a
    return QString::number(static_cast<int>(std::round(v))) + " " + m_symbol;
  }
  // manipulate b -> b' (int)
  const quint32 m = std::pow(10, prec);
  const quint32 b1 = std::round(m * b); // FIXME: case of b1 == m
  // print b'
  const QString r = ".%1 " + m_symbol;
  return r.arg(b1, prec, 10, QChar('0'));
}

Manager* Manager::instance() {
  static Manager* m = new Manager();
  return m;
}

Manager::Manager(QObject *parent)
  : QObject(parent)
  , m_distance(nullptr)
  , m_shortDistance(nullptr)
  , m_speed(nullptr)
  , m_depth(nullptr)
{
  handleUnitChange();
  connect(Settings::instance(), &Settings::unitsChanged, this, &Manager::handleUnitChange);
}

Manager::~Manager() {
  delete m_distance;
  delete m_shortDistance;
}

class KMConverter: public Converter {
  //% "km"
  static const inline char* symbol = QT_TRID_NOOP("qtnav-symbol-km");

public:

  KMConverter(): Converter(qtTrId(symbol), 1000) {}
};

class MiConverter: public Converter {
  //% "mi"
  static const inline char* symbol = QT_TRID_NOOP("qtnav-symbol-mi");

public:

  MiConverter(): Converter(qtTrId(symbol), 1609.344) {}
};


class NMConverter: public Converter {
  //% "M"
  static const inline char* symbol = QT_TRID_NOOP("qtnav-symbol-nm");
public:
  NMConverter(): Converter(qtTrId(symbol), 1852) {}
};

class MConverter: public Converter {
  //% "m"
  static const inline char* symbol = QT_TRID_NOOP("qtnav-symbol-m");
public:
  MConverter(): Converter(qtTrId(symbol), 1) {}
};

class NMSConverter: public Converter {
  //% "M"
  static const inline char* symbol = QT_TRID_NOOP("qtnav-symbol-nm");

public:
  NMSConverter(): Converter(qtTrId(symbol), 18.52) {}

  QString display(float v, int) const override {
    v *= .01;
    const int a = static_cast<int>(v);
    const int b = std::round(100 * (v - a)); // FIXME: case b == 100
    QString r = ".%1 " + m_symbol;
    if (a > 0) {
      r = QString::number(a) + r;
    }
    return r.arg(b, 2, 10, QChar('0'));
  }
};

class YdsConverter: public Converter {
  //% "yd"
  static const inline char* symbol = QT_TRID_NOOP("qtnav-symbol-yd");

public:
  YdsConverter(): Converter(qtTrId(symbol), 0.9144) {}
};


class KmphConverter: public Converter {
  //% "km/h"
  static const inline char* symbol = QT_TRID_NOOP("qtnav-symbol-kmph");
public:
  KmphConverter(): Converter(qtTrId(symbol), 0.277778) {}
};

class KnConverter: public Converter {
  //% "kn"
  static const inline char* symbol = QT_TRID_NOOP("qtnav-symbol-kn");
public:
  KnConverter(): Converter(qtTrId(symbol), 0.514444) {}
};

class MphConverter: public Converter {
  //% "mph"
  static const inline char* symbol = QT_TRID_NOOP("qtnav-symbol-mph");
public:
  MphConverter(): Converter(qtTrId(symbol), 0.44704) {}
};

class FathomConverter: public Converter {
  //% "ftm"
  static const inline char* symbol = QT_TRID_NOOP("qtnav-symbol-ftm");
public:
  FathomConverter(): Converter(qtTrId(symbol), 1.8288) {}
};

class FootConverter: public Converter {
  //% "ft"
  static const inline char* symbol = QT_TRID_NOOP("qtnav-symbol-ft");
public:
  FootConverter(): Converter(qtTrId(symbol), 0.3048) {}
};

void Manager::handleUnitChange() {
  delete m_distance;
  using EDist = Conf::Units::EnumDistance::type;

  switch (Conf::Units::Distance()) {
  case EDist::KM: m_distance = new KMConverter; break;
  case EDist::Mi: m_distance = new MiConverter; break;
  case EDist::NM: m_distance = new NMConverter; break;
  default: m_distance = nullptr;
  }

  emit distanceSymbolChanged();

  delete m_shortDistance;
  using ESDist = Conf::Units::EnumShortDistance::type;

  switch (Conf::Units::ShortDistance()) {
  case ESDist::M: m_shortDistance = new MConverter; break;
  case ESDist::NM: m_shortDistance = new NMSConverter; break;
  case ESDist::Yds: m_shortDistance = new YdsConverter; break;
  default: m_shortDistance = nullptr;
  }

  delete m_speed;
  using ESpeed = Conf::Units::EnumBoatSpeed::type;

  switch (Conf::Units::BoatSpeed()) {
  case ESpeed::Kmph: m_speed = new KmphConverter; break;
  case ESpeed::Kn: m_speed = new KnConverter; break;
  case ESpeed::Mph: m_speed = new MphConverter; break;
  default: m_speed = nullptr;
  }

  emit speedSymbolChanged();

  delete m_depth;
  using EDepth = Conf::Units::EnumDepth::type;

  switch (Conf::Units::Depth()) {
  case EDepth::Fathoms: m_depth = new FathomConverter; break;
  case EDepth::Feet: m_depth = new FootConverter; break;
  case EDepth::Meters: m_depth = new MConverter; break;
  default: m_depth = nullptr;
  }

  emit depthSymbolChanged();
}

