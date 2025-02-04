emcmake cmake -B build-web \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DBUILD_OPENGL_RENDERER=OFF \
    -DBUILD_METAL_RENDERER=OFF \
    -DBUILD_WEBGPU_RENDERER=ON \
    -DTBB_DIR=/Users/davidbauer/Downloads/oneTBB-2022.0.0/install/lib/cmake/TBB \
    .
cmake --build build-web -j