#include "CollisionSystem.h"
#include "ComputeShader.h"

bool CollisionSystem::initialize(ComputeShader* computeShader) {
    m_computeShader = computeShader;
    return true; 
}

void CollisionSystem::setupFragments(int numFragments) {
    m_numFragments = numFragments;
}
