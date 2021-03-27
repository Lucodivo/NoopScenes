## Purpose

The spirit of the creation of this project is to experiment with creating 3D scenes while attempting to play outside of
the OOP mindset where I have control. This is not due to a belief that objects or classes are bad, but as an attempt to
not limit my thought process to a single way of creating software. In the future, I ideally use classes for things I know
they are wonderful at and use a better tool for things they aren't.

### Building
1) Cloning the project will include most dependencies: [glm](https://github.com/g-truc/glm), [tinygltf](https://github.com/syoyo/tinygltf), [stb](https://github.com/nothings/stb), [glad](https://github.com/Dav1dde/glad), [json](https://github.com/nlohmann/json#examples)
  ```
  git clone https://github.com/Lucodivo/NoopScenes
  ```
  - If you are having problems with the glad dependency, use David Herberth's [website](https://glad.dav1d.de/) to 
    generate the files suitable for your system. [Link to [glad repository](https://github.com/Dav1dde/glad) too for 
    good measure]
  - If you downloaded the project directly from github or are missing the glm submodule somehow, run the following git 
    commands in the root directory of the project.
    ```
    git submodule init
    git submodule update
    ```
2) You will also need to acquire a compiled static library and header files for [GLFW](https://www.glfw.org/)
  - This can be accomplished by a [direct download of pre-compiled binaries](https://www.glfw.org/download.html) or by
    cloning the [GLFW repository](https://github.com/glfw/glfw) and 
    [compiling GLFW on your machine](https://www.glfw.org/docs/3.3/compile.html).
  - After you have the necessary [header files](https://github.com/glfw/glfw/tree/master/include/GLFW) and a compiled
    static library (ex: glfw3.lib), you will need to make edits to three lines in *CMakeList.txt* in the root
    directory of this repository.
      ```
        # These next three lines are what you need to edit to build this project
        set(GLFW_HEADER_LOCATIONS "C:/directory/that_contains/glfw3.h/and/glfw3native.h")
        set(GLFW_LIB_LOCATION "C:/directory/that_contains/glfw3.lib")
        set(GLFW_LIB actual-name-of-glfw3-static-library) # ex: glfw3-x64-d
      ```
3) Finally, build using *CMakeList.txt* in the root directory of this repository.     

### Standards
*In this project, consistency is often valued over absolute best convention.*

#### Dimensions
- Any dimensions/positions in the code will always refer to length in meters

#### Coordinate System
- Everything will be right handed-coordinate system but z-axis will represent the upward/downward movement.
  - Justifications include:
    - Right-handed system is used in both OpenGL & Blender (tools used for this project)
    - Z is up is used in Blender (tool used for this project)
    - Shortening a vec3 to a vec2 by cutting off the third value (z) to acquire "ground coordinates" is appealing 
    - [*disclaimer: mostly preference*] When a viewing angle shows the positive X-axis pointing right (a mostly universal 
      concept, in my experience), I would like the remaining positive axes to point forward and up. Since I am using a 
      right-handed system, my only solution is that the positive Y-axis points forward and the positive Z-axis points up.

#### Vertex & Fragment Shaders
- Using binding points for uniform buffer objects requires us to use at least GLSL #version 420

Vertex attribute input variables:
- According to documentation for glGet, GL_MAX_VERTEX_ATTRIBS must be at least 16.
- Four indices are reserved and will be using the following naming convention.

```
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;
```

Vertex uniform variables:
- std140 layout is currently the standard for the project.
- Layout binding index must be specified and all uniform data must go through a uniform buffer object.
- Alignment comments optional. 
  - [Reference on GLSL structure alignment - LearnOpenGL: Advanced GLSL](https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL)
  - [Reference for C/C++ structure packing - The Lost Art of Structure Packing by Eric S. Raymond](http://www.catb.org/esr/structure-packing/)
- Also be cognizant of the use of `vec3` and float arrays as they can easily lead to mismatched packing between C++/GLSL

```
// glsl vertex shader
layout (binding = 0, std140) uniform UBO { // base alignment   // aligned offset
  mat4 projection;                         // 64               // 0
  mat4 view;                               // 64               // 64
  mat4 model;                              // 64               // 128
  uvec2 resolution;                        // 8                // 192
  float time;                              // 4                // 200
  float timeDelta;                         // 4                // 204
} ubo;
```
```
// cpp
struct UBO {              // base alignment   // aligned offset
  Mat4 projection;        // 4                // 0
  Mat4 view;              // 4                // 64
  Mat4 model;             // 4                // 128
  uVec2 resolution;       // 4                // 192
  f32 time;               // 4                // 200
  f32 delta;              // 4                // 204
}
```

Vertex/Fragment shader output/input variables:
- According to documentation for glGet, GL_MAX_VERTEX_OUTPUT_COMPONENTS must be at least 64 and 
  GL_MAX_FRAGMENT_INPUT_COMPONENTS must be at least 128.
- Three layout location indices currently reserved for output/input vertex/fragment shader variables for normals, 
  texture coordinates, and color.
- The following naming convention and explicit prefix with layout location is enforced.

```
// glsl vertex shader
layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outTexCoord;
layout (location = 2) out vec3 outColor;
```
```
// glsl fragment shader
layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inTexCoord;
layout (location = 2) in vec3 inColor;
```

Fragment sampler uniform variables:
- OpenGL requires sampler variables to be explicitly declared as uniform
```
uniform samplerCube envMapTexture;
uniform sampler2D noiseTexture
```

Fragment shader output variables:
- No indices currently reserved for fragment shaders as render buffers vary greatly.
- The following naming convention and explicit prefix of layout location is enforced.

```
layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outPosition;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outAlbedo;
```


#### ⚠️WORK IN PROGRESS STANDARDS⚠️

Vertex instance input variables:
- No indices currently reserved for input instance variables.

```
layout (location = 4) in vec3 instPos;
layout (location = 5) in vec3 instRot;
layout (location = 6) in float instScale;
```


### Special Thanks

#### Environment Skyboxes
- [3delyvisions](https://opengameart.org/content/elyvisions-skyboxes)
- [Xonotic](https://opengameart.org/content/xonotic-skyboxes)
- [Ulukai - Jonathan Denil](https://opengameart.org/content/ulukais-space-skyboxes)
- [Mayhem](https://opengameart.org/content/mayhems-skyboxes)