#pragma once
#include <vector>

class zCamisa
{
public:
    bool iSLoadZ = false;
    int PixelW = 512;
    int PixelH = 512;
    int PixelChanel = 4;
    std::vector<unsigned char> ImgRGB;

    zCamisa() : ImgRGB(PixelW* PixelH* PixelChanel) {}

    bool LoadImg(const char* UbiImg);
    GLuint zGenTexture(GLint Mode);
    void RenderImg(GLint Texture);
};

inline bool zCamisa::LoadImg(const char* UbiImg)
{
    unsigned char* dataIMG = stbi_load(UbiImg, &PixelW, &PixelH, &PixelChanel, 4);
    if (!dataIMG)
    {
        iSLoadZ = false;
        return false;
    }

    size_t dataSize = PixelW * PixelH * 4;
    ImgRGB.assign(dataIMG, dataIMG + dataSize);
    stbi_image_free(dataIMG);
    iSLoadZ = true;
    return true;
}

inline GLuint zCamisa::zGenTexture(GLint Mode)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    GLenum format = (PixelChanel == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, PixelW, PixelH, 0, format, GL_UNSIGNED_BYTE, ImgRGB.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

inline void zCamisa::RenderImg(GLint Texture)
{
    if (Texture != 0) { glBindTexture(GL_TEXTURE_2D, Texture); }
}