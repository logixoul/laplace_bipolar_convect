#include "precompiled.h"
#if 1
//#include "ciextra.h"
#include "util.h"
#include <numeric>
#include "stuff.h"
#include "shade.h"
#include "gpgpu.h"
#include "gpuBlur2_4.h"

typedef gl::Texture Tex;
typedef Array2D<float> Image;
int wsx=800, wsy = 600;
int scale = 4;
int sx = wsx / scale;
int sy = wsy / scale;
Image img(sx, sy);
float mouseX, mouseY;
bool pause = false;
bool keys[256];
bool oneFastFrame = false;
Array2D<Vec2f> velocity(sx, sy, Vec2f::zero());
ci::Rectf area(sx * 1.0f/6.0f, sy * 1.0f/6.0f, sx * 5.0f/6.0f, sy * 5.0f/6.0f);
gl::Texture texNew;

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
		forxy(img)
		{
			img(p) = ci::randFloat();
		}
		texNew=maketex(img, GL_R16F);
	}
	
	void mouseDown(MouseEvent e)
	{
	}
	
	template<class T>
	Array2D<T> gauss3Weak(Array2D<T> src) {
		auto blurred = gauss3(src);
		auto blurredWeak = Array2D<T>(src.Size());
		forxy(blurredWeak) {
			blurredWeak(p) = lerp(src(p), blurred(p), .3f);
		}
		return blurredWeak;
	}

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
		/*int plustimes=10;
		float plusadd=(.5f-avg)*float(img.area)/float(plustimes);
		for(int i=0;i<plustimes;i++) { int x=ci::randInt(sx); int y=ci::randInt(sy); img(x,y)+=plusadd; }*/
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
		float abc = niceExpRangeX(mouseX, .001f, 40000.0f);
		float abc2 = niceExpRangeY(mouseY, .01f, 4000.0f);
		//float abc=min(1.0f, mouseX)*5.0f;// sign(mouseX) * exp(-3.0 + 10.0 * min(1.0f, abs(mouseX)));
		if(keys['o'])
		for(int x = 0; x < sx; x++)
		{
			for(int y = 0; y < sy; y++)
			{
				Vec2f p = Vec2f(x,y);
				Vec2f grad = gradients(x, y).safeNormalized();
				grad = Vec2f(-grad.y, grad.x);;
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
	Array2D<Vec3f> avgNormalize(Array2D<Vec3f> arr){
		float sum=0.0f;

		forxy(arr) { sum+=arr(p).dot(Vec3f::one()/3.0f); }
		float avg=sum/float(arr.area);
		auto arr2=arr.clone();
		forxy(arr2) arr2(p)=arr2(p)*.3f/avg;
		return arr2;
	}
	void draw()
	{
		mouseX = getMousePos().x / (float)wsx;
		mouseY = getMousePos().y / (float)wsy;

		gl::clear(Color(0, 0, 0));
		updateApp();

		/*auto imgt=maketex(img,GL_R16F);
		auto velt=maketex(velocity,GL_RG16F);
		globaldict["abc3"]=niceExpRangeX(mouseX,1.0,1000000.0);
		auto imgBigt=shade(list_of(imgt), "void shade(){_out=fetch3();}");
		int scalelog=2;
		int times=2*scalelog;globaldict["times"]=times;

		auto merged=shade(list_of(imgBigt)(velt),
			"void shade() { _out.x=fetch1(tex);_out.yz=fetch2(tex2); }", ShadeOpts().ifmt(GL_RGB16F));
		vector<gl::Texture> mergeds;
		cout<<"=="<<endl;
		for(int i=0;i<times;i++){
			auto sh=getShader("forward_convect.glsl");
			float fscale=(int(fmod(i,times/float(scalelog)))==0)?2.0f:1.0f;
			merged=shade(list_of(merged), "void shade() {_out=fetch3();}", fscale); // UPSCALE "merged"
			auto logVelt=shade(list_of(merged), "void shade() { vec2 v = fetch3(tex).yz; float len=length(v);"
				"if(len==0.0) { _out.xy=vec2(0.0); return; } else { _out.xy=normalize(v)*log(len+1.0); }"
				"}", ShadeOpts().ifmt(GL_RG16F));
			if(times-i<=3)mergeds.push_back(merged);
			merged=shade(list_of(merged)(logVelt), sh.c_str());
		}
		auto toDraw=shade(list_of(mergeds[mergeds.size()-1]),"void shade(){_out=vec3(fetch1());}");*/

		static auto envMap = gl::Texture(ci::loadImage("envmap4.png"));

		auto tex = maketex(img,GL_RGB16F);
		//tex = shade2(tex, "vec3 c = fetch3(); _out = c / (c + vec3(1.0));");
		auto grads = get_gradients_tex(tex);
		auto tex2 = shade2(tex, grads, envMap,
			"vec2 grad = fetch2(tex2);"
			"float img = fetch1(tex);"
			"vec3 N = normalize(vec3(-grad.x, -grad.y, -1.0));"
			"vec3 I=-normalize(vec3(tc.x-.5, tc.y-.5, 1.0));"
			"float eta=1.0/1.3;"
			"vec3 R=refract(I, N, eta);"
			"vec3 c = getEnv(R);"
			//"c = mix(albedo, c, pow(.9, fetch1(tex) * 50.0));" // tmp
			"R = reflect(I, N);"
			"float fresnelAmount = getFresnel(I, N);"
			"if(img > 0.0)"
			"	c += getEnv(R) * fresnelAmount * 10.0;" // *10.0 to tmp simulate one side of the envmap being brighter than the other
			"vec3 diffuse = vec3(1.0) * max(dot(R, I), 0.0);"
			"c += diffuse;"
			"c += img * vec3(1.0, 0.1, 0.0);"
			//"c += N * .5 + .5;"
			//"c += getEnv(vec3(tc.x, tc.y, 0.0));"
			//"if(fetch1(tex) <= surfTensionThres)"
			//"	c = vec3(0.0);"
			"_out = c;",
			ShadeOpts().ifmt(GL_RGB16F).scale(4.0f),
			"float PI = 3.14159265358979323846264;\n"
			"float getFresnel(vec3 I, vec3 N) {"
			"	float R0 = 0.01;" // maybe is ok. but wikipedia has a way for calculating it.
			"	float dotted = dot(I, N);"
			"	return R0 + (1.0-R0) * pow(1.0-dotted, 5.0);"
			"}"
			"vec2 latlong(vec3 v) {\n"
			"v = v.xzy;\n"
			"v = normalize(v);\n"
			"float theta = acos(-v.z);\n" // +z is up
			"\n"
			"v.y=-v.y;\n"
			"float phi = atan(v.y, v.x) + PI;\n"
			//"return vec2(phi / (2.0*PI), theta / (PI/2.0));\n"
			"return vec2(phi / (2.0*PI), theta / (PI));\n"
			"}\n"
			"vec3 w = vec3(.22, .71, .07);"
		"vec3 getEnv(vec3 v) {\n"
			"	vec3 c = fetch3(tex3, latlong(v));\n"
			//"	c = 5.0*pow(c, vec3(2.0));"
			"	c = pow(c, vec3(2.2));" // gamma correction
			//"	c=smoothstep(vec3(0.0),vec3(1.0),c);"
			"	float clum=dot(c, w);"
			"	c *= pow(clum,1.0);" // make it darker
			//"	c/=vec3(1.0)-c*.99; c*=1.0;"
			"	return c;"
			"}\n"
			);
		//tex = shade2(tex, "vec3 c = fetch3(); c *= 3.0; c /= c + 1.0; _out = c;");

		/*auto tex2b = gpuBlur2_4::run_longtail(tex2, 5, 1.0f);

		tex2 = shade2(tex2, tex2b,
			"_out = fetch3(tex) + fetch3(tex2) * .2;"
			);*/

		gl::draw(tex2, getWindowBounds());
	}
};

#include "define_winmain.h"
#endif