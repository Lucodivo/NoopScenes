#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define NO_FRAMEBUFFER_ATTACHMENT 0

struct Framebuffer {
  u32 id;
  u32 colorAttachment;
  u32 depthStencilAttachment;
  Extent2D extent;
};

enum FramebufferCreationFlags {
  FramebufferCreate_NoValue = 0,
  FramebufferCreate_NoDepthStencil = 1 << 0,
  FramebufferCreate_color_sRGB = 1 << 1,
};

void loadCubeMapTexture(const char* const imgLocations[6], GLuint* textureId, bool flipImageVert = false)
{
  glGenTextures(1, textureId);
  glBindTexture(GL_TEXTURE_CUBE_MAP, *textureId);

  s32 width, height, nrChannels;
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

Framebuffer initializeFramebuffer(Extent2D framebufferExtent, FramebufferCreationFlags flags = FramebufferCreate_NoValue)
{
  Framebuffer resultBuffer;
  resultBuffer.extent = framebufferExtent;

  GLint originalDrawFramebuffer, originalReadFramebuffer, originalActiveTexture, originalTexture0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &originalDrawFramebuffer);
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &originalReadFramebuffer);
  glGetIntegerv(GL_ACTIVE_TEXTURE, &originalActiveTexture);

  // creating frame buffer
  glGenFramebuffers(1, &resultBuffer.id);
  glBindFramebuffer(GL_FRAMEBUFFER, resultBuffer.id);

  // creating frame buffer color texture
  glGenTextures(1, &resultBuffer.colorAttachment);
  // NOTE: Binding the texture to the GL_TEXTURE_2D target, means that
  // NOTE: gl operations on the GL_TEXTURE_2D target will affect our texture
  // NOTE: while it is remains bound to that target
  glActiveTexture(GL_TEXTURE0);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &originalTexture0);
  glBindTexture(GL_TEXTURE_2D, resultBuffer.colorAttachment);
  GLint ernalFormat = (flags & FramebufferCreate_color_sRGB) ? GL_SRGB : GL_RGB;
  glTexImage2D(GL_TEXTURE_2D, 0/*LoD*/, ernalFormat, framebufferExtent.width, framebufferExtent.height, 0/*border*/, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // attach texture w/ color to frame buffer
  glFramebufferTexture2D(GL_FRAMEBUFFER, // frame buffer we're targeting (draw, read, or both)
                         GL_COLOR_ATTACHMENT0, // type of attachment and index of attachment
                         GL_TEXTURE_2D, // type of texture
                         resultBuffer.colorAttachment, // texture
                         0); // mipmap level

  if (flags & FramebufferCreate_NoDepthStencil)
  {
    resultBuffer.depthStencilAttachment = NO_FRAMEBUFFER_ATTACHMENT;
  } else {
    // creating render buffer to be depth/stencil buffer
    glGenRenderbuffers(1, &resultBuffer.depthStencilAttachment);
    glBindRenderbuffer(GL_RENDERBUFFER, resultBuffer.depthStencilAttachment);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, framebufferExtent.width, framebufferExtent.height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0); // unbind
    // attach render buffer w/ depth & stencil to frame buffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, // frame buffer target
                              GL_DEPTH_STENCIL_ATTACHMENT, // attachment po of frame buffer
                              GL_RENDERBUFFER, // render buffer target
                              resultBuffer.depthStencilAttachment);  // render buffer
  }

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
  }
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, originalDrawFramebuffer);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, originalReadFramebuffer);
  glBindTexture(GL_TEXTURE_2D, originalTexture0); // re-bind original texture
  glActiveTexture(originalActiveTexture);
  return resultBuffer;
}



void deleteFramebuffer(Framebuffer* framebuffer)
{
  glDeleteFramebuffers(1, &framebuffer->id);
  glDeleteTextures(1, &framebuffer->colorAttachment);
  if (framebuffer->depthStencilAttachment != NO_FRAMEBUFFER_ATTACHMENT)
  {
    glDeleteRenderbuffers(1, &framebuffer->depthStencilAttachment);
  }
  *framebuffer = {0, 0, 0, 0, 0};
}

void deleteFramebuffers(u32 count, Framebuffer** framebuffer)
{
  u32* deleteFramebufferObjects = new u32[count * 3];
  u32* deleteColorAttachments = deleteFramebufferObjects + count;
  u32* deleteDepthStencilAttachments = deleteColorAttachments + count;
  u32 depthStencilCount = 0;
  for (u32 i = 0; i < count; i++)
  {
    deleteFramebufferObjects[i] = framebuffer[i]->id;
    deleteColorAttachments[i] = framebuffer[i]->colorAttachment;
    if (framebuffer[i]->depthStencilAttachment != NO_FRAMEBUFFER_ATTACHMENT)
    {
      deleteDepthStencilAttachments[depthStencilCount++] = framebuffer[i]->depthStencilAttachment;
      *(framebuffer[i]) = {0, 0, 0, 0, 0};
    }
  }

  glDeleteFramebuffers(count, deleteFramebufferObjects);
  glDeleteTextures(count, deleteColorAttachments);
  if (depthStencilCount != 0)
  {
    glDeleteRenderbuffers(depthStencilCount, deleteDepthStencilAttachments);
  }

  delete[] deleteFramebufferObjects;
}