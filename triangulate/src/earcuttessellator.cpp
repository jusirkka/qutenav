/* -*- coding: utf-8-unix -*-
 *
 * File: triangulate/src/earcuttessellator.cpp
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
#include "earcuttessellator.h"
#include "earcut.hpp"

namespace mapbox {
namespace util {

template <>
struct nth<0, glm::vec2> {
  inline static auto get(const glm::vec2 &t) {
    return t.x;
  }
};

template <>
struct nth<1, glm::vec2> {
  inline static auto get(const glm::vec2 &t) {
    return t.y;
  }
};

} // namespace util
} // namespace mapbox


EarcutTessellator::EarcutTessellator(const GL::VertexVector &vertices,
                                     const GL::IndexVector &indices)
  : m_vertices(vertices)
  , m_indices(indices)
{}


void EarcutTessellator::addPolygon(uint offset, size_t count) {
  const glm::vec2* vs = reinterpret_cast<const glm::vec2*>(m_vertices.constData());

  QVector<glm::vec2> poly;

  if (m_indices.isEmpty()) {
    for (uint i = 0; i < count; i++) {
      poly.push_back(vs[offset + i]);
      m_indexCover << offset + i;
    }
  } else {
    for (uint i = 0; i < count; i++) {
      const int index = m_indices[offset + i];
      poly.push_back(vs[index]);
      m_indexCover << index;
    }
  }
  m_polygon.push_back(poly);
}

void EarcutTessellator::addHole(uint offset, size_t count) {
  addPolygon(offset, count);
}


GL::IndexVector EarcutTessellator::triangulate() {
  // Three subsequent indices form a triangle. Output triangles are clockwise.
  auto earcuts = mapbox::earcut<GLuint>(m_polygon);

  GL::IndexVector indices;

  // add triangle indices in ccw order
  for (uint i = 0; i < earcuts.size() / 3; i++)  {
    const GLuint i0 = m_indexCover[earcuts[3 * i]];
    const GLuint i1 = m_indexCover[earcuts[3 * i + 1]];
    const GLuint i2 = m_indexCover[earcuts[3 * i + 2]];
    indices << i0 << i2 << i1;
  }

  return indices;
}
