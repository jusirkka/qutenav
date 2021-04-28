/* -*- coding: utf-8-unix -*-
 *
 * File: triangulate/src/earcuttessellator.h
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

#include "tessellator.h"

class EarcutTessellator: public Tessellator {
public:
  EarcutTessellator(const GL::VertexVector& vertices,
                    const GL::IndexVector& indices = GL::IndexVector());

  void addPolygon(uint offset, size_t count);
  void addHole(uint offset, size_t count);
  GL::IndexVector triangulate();

private:

  const GL::VertexVector m_vertices;
  const GL::IndexVector m_indices;

  QVector<QVector<glm::vec2>> m_polygon;
  GL::IndexVector m_indexCover;

};
