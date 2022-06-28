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
  : m_lvl(10)
  , m_slots(x0 / m_lvl * y0 / m_lvl, 99)
  , m_slotsLeft(m_slots.size())
  , m_updated(false)
{}

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

  auto ix1 = static_cast<qint32>(std::floor((10. * sw.lng() + x0) / 2 / m_lvl));
  ix1 = std::min(static_cast<qint32>(x0 / m_lvl - 1), std::max(0, ix1));

  auto iy1 = static_cast<qint32>(std::floor((10 * sw.lat() + y0) / 2 / m_lvl));
  iy1 = std::min(static_cast<qint32>(y0 / m_lvl - 1), std::max(0, iy1));

  auto ix2 = static_cast<qint32>(std::floor((10. * ne.lng() + x0) / 2 / m_lvl));
  ix2 = std::min(static_cast<qint32>(x0 / m_lvl - 1), std::max(0, ix2));

  auto iy2 = static_cast<qint32>(std::floor((10 * ne.lat() + y0) / 2 / m_lvl));
  iy2 = std::min(static_cast<qint32>(y0 / m_lvl - 1), std::max(0, iy2));

  m_updated = false;
  for (qint32 iy = iy1; iy <= iy2; ++iy) {
    for (qint32 ix = ix1; ix <= ix2; ++ix) {
      const int index = iy * x0 / m_lvl + ix;
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
