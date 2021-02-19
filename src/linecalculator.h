#pragma once

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include "types.h"

namespace GL {

class LineCalculator {
public:

  static LineCalculator* instance();

  struct BufferData {
    QOpenGLBuffer buffer;
    GLsizei offset;
    GLsizei count;
  };

  void calculate(VertexVector& transforms, GLfloat period, const QRectF& va,
                 BufferData& vertices, BufferData& indices);

  ~LineCalculator();

private:

  const int compVertexBufferInBinding = 0;
  const int compIndexBufferInBinding = 1;
  const int compVertexBufferOutBinding = 2;

  LineCalculator();

};

}
