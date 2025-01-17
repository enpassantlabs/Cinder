//#version 150
#version 310 es

precision mediump float;


uniform mat4	ciModelViewProjection;
in vec4			ciPosition;
in vec2			ciTexCoord0;

out vec2	vTexCoord0;

void main( void )
{
	vTexCoord0 = ciTexCoord0;
	gl_Position = ciModelViewProjection * ciPosition;
}
