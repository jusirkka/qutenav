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

  var lat = Math.abs(p.coordinate.latitude);
  var latdeg = pad(Math.floor(lat).toString(), "00");
  var latmin = pad(((lat - Math.floor(lat)) * 60).toFixed(2), "00000");
  var n = p.coordinate.latitude > 0 ? 'N' : 'S';

  var lng = Math.abs(p.coordinate.longitude);
  var lngdeg = pad(Math.floor(lng).toString(), "000");
  var lngmin = pad(((lng - Math.floor(lng)) * 60).toFixed(2), "00000");
  var e = p.coordinate.longitude > 0 ? 'E' : 'W';

  return "" + latdeg + "°" + latmin + "‘" + n + " " + lngdeg + "°" + lngmin + "‘" + e;
}
