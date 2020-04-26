#include "sdfunction.h"
#include <random>
#include <functional>
#include <iostream>
#include "GeoCore.hpp"
#include <QDebug>

using namespace SD;

vec3 Function::projectToSurface(const vec3 &q) const {
  return marchToSurface(q);
}

vec3 Function::projectToSurfaceOld(const vec3& q) const {
  // minimize length(q - p) s.t signedDistance(p) = 0

  // already on the surface: happens if the surface is flat
  if (std::abs(signedDistance(q)) < eps) return q;

  // place initial point on surface
  vec3 p1 = marchToSurface(q);

  vec3 p;
  do {
    p = p1;
    const vec3 k0 = glm::normalize(p - q);
    vec3 k1, k2;
    TangentSpace(k0, k1, k2);
    const vec3 J = gradient(p);
    const scalar j0 = glm::dot(k0, J);
    const scalar j1 = glm::dot(k1, J);
    const scalar j2 = glm::dot(k2, J);
    const scalar r = glm::length(p - q);
    const scalar w0 = - (signedDistance(p) / j0 + r * (1 / j0 / j0 - 1));
    const scalar w1 = r * j1 / j0;
    const scalar w2 = r * j2 / j0;
    p1 = p + mat3(k0, k1, k2) * vec3(w0, w1, w2);
  } while (glm::length(p1 - p) > eps);

  return p1;
}

vec3 Function::marchToSurface(const vec3& q) const {
  const vec3 p = q - signedDistance(q) * gradient(q);
  if (std::abs(signedDistance(p)) > eps) {
    qFatal("marchToSurface: failure");
  }
  if (std::abs(glm::dot(gradient(p), gradient(q)) - 1.) > eps) {
    qFatal("marchToSurface: gradient failure");
  }
  return p;
}

vec3 Function::randomSurfacePoint() const {
  std::default_random_engine generator;
  std::uniform_real_distribution<scalar> distribution(-10., 10.);
  auto dice = bind(distribution, generator);
  return marchToSurface(vec3(dice(), dice(), dice()));
}

scalar Function::minimalROC(const vec3& p) const {
  const mat3 h = hessian(p);
  const vec3 n = gradient(p);
  vec3 k1, k2;
  TangentSpace(n, k1, k2);
  const scalar a = glm::dot(k1, h * k1);
  const scalar b = glm::dot(k2, h * k2);
  const scalar c = glm::dot(k1, h * k2);

  const scalar s = .5 * std::abs(a + b);
  const scalar d = .5 * sqrt((a - b) * (a - b) + c * c);
  if (s + d < eps) {
    return 2 * max_radius; // flat surface
  }

  return 1. / (s + d);
}


// Lift 2D func to 3D
scalar Lift::signedDistance(const vec3& p) const {
  // FIXME: dist inside volume is not correct
  const scalar d2 = m_fun->signedDistance(vec2(p.x, p.y));
  const scalar dz = std::abs(p.z) - m_cut;
  if (dz <= 0) return d2;
  if (d2 <= 0) return dz;
  return sqrt(d2 * d2 + dz * dz);
}

vec3 Lift::gradient(const vec3& p) const {
  // FIXME: gradient inside volume is not correct
  const scalar d2 = m_fun->signedDistance(vec2(p.x, p.y));
  const vec2 g2 = m_fun->gradient(vec2(p.x, p.y));
  const scalar dz = std::abs(p.z) - m_cut;
  const scalar sz = p.z > 0 ? 1. : -1.;
  if (dz <= 0) return vec3(g2, 0.);
  if (d2 <= 0) return vec3(0., 0., sz);
  const scalar d = sqrt(d2 * d2 + dz * dz);
  const vec3 g(d2 * g2.x / d, d2 * g2.y / d, sz * dz / d);
  if (std::abs(glm::length(g) - 1.) > eps) {
    qFatal("Lift::gradient: not correct");
  }
  return g;
}

mat3 Lift::hessian(const vec3& p) const {
  // FIXME: hessian inside volume is not correct
  const mat3 h2 = m_fun->hessian(vec2(p.x, p.y));
  const scalar dz = std::abs(p.z) - m_cut;
  const scalar sz = p.z > 0 ? 1. : -1.;
  if (dz <= 0) return mat3(h2);
  const scalar d2 = m_fun->signedDistance(vec2(p.x, p.y));
  if (d2 <= 0) return mat3(0.);
  const vec2 g2 = m_fun->gradient(vec2(p.x, p.y));
  const scalar d3 = sqrt(d2 * d2 + dz * dz);
  mat3 h = mat3(h2) * d2 / d3;
  h[2][2] = 0;
  h += mat3(
        dz * dz * g2.x * g2.x, dz * dz * g2.x * g2.y, -d2 * g2.x * sz * dz,
        dz * dz * g2.x * g2.y, dz * dz * g2.y * g2.y, -d2 * g2.y * sz * dz,
        -d2 * g2.x * sz * dz, -d2 * g2.y * sz * dz, d2 * d2
        ) * (1 / (d3 * d3 * d3));
  const vec3 g = gradient(p);
  if (std::abs(glm::dot(h[0], g)) > eps || std::abs(glm::dot(h[1], g)) > eps || std::abs(glm::dot(h[2], g)) > eps) {
    qDebug() << glm::dot(h[0], g) << glm::dot(h[1], g) << glm::dot(h[2], g);
    qDebug() << h2[0].x * g2.x +  h2[0].y * g2.y << h2[1].x * g2.x +  h2[1].y * g2.y;
    qFatal("Lift::hessian: not correct");
  }
  return h;
}

// Shift
scalar Shift::signedDistance(const vec3& p) const {
  return m_fun->signedDistance(p) - m_shift;
}

vec3 Shift::gradient(const vec3& p) const {
  return m_fun->gradient(p);
}

mat3 Shift::hessian(const vec3& p) const {
  return m_fun->hessian(p);
}

// Scale
scalar Scale::signedDistance(const vec3& p) const {
  return m_fun->signedDistance(m_scale * p) / m_scale;
}

vec3 Scale::gradient(const vec3& p) const {
  return m_fun->gradient(m_scale * p);
}

mat3 Scale::hessian(const vec3& p) const {
  return m_scale * m_fun->hessian(m_scale * p);
}

// Rotate
Rotate::Rotate(const Function* target, const vec3& v, scalar degrees)
  : m_fun(target) {

  const scalar a = RADS_PER_DEG * degrees;
  const scalar c = cos(a);
  const scalar s = sin(a);

  const vec3 axis(glm::normalize(v));
  const vec3 temp((1.f - c) * axis);

  m_rot_t[0][0] = c + temp[0] * axis[0];
  m_rot_t[1][0] = temp[0] * axis[1] + s * axis[2];
  m_rot_t[2][0] = temp[0] * axis[2] - s * axis[1];

  m_rot_t[0][1] = temp[1] * axis[0] - s * axis[2];
  m_rot_t[1][1] = c + temp[1] * axis[1];
  m_rot_t[2][1] = temp[1] * axis[2] + s * axis[0];

  m_rot_t[0][2] = temp[2] * axis[0] + s * axis[1];
  m_rot_t[1][2] = temp[2] * axis[1] - s * axis[0];
  m_rot_t[2][2] = c + temp[2] * axis[2];
}

scalar Rotate::signedDistance(const vec3& p) const {
  return m_fun->signedDistance(m_rot_t * p);
}

vec3 Rotate::gradient(const vec3& p) const {
  const vec3 g = glm::transpose(m_rot_t) * m_fun->gradient(m_rot_t * p);
  if (std::abs(glm::length(g) - 1.) > eps) {
    qFatal("Rotate::gradient: not correct");
  }
  return g;
}

mat3 Rotate::hessian(const vec3& p) const {
  const mat3 h = glm::transpose(m_rot_t) * m_fun->hessian(m_rot_t * p) * m_rot_t;
  const vec3 g = gradient(p);
  if (std::abs(glm::dot(h[0], g)) > eps || std::abs(glm::dot(h[1], g)) > eps || std::abs(glm::dot(h[2], g)) > eps) {
    qFatal("Rotate::hessian: not correct");
  }
  return h;
}


// Translate
scalar Translate::signedDistance(const vec3& p) const {
  return m_fun->signedDistance(p - m_trans);
}

vec3 Translate::gradient(const vec3& p) const {
  return m_fun->gradient(p - m_trans);
}

mat3 Translate::hessian(const vec3& p) const {
  return m_fun->hessian(p - m_trans);
}

// Revolve
scalar Revolve::signedDistance(const vec3& p) const {
  return m_fun->signedDistance(vec2(sqrt(p.x * p.x + p.z * p.z), p.y));
}

vec3 Revolve::gradient(const vec3& p) const {
  const scalar r = sqrt(p.x * p.x + p.z * p.z);
  const vec2 n = m_fun->gradient(vec2(r, p.y));
  const vec3 g(p.x / r * n.x, n.y, p.z / r * n.x);
  if (std::abs(glm::length(g) - 1.) > eps) {
    qFatal("Revolve::gradient: not correct");
  }
  return g;
}

mat3 Revolve::hessian(const vec3& p) const {
  const scalar r = sqrt(p.x * p.x + p.z * p.z);
  const mat2 h = m_fun->hessian(vec2(r, p.y));
  const scalar h11 = h[0][0];
  const scalar h22 = h[1][1];
  const scalar h12 = h[0][1];
  const vec2 g = m_fun->gradient(vec2(r, p.y));
  const scalar g1 = g.x;
  const scalar s = p.x / r;
  const scalar c = p.z / r;

  const mat3 h3 = mat3(h11 * s * s + g1 * c * c / r, h12 * s, h11 * s * c - g1 * s * c / r,
                      h12 * s, h22, h12 * c,
                      h11 * s * c - g1 * s * c / r, h12 * c, h11 * c * c + g1 * s * s / r);

  const vec3 g3 = gradient(p);
  if (std::abs(glm::dot(h3[0], g3)) > eps || std::abs(glm::dot(h3[1], g3)) > eps || std::abs(glm::dot(h3[2], g3)) > eps) {
    qFatal("Revolve::hessian: not correct");
  }
  return h3;
}


// Unit sphere
scalar Sphere::signedDistance(const vec3& p) const {
  return glm::length(p) - 1;
}

vec3 Sphere::gradient(const vec3& p) const {
  return p / glm::length(p);
}

mat3 Sphere::hessian(const vec3 &p) const {
  const scalar d = glm::length(p);
  const vec3 n = glm::normalize(p);
  return mat3(1 - n.x * n.x, -n.x * n.y, -n.x * n.z,
                 -n.y * n.x, 1 - n.y * n.y, -n.y * n.z,
                 -n.z * n.x, -n.z * n.y, 1 - n.z * n.z) / d;
}

// Translate2D
scalar Translate2D::signedDistance(const vec2& p) const {
  return m_fun->signedDistance(p - m_trans);
}

vec2 Translate2D::gradient(const vec2& p) const {
  return m_fun->gradient(p - m_trans);
}

mat2 Translate2D::hessian(const vec2& p) const {
  return m_fun->hessian(p - m_trans);
}


// Unit circle
scalar Circle::signedDistance(const vec2& p) const {
  return glm::length(p) - 1;
}

vec2 Circle::gradient(const vec2& p) const {
  return p / glm::length(p);
}

mat2 Circle::hessian(const vec2 &p) const {
  const scalar d = glm::length(p);
  const vec2 n = glm::normalize(p);
  mat2 h =  mat2(1 - n.x * n.x, -n.x * n.y,
                 -n.y * n.x, 1 - n.y * n.y) / d;
  if (std::abs(h[0].x * p.x +  h[0].y * p.y) > 1.e-7 || std::abs(h[1].x * p.x +  h[1].y * p.y) > 1.e-7) {
    qDebug() << h[0].x * p.x +  h[0].y * p.y << h[1].x * p.x +  h[1].y * p.y;
    qFatal("Circle::hessian: not correct");
  }
  return h;
}


// Box
scalar Box::signedDistance(const vec2& p) const {
  const scalar dx = std::abs(p.x) - 1;
  const scalar dy = std::abs(p.y) - m_ymax;
  if (dx <= 0 || dy <= 0) {
    if (dy > 0) return dy;
    if (dx > 0) return dx;
    return glm::max(dx, dy);
  }
  return sqrt(dx * dx + dy * dy);
}

vec2 Box::gradient(const vec2& p) const {
  const scalar dx = std::abs(p.x) - 1;
  const scalar dy = std::abs(p.y) - m_ymax;
  const scalar sx = p.x > 0 ? 1.: -1.;
  const scalar sy = p.y > 0 ? 1.: -1.;
  if (dx <= 0 || dy <= 0) {
    if (dy > 0) return vec2(0., sy);
    if (dx > 0) return vec2(sx, 0.);
    if (dx > dy) return vec2(sx, 0.);
    return vec2(0., sy);
  }
  const scalar d = sqrt(dx * dx + dy * dy);
  return vec2(sx * dx / d, sy * dy / d);
}

mat2 Box::hessian(const vec2 &p) const {
  const scalar dx = std::abs(p.x) - 1;
  const scalar dy = std::abs(p.y) - m_ymax;
  const scalar sx = p.x > 0 ? 1.: -1.;
  const scalar sy = p.y > 0 ? 1.: -1.;
  if (dx <= 0 || dy <= 0) {
    return mat2(0.);
  }
  const scalar d = sqrt(dx * dx + dy * dy);
  const scalar d3 = d * d * d;
  mat2 h = mat2(dy * dy / d3, - sx * sy * dx * dy / d3,
                - sx * sy * dx * dy / d3, dx * dx / d3);
  vec2 g = gradient(p);
  if (std::abs(h[0].x * g.x +  h[0].y * g.y) > 1.e-7 || std::abs(h[1].x * g.x +  h[1].y * g.y) > 1.e-7) {
    qDebug() << h[0].x * g.x +  h[0].y * g.y << h[1].x * g.x +  h[1].y * g.y;
    qFatal("Box::hessian: not correct");
  }
  return h;
}
