#pragma once

class ComputeShader;

struct CollisionStats {
    int activeFragments = 0;
    int sleepingFragments = 0;
    int groundCollisions = 0;
    int fragmentCollisions = 0;
};

class CollisionSystem {
private:
    ComputeShader* m_computeShader = nullptr;
    int m_numFragments = 0;

public:
    CollisionSystem() = default;
    ~CollisionSystem() = default;

    bool initialize(ComputeShader* computeShader);
    void setupFragments(int numFragments);
};