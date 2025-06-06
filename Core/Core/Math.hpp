#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

using Vec2 = Eigen::RowVector2f;
using Vec3 = Eigen::RowVector3f;
using Vec4 = Eigen::RowVector4f;

using Mat4 = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;

using Quat = Eigen::Quaternion<float>;

using Transform2D = Eigen::Transform<float, 2, Eigen::Affine, Eigen::RowMajor>;
using Transform3D = Eigen::Transform<float, 3, Eigen::Affine, Eigen::RowMajor>;

template <typename Type> constexpr Vec3 Cross(const Type& a, const Type& b) { return a.cross(b); }
template <typename Type> constexpr Type::Scalar Dot(const Type& a, const Type& b) { return a.dot(b); }

inline Mat4 Translation(const Vec3& translation)
{
    Mat4 matrix = Mat4::Identity();
    matrix.row(3) = Vec4{translation.x(), translation.y(), translation.z(), 1.0f};
    return matrix;
}
inline Mat4 Rotation(const float radians, const Vec3& vector)
{
    Transform3D transform = Transform3D::Identity();
    transform.rotate(Eigen::AngleAxisf{radians, vector});
    return transform.matrix();
}
inline Mat4 Scale(const Vec3& scale) { return Transform3D{}.scale(scale).matrix(); }

inline float ToRadians(const float degrees) { return degrees * std::numbers::pi_v<float> / 180.0f; }

// Based on glm::perspective_RH_NO: https://github.com/g-truc/glm/blob/master/glm/ext/matrix_clip_space.inl
template <typename T> Mat4 Perspective(T fovy, T aspect, T zNear, T zFar)
{
    T const tan_half_fovy = tan(fovy / static_cast<T>(2));

    Mat4 result{};
    result(0, 0) = static_cast<T>(1) / (aspect * tan_half_fovy);
    result(1, 1) = static_cast<T>(1) / (tan_half_fovy);
    result(2, 2) = -(zFar + zNear) / (zFar - zNear);
    result(2, 3) = -static_cast<T>(1);
    result(3, 2) = -(static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
    return result;
}

// Based on glm::lookAtRH: https://github.com/g-truc/glm/blob/master/glm/ext/matrix_transform.inl
inline Mat4 LookAt(const Vec3& eye, const Vec3& forward, const Vec3& up)
{
    const Vec3 s(Cross(forward, up).normalized());
    const Vec3 u(Cross(s, forward));

    Mat4 result = Mat4::Identity();
    result.col(0) = Vec4{s.x(), s.y(), s.z(), -Dot(s, eye)};
    result.col(1) = Vec4{u.x(), u.y(), u.z(), -Dot(u, eye)};
    result.col(2) = Vec4{-forward.x(), -forward.y(), -forward.z(), Dot(forward, eye)};
    return result;
}