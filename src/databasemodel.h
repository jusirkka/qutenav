/* -*- coding: utf-8-unix -*-
 *
 * databasemodel.h
 *
 * Created: 18/04/2021 2021 by Jukka Sirkka
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

class DatabaseModel: public QAbstractListModel {

  Q_OBJECT

public:

  DatabaseModel(QObject* parent, int sortColumn);

  QHash<int, QByteArray> roleNames() const override;
  QVariant data(const QModelIndex& index, int role) const override;
  bool setData(const QModelIndex& item, const QVariant& value, int role) override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  Q_INVOKABLE QVariant get(const QByteArray& roleName, int row) const;
  Q_INVOKABLE void sort();

  Q_PROPERTY(int count
             READ count
             NOTIFY countChanged)

  int count() const {return rowCount();}

  virtual ~DatabaseModel();

signals:

  void countChanged();

protected:

  QSqlTableModel* m_model;

  int m_sortColumn;

};


