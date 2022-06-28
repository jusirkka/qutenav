/* -*- coding: utf-8-unix -*-
 *
 * slotcounter.h
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

#pragma once

#include "types.h"


class SlotCounter {
public:

  SlotCounter();
  bool full() const {return m_slotsLeft <= 0;}
  void fill(const WGS84Point& sw, const WGS84Point& ne, int prio);
  bool updated() const {return m_updated;}

private:

  static const inline quint32 x0 = 1800;
  static const inline quint32 y0 = 800;

  const quint32 m_lvl;

  QVector<int> m_slots;


  int m_slotsLeft;
  bool m_updated;

};

