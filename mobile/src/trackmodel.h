/* -*- coding: utf-8-unix -*-
 *
 * trackmodel.h
 *
 * Created: 13/04/2021 2021 by Jukka Sirkka
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

#include <QSqlTableModel>
#include <QAbstractListModel>

class TrackModel: public QAbstractListModel {

  Q_OBJECT

public:

  TrackModel(QObject* parent = nullptr);

  QHash<int, QByteArray> roleNames() const override;
  QVariant data(const QModelIndex& index, int role) const override;
  bool setData(const QModelIndex& item, const QVariant& value, int role) override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  ~TrackModel();

private:

  QSqlTableModel* m_tracks;

};

