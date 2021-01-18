#pragma once

#include <QVector>
#include <QSize>
#include <ft2build.h>
#include <freetype/freetype.h>

/*
    C++ port of https://github.com/mapbox/tiny-sdf, which is in turn based on the
    Felzenszwalb/Huttenlocher distance transform paper (https://cs.brown.edu/~pff/papers/dt-final.pdf).
    Note there exists an alternative C++ implementation from the paper’s authors at
    https://cs.brown.edu/~pff/dt/, which this implementation is not based on.

    Takes an alpha channel raster input and transforms it into an alpha channel
    Signed Distance Field (SDF) output of the same dimensions.
*/

class TinySDF {
public:
  TinySDF(const FT_Bitmap& bmap, uint pad);
  const QSize& size() const {return m_size;}
  const uchar* transform(double radius, double cutoff);
  ~TinySDF();

private:

  TinySDF(uint w, uint h, uint maxdim, const uchar* bytes, uint pad);

  void edt(QVector<double>& grid);
  void edt1d(uint n);
  double intersection(uint q, uint k) const;
  QSize m_size;
  QVector<double> m_func;
  QVector<double> m_parabola;
  QVector<double> m_envelope;
  QVector<double> m_dist2;
  QVector<double> m_gridOuter;
  QVector<double> m_gridInner;


  uchar* m_sdf;

};


