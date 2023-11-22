cmake -B build .
cmake --build build -j
mv build/compile_commands.json ./
