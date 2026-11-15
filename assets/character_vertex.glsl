#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 boneMatrices[100];
uniform bool hasAnimation;
uniform int boneCount;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main() {
    vec4 localPos = vec4(aPos, 1.0);
    
    mat4 skin = mat4(1.0);  // fallback

    if (hasAnimation && boneCount > 0) {
        skin = mat4(0.0);
        float totalWeight = 0.0;
        
        for (int i = 0; i < 4; i++) {
            int id = aBoneIDs[i];
            float w = aWeights[i];
            
            if (id >= 0 && id < boneCount && w > 0.0) {
                skin += boneMatrices[id] * w;
                totalWeight += w;
            }
        }

        if (totalWeight > 0.0) {
            skin = skin / totalWeight;
            localPos = skin * localPos;
        } else {
            skin = mat4(1.0); // fallback
        }
    }

    vec4 worldPos = model * localPos;
    FragPos = vec3(worldPos);
    TexCoords = aTexCoords;

    // Normali corrette
    Normal = normalize(mat3(model * skin) * aNormal);

    gl_Position = projection * view * worldPos;
}
