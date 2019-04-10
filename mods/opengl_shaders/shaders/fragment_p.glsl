#version 100

#ifdef GL_ES
precision mediump float;
precision mediump int;
#endif

varying vec4 point_color;

void main()
{
	gl_FragColor = point_color;
}
