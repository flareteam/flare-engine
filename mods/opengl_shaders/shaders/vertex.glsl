#version 100

attribute vec2 position;

uniform vec4 offset;

uniform vec4 texelOffset;

varying vec2 texcoord;

void main()
{
	if (offset == vec4(0.0))
	{
		gl_Position = vec4(position, 0.0, 1.0);
	}
	else
	{
		float leftCornerX = position.x * offset.z - 1.0 + offset.z;
		float leftCornerY =  - (position.y * offset.w - 1.0 + offset.w);

		gl_Position = vec4(leftCornerX + offset.x, leftCornerY - offset.y, 0.0, 1.0);
	}
    texcoord.x = (position.x * 0.5 + 0.5) / texelOffset.x + texelOffset.y;
    texcoord.y = (position.y * 0.5 + 0.5) / texelOffset.z + texelOffset.w;
}
