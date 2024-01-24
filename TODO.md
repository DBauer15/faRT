# TODOs

## General
- [ ] GitHub CI

## GLSL
- [ ] Find a scale-dependent offset of ray origins to avoid self-intersections. (Ray Tracing Gems Chapter 6)
- [ ] Implement path regularization to address fireflies. Currently, I use clamping. (Ray Tracing Gems Chapter 17)
- [ ] Fix BVH. Crashes for some scenes.
- [X] Add accumulation buffer.
- [X] Fix RNG 
- [ ] Implement low-discrepancy sequence samplers
- [X] Implement SAH as BVH split function.
- [ ] Fix BVH memory consumption. BVH copies vertex/index data from Scene.
- [ ] Flatten BVH as DFS for (potentially) better cache coherence.
- [X] Improve SSBO alignment. Renderer copies vertices with 1 empty float buffer to align vec3s to vec4s.
- [ ] Implement render modes (Albedo, Normal, Depth, BVH)
