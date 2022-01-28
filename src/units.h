/* -*- coding: utf-8-unix -*-
 *
 * File: src/units.h
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
#pragma once

#include <QObject>
#include "conf_units.h"
#include "logging.h"
#include <cmath>
#include "types.h"
#include <QGeoCoordinate>

namespace Units {

class Converter {
public:
  float fromSI(float v) const {return v / m_factor;}
  float toSI(float v) const {return v * m_factor;}

  const QString& symbol() const {return m_symbol;}

  virtual QString display(float v, int prec = 0) const;

  QString displaySI(float v, int prec = 0) const {return display(fromSI(v), prec);}

  virtual ~Converter() = default;
protected:
  Converter(const QString& symbol, float factor)
    : m_symbol(symbol)
    , m_factor(factor) {}

  const QString m_symbol;
  const float m_factor;
};

class Manager: public QObject {
  Q_OBJECT

public:

  ~Manager();

  static Manager* instance();

  Q_PROPERTY(QString speedSymbol
             READ speedSymbol
             NOTIFY speedSymbolChanged)

  QString speedSymbol() const {
    return m_speed->symbol();
  }

  Q_INVOKABLE float speed(float v) const {return m_speed->fromSI(v);}
  Q_INVOKABLE QString displaySpeed(float v) const {return m_speed->displaySI(v);}

  Q_PROPERTY(QString distanceSymbol
             READ distanceSymbol
             NOTIFY distanceSymbolChanged)

  QString distanceSymbol() const {
    return m_distance->symbol();
  }

  Q_INVOKABLE float distance(float v) const {return m_distance->fromSI(v);}
  Q_INVOKABLE QString displayDistance(float v) const {return m_distance->displaySI(v);}


  Q_PROPERTY(QString depthSymbol
             READ depthSymbol
             NOTIFY depthSymbolChanged)

  QString depthSymbol() const {
    return m_depth->symbol();
  }

  Q_INVOKABLE float depth(float v) const {return m_depth->fromSI(v);}
  Q_INVOKABLE float depthSI(float v) const {return m_depth->toSI(v);}


  const Converter* distance() const {return m_distance;}
  const Converter* shortDistance() const {return m_shortDistance;}
  const Converter* depth() const {return m_depth;}

  Q_INVOKABLE QString location(const QGeoCoordinate& q, quint8 prec = 4) const {
    WGS84Point p = WGS84Point();
    if (q.isValid()) {
      p = WGS84Point::fromLL(q.longitude(), q.latitude());
    }
    return p.print(locMap[Conf::Units::Location()], prec);
  }

private slots:

  void handleUnitChange();

signals:

  void speedSymbolChanged();
  void distanceSymbolChanged();
  void depthSymbolChanged();

private:

  Manager(QObject* parent = nullptr);

  Converter* m_distance;
  Converter* m_shortDistance;
  Converter* m_speed;
  Converter* m_depth;

  static const inline QMap<Conf::Units::EnumLocation::type, WGS84Point::Units> locMap = {
    {Conf::Units::EnumLocation::type::Deg, WGS84Point::Units::Deg},
    {Conf::Units::EnumLocation::type::DegMin, WGS84Point::Units::DegMin},
    {Conf::Units::EnumLocation::type::DegMinSec, WGS84Point::Units::DegMinSec}
  };

};

}
