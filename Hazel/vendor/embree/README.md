# Intel Embree 4.3.3

This directory contains the Win64 Embree 4.3.3 headers and libraries shipped
with the local Unreal Engine source tree. Hazel uses Embree 4 for CPU mesh SDF
baking, matching Unreal's `MeshDistanceFieldUtilities.cpp` query sequence:

1. build and commit one immutable triangle scene;
2. use `rtcPointQuery` for the closest unsigned triangle distance;
3. use 98 stratified hemisphere rays to classify the sign from back-face hits;
4. clamp the trace band to four voxel diagonals.

The current Hazel asset format still stores dense `float` samples. UE's later
7x7x7 sparse brick packing and PF_G8 quantization will be added after the
distance query path is validated.

Embree requires the matching oneTBB runtime DLL on Windows. See
`LICENSE.txt` and `oneTBB_LICENSE.txt` for the redistributed licenses.
