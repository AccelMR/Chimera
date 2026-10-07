// Helpers to move vectors between spaces. Matrices come from the engine stored row by row
// and are applied to row vectors, as mul(vector, matrix).
#pragma once

// A normal needs the inverse transpose of the model matrix, or it tilts under non-uniform
// scale. The cofactor matrix is that inverse transpose times the determinant, so after
// normalizing only the determinant's sign matters (a mirrored object). It costs three
// cross products instead of an inverse.
float3
transformNormal(float3 normal, row_major float4x4 model)
{
  const float3 row0 = model[0].xyz;
  const float3 row1 = model[1].xyz;
  const float3 row2 = model[2].xyz;
  const float3 cross12 = cross(row1, row2);
  const float3x3 cofactor = float3x3(cross12, cross(row2, row0), cross(row0, row1));
  const float determinantSign = dot(row0, cross12) < 0.0f ? -1.0f : 1.0f;
  return normalize(mul(normal, cofactor)) * determinantSign;
}
