#version 100

attribute vec2 position;

uniform vec4 color;

varying vec4 point_color;

void main()
{
	point_color = color;
	gl_Position = vec4(position, 0.0, 1.0);
}
