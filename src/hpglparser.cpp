/* -*- coding: utf-8-unix -*-
 *
 * File: src/hpglparser.cpp
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
#include "hpglparser.h"
#include <QDebug>
#include "s52presentation.h"

#include "s52hpgl_parser.h"
#define YYLTYPE HPGL::LocationType
#define YYSTYPE HPGL::ValueType
#include "s52hpgl_scanner.h"



void s52hpgl_error(HPGL::LocationType* loc,
                   HPGL::Parser*,
                   yyscan_t sc,
                   const char*  msg) {
  if (loc == nullptr) loc = s52hpgl_get_lloc(sc);
  QString err("HPGL error at position %1-%2: %3");
  qWarning() << err.arg(loc->pos).arg(loc->prev_pos).arg(msg);
}


HPGL::Parser::Parser(const QString& colors)
  : m_ok(true)
{
  parseColorRef(colors);
  if (!m_ok) {
    qWarning() << "HPGLParser: unknown color in" << colors;
    return;
  }
}

void HPGL::Parser::parse(const QString& src) {
  if (!m_ok) return;

  if (src.isEmpty()) {
    m_ok = false;
    qWarning() << "HPGL::Parser: empty source";
    return;
  }

  // qDebug() << src;

  yyscan_t scanner;
  s52hpgl_lex_init(&scanner);
  s52hpgl__scan_string(src.toUtf8().constData(), scanner);
  int err = s52hpgl_parse(this, scanner);
  s52hpgl_lex_destroy(scanner);

  m_ok = err == 0;
  if (!m_ok) {
    qWarning() << "HPGL Parse error:" << src;
  }
}

void HPGL::Parser::parseColorRef(const QString& cmap) {
  int index = 0;
  while (index < cmap.length()) {
    const char key = cmap.mid(index, 1).at(0).toLatin1();
    index += 1;
    const QString cref = cmap.mid(index, 5);
    bool ok;
    m_cmap[key] = S52::FindIndex(cref, &ok);
    if (!ok) {
      m_ok = false;
      return;
    }
    index += 5;
  }
}

