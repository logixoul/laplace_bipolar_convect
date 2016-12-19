void shade(){
	vec2 srcL=fetch2(tex,tc+tsize*vec2(-1.0,0.0));
	vec2 srcR=fetch2(tex,tc+tsize*vec2(1.0,0.0));
	vec2 srcT=fetch2(tex,tc+tsize*vec2(0.0,-1.0)); 
	vec2 srcB=fetch2(tex,tc+tsize*vec2(0.0,1.0));
	float dx=(srcR.x-srcL.x)/2.0;
	float dy=(srcB.y-srcT.y)/2.0;
	_out=vec3(dx+dy);
}