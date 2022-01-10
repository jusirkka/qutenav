/* -*- coding: utf-8-unix -*-
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */

function pad(s, p) {
  return String(p + s).slice(- p.length);
}

function printPos(p) {

  if (p === undefined || !p.longitudeValid || !p.latitudeValid) {
    return "N/A"
  }
  return units.location(p.coordinate)
}


function printDuration(v) {
  var s = Math.round(v)
  var hours = pad(Math.floor(s / 3600), "00")
  var mins = pad(Math.floor(s / 60), "00")
  var secs = pad(s % 60, "00")
  return hours + ":" + mins + ":" + secs
}
