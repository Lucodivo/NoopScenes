#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

void loadCubeMapTexture(const char* const imgLocations[6], GLuint* textureId, bool flipImageVert = false)
{
  glGenTextures(1, textureId);
  glBindTexture(GL_TEXTURE_CUBE_MAP, *textureId);

  int width, height, nrChannels;
  stbi_set_flip_vertically_on_load(flipImageVert);
  for (u32 i = 0; i < 6; i++)
  {
    unsigned char* data = stbi_load(imgLocations[i], &width, &height, &nrChannels, 0);
    if (data)
    {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                   0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
      );
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    } else
    {
      std::cout << "Cubemap texture failed to load at path: " << imgLocations[i] << std::endl;
    }
    stbi_image_free(data);
  }
}