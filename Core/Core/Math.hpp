#pragma once

#include "Tools/Types.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <format>
#include <iostream>

// Math types don't really need to be in the Math namespace, makes them easier to work with.
using Vec2 = Eigen::RowVector2f;
using Vec3 = Eigen::RowVector3f;
using Vec4 = Eigen::RowVector4f;

using Mat3 = Eigen::Matrix<float, 3, 3, Eigen::RowMajor>;
using Mat4 = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;

using Quat = Eigen::Quaternion<float>;

namespace Math
{
    using namespace Eigen;
    using namespace Eigen::numext;

    template <typename Type>
    constexpr Type PI{static_cast<Type>(EIGEN_PI)};

    template <typename Type>
    constexpr Type ToRadians(const Type degrees)
    {
        return degrees * PI<Type> / static_cast<Type>(180.0);
    }

    template <typename Type>
    constexpr Type Min(const Type a, const Type b)
    {
        return mini<Type>(a, b);
    }
    template <typename Type>
    constexpr Type Max(const Type a, const Type b)
    {
        return maxi<Type>(a, b);
    }

    template <typename Type>
    constexpr Type sin(const Type angle)
    {
        return sin<Type>(angle);
    }
    template <typename Type>
    constexpr Type cos(const Type angle)
    {
        return cos<Type>(angle);
    }

    using Transform2D = Transform<float, 2, Affine, RowMajor>;
    using Transform3D = Transform<float, 3, Affine, RowMajor>;

    template <typename MatrixType>
    constexpr MatrixType Identity()
    {
        return MatrixType::Identity();
    }
    template <typename MatrixType>
    constexpr MatrixType Transpose(const MatrixType& matrix)
    {
        return matrix.transpose();
    }

    template <typename Type>
    constexpr Vec3 Cross(const Type& a, const Type& b)
    {
        return a.cross(b);
    }
    template <typename Type>
    constexpr typename Type::Scalar Dot(const Type& a, const Type& b)
    {
        return a.dot(b);
    }

    inline Mat4 Translation(const Vec3& translation)
    {
        Mat4 matrix = Identity<Mat4>();
        matrix.row(3) = Vec4{translation.x(), translation.y(), translation.z(), 1.0f};
        return matrix;
    }
    inline Mat4 Rotation(const float radians, const Vec3& vector)
    {
        return Identity<Transform3D>().rotate(Eigen::AngleAxisf{radians, vector}).matrix();
    }
    inline Mat4 Scale(const Vec3& scale)
    {
        Mat4 matrix = Identity<Mat4>();
        matrix.row(0) *= scale.x();
        matrix.row(1) *= scale.y();
        matrix.row(2) *= scale.z();
        return matrix;
    }

    // Based on glm::perspective_RH_NO: https://github.com/g-truc/glm/blob/master/glm/ext/matrix_clip_space.inl
    template <typename T>
    Mat4 PerspectiveNO(T fovy, T aspect, T zNear, T zFar)
    {
        T const tan_half_fovy = std::tan(fovy / static_cast<T>(2));

        Mat4 result{};
        result(0, 0) = static_cast<T>(1) / (aspect * tan_half_fovy);
        result(1, 1) = static_cast<T>(1) / tan_half_fovy;
        result(2, 2) = -(zFar + zNear) / (zFar - zNear);
        result(2, 3) = -static_cast<T>(1);
        result(3, 2) = -(static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
        return result;
    }

    // Based on glm::perspective_RH_ZO: https://github.com/g-truc/glm/blob/master/glm/ext/matrix_clip_space.inl
    template <typename T>
    Mat4 PerspectiveZO(T fovy, T aspect, T zNear, T zFar)
    {
        T const tan_half_fovy = std::tan(fovy / static_cast<T>(2));

        Mat4 result{};
        result(0, 0) = static_cast<T>(1) / (aspect * tan_half_fovy);
        result(1, 1) = static_cast<T>(1) / tan_half_fovy;
        result(2, 2) = zFar / (zNear - zFar);
        result(2, 3) = -static_cast<T>(1);
        result(3, 2) = -(zFar * zNear) / (zFar - zNear);
        return result;
    }

    // Based on glm::lookAtRH: https://github.com/g-truc/glm/blob/master/glm/ext/matrix_transform.inl
    inline Mat4 LookAt(const Vec3& eye, const Vec3& forward, const Vec3& up)
    {
        const Vec3 s = Cross(forward, up).normalized();
        const Vec3 u = Cross(s, forward);

        Mat4 result = Identity<Mat4>();
        result.col(0) = Vec4{s.x(), s.y(), s.z(), -Dot(s, eye)};
        result.col(1) = Vec4{u.x(), u.y(), u.z(), -Dot(u, eye)};
        result.col(2) = Vec4{-forward.x(), -forward.y(), -forward.z(), Dot(forward, eye)};
        return result;
    }

    namespace Debug
    {
        inline void PrintMatrix(const Mat4& matrix)
        {
            for (size_t y = 0; y < 4; y++)
            {
                std::cout << "[ ";
                for (size_t x = 0; x < 4; x++)
                {
                    std::cout << std::format("{: >5.2f}", matrix(y, x));
                    std::cout << ", ";
                }
                std::cout << "]\n";
            }
        }
    } // namespace Debug
} // namespace Math
