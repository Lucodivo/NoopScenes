#pragma once

internal_func u32 loadShader(const char* shaderPath, GLenum shaderType);

ShaderProgram createShaderProgram(const char* vertexPath, const char* fragmentPath) {
  ShaderProgram shaderProgram{};
  shaderProgram.vertexShader = loadShader(vertexPath, GL_VERTEX_SHADER);
  shaderProgram.fragmentShader = loadShader(fragmentPath, GL_FRAGMENT_SHADER);

  // shader program
  shaderProgram.id = glCreateProgram(); // NOTE: returns 0 if error occurs when creating program
  glAttachShader(shaderProgram.id, shaderProgram.vertexShader);
  glAttachShader(shaderProgram.id, shaderProgram.fragmentShader);
  glLinkProgram(shaderProgram.id);

  s32 linkSuccess;
  glGetProgramiv(shaderProgram.id, GL_LINK_STATUS, &linkSuccess);
  if (!linkSuccess)
  {
    char infoLog[512];
    glGetProgramInfoLog(shaderProgram.id, 512, NULL, infoLog);
    std::cout << "ERROR::PROGRAM::SHADER::LINK_FAILED\n" << infoLog << std::endl;
    exit(-1);
  }

  glDetachShader(shaderProgram.id, shaderProgram.vertexShader);
  glDetachShader(shaderProgram.id, shaderProgram.fragmentShader);
  return shaderProgram;
}

void deleteShaderProgram(ShaderProgram shaderProgram)
{
  // delete the shaders
  glDeleteShader(shaderProgram.vertexShader);
  glDeleteShader(shaderProgram.fragmentShader);
  glDeleteProgram(shaderProgram.id);
}

void deleteShaderPrograms(ShaderProgram* shaderPrograms, u32 count) {
  for(u32 i = 0; i < count; i++) {
    deleteShaderProgram(shaderPrograms[i]);
  }
}

// utility uniform functions
inline void setUniform(GLuint shaderId, const std::string& name, bool value)
{
  glUniform1i(glGetUniformLocation(shaderId, name.c_str()), (int)value);
}

inline void setUniform(GLuint shaderId, const std::string& name, s32 value)
{
  glUniform1i(glGetUniformLocation(shaderId, name.c_str()), value);
}

inline void setUniform(GLuint shaderId, const std::string& name, u32 value)
{
  glUniform1ui(glGetUniformLocation(shaderId, name.c_str()), value);
}

inline void setSamplerCube(GLuint shaderId, const std::string& name, GLint activeTextureIndex) {
  glUniform1i(glGetUniformLocation(shaderId, name.c_str()), activeTextureIndex);
}

inline void setSampler2D(GLuint shaderId, const std::string& name, GLint activeTextureIndex) {
  glUniform1i(glGetUniformLocation(shaderId, name.c_str()), activeTextureIndex);
}

inline void setUniform(GLuint shaderId, const std::string& name, f32 value)
{
  glUniform1f(glGetUniformLocation(shaderId, name.c_str()), value);
}

inline void setUniform(GLuint shaderId, const std::string& name, f32 value1, f32 value2)
{
  glUniform2f(glGetUniformLocation(shaderId, name.c_str()), value1, value2);
}

inline void setUniform(GLuint shaderId, const std::string& name, f32 value1, f32 value2, f32 value3)
{
  glUniform3f(glGetUniformLocation(shaderId, name.c_str()), value1, value2, value3);
}

inline void setUniform(GLuint shaderId, const std::string& name, f32 value1, f32 value2, f32 value3, f32 value4)
{
  glUniform4f(glGetUniformLocation(shaderId, name.c_str()), value1, value2, value3, value4);
}

inline void setUniform(GLuint shaderId, const std::string& name, const mat4* mat)
{
  glUniformMatrix4fv(glGetUniformLocation(shaderId, name.c_str()),
                     1, // count
                     GL_FALSE, // transpose: swap columns and rows (true or false)
                     mat->valuesPtr); // pointer to float values
}

inline void setUniform(GLuint shaderId, const std::string& name, const mat4* matArray, const u32 arraySize)
{
  glUniformMatrix4fv(glGetUniformLocation(shaderId, name.c_str()),
                     arraySize, // count
                     GL_FALSE, // transpose: swap columns and rows (true or false)
                     matArray->valuesPtr); // pointer to float values
}

inline void setUniform(GLuint shaderId, const std::string& name, const float* floatArray, const u32 arraySize)
{
  glUniform1fv(glGetUniformLocation(shaderId, name.c_str()), arraySize, floatArray);
}

inline void setUniform(GLuint shaderId, const std::string& name, const vec2& vector2)
{
  setUniform(shaderId, name, vector2.x, vector2.y);
}

inline void setUniform(GLuint shaderId, const std::string& name, const vec3& vector3)
{
  setUniform(shaderId, name, vector3.x, vector3.y, vector3.z);
}

inline void setUniform(GLuint shaderId, const std::string& name, const vec4& vector4)
{
  setUniform(shaderId, name, vector4.x, vector4.y, vector4.z, vector4.w);
}

inline void bindBlockIndex(GLuint shaderId, const std::string& name, u32 index)
{
  u32 blockIndex = glGetUniformBlockIndex(shaderId, name.c_str());
  glUniformBlockBinding(shaderId, blockIndex, index);
}
  
  
void readShaderCodeAsString(const char* shaderPath, std::string* shaderCode)
{
  try
  {
    std::ifstream file;
    // ensure ifstream objects can throw exceptions:
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    // open file
    file.open(shaderPath);
    std::stringstream shaderStream;
    // read file's buffer contents into streams
    shaderStream << file.rdbuf();
    // close file handler
    file.close();
    // convert stream into string
    *shaderCode = shaderStream.str();
  } catch (std::ifstream::failure e)
  {
    std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
  }
}

/*
 * parameters:
 * shaderType can be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, or GL_GEOMETRY_SHADER
 */
internal_func u32 loadShader(const char* shaderPath, GLenum shaderType) {
  std::string shaderTypeStr;
  if(shaderType == GL_VERTEX_SHADER) {
    shaderTypeStr = "VERTEX";
  } else if(shaderType == GL_FRAGMENT_SHADER){
    shaderTypeStr = "FRAGMENT";
  }

  std::string shaderCode;
  readShaderCodeAsString(shaderPath, &shaderCode);
  const char* shaderCodeCStr = shaderCode.c_str();

  u32 shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, &shaderCodeCStr, NULL);
  glCompileShader(shader);

  s32 shaderSuccess;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderSuccess);
  if (shaderSuccess != GL_TRUE)
  {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::" << shaderTypeStr << "::COMPILATION_FAILED\n" << infoLog << std::endl;
  }

  return shader;
}