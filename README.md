## Purpose

The spirit of the creation of this project was to experiment with creating 3D scenes while attempting to play outside of
the OOP mindset where I have control. This was not due to a belief that objects or classes are bad, but as an attempt to
not limit my thought process to a single way of creating software. It was about personal experimentation and investigation.
In the future, I ideally use classes for things I know they are wonderful at and use a better tool for things they aren't.
At the time of starting this project I felt fairly tied down to the OOP way of thinking.

## Final Result

The final result is a scene containing a "gate" with portals that lead to other space-defying scenes. \
[Here is a YouTube video link that shows the final result of the project.](https://www.youtube.com/watch?v=iAz9by0D5xM) \
The following is a single snapshot (although unable to really capture the project):
![Noop Scenes Example](https://raw.githubusercontent.com/Lucodivo/RepoSampleImages/master/NoopScenes/octahedron.png)

## Building
1) Cloning the project will include most dependencies: [tinygltf](https://github.com/syoyo/tinygltf), [stb](https://github.com/nothings/stb), [glad](https://github.com/Dav1dde/glad), [json](https://github.com/nlohmann/json#examples)
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
        set(GLFW_HEADER_LOCATIONS "C:/directory/that_contains/glfw3.h")
        set(GLFW_LIB_LOCATION "C:/directory/that_contains/glfw3.lib")
        set(GLFW_LIB actual-name-of-glfw3-static-library) # ex: glfw3-x64-d
      ```
3) Finally, build using *CMakeList.txt* in the root directory of this repository. When running the project, ensure that
the working directory is at the root directory.
   
## Running
- This project needs to be run with the project's root directory as the working directory.

## Standards
*In this project, consistency is often valued over absolute best convention.*

### Dependencies/Building
- NoopScenes uses a unity build system, which means all of the headers are included in *noop_scenes.h* and the project
  is built by simply compiling *noop_scenes.cpp*. The obvious benefit is that the projects chain of dependencies 
  only needs to be ordered correctly in a single file. The drawback is that the entire project must be built for every
  build.

### Testing
- Tests were written as a separate project and are exclusively for testing the custom math written for this project.

### Dimensions
- Any dimensions/positions in the code will always refer to length in meters where a concrete unit matters.

### 3D Math
- mat3/mat4 are all considered to be column major. (ex: aMat3[ i ][ j ] leads you to column i, row j)
- vec2/vec3/vec4 are all considered to be column vectors.
    - A consequence of this is that M * *v* is legal but *v* * M is not.


### Coordinate System
- Right handed-coordinate system with the z-axis representing the upward/downward movement. 
    - Although mostly arbitrary, here are my justifications:
        - Blender and the gltf file format use right-handed coordinate systems and are used for this project.
        - ~~Z is up is used in Blender and is a tool used for this project.~~ [Working in Blender uses Z-up but Blender exports Y-up by default.]
        - Shortening a 3D vector to a 2D by cutting off the third value (z) to acquire "ground coordinates" is appealing.
    - And the negatives:
        - When accessing `samplerCube` textures in OpenGL, adjustments need to be made to ensure Y is actually "up".
    
### Etc.
- If a function argument may be modified in the function, it will be passed in as a pointer type.
- If a function argument may not be modified in the function, it will be passed as a constant reference.
- Non-constant reference arguments will not be used as function arguments in this project.
```
// with non-const reference
void someFunc(int& xArg);
void main() {
    ...
    int x;
    someFunc(x); // Without knowing the function header, how do I know x might change?
    ...
}

// with pointer
void someFunc(int* xArg);
void main() {
    ...
    int x;
    someFunc(&x); // explicit at call site that x might change
}
```

### Vertex & Fragment Shaders
- Standards for vertex/fragment shaders make it so that swapping in/out shaders is as simple as possible
- Using binding points for uniform buffer objects requires us to use at least GLSL #version 420

#### Vertex attribute input variables
- According to documentation for glGet, GL_MAX_VERTEX_ATTRIBS must be at least 16.
- Four indices are reserved and will be using the following naming convention.
```
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;
```

#### Vertex uniform variables
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
} ubo;
```
```
// cpp
struct ProjectionViewModelUBO { // base alignment   // aligned offset
  mat4 projection;              // 64               // 0
  mat4 view;                    // 64               // 64
  mat4 model;                   // 64               // 128
}
```

#### Vertex/Fragment shader output/input variables
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

#### Fragment uniform variables
- [see Vertex uniform variables](#vertex-uniform-variables)
- The light uniform buffer object uses a double-sided stack to store directional and positional lights in the same array.
  This allows us to have a maximum of N lights without specific restrictions on each light type. Lights are also passed in
  a generic struct called InLight."
```
// glsl fragment shader
layout (binding = 1, std140) uniform FragUBO { // base alignment   // aligned offset
  f32 time;                                    // 4                // 0
} fragUbo;

struct InLight {
  vec4 color;
  vec4 pos; // position of positional light, direction TO SOURCE of directional light
};

layout (binding = 2, std140) uniform LightInfoUBO {// base alignment   // aligned offset
  vec4 ambientLightColor;                          // 16               // 0
  Light dirPosLightStack[8];                       // 16               // 16
  uint dirLightCount;                              // 4                // 272
  uint posLightCount;                              // 4                // 276
} lightInfoUbo;
```
```
// cpp
struct FragUBO {
  f32 time;
};

struct LightUniform {
  vec4 color; // fourth component used for light power
  vec4 pos; // fourth component for padding, currently un-defined
};

struct LightUBO {
  vec4 ambientLight;
  LightUniform dirPosLightStack[8];
  u32 dirLightCount;
  u32 posLightCount;
};
```

#### Fragment sampler uniform variables
- OpenGL requires sampler variables to be explicitly declared as uniform
```
uniform samplerCube envMapTex;
uniform sampler2D noiseTex;
```

#### Fragment shader output variables
- No indices currently reserved for fragment shaders as render buffers vary greatly.
- The following naming convention and explicit prefix of layout location is enforced.
```
layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outPosition;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outAlbedo;
```

#### Vertex instance input variables
- No indices currently reserved for input instance variables.
- The following naming convention and explicit prefix of layout location is enforced.
```
layout (location = 4) in vec3 instPos;
layout (location = 5) in vec3 instRot;
layout (location = 6) in float instScale;
```


## Special Thanks

### Model Textures
- [FreePBR](https://freepbr.com/)

### Environment Skyboxes
- [3delyvisions](https://opengameart.org/content/elyvisions-skyboxes)
- [Xonotic](https://opengameart.org/content/xonotic-skyboxes)
- [Ulukai - Jonathan Denil](https://opengameart.org/content/ulukais-space-skyboxes)
- [Mayhem](https://opengameart.org/content/mayhems-skyboxes)

### Papers
- [Oblique View Frustum Depth Projection and Clipping by Eric Lengyel](http://www.terathon.com/lengyel/Lengyel-Oblique.pdf)
- [How scrolling textures gave Super Mario Galaxy 2 its charm by Jasper](https://www.youtube.com/watch?v=8rCRsOLiO7k)