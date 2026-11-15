#version 330 core

layout (location = 0) in vec3 aPos;    
layout (location = 1) in vec3 aNormal;  
layout (location = 2) in vec2 aTexCoord;  

uniform mat4 model;         // Model matrix (object → world)
uniform mat4 view;          // View matrix (world → camera)
uniform mat4 projection;    // Projection matrix (camera → screen)
uniform mat3 normalMatrix;  // Normal transformation matrix

uniform vec3 lightPos;     
uniform vec3 viewPos;      

out vec3 FragPos;           
out vec3 Normal;            
out vec2 TexCoord;         
out vec3 LightPos;          
out vec3 ViewPos;       

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));

    Normal = normalMatrix * aNormal;

    TexCoord = aTexCoord;

    LightPos = lightPos;
    ViewPos = viewPos;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}