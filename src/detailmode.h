#pragma once

#include "camera.h"
#include "drawable.h"

class DetailMode: public QObject {

  Q_OBJECT

public:

  DetailMode(QObject* parent = nullptr);

  static DetailMode* RestoreState();
  static Camera* RestoreCamera();
  Camera* camera() {return m_camera;}
  const Camera* camera() const {return m_camera;}
  DrawableVector& drawables() {return m_drawables;}
  virtual DetailMode* largerScaleMode() const = 0;
  virtual DetailMode* smallerScaleMode() const = 0;
  virtual Camera* cloneCamera() const = 0;
  virtual ~DetailMode() = default;

  void saveState() const;

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
