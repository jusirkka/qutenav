#pragma once

#include "precision.h"

namespace SD {


class Function {
public:

  static scalar constexpr max_radius = 10000.;
  vec3 projectToSurface(const vec3& q) const;
  vec3 projectToSurfaceOld(const vec3& q) const;
  vec3 marchToSurface(const vec3& q) const;
  vec3 randomSurfacePoint() const;
  scalar minimalROC(const vec3& p) const;
  virtual scalar signedDistance(const vec3& p) const = 0;
  virtual vec3 gradient(const vec3& p) const = 0;
  virtual mat3 hessian(const vec3& p) const = 0;

  virtual ~Function() = default;

  constexpr static scalar eps = 1.e-7;
  constexpr static scalar RADS_PER_DEG =  0.01745329251994329577;

};

class Function2D {
public:

  virtual vec2 gradient(const vec2& p) const = 0;
  virtual scalar signedDistance(const vec2& p) const = 0;
  virtual mat2 hessian(const vec2& p) const = 0;
  virtual ~Function2D() = default;

};



class Lift: public Function {
public:
  // extrude in z direction and cut at +-a
  Lift(const Function2D* target, scalar a)
    : m_fun(target)
    , m_cut(a) {}
  virtual ~Lift() {delete m_fun;}

  scalar signedDistance(const vec3& p) const override;
  vec3 gradient(const vec3& p) const override;
  mat3 hessian(const vec3& p) const override;

private:

  Lift(Lift&);
  Lift& operator=(const Lift&);

  const Function2D* m_fun;
  scalar m_cut;
};


class Shift: public Function {
public:
  Shift(const Function* target, scalar s)
    : m_fun(target)
    , m_shift(s) {}

  ~Shift() {delete m_fun;}

  scalar signedDistance(const vec3& p) const override;
  vec3 gradient(const vec3& p) const override;
  mat3 hessian(const vec3& p) const override;

private:

  Shift(Shift&);
  Shift& operator=(const Shift&);

  const Function* m_fun;
  const scalar m_shift;

};



class Scale: public Function {
public:
  Scale(const Function* target, scalar s)
    : m_fun(target)
    , m_scale(s) {}

  ~Scale() {delete m_fun;}

  scalar signedDistance(const vec3& p) const override;
  vec3 gradient(const vec3& p) const override;
  mat3 hessian(const vec3& p) const override;

private:

  Scale();
  Scale(Scale&);
  Scale& operator=(const Scale&);

  const Function* m_fun;
  const scalar m_scale;

};


class Rotate: public Function {
public:

  Rotate(const Function* target, const vec3& v, scalar degrees);

  ~Rotate() {delete m_fun;}

  scalar signedDistance(const vec3& p) const override;
  vec3 gradient(const vec3& p) const override;
  mat3 hessian(const vec3& p) const override;

private:

  Rotate();
  Rotate(Rotate&);
  Rotate& operator=(const Rotate&);

  const Function* m_fun;
  mat3 m_rot_t;


};

class Translate: public Function {
public:
  Translate(const Function* target, const vec3& t)
    : m_fun(target)
    , m_trans(t) {}

  ~Translate() {delete m_fun;}

  scalar signedDistance(const vec3& p) const override;
  vec3 gradient(const vec3& p) const override;
  mat3 hessian(const vec3& p) const override;

private:

  Translate();
  Translate(Translate&);
  Translate& operator=(const Translate&);

  const Function* m_fun;
  const vec3 m_trans;


};

class Revolve: public Function {
public:
  Revolve(const Function2D* target): m_fun(target) {}

  ~Revolve() {delete m_fun;}

  scalar signedDistance(const vec3& p) const override;
  vec3 gradient(const vec3& p) const override;
  mat3 hessian(const vec3& p) const override;

private:

  Revolve() = delete;
  Revolve(Revolve&) = delete;
  Revolve& operator=(const Revolve&) = delete;

  const Function2D* m_fun;

};

class Translate2D: public Function2D {
public:
  Translate2D(const Function2D* target, const vec2& t)
    : m_fun(target)
    , m_trans(t) {}

  ~Translate2D() {delete m_fun;}

  scalar signedDistance(const vec2& p) const override;
  vec2 gradient(const vec2& p) const override;
  mat2 hessian(const vec2& p) const override;

private:

  Translate2D() = delete;
  Translate2D(Translate2D&) = delete;
  Translate2D& operator=(const Translate2D&) = delete;

  const Function2D* m_fun;
  const vec2 m_trans;
};


class Sphere: public Function {
public:
  Sphere() = default;

  scalar signedDistance(const vec3& p) const override;
  vec3 gradient(const vec3& p) const override;
  mat3 hessian(const vec3& p) const override;

private:

  Sphere(Sphere&);
  Sphere& operator=(const Sphere&);

};

class Box: public Function2D {
public:

  Box(scalar y): m_ymax(std::abs(y)) {}

  scalar signedDistance(const vec2& p) const override;
  vec2 gradient(const vec2& p) const override;
  mat2 hessian(const vec2& p) const override;

private:

  Box();
  Box(Box&);
  Box& operator=(const Box&);

  scalar m_ymax;

};

class Circle: public Function2D {
public:
  Circle() = default;

  scalar signedDistance(const vec2& p) const override;
  vec2 gradient(const vec2& p) const override;
  mat2 hessian(const vec2& p) const override;

private:

  Circle(Circle&);
  Circle& operator=(const Circle&);

};

}

