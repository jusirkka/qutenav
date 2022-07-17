/* -*- coding: utf-8-unix -*-
 *
 * File: dbupdater/src/updater.h
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

#include <QStateMachine>

class QState;

namespace State {
class Busy;
}

class Updater: public QObject {

  Q_OBJECT

public:

  Updater(QObject* parent = nullptr);
  ~Updater();

public slots:

  void sync(const QStringList& paths);
  void fullSync(const QStringList& paths);
  QString ping() const;


signals:

  void ready(bool clearCache);
  void status(const QString& msg);

  void start(const QStringList& paths, bool full);
  void abort();


private:

  QStateMachine m_states;
  State::Busy* m_busyState;

};
