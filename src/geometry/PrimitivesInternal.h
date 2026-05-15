#pragma once

#include "geometry/Mesh4D.h"

namespace hopf::geometry::detail {

Mesh4D buildTriangle2D(float size);
Mesh4D buildSquare2D(float size);
Mesh4D buildPentagon2D(float size);
Mesh4D buildHexagon2D(float size);
Mesh4D buildStar2D(float size);
Mesh4D buildCircle2D(float size);

Mesh4D buildCube3D(float size);
Mesh4D buildTetrahedron3D(float size);
Mesh4D buildOctahedron3D(float size);
Mesh4D buildIcosahedron3D(float size);
Mesh4D buildDodecahedron3D(float size);
Mesh4D buildIcosphere3D(float size);
Mesh4D buildSphere3D(float size);
Mesh4D buildCylinder3D(float size);
Mesh4D buildCone3D(float size);
Mesh4D buildTorus3D(float size);
Mesh4D buildPyramid3D(float size);
Mesh4D buildPrism3D(float size);
Mesh4D buildCapsule3D(float size);

Mesh4D buildTesseract(float size);
Mesh4D buildPentachoron(float size);
Mesh4D buildHexadecachoron(float size);
Mesh4D buildIcositetrachoron(float size);
Mesh4D buildTetrahedralPrism(float size);
Mesh4D buildCubicalPyramid(float size);
Mesh4D buildOctahedralPrism(float size);
Mesh4D buildIcosahedralPrism(float size);
Mesh4D buildCubinder(float size);
Mesh4D buildSpherinder(float size);
Mesh4D buildDuoprism33(float size);
Mesh4D buildDuoprism55(float size);

} // namespace hopf::geometry::detail
