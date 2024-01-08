# TODOs

## General
- [ ] GitHub CI

## GLSL
- [ ] Fix BVH memory consumption. BVH copies vertex/index data from Scene.
- [ ] Flatten BVH as DFS for (potentially) better cache coherence.
- [ ] Improve SSBO alignment. Renderer copies vertices with 1 empty float buffer to align vec3s to vec4s.
- [ ] Restructure GLSL. Create hitgroups for different purposes (primary, shadow, GI).
- [ ] Implement render modes (Albedo, Normal, Depth, GI)
