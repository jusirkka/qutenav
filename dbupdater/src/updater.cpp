/* -*- coding: utf-8-unix -*-
 *
 * File: dbupdater/src/updater.cpp
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
#include "updater.h"
#include <QDebug>
#include <QState>
#include "state.h"


Updater::Updater(QObject* parent)
  : QObject(parent)
{
  auto idle = new QState();
  m_states.addState(idle);

  m_busyState = new State::Busy();
  m_states.addState(m_busyState);

  idle->addTransition(this, &Updater::start, m_busyState);

  m_busyState->addTransition(this, &Updater::abort, idle);
  m_busyState->addTransition(m_busyState, &State::Busy::jobsDone, idle);

  connect(m_busyState, &State::Busy::status, this, &Updater::status);
  connect(m_busyState, &State::Busy::ready, this, &Updater::ready);

  m_states.setInitialState(idle);
  m_states.start();
}

QString Updater::ping() const {
  return "pong";
}


Updater::~Updater() {}

void Updater::fullSync(const QStringList& paths) {
  // qDebug() << paths;
  emit abort();
  Q_ASSERT(!m_busyState->active());
  emit start(paths, true);
}

void Updater::sync(const QStringList& paths) {
  // qDebug() << paths;
  emit abort();
  Q_ASSERT(!m_busyState->active());
  emit start(paths, false);
}
