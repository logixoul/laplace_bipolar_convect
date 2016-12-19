/*forxy(img) {
			if(laplace(p) > 0.0f) { // concave
				velocity(p) += gradients(p).safeNormalized() * abc2;
			} else {
				velocity(p) += -gradients(p).safeNormalized() * abc2;
			}
		}*/

void shade(){
	vec2 v=fetch2(tex);
	vec2 g=fetch2(tex2);
	float laplace=fetch1(tex3);
	if(laplace > 0.0) { // concave
		v += safeNormalized(g) * abc2;
	} else {
		v += -safeNormalized(g) * abc2;
	}
	_out.xy=v;
}