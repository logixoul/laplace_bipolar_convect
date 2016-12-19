#pragma once

#include "precompiled.h"
#include "util.h"

template<class T>
Array2D<T> gettexdata(gl::Texture tex, GLenum format, GLenum type) {
	Array2D<T> data(tex.getWidth(), tex.getHeight());
	tex.bind();
	glGetTexImage(GL_TEXTURE_2D, 0, format, type, data.data);
	GLenum errCode;
	if ((errCode = glGetError()) != GL_NO_ERROR) 
	{
		cout<<"ERROR"<<errCode<<endl;
	}
	return data;
}

/*template<class T>
Vec2i wrapPoint(Array2D<T> const& src, Vec2i p)
{
	Vec2i wp = p;
	wp.x %= src.w; if(wp.x < 0) wp.x += src.w;
	wp.y %= src.h; if(wp.y < 0) wp.y += src.h;
	return wp;
}
template<class T>
T const& getWrapped(Array2D<T> const& src, Vec2i p)
{
	return src(wrapPoint(src, p));
}
template<class T>
T const& getWrapped(Array2D<T> const& src, int x, int y)
{
	return getWrapped(src, Vec2i(x, y));
}
template<class T>
T& getWrapped(Array2D<T>& src, Vec2i p)
{
	return src(wrapPoint(src, p));
}
template<class T>
T& getWrapped(Array2D<T>& src, int x, int y)
{
	return getWrapped(src, Vec2i(x, y));
}*/
template<class T>
Array2D<T> blur(Array2D<T> const& src, int r)
{
	T zero=::zero<T>();
	Array2D<T> newImg(sx, sy);
	float divide = pow(2.0f * r + 1.0f, 2.0f);
	for(int x = 0; x < sx; x++)
	{
		T sum = zero;
		for(int y = -r; y <= r; y++)
		{
			sum += get_wrapZeros(src, x, y);
		}
		for(int y = 0; y < sy; y++)
		{
			newImg(x, y) = sum;
			sum -= get_wrapZeros(src, x, y - r);
			sum += get_wrapZeros(src, x, y + r + 1);
		}
	}
	Array2D<T> newImg2(sx, sy);
	for(int y = 0; y < sy; y++)
	{
		T sum = zero;
		for(int x = -r; x <= r; x++)
		{
			sum += get_wrapZeros(newImg, x, y);
		}
		for(int x = 0; x < sx; x++)
		{
			newImg2(x, y) = sum;
			sum -= get_wrapZeros(newImg, x - r, y);
			sum += get_wrapZeros(newImg, x + r + 1, y);
		}
	}
	forxy(newImg2)
		newImg2(p) /= divide;
	return newImg2;
}
namespace {
	string glsl_hsvToRGB="vec3 hsvToRGB(float h, float s, float v)"
"{"
"	float c = v * s;"
"	h = mod((h * 6.0), 6.0);"
"	float x = c * (1.0 - abs(mod(h, 2.0) - 1.0));"
"	vec4 color;"
 ""
	"if (0.0 <= h && h < 1.0) {"
	"	color = vec4(c, x, 0.0, a);"
	"} else if (1.0 <= h && h < 2.0) {"
	"	color = vec4(x, c, 0.0, a);"
	"} else if (2.0 <= h && h < 3.0) {"
"		color = vec4(0.0, c, x, a);"
"	} else if (3.0 <= h && h < 4.0) {"
"		color = vec4(0.0, x, c, a);"
"	} else if (4.0 <= h && h < 5.0) {"
"		color = vec4(x, 0.0, c, a);"
"	} else if (5.0 <= h && h < 6.0) {"
"		color = vec4(c, 0.0, x, a);"
"	} else {"
"		color = vec4(0.0, 0.0, 0.0, a);"
"	}"
 ""
	"color.rgb += v - c;"
 ""
	"return color.rgb;";
}
inline float sin1(float x){
	return .5f+.5f*sin(x);
}
template<class T>
Array2D<T> gauss3(Array2D<T> src) {
	T zero=::zero<T>();
	Array2D<T> dst1(sx, sy);
	Array2D<T> dst2(sx, sy);
	forxy(dst1)
		dst1(p) = .25f * (2 * get_wrapZeros(src, p.x, p.y) + get_wrapZeros(src, p.x-1, p.y) + get_wrapZeros(src, p.x+1, p.y));
	forxy(dst2)
		dst2(p) = .25f * (2 * get_wrapZeros(dst1, p.x, p.y) + get_wrapZeros(dst1, p.x, p.y-1) + get_wrapZeros(dst1, p.x, p.y+1));
	return dst2;
}
template<class T>
void aaPoint(Array2D<T>& dst, Vec2f const& p, T const& value)
{
	aaPoint(dst, p.x, p.y, value);
}
template<class T>
T& zero() {
	static T val=T()*0.0f;
	val = T()*0.0f;
	return val;
}
//get_wrapZeros
template<class T>
T& get_wrapZeros(Array2D<T>& src, int x, int y)
{
	if(x < 0 || y < 0 || x >= src.w || y >= src.h)
	{
		return zero<T>();
	}
	return src(x, y);
}
template<class T>
T const& get_wrapZeros(Array2D<T> const& src, int x, int y)
{
	if(x < 0 || y < 0 || x >= src.w || y >= src.h)
	{
		return zero<T>();
	}
	return src(x, y);
}
template<class T>
void aaPoint(Array2D<T>& dst, float x, float y, T const& value)
{
	int ix = x, iy = y;
	float fx = ix, fy = iy;
	if(x < 0.0f && fx != x) { fx--; ix--; }
	if(y < 0.0f && fy != y) { fy--; iy--; }
	float fractx = x - fx;
	float fracty = y - fy;
	float fractx1 = 1.0 - fractx;
	float fracty1 = 1.0 - fracty;
	
	if(ix<0||iy<0||ix>=dst.w-1||iy>=dst.h-1)
	{
		get_wrapZeros(dst, ix, iy) += (fractx1 * fracty1) * value;
		get_wrapZeros(dst, ix, iy+1) += (fractx1 * fracty) * value;
		get_wrapZeros(dst, ix+1, iy) += (fractx * fracty1) * value;
		get_wrapZeros(dst, ix+1, iy+1) += (fractx * fracty) * value;
		return;
	}
	dst(ix, iy) += (fractx1 * fracty1) * value;
	dst(ix, iy+1) += (fractx1 * fracty) * value;
	dst(ix+1, iy) += (fractx * fracty1) * value;
	dst(ix+1, iy+1) += (fractx * fracty) * value;
}
inline Array2D<float> to01(Array2D<float> a) {
	auto minn = *std::min_element(a.begin(), a.end());
	auto maxx = *std::max_element(a.begin(), a.end());
	auto b = a.clone();
	forxy(b) {
		b(p) -= minn;
		b(p) /= (maxx - minn);
	}
	return b;
}
template<class T>
T getBilinear(Array2D<T>& src, Vec2f const& p)
{
	return getBilinear(src, p.x, p.y);
}
template<class T>
T getBilinear(Array2D<T>& src, float x, float y)
{
	int ix = x, iy = y;
	float fx = ix, fy = iy;
	if(x < 0.0f && fx != x) { fx--; ix--; }
	if(y < 0.0f && fy != y) { fy--; iy--; }
	float fractx = x - fx;
	float fracty = y - fy;
	
	if(ix<0||iy<0||ix>=src.w-1||iy>=src.h-1)
	{
		return lerp(
			lerp(get_wrapZeros(src, ix, iy), get_wrapZeros(src, ix + 1, iy), fractx),
			lerp(get_wrapZeros(src, ix, iy + 1), get_wrapZeros(src, ix + 1, iy + 1), fractx),
			fracty);
	}
	return lerp(
		lerp(src(ix, iy), src(ix + 1, iy), fractx),
		lerp(src(ix, iy + 1), src(ix + 1, iy + 1), fractx),
		fracty);
}
/*template<class T>
Vec2f gradient_f(Array2D<T> src, Vec2f p)
{
	Vec2f gradient;
	gradient.x = getBilinear(src, p.x + 1, p.y) - getBilinear(src, p.x - 1, p.y);
	gradient.y = getBilinear(src, p.x, p.y + 1) - getBilinear(src, p.x, p.y - 1);
	return gradient;
}*/

template<class Func>
class MapHelper {
private:
	static Func* func;
public:
	typedef typename decltype((*func)(Vec2i(0, 0))) result_dtype;
};

template<class TSrc, class Func>
auto map(Array2D<TSrc> a, Func func) -> Array2D<typename MapHelper<Func>::result_dtype> {
	auto result = Array2D<typename MapHelper<Func>::result_dtype>(a.w, a.h);
	forxy(a) {
		result(p) = func(p);
	}
	return result;
}
template<class T>
Vec2f gradient_i(Array2D<T> src, Vec2i p)
{
	//if(p.x<1||p.y<1||p.x>=src.w-1||p.y>=src.h-1)
	//	return Vec2f::zero();
	Vec2f gradient;
	gradient.x = get_wrapZeros(src,p.x + 1, p.y) - get_wrapZeros(src, p.x - 1, p.y);
	gradient.y = get_wrapZeros(src,p.x, p.y + 1) - get_wrapZeros(src, p.x, p.y - 1);
	return gradient;
}
inline Array2D<float> div(Array2D<Vec2f> a) {
	return ::map(a, [&](Vec2i p) -> float {
		auto dGx_dx = (a.wr(p.x+1,p.y).x-a.wr(p.x-1,p.y).x) / 2.0f;
		auto dGy_dy = (a.wr(p.x,p.y+1).y-a.wr(p.x,p.y-1).y) / 2.0f;
		return dGx_dx + dGy_dy;
	});
}

inline int sign(float f)
{
	if(f < 0)
		return -1;
	if(f > 0)
		return 1;
	return 0;
}

inline float expRange(float x, float min, float max) {
	return exp(lerp(log(min),log(max), x));
}

inline float niceExpRangeX(float mouseX, float min, float max) {
	float x2=sign(mouseX)*std::max(0.0f,abs(mouseX)-40.0f/(float)App::get()->getWindowWidth());
	return sign(x2)*expRange(abs(x2), min, max);
}

inline float niceExpRangeY(float mouseY, float min, float max) {
	float y2=sign(mouseY)*std::max(0.0f,abs(mouseY)-40.0f/(float)App::get()->getWindowHeight());
	return sign(y2)*expRange(abs(y2), min, max);
}
inline unsigned int ilog2 (unsigned int val) {
    unsigned int ret = -1;
    while (val != 0) {
        val >>= 1;
        ret++;
    }
    return ret;
}
class ShaderDb {
public:
	std::map<string,string> db;
};
extern ShaderDb shaderDb;
inline string getShader(string filename) {
	//void loadFile(std::vector<unsigned char>& buffer, const std::string& filename);
	auto& db=shaderDb.db;
	if(db.find(filename)==db.end()) {
		std::vector<unsigned char> buffer;
		loadFile(buffer,filename);
		string bufferStr(&buffer[0],&buffer[buffer.size()]);
		db[filename]=bufferStr;
	}
	return db[filename];
}
