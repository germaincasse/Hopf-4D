#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "hopf/math/Mat5.h"
#include "hopf/math/Rotor4.h"
#include "hopf/math/Vec4.h"

using namespace hopf::math;
using Catch::Approx;

TEST_CASE("Vec4 arithmetic", "[math][vec4]") {
    constexpr Vec4 a{1.f, 2.f, 3.f, 4.f};
    constexpr Vec4 b{0.5f, 1.f, -1.f, 2.f};

    REQUIRE(dot(a, b) == Approx(0.5f + 2.f - 3.f + 8.f));

    const Vec4 sum = a + b;
    REQUIRE(sum.x == Approx(1.5f));
    REQUIRE(sum.w == Approx(6.f));

    const Vec4 scaled = 2.f * a;
    REQUIRE(scaled.z == Approx(6.f));

    const Vec4 unit = normalize({3.f, 0.f, 4.f, 0.f});
    REQUIRE(length(unit) == Approx(1.f));
}

TEST_CASE("Mat5 identity is neutral", "[math][mat5]") {
    const Mat5 I = Mat5::identity();
    const Vec4 v{1.f, -2.f, 3.f, 0.5f};
    const Vec4 r = I.transformPoint(v);
    REQUIRE(r.x == Approx(v.x));
    REQUIRE(r.y == Approx(v.y));
    REQUIRE(r.z == Approx(v.z));
    REQUIRE(r.w == Approx(v.w));
}

TEST_CASE("Mat5 translation moves points but not directions", "[math][mat5]") {
    const Mat5 T = translate4({10.f, -2.f, 0.f, 5.f});
    const Vec4 p = T.transformPoint({1.f, 1.f, 1.f, 1.f});
    REQUIRE(p.x == Approx(11.f));
    REQUIRE(p.w == Approx(6.f));

    const Vec4 d = T.transformDirection({1.f, 0.f, 0.f, 0.f});
    REQUIRE(d.x == Approx(1.f));
    REQUIRE(d.y == Approx(0.f));
    REQUIRE(d.w == Approx(0.f));
}

TEST_CASE("Plane rotation preserves orthogonal axes", "[math][mat5]") {
    const Mat5 R = rotateXW(1.234f);
    const Vec4 y = R.transformDirection({0.f, 1.f, 0.f, 0.f});
    const Vec4 z = R.transformDirection({0.f, 0.f, 1.f, 0.f});
    REQUIRE(y.y == Approx(1.f));
    REQUIRE(z.z == Approx(1.f));
}

TEST_CASE("Rotor4 inverse undoes the rotation", "[math][rotor4]") {
    Rotor4 r;
    r.xy = 0.3f;
    r.xw = -0.7f;
    r.zw = 1.1f;

    const Mat5 R    = r.toMatrix();
    const Mat5 Rinv = r.toInverseMatrix();
    const Vec4 v{0.5f, -1.5f, 2.f, 0.25f};
    const Vec4 v2 = Rinv.transformPoint(R.transformPoint(v));

    REQUIRE(v2.x == Approx(v.x).margin(1e-5f));
    REQUIRE(v2.y == Approx(v.y).margin(1e-5f));
    REQUIRE(v2.z == Approx(v.z).margin(1e-5f));
    REQUIRE(v2.w == Approx(v.w).margin(1e-5f));
}
