/* -*- coding: utf-8-unix -*-
 *
 * File: triangulate/src/triangulator.cpp
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
#include "triangulator.h"
#include "earcuttessellator.h"

Triangulator::Triangulator(const GL::VertexVector &vertices,
                           const GL::IndexVector &indices)
  : m_tess(new EarcutTessellator(vertices, indices))
{}


void Triangulator::addPolygon(uint offset, size_t count) {
  m_tess->addPolygon(offset, count);
}

void Triangulator::addHole(uint offset, size_t count) {
  m_tess->addHole(offset, count);
}


GL::IndexVector Triangulator::triangulate() {
  return m_tess->triangulate();
}

Triangulator::~Triangulator() {
  delete m_tess;
}
