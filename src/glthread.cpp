/* -*- coding: utf-8-unix -*-
 *
 * File: src/glthread.cpp
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
#include "glthread.h"
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QDebug>

GL::Thread::Thread(QOpenGLContext *ctx)
  : QThread()
{
  // qDebug() << "GL::Thread";
  m_context = new QOpenGLContext;
  // qDebug() << "GL::Thread: set format";
  m_context->setFormat(QSurfaceFormat::defaultFormat());
  // qDebug() << "GL::Thread: set share context" << ctx;
  m_context->setShareContext(ctx);
  // qDebug() << "GL::Thread: set screen" << ctx->screen();
  m_context->setScreen(ctx->screen());
  m_context->create();
  // qDebug() << "GL::Thread: context: move to thread";
  m_context->moveToThread(this);

  // qDebug() << "GL::Thread: new surface";
  m_surface = new QOffscreenSurface;
  m_surface->setFormat(m_context->format());
  // qDebug() << "GL::Thread: new surface: create";
  m_surface->create();

  // qDebug() << "GL::Thread: move to thread";
  moveToThread(this);
  // qDebug() << "GL::Thread: leave";
}

void GL::Thread::run() {
  // qDebug() << "GL::Thread: run: make current";
  m_context->makeCurrent(m_surface);
  // qDebug() << "GL::Thread: run: exec";
  exec();
}

GL::Thread::~Thread() {
  m_context->doneCurrent();
  delete m_context;
  // schedule this to be deleted only after we're done cleaning up
  m_surface->deleteLater();
}
