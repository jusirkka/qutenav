/* -*- coding: utf-8-unix -*-
 *
 * slotcounter.cpp
 *
 * Created: 2022-06-28 2022 by Jukka Sirkka
 *
 * Copyright (C) 2022 Jukka Sirkka
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

#include "slotcounter.h"
#include <QDebug>

SlotCounter::SlotCounter()
  : m_nx(x0)
  , m_x0(- m_nx)
  , m_dx(2 * m_nx)
  , m_ny(y0)
  , m_y0(- m_ny)
  , m_dy(2 * m_ny)
  , m_slots(m_nx * m_ny, 99)
  , m_slotsLeft(m_slots.size())
  , m_updated(false)
{}

SlotCounter::SlotCounter(const WGS84Point& sw0, const WGS84Point& ne0, quint32 sc)
  : m_nx(sc > 200000 ? 50 : sc > 50000 ? 10 : 5)
  , m_x0(sw0.lng())
  , m_dx(ne0.lng() - m_x0)
  , m_ny(m_nx)
  , m_y0(sw0.lat())
  , m_dy(ne0.lat() - m_y0)
  , m_slots(m_nx * m_ny, 99)
  , m_slotsLeft(m_slots.size())
  , m_updated(false)
{
  // TODO: handle dateline
}

void SlotCounter::fill(const WGS84Point& sw, const WGS84Point& ne, int prio) {

  // handle dateline
  if (ne.lng() < sw.lng()) {
    const auto ne1 = WGS84Point::fromLL(180., ne.lat());
    fill(sw, ne1, prio);
    const auto upd = m_updated;
    const auto sw1 = WGS84Point::fromLL(-180., sw.lat());
    fill(sw1, ne, prio);
    m_updated = upd || m_updated;
    return;
  }

  auto ix1 = static_cast<int>(std::floor((sw.lng() - m_x0) * m_nx / m_dx));
  ix1 = std::min(m_nx - 1, std::max(0, ix1));

  auto iy1 = static_cast<int>(std::floor((sw.lat() - m_y0) * m_ny / m_dy));
  iy1 = std::min(m_ny - 1, std::max(0, iy1));


  auto ix2 = static_cast<int>(std::floor((ne.lng() - m_x0) * m_nx / m_dx));
  ix2 = std::min(m_nx - 1, std::max(0, ix2));

  auto iy2 = static_cast<int>(std::floor((ne.lat() - m_y0) * m_ny / m_dy));
  iy2 = std::min(m_ny - 1, std::max(0, iy2));

  m_updated = false;
  for (int iy = iy1; iy <= iy2; ++iy) {
    for (int ix = ix1; ix <= ix2; ++ix) {
      const int index = iy * m_nx + ix;
      if (m_slots[index] >= prio) {
        if (m_slots[index] > prio) {
          m_slotsLeft--;
          m_slots[index] = prio;
        }
        m_updated = true;
      }
    }
  }
  // if (m_updated) qDebug() << m_slotsLeft << "slots left";
}
