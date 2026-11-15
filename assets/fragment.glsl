#version 330 core

in vec3 FragPos;        
in vec3 Normal;        
in vec2 TexCoord;       
in vec3 LightPos;       
in vec3 ViewPos;   

out vec4 FragColor;

// Material uniforms
uniform vec3 objectColor;      
uniform vec3 lightColor;       
uniform float ambientStrength; 
uniform float specularStrength;
uniform int shininess;        

uniform bool useTexture;       
uniform sampler2D texture_diffuse1; 

uniform float explosionFactor; 
uniform bool isFragment;      
uniform bool useObjectColor;  
uniform float time;          

void main() {

    vec3 norm = normalize(Normal);
    

    vec3 lightDir = normalize(LightPos - FragPos);

    vec3 ambient = ambientStrength * lightColor;
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(ViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 baseColor;
    
    if (useObjectColor) {
        baseColor = objectColor;
    } else if (useTexture) {
        baseColor = texture(texture_diffuse1, TexCoord).rgb;
    } else {
        baseColor = objectColor;
    }
    
    if (isFragment && explosionFactor > 0.01) {
        
        float explosionIntensity = clamp(explosionFactor, 0.0, 1.0);
        
        vec3 hotExplosionColor = vec3(1.0, 0.25, 0.05);  // Arancione rosso
        vec3 fireColor = vec3(1.0, 0.45, 0.1);           // Arancione medio
        
        vec3 explosionColor = mix(hotExplosionColor, fireColor, explosionIntensity * 0.5);
        
        vec3 fragmentColor = mix(baseColor, explosionColor, 0.95);
        
        float coolingFactor = max(0.0, 1.0 - explosionIntensity * 0.8);
        float realisticPulse = 0.85 + 0.15 * sin(time * 1.8) * coolingFactor;
        
        float emissionIntensity = explosionIntensity * 0.6 * realisticPulse;
        vec3 emission = vec3(1.0, 0.7, 0.3) * emissionIntensity;
        
        float lightBoost = 1.0 + (explosionIntensity * 0.8);
        
        vec3 result = (ambient + diffuse * lightBoost + specular) * fragmentColor + emission;
        
        FragColor = vec4(result, 1.0);
        
    } else {

        vec3 buildingColor = baseColor;
        
        if (!useObjectColor && !useTexture) {
            buildingColor = vec3(0.65, 0.65, 0.65);
        }
        
        vec3 enhancedAmbient = ambient * 1.8;
        float enhancedDiff = max(dot(norm, lightDir) * 0.8 + 0.2, 0.0);
        vec3 enhancedDiffuse = enhancedDiff * lightColor;
        
        vec3 result = (enhancedAmbient + enhancedDiffuse + specular) * buildingColor;
        
        FragColor = vec4(result, 1.0);
    }  
    FragColor.rgb = clamp(FragColor.rgb, vec3(0.0), vec3(1.0));
}