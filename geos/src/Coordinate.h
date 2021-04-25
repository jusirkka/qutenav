/* -*- coding: utf-8-unix -*-
 *
 * Coordinate.h
 *
 * Created: 22/04/2021 2021 by Jukka Sirkka
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

#include <glm/glm.hpp>
#include <QVector>

namespace geos {

namespace geom {

using Coordinate = glm::dvec2;
using CoordinateSequence = QVector<Coordinate>;

static const inline double eps = 1.e-7;

static const inline double undefined = std::numeric_limits<double>::quiet_NaN();

}

namespace util {

class IllegalArgumentException {
public:
  IllegalArgumentException(QString msg): m_detail(std::move(msg)) {}
  const QString msg() const {return m_detail;}
private:
  QString m_detail;
};

class IllegalStateException {
public:
  IllegalStateException(QString msg): m_detail(std::move(msg)) {}
  const QString msg() const {return m_detail;}
private:
  QString m_detail;
};

}
}
