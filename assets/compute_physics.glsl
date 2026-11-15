#version 430

#extension GL_ARB_compute_shader : require
#extension GL_ARB_shader_storage_buffer_object : require

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

struct FragmentData {
    vec3 position;
    float mass;
    vec3 velocity;
    float boundingRadius;
    vec3 angularVelocity;
    float restitution;
    vec4 orientation;
    vec3 force;     
    float padding1;
    vec3 initialPosition;
    float structuralIntegrity;
};

layout(std140, binding = 0) uniform PhysicsParams {
    vec3 explosionCenter;
    float explosionForce;
    vec3 gravity;
    float airDamping;
    float groundLevel;
    float deltaTime;
    float currentTime;
    float maxTime;
    int   numFragments;
    int   resetSimulation;
    float explosionRadius;
    float padding;   
};

layout(std430, binding = 1) restrict buffer FragmentsBuffer {
    FragmentData fragments[];
};

// Substep target: Riduce instabilità con dt grandi.
const float TARGET_DT = 0.008; 
const int   MAX_SUBSTEPS = 16;

// Attrito semplificato sul terreno
const float GROUND_FRICTION = 0.6;
const float GROUND_ANG_FRICTION = 0.7;

// Margini di sicurezza per il terreno per evitare attraversamenti
const float GROUND_MARGIN = 0.05;
const float MIN_GROUND_VELOCITY = 0.05; // Velocità minima per fermare i frammenti
const float MAX_BOUNDING_RADIUS = 0.5;  // Limita il raggio massimo per evitare problemi

float noise(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 45.164))) * 43758.5453);
}

vec4 quatMul(vec4 a, vec4 b) {
    vec3 va = a.xyz;
    vec3 vb = b.xyz;
    float wa = a.w;
    float wb = b.w;
    return vec4( wa*vb + wb*va + cross(va, vb),
                 wa*wb - dot(va, vb) );
}

vec4 quatNormalize(vec4 q) {
    float invLen = inversesqrt(dot(q, q));
    return q * invLen;
}

// Integra orientazione da velocità angolare (rad/s) su dt.
vec4 integrateOrientation(vec4 q, vec3 omega, float dt) {
    float wlen = length(omega);
    if (wlen < 1e-6) return q;
    vec4 omegaQ = vec4(omega, 0.0);
    // Semi-esplicito semplice: passo di Eulero sul quaternion, poi normalize
    vec4 dq = quatMul(omegaQ, q) * (0.5 * dt);
    q = q + dq;
    return quatNormalize(q);
}

// Reset singolo frammento
void resetFragment(inout FragmentData fragment) {
    fragment.position = fragment.initialPosition;
    fragment.velocity = vec3(0.0);
    fragment.angularVelocity = vec3(0.0);
    fragment.orientation = vec4(0.0, 0.0, 0.0, 1.0);
    fragment.structuralIntegrity = 1.0;
}

// Funzione per gestire la collisione con il terreno per ogni frammento
void handleGroundCollision(inout FragmentData fragment) {
    // Normalizzo il bounding radius per evitare valori estremi
    float safeBoundingRadius = clamp(fragment.boundingRadius, 0.01, MAX_BOUNDING_RADIUS);
    
    // Calcolo il ground level specifico per questo frammento
    // Ogni frammento atterra alla sua altezza specifica basata sul suo raggio
    float fragmentGroundLevel = groundLevel + safeBoundingRadius + GROUND_MARGIN;
    
    // Controlla se il centro del frammento è sotto il suo ground level specifico
    if (fragment.position.y <= fragmentGroundLevel) {
        // Correggo la posizione per questo frammento specifico
        fragment.position.y = fragmentGroundLevel + 0.01; // Margine extra di sicurezza
        
        // Applico restituzione solo se il frammento si sta muovendo verso il basso
        if (fragment.velocity.y < 0.0) {
            fragment.velocity.y = -fragment.velocity.y * fragment.restitution;
            
            // Se la velocità di rimbalzo è troppo bassa, la blocco
            if (abs(fragment.velocity.y) < MIN_GROUND_VELOCITY) {
                fragment.velocity.y = 0.0;
            }
        }
        
        // Applico attrito orizzontale quando è a contatto con il terreno
        float horizontalSpeed = length(fragment.velocity.xz);
        if (horizontalSpeed > 0.001) {
            vec2 horizontalDir = normalize(fragment.velocity.xz);
            float frictionForce = horizontalSpeed * GROUND_FRICTION;
            float newHorizontalSpeed = max(0.0, horizontalSpeed - frictionForce);
            fragment.velocity.xz = horizontalDir * newHorizontalSpeed;
        } else {
            fragment.velocity.xz = vec2(0.0);
        }
        
        // Applico smorzamento angolare quando tocca il terreno
        fragment.angularVelocity *= GROUND_ANG_FRICTION;
    }
    
    // Controllo di Sicurezza Extra
    float absoluteMinimum = groundLevel - 0.1;
    if (fragment.position.y < absoluteMinimum) {
        fragment.position.y = fragmentGroundLevel + 0.01;
        fragment.velocity.y = max(0.0, fragment.velocity.y);
    }
}

void main() {
    uint index = gl_GlobalInvocationID.x;
    if (index >= uint(numFragments)) return;

    if (resetSimulation != 0) {
        resetFragment(fragments[index]);
        return;
    }

    FragmentData fragment = fragments[index];

    // Esplosione
    if (currentTime < 1.0) {
        vec3 toFragment = fragment.initialPosition - explosionCenter;
        float distance = length(toFragment);

        if (explosionRadius > 0.0 && distance < explosionRadius) {
            vec3 explosionDirection = (distance > 1e-3) ? normalize(toFragment) : vec3(0.0, 1.0, 0.0);

            // Forza che decresce quadraticamente con la distanza
            float forceFalloff = 1.0 - (distance / explosionRadius);
            forceFalloff *= forceFalloff;

            // Rumore per caos direzionale
            vec3 rnd = vec3(
                noise(fragment.initialPosition + vec3(1.0, 0.0, 0.0)) - 0.5,
                noise(fragment.initialPosition + vec3(0.0, 1.0, 0.0)) - 0.5,
                noise(fragment.initialPosition + vec3(0.0, 0.0, 1.0)) - 0.5
            ) * 0.5;

            // Velocità iniziale
            vec3 explosionVelocity = (explosionDirection + rnd) * explosionForce * forceFalloff;

            // Spingo verso l'alto
            explosionVelocity.y = abs(explosionVelocity.y) + explosionForce * 0.3;

            fragment.velocity = explosionVelocity;
            fragment.structuralIntegrity = 0.0; // totalmente distrutto

            // Rotazione iniziale casuale
            fragment.angularVelocity = rnd * 5.0;
        }
    }

    // Fisica
    if (fragment.structuralIntegrity <= 0.0) {
        int substeps = int(ceil(deltaTime / TARGET_DT));
        substeps = clamp(substeps, 1, MAX_SUBSTEPS);
        float dt = (substeps > 0) ? (deltaTime / float(substeps)) : deltaTime;

        for (int s = 0; s < substeps; ++s) {
            // Accelerazione costante: gravità 
            // Integratore: Eulero semi-implicito con smorzamento esponenziale
            float linDamp = exp(-airDamping * dt);
            fragment.velocity = (fragment.velocity + gravity * dt) * linDamp;

            fragment.position += fragment.velocity * dt;

            // Gestione collisione terreno con la nuova funzione
            handleGroundCollision(fragment);

            // Integrazione orientazione + smorzamento angolare esponenziale
            fragment.angularVelocity *= exp(-airDamping * dt);
            fragment.orientation = integrateOrientation(fragment.orientation, fragment.angularVelocity, dt);
        }
    }

    fragments[index] = fragment;
}