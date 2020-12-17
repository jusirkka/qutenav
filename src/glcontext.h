#pragma once

class QOpenGLWindow;

namespace GL {

class Context {
public:
  Context() = default;

  static Context* instance();
  void makeCurrent();
  void doneCurrent();
  void initializeContext(QOpenGLWindow* w);

private:

  QOpenGLWindow* m_window;

};

} // namespace GL

