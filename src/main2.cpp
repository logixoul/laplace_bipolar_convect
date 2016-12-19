#include "precompiled.h"
#if 0

//#include "ciextra.h"
#include "util.h"
#include <numeric>
#include "stuff.h"
#include "glutil.h"
#include "shade.h"

typedef gl::Texture Tex;
typedef Array2D<float> Image;
int wsx=800, wsy = 600;
int scale = 4;
int sx = wsx / scale;
int sy = wsy / scale;
float mouseX, mouseY;
bool pause = false;
bool keys[256];
bool oneFastFrame = false;
ci::Rectf area(sx * 1.0f/6.0f, sy * 1.0f/6.0f, sx * 5.0f/6.0f, sy * 5.0f/6.0f);
gl::Texture img,velocity;
GLenum img_ifmt = GL_RGBA16F;
GLenum img_fmt = GL_LUMINANCE;
GLenum velocity_ifmt = GL_RGBA16F;
GLenum velocity_fmt = GL_RG;
struct SApp : AppBasic {
	void setup()
	{
		createConsole();

		glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
		glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
		glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);

		reset();
		setWindowSize(wsx, wsy);
	}
	void keyDown(KeyEvent e)
	{
		if(e.getChar() == 'p'||e.getChar()=='2')
			pause = !pause;
		if(e.getChar() == 'r')
			reset();
		keys[e.getChar()] = true;
	}
	void keyUp(KeyEvent e)
	{
		keys[e.getChar()] = false;
	}
	void reset()
	{
		Array2D<float> img_data(sx, sy);
		
		forxy(img_data)
		{
			img_data(p) = ci::randFloat();
		}
		img=maketex(img_data, img_ifmt);
		velocity=maketex(sx,sy,velocity_ifmt);
	}
	
	void mouseDown(MouseEvent e)
	{
	}
	
	void processFluid(gl::Texture vectorField, gl::Texture src, gl::Texture& dst)
	{
		vectorField.setWrap(GL_REPEAT, GL_REPEAT);
		src.setWrap(GL_REPEAT, GL_REPEAT);
		dst.setWrap(GL_REPEAT, GL_REPEAT);
		dst = shade(list_of(src)(vectorField), getShader("sh1_main2.glsl").c_str());
	}
	void updateApp2()
	{
		if(pause)
			return;
		auto img2=maketex(sx,sy,img_ifmt);
		auto vel2=maketex(sx,sy,velocity_ifmt);
		auto logVel=shade(list_of(velocity), "void shade() { vec2 v = fetch3(tex).xy; float len=length(v);"
			"if(len==0.0) { _out.xy=vec2(0.0); return; } else { _out.xy=normalize(v)*log(len+1.0); }"
			"}");
		processFluid(logVel, img,img2);
		processFluid(logVel, velocity,vel2);

		img=img2;
		velocity=vel2;

		auto imgdata=gettexdata<float>(img,img_fmt,GL_FLOAT);

		float sum = std::accumulate(imgdata.begin(), imgdata.end(), 0.0f);
		float avg = sum / (float)imgdata.area;
		cout<<"avg"<<avg<<endl;
		forxy(imgdata)
		{
			imgdata(p) += .5f - avg;
		}
		img=maketex(imgdata, img_ifmt);

		/*
		velocity=gauss3(velocity);
		img=gauss3(img);
		*/
		globaldict["abc2"]=1.0f;//sign(mouseY) * expRange(abs(mouseY), .1f, 40000.0f);
		auto gradients=shade(list_of(img), getShader("gradients.glsl").c_str());
		auto div=shade(list_of(gradients), getShader("div.glsl").c_str());
		velocity=shade(list_of(velocity)(gradients)(div), getShader("sh2.glsl").c_str());
	}
#if 0
	void updateApp()
	{
		if(pause)
			return;
		
		Image img2(sx, sy);
		Array2D<Vec2f> vel2(sx, sy);
		float maxOffset=0.0f;
		forxy(img)
		{
			Vec2f offset = velocity(p);
			offset = offset.safeNormalized() * std::log(offset.length() + 1.0f);
			maxOffset=max(maxOffset,offset.length());
			if(keys['i']){
				img2(p) = getBilinear(img, Vec2f(p) - offset);
				vel2(p) = getBilinear(velocity, Vec2f(p) - offset);
			} else {
				aaPoint(img2, Vec2f(p) + offset, img(p));
				aaPoint(vel2, Vec2f(p) + offset, velocity(p));
			}
		}
		cout<<"maxoffset"<<maxOffset<<endl;
		velocity = vel2;
		img = img2;

		float sum = std::accumulate(img.begin(), img.end(), 0.0f);
		float avg = sum / (float)img.area;
		forxy(img)
		{
			img(p) += .5f - avg;
		}

		//std::fill(velocity.begin(), velocity.end(), Vec2f::zero());
		velocity=gauss3(velocity);
		img=gauss3(img);
		Array2D<Vec2f> gradients(sx, sy);
		forxy(gradients)
		{
			gradients(p) = gradient_i(img, Vec2f(p));
			//gradients(p) = Vec2f(-gradients(p).y, gradients(p).x);
		}
		float abc = sign(mouseX) * expRange(abs(mouseX), .001f, 40000.0f);
		float abc2 = sign(mouseY) * expRange(abs(mouseY), .1f, 40000.0f);
		//float abc=min(1.0f, mouseX)*5.0f;// sign(mouseX) * exp(-3.0 + 10.0 * min(1.0f, abs(mouseX)));
		if(keys['o'])
		for(int x = 0; x < sx; x++)
		{
			for(int y = 0; y < sy; y++)
			{
				Vec2f p = Vec2f(x,y);
				Vec2f grad = gradients(x, y).safeNormalized();
				Vec2f grad_a = getBilinear(gradients, p+grad).safeNormalized();
				grad_a = -Vec2f(-grad_a.y, grad_a.x);
				Vec2f grad_b = getBilinear(gradients, p-grad).safeNormalized();
				grad_b = Vec2f(-grad_b.y, grad_b.x);
				Vec2f dir = grad_a + grad_b;
				dir *= abc;
				//aaPoint(velocity, p, dir);
				velocity(x, y) += dir;
			}
		}
		auto laplace = div(gradients);
		forxy(img) {
			if(laplace(p) > 0.0f) { // concave
				velocity(p) += gradients(p).safeNormalized() * abc2;
			} else {
				velocity(p) += -gradients(p).safeNormalized() * abc2;
			}
		}
	}
#endif
	void draw()
	{
		mouseX = getMousePos().x / (float)wsx;
		mouseY = getMousePos().y / (float)wsy;

		gl::clear(Color(0, 0, 0));
		updateApp2();

		auto tex2=shade(list_of(img), "void shade() { _out=vec3(fetch1()); }");
		gl::draw(tex2, getWindowBounds());
	}
};

#include "define_winmain.h"
#endif