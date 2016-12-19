void shade(){
	float srcL=fetch1(tex,tc+tsize*vec2(-1.0,0.0));
	float srcR=fetch1(tex,tc+tsize*vec2(1.0,0.0));
	float srcT=fetch1(tex,tc+tsize*vec2(0.0,-1.0)); 
	float srcB=fetch1(tex,tc+tsize*vec2(0.0,1.0));
	float dx=(srcR-srcL)/2.0;
	float dy=(srcB-srcT)/2.0;
	_out.xy=vec2(dx,dy);
}