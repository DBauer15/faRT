cmake -B build ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DBUILD_OPENGL_RENDERER=ON ^
    -DBUILD_METAL_RENDERER=OFF ^
    .

cmake --build build --config Debug -j
