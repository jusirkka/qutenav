/* -*- coding: utf-8-unix -*-
 *
 * File: oesencreader/src/oesencreader.h
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

#include "chartfilereader.h"

class OesuReaderFactory;

class OesuReader: public ChartFileReader {

  friend class OesuReaderFactory;

public:

  const GeoProjection* geoprojection() const override;
  GeoProjection* configuredProjection(const QString &path) const override;

  S57ChartOutline readOutline(const QString& path, const GeoProjection* proj) const override;

  void readChart(GL::VertexVector& vertices,
                 GL::IndexVector& indices,
                 S57::ObjectVector& objects,
                 const QString& path,
                 const GeoProjection* proj) const override;

  ~OesuReader();


private:

  OesuReader(const QString& name);

  GeoProjection* m_proj;

};

class OesuReaderFactory: public QObject, public ChartFileReaderFactory {

  Q_OBJECT
  Q_PLUGIN_METADATA(IID "net.kvanttiapina.qutenav.ChartFileReaderFactory/1.0")
  Q_INTERFACES(ChartFileReaderFactory)

public:

  QString name() const override;
  QString displayName() const override;
  QStringList filters() const override;

protected:

  void initialize(const QStringList&) const override;
  ChartFileReader* create() const override;

};

