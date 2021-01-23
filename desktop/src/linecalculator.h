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

  QOpenGLShaderProgram* m_program;

  QOpenGLBuffer m_outBuffer;

  struct _locations {
    int period;
    int viewArea;
    int numOutIndices;
    int indexOffset;
    int vertexOffset;
  } m_locations;

  struct Transform {
    GLfloat num;
    GLfloat ca;
    GLfloat sa;
    GLfloat pad;
    glm::vec2 first;
    glm::vec2 dir;
  };

  void createTransforms(VertexVector& tr, const Transform* source, int numIndices) const;

};

}
