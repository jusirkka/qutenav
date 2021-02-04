#include "glyphmanager.h"
#include <fontconfig/fontconfig.h>
#include <QDebug>
#include "tiny_sdf.h"
#include "harfbuzz/hb-ft.h"
#include <QFontDatabase>
#include <QMutex>
#include <QMutexLocker>

GlyphBin::GlyphBin(QMutex *mutex)
  : m_mutex(mutex)
  , m_width(1024)
  , m_height(1024)
  , m_data(new uchar[m_width * m_height]())
  , m_pad(2)
{
  SkylineNode node;
  node.x = 0;
  node.y = 0;
  node.width = m_width;
  m_skyLine.push_back(node);
}

GlyphBin::~GlyphBin() {
  delete [] m_data;
}

QRect GlyphBin::insert(const FT_Bitmap &bmap) {
  QSize s(bmap.width, bmap.rows);
  if (s.isEmpty()) return QRect(0, 0, 0, 0);
  s.rheight() += 2 * m_pad;
  s.rwidth() += 2 * m_pad;
  NodePtr bestNode;
  QPoint p = findPositionForNewNodeBottomLeft(s, bestNode);

  while (bestNode == m_skyLine.cend()) {
    qDebug() << "enlarging ...";
    if (m_height < m_width) {
      enlargeBin(m_width, 2 * m_height);
    } else {
      enlargeBin(2 * m_width, m_height);
    }
    p = findPositionForNewNodeBottomLeft(s, bestNode);
  }
  addSkylineLevel(bestNode, p, s);

  TinySDF sdf(bmap, m_pad);

  {
    QMutexLocker lock(m_mutex);
    fillBlock(p, s, sdf.transform(8, .25));
  }

  // fillBlock(p, s, bmap.buffer);
  return QRect(p, s);
}

void GlyphBin::enlargeBin(uint w, uint h) {

  QMutexLocker lock(m_mutex);

  uchar* block = m_data;
  const uint block_w = m_width;
  const uint block_h = m_height;
  m_width = w;
  m_height = h;
  m_data = new uchar[m_width * m_height] ();
  fillBlock(QPoint(0, 0), QSize(block_w, block_h), block);
  delete [] block;
  // add node reflecting the gained space on the right
  if (m_width > block_w) {
    SkylineNode node;
    node.x = block_w;
    node.y = 0;
    node.width = m_width - block_w;
    m_skyLine.push_back(node);
  }
}

QPoint GlyphBin::findPositionForNewNodeBottomLeft(const QSize &s, NodePtr& bestNode) const {
  int bestHeight = std::numeric_limits<int>::max();
  bestNode = m_skyLine.end();
  // Used to break ties if there are nodes at the same level. Then pick the narrowest one.
  int bestWidth = std::numeric_limits<int>::max();
  QPoint p(0, 0);
  for (auto it = m_skyLine.begin(); it != m_skyLine.end(); ++it) {
    int y;
    if (rectangleFits(it, s, y)) {
      if (y + s.height() < bestHeight ||
          (y + s.height() == bestHeight &&
           it->width < bestWidth)) {
        bestNode = it;
        bestHeight = y + s.height();
        bestWidth = it->width;
        p.rx() = it->x;
        p.ry() = y;
      }
    }
  }
  return p;
}

void GlyphBin::addSkylineLevel(const NodePtr& nodePtr, const QPoint &p, const QSize& s) {
  SkylineNode newNode;
  newNode.x = p.x();
  newNode.y = p.y() + s.height();
  newNode.width = s.width();

  for (auto it = std::next(m_skyLine.insert(nodePtr, newNode)); it != m_skyLine.end(); ++it) {
    auto pr = std::prev(it);
    const int shrink = pr->x + pr->width - it->x;
    if (shrink > 0) {
      it->x += shrink;
      it->width -= shrink;

      if (it->width <= 0) {
        m_skyLine.erase(it);
        it = pr;
      } else break;
    } else break;
  }

  mergeSkylines();
}

bool GlyphBin::rectangleFits(const NodePtr& node, const QSize& s, int &y) const {
  const int x = node->x;
  if (x + s.width() > m_width) return false;
  int widthLeft = s.width();
  auto it = node;
  y = it->y;
  while (widthLeft > 0) {
    y = std::max(y, it->y);
    if (y + s.height() > m_height) return false;
    widthLeft -= it->width;
    ++it;
  }
  return true;
}

void GlyphBin::mergeSkylines() {
  auto it = m_skyLine.begin();
  auto nx = std::next(it);
  while (nx != m_skyLine.end()) {
    if (it->y == nx->y) {
      it->width += nx->width;
      m_skyLine.erase(nx);
    } else {
      ++it;
    }
    nx = std::next(it);
  }
}

void GlyphBin::fillBlock(const QPoint &p, const QSize &s, const uchar *src) {
  const uint rows = s.height();
  const uint width = s.width();
  const auto start = p.x() + p.y() * m_width;
  for (uint r = 0; r < rows; ++r) {
    memcpy(m_data + start + r * m_width, src + r * width, width);
  }
}


Font::Font(const QString &family,
           quint16 pixelSize,
           TXT::Weight weight,
           bool italic,
           GlyphBinPtr bin,
           FT_Library lib)
  : m_bin(bin)
  , m_features()
{

  int iweight = weight == TXT::Weight::Light ?
        FC_WEIGHT_LIGHT : weight == TXT::Weight::Medium ?
          FC_WEIGHT_MEDIUM : FC_WEIGHT_BOLD;

  int slant = italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN;

  FcInit();
  FcPattern* pattern = FcPatternCreate();
  FcPatternAddDouble(pattern, FC_SIZE, pixelSize);
  FcPatternAddInteger(pattern, FC_WEIGHT, iweight);
  FcPatternAddInteger(pattern, FC_SLANT, slant);
  FcPatternAddString(pattern, FC_FAMILY,
                     reinterpret_cast<const FcChar8*>(family.toUtf8().constData()));
  FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
  FcDefaultSubstitute(pattern);
  FcResult result;
  FcPattern *match = FcFontMatch(nullptr, pattern, &result);
  FcPatternDestroy(pattern);

  if (result != FcResultMatch) {
    FontNotFound e;
    if (result == FcResultNoMatch) {
      e.reason = "No match";
    } else if (result == FcResultTypeMismatch) {
      e.reason = "Type mismatch";
    } else if (result == FcResultNoId) {
      e.reason = "No id";
    } else if (result == FcResultOutOfMemory) {
      e.reason = "Out of memory";
    }
    e.suggestion = false;

    throw e;
  }

  FcChar8* fname;
  int index;
  FcPatternGetString(match, FC_FILE, 0, &fname);
  FcPatternGetInteger(match, FC_INDEX, 0, &index);

  FT_New_Face(lib, reinterpret_cast<const char*>(fname), index, &m_face);

  FcPatternDestroy(match);

  FT_Select_Charmap(m_face, FT_ENCODING_UNICODE);
  FT_Set_Char_Size(m_face, 0, pixelSize * 64,
                   dots_per_inch_x,
                   dots_per_inch_y);

  m_font = hb_ft_font_create(m_face, nullptr);
}

Font::~Font() {
  qDeleteAll(m_glyphs.values());
  FT_Done_Face(m_face);
  hb_font_destroy(m_font);
}

const Glyph* Font::getGlyph(quint32 codepoint, bool* newGlyph) {
  if (m_glyphs.contains(codepoint)) {
    *newGlyph = false;
    return m_glyphs[codepoint];
  }


  FT_UInt index = FT_Get_Char_Index(m_face, codepoint);
  auto err = FT_Load_Glyph(m_face, index, FT_LOAD_RENDER);
  if (err) {
    qFatal("FT_Load_Glyph failed: %d", err);
  }
  assert(m_face->glyph->format == FT_GLYPH_FORMAT_BITMAP);
  QRect r = m_bin->insert(m_face->glyph->bitmap);

  auto glyph = new Glyph;

  auto metrics = m_face->glyph->metrics;

  // unpadded bounding box upper left corner
  QPoint offset(metrics.horiBearingX / 64,
                metrics.horiBearingY / 64);
  // padding adjustment
  glyph->m_offset = offset + QPoint(-m_bin->pad(), m_bin->pad());

  // bounding box size (with padding)
  glyph->m_size = r.size();

  glyph->m_texUL = QPointF(r.x(), r.y());
  glyph->m_texLR = glyph->m_texUL + QPointF(r.width(),
                                            r.height());

  m_glyphs[codepoint] = glyph;
  *newGlyph = true;
  return glyph;
}

GL::Mesh* Font::shapeText(const HB::Text &txt, hb_buffer_t *buf, bool* newGlyphs) {
  hb_buffer_reset(buf);

  hb_buffer_set_direction(buf, txt.direction);
  hb_buffer_set_script(buf, txt.script);

  const QByteArray lang = txt.lang.toUtf8();
  hb_buffer_set_language(buf, hb_language_from_string(lang.constData(), lang.size()));

  // qDebug() << "Language:" << QString::fromUtf8(hb_language_to_string(hb_buffer_get_language(buf)));

  const QByteArray data = txt.string.toUtf8();
  hb_buffer_add_utf8(buf, data.constData(), data.length(), 0, data.length());

  uint glyphCount;
  const hb_glyph_info_t *glyphInfo = hb_buffer_get_glyph_infos(buf, &glyphCount);
  // save codepoints: hb_shape modifies them to glyph indices
  QVector<quint32> codepoints;
  for (uint i = 0; i < glyphCount; i++) codepoints << glyphInfo[i].codepoint;

  // harfbuzz shaping
  hb_shape(m_font, buf, m_features.constData(), m_features.size());

  const hb_glyph_position_t *glyphPos = hb_buffer_get_glyph_positions(buf, &glyphCount);

  QPointF pen(0, 0);
  const QVector<GLuint> triangles {0, 1, 2, 0, 2, 3};
  uint ioffset = 0;
  auto mesh = new GL::Mesh;

  *newGlyphs = false;
  float ymin = 1.e15;
  float ymax = -1.e15;
  for(uint i = 0; i < glyphCount; ++i) {
    bool newGlyph;
    auto glyph = getGlyph(codepoints[i], &newGlyph);
    *newGlyphs = *newGlyphs || newGlyph;

    // upper left
    const QPointF p0 = pen + HB::offset(glyphPos[i]) + glyph->offset();
    // lower right
    const QPointF p1 = p0 + QPointF(glyph->size().width(), - glyph->size().height());

    const QPointF t0 = glyph->texUL();
    const QPointF t1 = glyph->texLR();

    mesh->vertices << p0.x() << p0.y() << t0.x() << t0.y();
    mesh->vertices << p1.x() << p0.y() << t1.x() << t0.y();
    mesh->vertices << p1.x() << p1.y() << t1.x() << t1.y();
    mesh->vertices << p0.x() << p1.y() << t0.x() << t1.y();

    for (GLuint t: triangles) {
      mesh->indices << ioffset + t;
    }
    ioffset += 4;

    pen += HB::advance(glyphPos[i]);

    if (p0.y() > ymax) ymax = p0.y();
    if (p1.y() < ymin) ymin = p1.y();
  }
  const float xmin = mesh->vertices.first();
  const float xmax = mesh->vertices[mesh->vertices.size() - 8];

  mesh->bbox = QRectF(QPointF(xmin, ymin), QSizeF(xmax - xmin, ymax - ymin));

  return mesh;
}

void Font::setFeatures(const HB::FeatureVector &features) {
  m_features = features;
}


GlyphManager::GlyphManager(QMutex* mutex)
  : m_family(QFontDatabase::systemFont(QFontDatabase::GeneralFont).family())
  , m_bin(new GlyphBin(mutex))
  , m_currentFont(-1)
{
  FT_Init_FreeType(&m_library);
  m_buffer = hb_buffer_create();
  hb_buffer_allocation_successful(m_buffer);
}

void GlyphManager::setFont(TXT::Weight weight) {

  if (m_fontIndex.contains(weight)) {
    m_currentFont = m_fontIndex[weight];
    return;
  }
  try {
    auto font = new Font(m_family, pixelSize, weight, italic, m_bin, m_library);
    font->setFeatures({HB::CligOn, HB::KerningOn, HB::LigatureOn});
    m_currentFont = m_fonts.size();
    m_fontIndex[weight] = m_currentFont;
    m_fonts.append(font);
  } catch (FontNotFound& e) {
    throw e;
  };
}

GL::Mesh* GlyphManager::shapeText(const HB::Text &txt, bool *newGlyphs) {
  if (m_currentFont < 0) return nullptr;
  return m_fonts[m_currentFont]->shapeText(txt, m_buffer, newGlyphs);
}

GlyphManager::~GlyphManager() {
  qDeleteAll(m_fonts);
  FT_Done_FreeType(m_library);
  hb_buffer_destroy(m_buffer);
}


