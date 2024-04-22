cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DBUILD_OPENGL_RENDERER=OFF \
    -DBUILD_METAL_RENDERER=OFF \
    -DBUILD_OPTIX_RENDERER=ON \
    .
cmake --build build -j
mv build/compile_commands.json ./
