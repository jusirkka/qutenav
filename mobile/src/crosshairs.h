/* -*- coding: utf-8-unix -*-
 *
 * crosshairs.h
 *
 * Created: 07/02/2021 2021 by Jukka Sirkka
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

#include <QQuickItem>

class CrossHairs: public QQuickItem {

  Q_OBJECT

public:

  CrossHairs(QQuickItem* parent = nullptr);
  ~CrossHairs();

  Q_PROPERTY(QPointF peepHole
             READ peepHole
             WRITE setPeepHole
             NOTIFY peepHoleChanged)

  QPointF peepHole() const {return m_peepHole;}
  void setPeepHole(const QPointF& p);

  QSGNode *updatePaintNode(QSGNode* node, UpdatePaintNodeData*) override;

signals:

  void peepHoleChanged(const QPointF& p);

private:

  static const uint lineWidth = 4;

  QPointF m_peepHole;
};

