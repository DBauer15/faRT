cmake -B build -DCMAKE_BUILD_TYPE=Debug .
cmake --build build -j
mv build/compile_commands.json ./
