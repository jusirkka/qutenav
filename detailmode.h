#pragma once

#include "camera.h"
#include "drawable.h"

class DetailMode {
public:

  static DetailMode* RestoreState();
  Camera* camera() {return m_camera;}
  const Camera* camera() const {return m_camera;}
  DrawableVector& drawables() {return m_drawables;}
  virtual DetailMode* largerScaleMode(float wmm, float hmm) const = 0;
  virtual DetailMode* smallerScaleMode(float wmm, float hmm) const = 0;
  virtual ~DetailMode() = default;

  void saveState(float wmm, float hmm) const;

protected:

  virtual QString className() const = 0;

  Camera* m_camera;
  DrawableVector m_drawables;

};


class ModeError {
public:
  ModeError(QString msg): m_detail(std::move(msg)) {}
  const QString msg() const {return m_detail;}
private:
  QString m_detail;
};
