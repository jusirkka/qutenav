#pragma once

#include <QOpenGLShaderProgram>

class Camera;

namespace S57 {
class TriangleData;
class SolidLineData;
class DashedLineData;
}

namespace GL {

class Shader {
public:

  const QOpenGLShaderProgram* prog() const {return m_program;}
  QOpenGLShaderProgram* prog() {return m_program;}

  virtual void setTransform(const QMatrix4x4& pvm) = 0;
  virtual void setDepth(GLfloat depth) = 0;
  virtual void initializePaint();

  virtual ~Shader();

protected:

  Shader() = default;

  struct Source {
    QOpenGLShader::ShaderTypeBit stype;
    QString fname;
  };

  Shader(const QVector<Source>& sources);

  QOpenGLShaderProgram* m_program;

};

class AreaShader: public Shader {

  friend class S57::TriangleData;

public:
  static AreaShader* instance();
  void setTransform(const QMatrix4x4& pvm) override;
  void setDepth(GLfloat depth) override;

private:
  AreaShader();

  struct _locations {
    int base_color;
    int m_pvm;
    int depth;
  } m_locations;

};

class SolidLineShader: public Shader {

  friend class S57::SolidLineData;

public:
  static SolidLineShader* instance();
  void setTransform(const QMatrix4x4& pvm) override;
  void setDepth(GLfloat depth) override;

  void setScreen(const Camera* cam);

private:
  SolidLineShader();

  struct _locations {
    int base_color;
    int m_pvm;
    int depth;
    int screenXMax;
    int screenYMax;
    int lineWidth;
  } m_locations;

  const float m_dots_per_mm_x;
  const float m_dots_per_mm_y;
};


class DashedLineShader: public Shader {

  friend class S57::DashedLineData;

public:
  static DashedLineShader* instance();
  void setTransform(const QMatrix4x4& pvm) override;
  void setDepth(GLfloat depth) override;

  void setScreen(const Camera* cam);

private:
  DashedLineShader();

  static constexpr uint linePatlen = 18;
  static constexpr uint linefactor = 1;

  struct _locations {
    int base_color;
    int m_pvm;
    int depth;
    int screenXMax;
    int screenYMax;
    int lineWidth;
    int pattern;
    int patlen;
    int factor;
  } m_locations;

  const float m_dots_per_mm_x;
  const float m_dots_per_mm_y;

};


}
