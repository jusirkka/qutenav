/* -*- coding: utf-8-unix -*-
 *
 * platform.cpp
 *
 * Created: 13/05/2021 2021 by Jukka Sirkka
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

#include <QGuiApplication>
#include <QScreen>
#include <fstream>
#include "platform.h"

float Platform::dots_per_mm_x() {
  static float v = QGuiApplication::primaryScreen()->physicalDotsPerInchX() / 25.4;
  return v;
}

float Platform::dots_per_mm_y() {
  static float v = QGuiApplication::primaryScreen()->physicalDotsPerInchY() / 25.4;
  return v;
}

float Platform::dots_per_inch_x() {
  static float v = QGuiApplication::primaryScreen()->physicalDotsPerInchX();
  return v;
}

float Platform::dots_per_inch_y() {
  static float v = QGuiApplication::primaryScreen()->physicalDotsPerInchY();
  return v;
}

const QString& Platform::base_app_name() {
  // qutenav, harbour-qutenav, qutenav_dbupdater or harbour_qutenav_updater ->
  // qutenav or harbour-qutenav
  static QString name = qAppName().replace("_dbupdater", "").replace("_", "-");
  return name;
}

static int computeNumThreads() {
  std::ifstream cpuinfo("/proc/cpuinfo");

  return std::count(std::istream_iterator<std::string>(cpuinfo),
                    std::istream_iterator<std::string>(),
                    std::string("processor"));
}

int Platform::number_of_chart_threads() {
  static int numChartThreads = std::max(1, computeNumThreads() - 1);
  return numChartThreads;
}

static float dpmm0 = 6.2;
static float delta_dpmm1 = 9.7;
static float nominal_dpmm = 3.125;

float Platform::display_length_scaling() {
  static float y0 = 6.2 / nominal_dpmm;
  static float y1 = 9.0 / nominal_dpmm;
  static float t = (dots_per_mm_y() - dpmm0) / delta_dpmm1;
  static float y = y1 * t + y0 * (1 - t);
  return y;
}

float Platform::display_text_size_scaling() {
  static float y0 = 3.5;
  static float y1 = 4.2;
  static float t = (dots_per_mm_y() - dpmm0) / delta_dpmm1;
  static float y = y1 * t + y0 * (1 - t);
  return y;
}

float Platform::display_line_width_scaling() {
  static float y0 = .7;
  static float y1 = .3;
  static float t = (dots_per_mm_y() - dpmm0) / delta_dpmm1;
  static float y = y1 * t + y0 * (1 - t);
  return y;
}

float Platform::display_raster_symbol_scaling() {
  static float y0 = 1.;
  static float y1 = .5;
  static float t = (dots_per_mm_y() - dpmm0) / delta_dpmm1;
  static float y = y1 * t + y0 * (1 - t);
  return y;
}

float Platform::pick_icon_size() {
  static float y0 = 48.;
  static float y1 = 96.;
  static float t = (dots_per_mm_y() - dpmm0) / delta_dpmm1;
  static float y = y1 * t + y0 * (1 - t);
  return y;
}

float Platform::pick_icon_min_size() {
  static float y = .50 * pick_icon_size();
  return y;
}

float Platform::pick_icon_max_size() {
  static float y = .75 * pick_icon_size();
  return y;
}

float Platform::peep_hole_size() {
  static float y0 = 20.;
  static float y1 = 40.;
  static float t = (dots_per_mm_y() - dpmm0) / delta_dpmm1;
  static float y = y1 * t + y0 * (1 - t);
  return y;
}


