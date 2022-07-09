/* -*- coding: utf-8-unix -*-
 *
 * router.h
 *
 * Created: 17/04/2021 2021 by Jukka Sirkka
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
#include <QSGGeometry>
#include "types.h"
#include "trackdatabase.h"


class Router: public QQuickItem {

  Q_OBJECT

public:

  Router(QQuickItem* parent = nullptr);

  Q_PROPERTY(bool edited
             READ edited
             NOTIFY editedChanged)

  bool edited() {return m_edited;}

  Q_PROPERTY(bool empty
             READ empty
             NOTIFY emptyChanged)

  bool empty() {return m_vertices.isEmpty();}

  Q_INVOKABLE void sync();

  Q_INVOKABLE int append(const QPointF& pos);
  Q_INVOKABLE void move(int index, const QPointF& pos);
  Q_INVOKABLE void remove(int index);
  Q_INVOKABLE QPointF insert(int index);
  Q_INVOKABLE int length() const;
  Q_INVOKABLE QPointF vertex(int index) const;
  Q_INVOKABLE void enableEditMode(bool enabled);

  Q_INVOKABLE void save();
  Q_INVOKABLE void clear();
  Q_INVOKABLE void reverse();
  Q_INVOKABLE void load(int rid);

  Q_INVOKABLE QString name() const;

  QSGNode *updatePaintNode(QSGNode* node, UpdatePaintNodeData*) override;

  WGS84PointVector waypoints() const {return m_positions;}

signals:

  void emptyChanged();
  void editedChanged();
  void distanceChanged(qreal distance);

public slots:

private:

  void updateDistance();

  static const int lineWidth = 4;

  using PointVector = QVector<QPointF>;

  PointVector m_vertices;
  WGS84PointVector m_positions;

  bool m_edited;
  bool m_synced;
  bool m_modified;
  int m_routeId;
  qreal m_distance;

};

