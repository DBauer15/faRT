# Fabulous Ray Tracer
Hobby project that implements real-time path tracing for various backends to facilitate learning of different graphics APIs and experiment with various rendering techniques like reservor sampling.

## Building FaRT
The project can be built via CMake. Dependencies are included in the repository as submodules. Make sure they are correctly checked out.
If you have not cloned the repository yet run:

```bash
git clone --recursive git@github.com:DBauer15/FaRT.git
```

Alternatively, on a local copy run:

```bash
git submodules update --init --recursive
```

To build the application run:

```bash
cmake -B build -D[RENDERER] .
cmake --build build -j
```

Where ``[RENDERER]`` is one of the supported rendering backends (see below).

### Building Renderers
FaRT supports various rendering backends which can be built by setting corresponding CMake flags.
Currently, only a single backend is supported at a time. To build a backend, pass the approriate flag.

**OpenGL (Windows, Linux)**

```bash
cmake -B build -DBUILD_OPENGL_RENDERER=ON
```

**Metal (Apple)**

```bash
cmake -B build -DBUILD_METAL_RENDERER=ON 
```

## Running FaRT
The app can be started by calling the compiled binary with the desired scene as an argument.

```bash
cd build 
./fart [SCENE_FILE]
```

Where `[SCENE_FILE]` is a 3D file of any of the supported formats (see "Supported 3D Formats" for details).

## Controls
The renderer implements a simple arcball camera model. The camera can be controlled via mouse inputs.

* LMB - Rotate the camera
* RMB - Zoom in/out 
* MMB - Pan the view

## Supported 3D Formats
Here is an ever evolving list of supported file format.

- [X] Wavefront OBJ
- [ ] Autodesk FBX
- [ ] Stanford PLY
- [ ] GL Transmission Format glTF
- [ ] Pixar Universal Scene Descriptor USD
