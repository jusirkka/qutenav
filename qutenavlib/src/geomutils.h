/* -*- coding: utf-8-unix -*-
 *
 * geomutils.h
 *
 * Created: 09/02/2021 2021 by Jukka Sirkka
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

#include <QRectF>
#include <glm/glm.hpp>

// Cohen-Sutherland test
bool crossesBox(const glm::vec2& v1, const glm::vec2& v2, const QRectF& box);

bool insidePolygon(uint count, uint offset, const glm::vec2* q, const uint* indices, const QPointF& p);
