#pragma once

#include "Tools/Types.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <format>
#include <iostream>
#include <numbers>

// Math types don't really need to be in the Math namespace, makes them easier to work with.
using float2 = Eigen::RowVector2f;
using float3 = Eigen::RowVector3f;
using float4 = Eigen::RowVector4f;

template <size Rows, size Columns>
using Matrix = Eigen::Matrix<float, Rows, Columns, Eigen::RowMajor>;
using Matrix3 = Matrix<3, 3>;
using Matrix4 = Matrix<4, 4>;

using Quat = Eigen::Quaternion<float>;

namespace Math
{
    template <typename Type = float>
    constexpr Type PI{std::numbers::pi_v<Type>};

    static inline const float3 RIGHT{1.0f, 0.0f, 0.0f};
    static inline const float3 UP{0.0f, 1.0f, 0.0f};
    static inline const float3 FORWARD{0.0f, 0.0f, -1.0f};

    template <typename Type>
    constexpr Type ToRadians(const Type degrees)
    {
        return degrees * PI<Type> / static_cast<Type>(180.0);
    }

    template <typename Type>
    constexpr Type Min(const Type a, const Type b)
    {
        return Eigen::numext::mini<Type>(a, b);
    }
    template <typename Type>
    constexpr Type Max(const Type a, const Type b)
    {
        return Eigen::numext::maxi<Type>(a, b);
    }
    template <typename Type>
    constexpr Type Clamp(const Type value, const Type min, const Type max)
    {
        return Min<Type>(Max<Type>(value, min), max);
    }

    template <typename Type>
    constexpr Type Sin(const Type angle)
    {
        return Eigen::numext::sin<Type>(angle);
    }
    template <typename Type>
    constexpr Type Cos(const Type angle)
    {
        return Eigen::numext::cos<Type>(angle);
    }

    using Transform2D = Eigen::Transform<float, 2, Eigen::Affine, Eigen::RowMajor>;
    using Transform3D = Eigen::Transform<float, 3, Eigen::Affine, Eigen::RowMajor>;

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
    template <typename MatrixType>
    constexpr MatrixType Inverse(const MatrixType& matrix)
    {
        return matrix.inverse();
    }

    template <typename Type>
    constexpr float3 Cross(const Type& a, const Type& b)
    {
        return a.cross(b);
    }
    template <typename Type>
    constexpr typename Type::Scalar Dot(const Type& a, const Type& b)
    {
        return a.dot(b);
    }

    inline Matrix4 Translation(const float3& translation)
    {
        auto matrix = Identity<Matrix4>();
        matrix.row(3) = float4{translation.x(), translation.y(), translation.z(), 1.0f};
        return matrix;
    }
    inline Matrix4 Rotation(const float radians, const float3& vector)
    {
        return Identity<Transform3D>().rotate(Eigen::AngleAxisf{radians, vector}).matrix();
    }
    inline Matrix4 Rotation(const Quat& quat) { return Identity<Transform3D>().rotate(quat).matrix(); }
    inline Matrix4 Scale(const float3& scale)
    {
        Matrix4 matrix = Identity<Matrix4>();
        matrix.row(0) *= scale.x();
        matrix.row(1) *= scale.y();
        matrix.row(2) *= scale.z();
        return matrix;
    }

    inline float3 TransformPoint(const float3& point, const Matrix4& matrix)
    {
        float4 temp;
        temp << point, 1.0f;
        return (temp * matrix).head<3>();
    }
    inline float3 TransformVector(const float3& vector, const Matrix4& matrix)
    {
        float4 temp;
        temp << vector, 0.0f;
        return (temp * matrix).head<3>();
    }

    // Based on glm::perspective_RH_NO: https://github.com/g-truc/glm/blob/master/glm/ext/matrix_clip_space.inl
    template <typename Type>
    Matrix4 PerspectiveNO(const Type fov_y, const Type aspect, const Type near, const Type far)
    {
        const Type tan_half_fov_y = Eigen::numext::tan<Type>(fov_y / static_cast<Type>(2));

        Matrix4 result = Matrix4::Zero();
        result(0, 0) = static_cast<Type>(1) / (aspect * tan_half_fov_y);
        result(1, 1) = static_cast<Type>(1) / tan_half_fov_y;
        result(2, 2) = -(far + near) / (far - near);
        result(2, 3) = -static_cast<Type>(1);
        result(3, 2) = -(static_cast<Type>(2) * far * near) / (far - near);
        return result;
    }

    // Based on glm::perspective_RH_ZO: https://github.com/g-truc/glm/blob/master/glm/ext/matrix_clip_space.inl
    template <typename Type>
    Matrix4 PerspectiveZO(const Type fov_y, const Type aspect, const Type near, const Type far)
    {
        const Type tan_half_fov_y = Eigen::numext::tan<Type>(fov_y / static_cast<Type>(2));

        Matrix4 result = Matrix4::Zero();
        result(0, 0) = static_cast<Type>(1) / (aspect * tan_half_fov_y);
        result(1, 1) = static_cast<Type>(1) / tan_half_fov_y;
        result(2, 2) = far / (near - far);
        result(2, 3) = -static_cast<Type>(1);
        result(3, 2) = -(far * near) / (far - near);
        return result;
    }

    // Based on glm::lookAtRH: https://github.com/g-truc/glm/blob/master/glm/ext/matrix_transform.inl
    inline Matrix4 LookAt(const float3& eye, const float3& forward, const float3& up)
    {
        const float3 s = Cross(forward, up).normalized();
        const float3 u = Cross(s, forward);

        Matrix4 result = Identity<Matrix4>();
        result.col(0) = float4{s.x(), s.y(), s.z(), -Dot(s, eye)};
        result.col(1) = float4{u.x(), u.y(), u.z(), -Dot(u, eye)};
        result.col(2) = float4{-forward.x(), -forward.y(), -forward.z(), Dot(forward, eye)};
        return result;
    }

    namespace Debug
    {
        inline void PrintMatrix(const Matrix4& matrix)
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
