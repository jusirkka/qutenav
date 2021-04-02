/* -*- coding: utf-8-unix -*-
 *
 * test_region.cpp
 *
 * Created: 2021-03-24 2021 by Jukka Sirkka
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

#include <QtTest/QTest>
#include "region.h"
#include <QDebug>

class TestRegion: public QObject {

  Q_OBJECT

private slots:

  void testAddition1();
  void testAddition2();
  void testSubtraction1();
  void testSubtraction2();
  void testSubtraction3();
  void testOther();
};

void TestRegion::testAddition1() {
  KV::Region r1(QRectF(0, 0, 1, 1));
  KV::Region r2(QRectF(1, 0, 1, 1));

  KV::Region r = r1 + r2;

  QCOMPARE(r.rectCount(), 1);

  for (const QRectF& rect: r) {
    QCOMPARE(rect, QRectF(0, 0, 2, 1));
  }

  r += QRectF(0, 1, 1, 1);

  //  for (const QRectF& rect: r) {
  //    qDebug() << rect;
  //  }

  QCOMPARE(r.rectCount(), 2);

  QVector<QRectF> expected {QRectF(0, 0, 2, 1), QRectF(0, 1, 1, 1)};

  for (const QRectF& rect: r) {
    QCOMPARE(rect, expected.first());
    expected.removeFirst();
  }

}

void TestRegion::testAddition2() {

  const qreal x0 = 28959.79543256586109876576576775;
  const qreal y0 = 93585.84126442670898989938090985;
  const qreal dx = 205050.8481483981309832029438023;
  const qreal dy = 155908.2438535410903295138573187;

  QVector<QPoint> ps {
    {1, 1}, {2, 1},
    {1, 2}, {2, 2},
    {0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3}, {5, 3}, {6, 3}, {7, 3}, {8, 3},
    {1, 4}, {2, 4},
    {1, 5}, {2, 5},
    {1, 6}, {2, 6},
    {1, 7}, {2, 7},
    {1, 8}, {2, 8},
    {9, 9},
    {9, 10},
  };
  QVector<QRectF> boxes;
  for (auto p: ps) {
    boxes << QRectF(x0 + p.x() * dx, y0 + p.y() * dy, dx, dy);
  }

  // qDebug() << boxes;

  KV::Region r = std::accumulate(boxes.begin(), boxes.end(), KV::Region());

  for (const QRectF& rect: r) {
    qDebug() << rect;
  }

  QCOMPARE(r.rectCount(), 4);

  QVector<QRectF> expected {
    QRectF(x0 + 1 * dx, y0 + 1 * dy, 2 * dx, 2 * dy),
    QRectF(x0 + 0 * dx, y0 + 3 * dy, 9 * dx, 1 * dy),
    QRectF(x0 + 1 * dx, y0 + 4 * dy, 2 * dx, 5 * dy),
    QRectF(x0 + 9 * dx, y0 + 9 * dy, 1 * dx, 2 * dy),
  };

  for (const QRectF& rect: r) {
    QCOMPARE(rect, expected.first());
    expected.removeFirst();
  }


}

void TestRegion::testSubtraction1() {
  KV::Region r1(QRectF(0, 0, 1, 1));
  KV::Region r2(QRectF(0, 0, 2, 2));

  KV::Region r = r1 - r2;
  QCOMPARE(r.rectCount(), 0);

  r = r1;
  r -= r2;
  QCOMPARE(r.rectCount(), 0);

  r -= r1;
  QCOMPARE(r.rectCount(), 0);

}

void TestRegion::testSubtraction2() {
  KV::Region r1(QRectF(0, 0, 4, 3));
  KV::Region r2(QRectF(1, 1, 1, 1));

  KV::Region r = r1 - r2;

  //  for (const QRectF& rect: r) {
  //    qDebug() << rect;
  //  }

  QCOMPARE(r.rectCount(), 4);

  QVector<QRectF> expected {
    QRectF(0, 0, 4, 1),
    QRectF(0, 1, 1, 1), QRectF(2, 1, 2, 1),
    QRectF(0, 2, 4, 1)
  };

  for (const QRectF& rect: r) {
    QCOMPARE(rect, expected.first());
    expected.removeFirst();
  }

}


void TestRegion::testOther() {

  QVector<QRectF> boxes {
    QRectF(0, 0, 4, 1),
    QRectF(0, 1, 1, 1), QRectF(2, 1, 2, 1),
    QRectF(0, 2, 4, 1)
  };

  KV::Region r = std::accumulate(boxes.begin(), boxes.end(), KV::Region());

  for (const QRectF& rect: r) {
    qDebug() << rect;
  }

  QCOMPARE(r.rectCount(), 4);

  for (const QRectF& rect: r) {
    QCOMPARE(rect, boxes.first());
    boxes.removeFirst();
  }

  QCOMPARE(r.area(), 11);

}

void TestRegion::testSubtraction3() {

  const qreal x0 = 28959.79543256586109876576576775;
  const qreal y0 = 93585.84126442670898989938090985;
  const qreal dx = 205050.8481483981309832029438023;
  const qreal dy = 155908.2438535410903295138573187;

  QVector<QPoint> ps {
    {1, 1},
    {0, 2}, {1, 2}, {2, 2}, {3, 2},
  };
  QVector<QRectF> boxes;
  for (auto p: ps) {
    boxes << QRectF(x0 + p.x() * dx, y0 + p.y() * dy, dx, dy);
  }

  KV::Region r = KV::Region(QRectF(x0, y0, 4 * dx, 4 * dy)) - std::accumulate(boxes.begin(), boxes.end(), KV::Region());

  for (const QRectF& rect: r) {
    qDebug() << rect;
  }

  QCOMPARE(r.rectCount(), 4);

  QVector<QRectF> expected {
    QRectF(x0 + 0 * dx, y0 + 0 * dy, 4 * dx, 1 * dy),
    QRectF(x0 + 0 * dx, y0 + 1 * dy, 1 * dx, 1 * dy),
    QRectF(x0 + 2 * dx, y0 + 1 * dy, 2 * dx, 1 * dy),
    QRectF(x0 + 0 * dx, y0 + 3 * dy, 4 * dx, 1 * dy),
  };

  for (const QRectF& rect: r) {
    QCOMPARE(rect, expected.first());
    expected.removeFirst();
  }



}


QTEST_APPLESS_MAIN(TestRegion)


#include "test_region.moc"
