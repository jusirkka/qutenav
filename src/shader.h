/* -*- coding: utf-8-unix -*-
 *
 * File: src/shader.h
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

#include <QOpenGLShaderProgram>

class Camera;
class WGS84Point;

namespace S57 {
class TriangleData;
class LineElemData;
class LineArrayData;
class SegmentArrayData;
class TextElemData;
class RasterHelper;
class VectorHelper;
}


namespace GL {

class Shader {
public:

  const QOpenGLShaderProgram* prog() const {return m_program;}
  QOpenGLShaderProgram* prog() {return m_program;}
  void setDepth(int chartPrio, int prio);

  virtual void setGlobals(const Camera* cam, const QMatrix4x4& mt) = 0;
  virtual void initializePaint();

  virtual ~Shader();

protected:

  Shader() = default;

  struct Source {
    QOpenGLShader::ShaderTypeBit stype;
    QString fname;
  };

  Shader(const QVector<Source>& sources, int ds);

  QOpenGLShaderProgram* m_program;

  const int m_depthShift;
  int m_depth;

};

class AreaShader: public Shader {

  friend class S57::TriangleData;

public:
  static AreaShader* instance();
  void setGlobals(const Camera* cam, const QMatrix4x4& mt) override;

private:
  AreaShader();

  struct _locations {
    int m_p;
    int m_model;
    int base_color;
  } m_locations;

};


class LineElemShader: public Shader {

  friend class S57::LineElemData;

public:
  static LineElemShader* instance();
  void setGlobals(const Camera* cam, const QMatrix4x4& mt) override;

private:
  LineElemShader();

  static constexpr uint linePatlen = 18;
  static constexpr uint linefactor = 1;

  struct _locations {
    int m_p;
    int m_model;
    int windowScale;
    int lineWidth;
    int base_color;
    int pattern;
    int vertexOffset;
    int indexOffset;
  } m_locations;
};


class LineArrayShader: public Shader {

  friend class S57::LineArrayData;

public:
  static LineArrayShader* instance();
  void setGlobals(const Camera* cam, const QMatrix4x4& mt) override;

private:
  LineArrayShader();

  static constexpr uint linePatlen = 18;
  static constexpr uint linefactor = 1;

  struct _locations {
    int m_p;
    int m_model;
    int windowScale;
    int lineWidth;
    int base_color;
    int pattern;
    int vertexOffset;
  } m_locations;
};

class SegmentArrayShader: public Shader {

  friend class S57::SegmentArrayData;

public:
  static SegmentArrayShader* instance();
  void setGlobals(const Camera* cam, const QMatrix4x4& mt) override;

private:
  SegmentArrayShader();

  static constexpr uint linePatlen = 18;
  static constexpr uint linefactor = 1;

  struct _locations {
    int m_p;
    int m_model;
    int windowScale;
    int lineWidth;
    int base_color;
    int pattern;
    int vertexOffset;
  } m_locations;
};

class TextShader: public Shader {

  friend class S57::TextElemData;

public:
  static TextShader* instance();
  void setGlobals(const Camera* cam, const QMatrix4x4& mt) override;
  void initializePaint() override;

private:
  TextShader();

  struct _locations {
    int m_p;
    int m_model;
    int w_atlas;
    int h_atlas;
    int windowScale;
    int base_color;
  } m_locations;
};

class RasterSymbolShader: public Shader {

  friend class S57::RasterHelper;

public:
  static RasterSymbolShader* instance();
  void setGlobals(const Camera* cam, const QMatrix4x4& mt) override;
  void initializePaint() override;

private:
  RasterSymbolShader();

  struct _locations {
    int m_p;
    int m_model;
    int windowScale;
    int offset;
  } m_locations;
};

class VectorSymbolShader: public Shader {

  friend class S57::VectorHelper;

public:
  static VectorSymbolShader* instance();
  void setGlobals(const Camera* cam, const QMatrix4x4& mt) override;
  void initializePaint() override;

private:
  VectorSymbolShader();

  struct _locations {
    int m_p;
    int m_model;
    int base_color;
    int windowScale;
  } m_locations;
};

class TextureShader: public Shader {


public:
  static TextureShader* instance();
  void setGlobals(const Camera* cam, const QMatrix4x4& mt) override;
  void initializePaint() override;
  void setUniforms(const Camera *cam,
                   const WGS84Point& ref,
                   const QRectF &va);

private:
  TextureShader();

  struct _locations {
    int m_pv;
    int scale_tex;
    int scale_vertex;
    int tr_tex;
    int tr_vertex;
  } m_locations;
};


}
