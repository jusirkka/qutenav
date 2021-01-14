#pragma once

#include <QString>
#include <QSize>
#include <QPointF>
#include <QMap>
#include <QSharedPointer>
#include <freetype/freetype.h>
#include <harfbuzz/hb.h>
#include <QRectF>
#include <qopengl.h>
#include "types.h"

class Font;

struct FontNotFound {
  QString reason;
  bool suggestion;
  QString name;
  quint16 size;
  bool bold;
  bool italic;
};

class Glyph {
  friend class Font;
public:
  const QPoint& offset() const {return m_offset;}
  const QSize& size() const {return m_size;}
  const QPointF& texUL() const {return m_texUL;}
  const QPointF& texLR() const {return m_texLR;}

private:

  Glyph() = default;
  Glyph(const Glyph&) = delete;
  Glyph& operator= (const Glyph&) = delete;

  QPointF m_texUL;
  QPointF m_texLR;

  QPoint m_offset;
  QSize m_size;
};


class GlyphManager;
class QMutex;

class GlyphBin {
  friend class GlyphManager;
  friend class Font;

public:

  ~GlyphBin();

private:


  GlyphBin(QMutex* mutex);
  GlyphBin(const GlyphBin&) = delete;
  GlyphBin& operator= (const GlyphBin&) = delete;

  quint16 width() const {return m_width;}
  quint16 height() const {return m_height;}
  const uchar* data() const {return m_data;}
  quint16 pad() const {return m_pad;}
  QRect insert(const FT_Bitmap& bmap);

  QMutex* m_mutex;
  quint16 m_width;
  quint16 m_height;
  uchar* m_data;
  quint16 m_pad;

  /// Represents a single level (a horizontal line) of the skyline/horizon/envelope.
  struct SkylineNode
  {
    /// The starting x-coordinate (leftmost).
    int x;

    /// The y-coordinate of the skyline level line.
    int y;

    /// The line width. The ending coordinate (inclusive) will be x+width-1.
    int width;
  };

  std::list<SkylineNode> m_skyLine;
  using NodePtr = std::list<SkylineNode>::const_iterator;


  QPoint findPositionForNewNodeBottomLeft(const QSize& s, NodePtr& bestNode) const;
  bool rectangleFits(const NodePtr& node, const QSize& s, int &y) const;
  void addSkylineLevel(const NodePtr& node, const QPoint &p, const QSize& s);
  /// Merges all skyline nodes that are at the same level.
  void mergeSkylines();
  void enlargeBin(uint w, uint h);
  void fillBlock(const QPoint& p, const QSize& s, const uchar* data);

};

using GlyphBinPtr = QSharedPointer<GlyphBin>;

namespace GL {

struct Mesh {
  VertexVector vertices;
  IndexVector indices;
  QRectF bbox;
};
}

namespace HB {

struct Text {
  QString string;
  hb_direction_t direction;
  hb_script_t script;
  QString lang;

  Text(const QString& d,
       hb_direction_t dir = HB_DIRECTION_LTR,
       hb_script_t s = HB_SCRIPT_LATIN,
       const QString l = "en_US")
    : string(d)
    , direction(dir)
    , script(s)
    , lang(l) {}
};

inline QPointF offset(const hb_glyph_position_t& pos) {
  return QPointF(pos.x_offset / 64., pos.y_offset / 64.);
}

inline QPointF advance(const hb_glyph_position_t& pos) {
  return QPointF(pos.x_advance / 64., pos.y_advance / 64.);
}

using FeatureVector = QVector<hb_feature_t>;

const hb_tag_t KernTag = HB_TAG('k', 'e', 'r', 'n'); // kerning operations
const hb_tag_t LigaTag = HB_TAG('l', 'i', 'g', 'a'); // standard ligature substitution
const hb_tag_t CligTag = HB_TAG('c', 'l', 'i', 'g'); // contextual ligature substitution

constexpr hb_feature_t LigatureOff = {LigaTag, 0, 0, std::numeric_limits<unsigned int>::max()};
constexpr hb_feature_t LigatureOn = {LigaTag, 1, 0, std::numeric_limits<unsigned int>::max()};
constexpr hb_feature_t KerningOff = {KernTag, 0, 0, std::numeric_limits<unsigned int>::max()};
constexpr hb_feature_t KerningOn = {KernTag, 1, 0, std::numeric_limits<unsigned int>::max()};
constexpr hb_feature_t CligOff = {CligTag, 0, 0, std::numeric_limits<unsigned int>::max()};
constexpr hb_feature_t CligOn = {CligTag, 1, 0, std::numeric_limits<unsigned int>::max()};

}

class Font {
  friend class GlyphManager;

public:

  ~Font();

private:

  using GlyphMap = QMap<quint32, Glyph*>;

  Font(const QString& family, quint16 size, TXT::Weight weight, bool italic, GlyphBinPtr bin, FT_Library lib);
  Font() = delete;
  Font(const Font&) = delete;
  Font& operator= (const Font&) = delete;

  const Glyph* getGlyph(quint32 codepoint, bool* newGlyph);
  GL::Mesh* shapeText(const HB::Text& txt, hb_buffer_t* buf, bool* newGlyphs);
  void setFeatures(const HB::FeatureVector& features);

  GlyphMap m_glyphs;
  GlyphBinPtr m_bin;
  FT_Face m_face;
  hb_font_t* m_font;
  HB::FeatureVector m_features;
};


class GlyphManager
{
public:

  GlyphManager(QMutex* mutex);
  ~GlyphManager();

  const uchar* data() const {return m_bin->data();}
  quint16 width() const {return m_bin->width();}
  quint16 height() const {return m_bin->height();}
  void setFont(TXT::Weight weight);
  GL::Mesh* shapeText(const HB::Text& txt, bool* newGlyphs);

private:

  static const bool italic = false;
  static const int pixelSize = 48;

  const QString m_family;

  using FontIndex = QMap<TXT::Weight, int>;
  using FontCollection = QVector<Font*>;

  FontIndex m_fontIndex;
  FontCollection m_fonts;
  GlyphBinPtr m_bin;
  int m_currentFont;
  FT_Library m_library;
  hb_buffer_t* m_buffer;

};

