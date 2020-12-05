#pragma once

#include <QOpenGLShaderProgram>

class Camera;

namespace S57 {
class TriangleData;
class SolidLineData;
class DashedLineData;
class TextElemData;
class RasterSymbolElemData;
}

namespace GL {

class Shader {
public:

  const QOpenGLShaderProgram* prog() const {return m_program;}
  QOpenGLShaderProgram* prog() {return m_program;}
  void setDepth(int prio);

  virtual void setGlobals(const Camera* cam, const QPointF& tr) = 0;
  virtual void initializePaint();

  virtual ~Shader();

protected:

  Shader() = default;

  struct Source {
    QOpenGLShader::ShaderTypeBit stype;
    QString fname;
  };

  Shader(const QVector<Source>& sources, GLfloat ds);

  QOpenGLShaderProgram* m_program;

  GLfloat m_depthShift;
  int m_depth;

};

class AreaShader: public Shader {

  friend class S57::TriangleData;

public:
  static AreaShader* instance();
  void setGlobals(const Camera* cam, const QPointF& tr) override;

private:
  AreaShader();

  struct _locations {
    int m_pv;
    int tr;
    int base_color;
  } m_locations;

};

class SolidLineShader: public Shader {

  friend class S57::SolidLineData;

public:
  static SolidLineShader* instance();
  void setGlobals(const Camera* cam, const QPointF& t0) override;

private:
  SolidLineShader();

  struct _locations {
    int m_pv;
    int tr;
    int windowScale;
    int lineWidth;
    int base_color;
  } m_locations;

  const float m_dots_per_mm_y;

};


class DashedLineShader: public Shader {

  friend class S57::DashedLineData;

public:
  static DashedLineShader* instance();
  void setGlobals(const Camera* cam, const QPointF& t0) override;

private:
  DashedLineShader();

  static constexpr uint linePatlen = 18;
  static constexpr uint linefactor = 1;

  struct _locations {
    int m_pv;
    int tr;
    int windowScale;
    int lineWidth;
    int base_color;
    int pattern;
    int patlen;
    int factor;
  } m_locations;

  const float m_dots_per_mm_y;

};

class TextShader: public Shader {

  friend class S57::TextElemData;

public:
  static TextShader* instance();
  void setGlobals(const Camera* cam, const QPointF& t0) override;
  void initializePaint() override;

private:
  TextShader();

  struct _locations {
    int m_pv;
    int tr;
    int w_atlas;
    int h_atlas;
    int windowScale;
    int textScale;
    int pivot;
    int pivotShift;
    int atlas;
    int base_color;
  } m_locations;

  const float m_dots_per_mm_y;

};

class RasterSymbolShader: public Shader {

  friend class S57::RasterSymbolElemData;

public:
  static RasterSymbolShader* instance();
  void setGlobals(const Camera* cam, const QPointF& t0) override;
  void initializePaint() override;

private:
  RasterSymbolShader();

  struct _locations {
    int m_pv;
    int tr;
    int windowScale;
    int pivot;
    int pivotShift;
    int atlas;
  } m_locations;

  const float m_dots_per_mm_y;

};


}
