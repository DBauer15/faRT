cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DBUILD_OPENGL_RENDERER=ON \
    -DBUILD_METAL_RENDERER=OFF \
    .
cmake --build build -j
mv build/compile_commands.json ./
