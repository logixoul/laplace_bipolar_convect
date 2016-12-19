#pragma once
#include "precompiled.h"

extern gl::Texture NO_DEPTH_TEX;
/*inline gl::Texture maketex(int w, int h, GLint format, GLint internalFormat) {
	gl::Texture::Format fmt; fmt.setInternalFormat(internalFormat); return gl::Texture(NULL, format, w, h, fmt);
}*/
inline gl::Texture maketex(int w, int h, GLint internalFormat) {
	gl::Texture::Format fmt; fmt.setInternalFormat(internalFormat); return gl::Texture(NULL, GL_RGBA, w, h, fmt);
}
inline gl::Texture maketex(Array2D<Vec3f> arr, GLint internalFormat) {
	gl::Texture::Format fmt; fmt.setInternalFormat(internalFormat);
	auto tex = gl::Texture(arr.w, arr.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, arr.w, arr.h, GL_RGB, GL_FLOAT, arr.data);
	return tex;
}
inline gl::Texture maketex(Array2D<Vec2f> arr, GLint internalFormat) {
	gl::Texture::Format fmt; fmt.setInternalFormat(internalFormat);
	auto tex = gl::Texture(arr.w, arr.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, arr.w, arr.h, GL_RG, GL_FLOAT, arr.data);
	return tex;
}
inline gl::Texture maketex(Array2D<float> arr, GLint internalFormat) {
	gl::Texture::Format fmt; fmt.setInternalFormat(internalFormat);
	auto tex = gl::Texture(arr.w, arr.h, fmt);
	tex.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, arr.w, arr.h, GL_LUMINANCE, GL_FLOAT, arr.data);
	return tex;
}
#define checkGL() \
{\
	auto error=glGetError();\
	if(error!=GL_NO_ERROR){\
		cout<<"["<<__LINE__<<"] GL ERROR "<<error<<endl;\
		/*system("PAUSE");*/\
	}\
}
inline void beginRTT(gl::Texture fbotex, gl::Texture& depthtex=NO_DEPTH_TEX)
{
	static unsigned int fboid = 0;
	if(fboid == 0)
	{
		glGenFramebuffersEXT(1, &fboid);
		checkGL();
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboid);
	checkGL();
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fbotex.getId(), 0);
	checkGL();
	if(&depthtex != &NO_DEPTH_TEX)
	{
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, depthtex.getId(), 0);
		checkGL();
	}
	else
	{
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);
		checkGL();
	}
}
inline void endRTT()
{
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, 0, 0);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}
