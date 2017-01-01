#include "precompiled.h"
#if 1
//#include "ciextra.h"
#include "util.h"
#include <numeric>
#include "stuff.h"
#include "shade.h"
#include "gpgpu.h"
#include "gpuBlur2_4.h"
#include "hdrwrite.h"

typedef gl::Texture Tex;
typedef Array2D<float> Image;
gl::Texture toSave;
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
		if(e.getChar() == 'v') {// set velocities to be non-grandfloating
			auto sum = std::accumulate(velocity.begin(), velocity.end(), velocity(0, 0) * 0.0f);
			auto avg = sum / (float)img.area;
			auto desiredAvg = Vec2f::one() * .5f;
			forxy(velocity)
			{
				velocity(p) += desiredAvg - avg;
			}
		}
		if(e.getChar() == 's') {
			saveHdrScreenshot();
		}
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
			velocity(p) = Vec2f::zero();
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
	void saveHdrScreenshot()
	{
		Array2D<Vec3f> readback(::toSave.getSize());
		::toSave.bind();
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, readback.data);
		writeRgbeFile("output.hdr", readback.Size(), (float*)readback.data);
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
			aaPoint(img2, Vec2f(p) + offset, img(p));
			aaPoint(vel2, Vec2f(p) + offset, velocity(p));
		}
		//cout<<"maxoffset"<<maxOffset<<endl;
		velocity = vel2;
		img = img2;

		float sum = std::accumulate(img.begin(), img.end(), 0.0f);
		float avg = sum / (float)img.area;
		/*int plustimes=1;
		float plusadd=(.5f-avg)*float(img.area)/float(plustimes);
		for(int i=0;i<plustimes;i++) { int x=ci::randInt(sx); int y=ci::randInt(sy); img(x,y)+=plusadd; }*/
		forxy(img)
		{
			img(p) += .5f - avg;
		}

		//std::fill(velocity.begin(), velocity.end(), Vec2f::zero());
		for(int i = 0; i < 3; i++) {
			velocity=gauss3(velocity);
			img=gauss3(img);
		}
		// iterate main logic without blur (and w/o convection?) for stronger effect with less velocity. So it's smoother.
		for(int i = 0; i < 3; i++) {
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
	}
	Array2D<Vec3f> avgNormalize(Array2D<Vec3f> arr){
		float sum=0.0f;

		forxy(arr) { sum+=arr(p).dot(Vec3f::one()/3.0f); }
		float avg=sum/float(arr.area);
		auto arr2=arr.clone();
		forxy(arr2) arr2(p)=arr2(p)*.3f/avg;
		return arr2;
	}

	static Array2D<float> contrastize(Array2D<float> img)
	{
		auto result = img.clone();
		forxy(img) {
			float& here = result(p);
			here -= .5;
			here *= 20.0;
			here += .5;
			//here = max(0.0f, here);
		}
		return result;
	}

	void draw()
	{
		mouseX = getMousePos().x / (float)wsx;
		mouseY = getMousePos().y / (float)wsy;

		gl::clear(Color(0, 0, 0));
		updateApp();

		auto contrastized = contrastize(img);

		static auto envMap = gl::Texture(ci::loadImage("envmap4.png"));
		static auto colorSource = gl::Texture(ci::loadImage("color.png"));

		auto tex = maketex(contrastized,GL_R16F);
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
			/*"float fresnelAmount = getFresnel(I, N);"
			"fresnelAmount = 1.0;"
			"c += getEnv(R) * fresnelAmount;"*/
			"vec3 diffuse = vec3(1.0) * max(dot(R, I), 0.0);"
			"c += diffuse;"
			"img = max(img, 0.0);"
			"c += img * vec3(1.0, 0.1, 0.0);"
			//"c += N * .5 + .5;"
			//"c += getEnv(vec3(tc.x, tc.y, 0.0));"
			//"if(fetch1(tex) <= surfTensionThres)"
			//"	c = vec3(0.0);"
			"_out = c;",
			ShadeOpts().ifmt(GL_RGB16F).scale(::scale / 2.0f), // NOTE I'm dividing by 2!
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
			//"	c *= pow(clum,1.0);" // make it darker
			//"	c/=vec3(1.0)-c*.99; c*=1.0;"
			"	c = pow(c, vec3(2.0));"
			"	c *= 2.0;"
			"	return c;"
			"}\n"
			);
		// the bloom is the slow part
		tex2=bloom(tex2);
		::toSave = tex2;

		tex2 = shade2(tex2, colorSource,
			"vec3 c = fetch3();"
			/*"float clum=dot(c, w);"
			"c /= clum + 1.0;"
			"c *= 1.5;"*/
			"c = min(c, vec3(1.0));"
			"vec3 colorSrc = fetch3(tex2);"
			"vec3 cHCL = RGB2HCL(c);"
			"vec3 colorSrcHCL = RGB2HCL(colorSrc);"
			"cHCL.x = colorSrcHCL.x;"
			"c = HCL2RGB(cHCL);"
			"_out = c;"
			,
			ShadeOpts(),
			getShader("hcl_lib.fs")
			+ "vec3 w = vec3(.22, .71, .07);");

		tex2 = gammaCorrect(tex2);
		gl::draw(tex2, getWindowBounds());
	}

	gl::Texture gammaCorrect(gl::Texture& tex) {
		auto texG = shade2(tex, "vec3 c = fetch3(); c = max(c, vec3(0.0)); c = pow(c, vec3(1.0/2.2)); _out = c;");
		return texG;
	}

	gl::Texture bloom(gl::Texture tex) {
		auto texForBlur = shade2(tex,
			"vec3 c = fetch3();"
			"vec3 w = vec3(.22, .71, .07);"
			"float lum = dot(w, c);"
			"if(lum < 1.0) {"
			"	c = vec3(0.0);"
			"}"
			"_out = c;");

		auto texb = gpuBlur2_4::run_longtail(texForBlur, 5, 1.0f);

		auto texBloom = shade2(tex, texb,
			"_out = fetch3(tex) + fetch3(tex2) * .2;"
			);

		return texBloom;
	}
};

#include "define_winmain.h"
#endif