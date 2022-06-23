#version 410

in vec2 texture_coords;

uniform sampler2D spriteCenario;
uniform sampler2D spriteObjeto;
uniform float offsetx;
uniform float offsety;
uniform bool isObjectx;

uniform float weight;

out vec4 frag_color; 

void main () {
    if(isObjectx){
		 vec4 texel = texture (spriteObjeto, 
        vec2(texture_coords.x + offsetx, texture_coords.y + offsety));
        if(texel.a < 0.5) {
            discard;
        }
        frag_color = texel;
    }
	else {
		 vec4 texel = texture (spriteCenario, 
        vec2(texture_coords.x + offsetx, texture_coords.y + offsety));
        if(texel.a < 0.5) {
            discard;
        }
        frag_color = texel;
	}
}
