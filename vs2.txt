#version 330 core

layout(location=0) in vec3 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec3 normal;
 
out vec2 fTexcoord;
 
void main()
{
  fTexcoord = texcoord;
  gl_Position = vec4( data[ gl_VertexID ], 0.0, 1.0);
}
