#pragma once
#include <functional>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct AABB {                
    glm::vec3 c;              // center
    glm::vec3 e;              // half-extents
};

inline AABB ToWorldAABB(const AABB& local, const glm::mat4& M) {
    glm::vec3 C = glm::vec3(M * glm::vec4(local.c, 1.0f));
    glm::mat3 R = glm::mat3(M);
    glm::mat3 AR = glm::mat3(glm::abs(R[0]), glm::abs(R[1]), glm::abs(R[2]));
    glm::vec3 E = AR * local.e;
    return { C, E };
}

// Matrice per disegnare un cubo unitario sull'AABB in wireframe
inline glm::mat4 AABBModelMatrix(const AABB& w) {
    return glm::translate(glm::mat4(1.0f), w.c) * glm::scale(glm::mat4(1.0f), w.e * 2.0f);
}

// Disegna un AABB world come cubo wireframe
inline void DrawAABB_Wire(
    const AABB& wbox, const glm::vec3& color,
    const glm::mat4& view, const glm::mat4& proj,
    const std::function<void(const glm::mat4&, const glm::mat4&, const glm::mat4&, const glm::vec3&)>& renderCube)
{
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    renderCube(AABBModelMatrix(wbox), view, proj, color);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);
}

struct Capsule { glm::vec3 a, b; float r; };

inline glm::vec3 ClosestPointOnAABB(const glm::vec3& p, const AABB& b){
    glm::vec3 minB = b.c - b.e, maxB = b.c + b.e;
    return glm::clamp(p, minB, maxB);
}

inline bool CapsuleVsAABB(const Capsule& cap, const AABB& box, glm::vec3& contact){
    const int N = 8; float best = 1e9f;
    glm::vec3 bestP(0);
    for(int i=0;i<=N;i++){
        float t = i*(1.0f/N);
        glm::vec3 p = glm::mix(cap.a, cap.b, t);
        glm::vec3 q = ClosestPointOnAABB(p, box);
        float d2 = glm::dot(p - q, p - q);
        if(d2<best){ best=d2; bestP=q; }
    }
    contact = bestP;
    return best <= cap.r*cap.r;
}

// AABB ↔ AABB + vettore di separazione minimo (per push-out)
inline bool AABBvsAABB_Resolve(const AABB& A, const AABB& B, glm::vec3& push){
    glm::vec3 d = A.c - B.c;
    glm::vec3 o = (A.e + B.e) - glm::abs(d);
    if (o.x<=0 || o.y<=0 || o.z<=0){ push=glm::vec3(0); return false; }
    if (o.x < o.y && o.x < o.z)      push = glm::vec3((d.x<0?-o.x:o.x), 0, 0);
    else if (o.y < o.z)              push = glm::vec3(0, (d.y<0?-o.y:o.y), 0);
    else                             push = glm::vec3(0, 0, (d.z<0?-o.z:o.z));
    return true;
}

inline bool SweptCapsuleVsAABB(const Capsule& prev,
    const Capsule& curr,
    const AABB& box,
    float& toi,
    glm::vec3& contact)
{
    const int STEPS = 12;
    Capsule c; c.r = curr.r;

    glm::vec3 hit0;
    if (CapsuleVsAABB(prev, box, hit0)) { toi = 0.0f; contact = hit0; return true; }

    for (int i = 1; i <= STEPS; ++i) {
        float t = float(i) / float(STEPS);
        c.a = glm::mix(prev.a, curr.a, t);
        c.b = glm::mix(prev.b, curr.b, t);
        glm::vec3 h;
        if (CapsuleVsAABB(c, box, h)) {
            toi = t;
            contact = h;
            return true;
        }
    }
    return false;
}
