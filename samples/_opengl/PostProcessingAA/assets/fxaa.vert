//#version 150
#version 310 es
precision mediump float;

uniform mat4	ciModelViewProjection;

in vec4			ciPosition;

void main()
{	
	gl_Position = ciModelViewProjection * ciPosition;
}
