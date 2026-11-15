#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <memory>

struct FragmentData {
    glm::vec3 position;
    float mass;
    glm::vec3 velocity;
    float boundingRadius;
    glm::vec3 angularVelocity;
    float restitution;
    glm::vec4 orientation;
    glm::vec3 force;
    float padding1;
    glm::vec3 initialPosition;
    float structuralIntegrity;
};

// Physics parameters
struct PhysicsParams {
    glm::vec3 explosionCenter;   // Epicentro dell'esplosione
    float explosionForce;        // Forza dell'esplosione

    glm::vec3 gravity;           // Gravità
    float airDamping;            // Resistenza dell'Aria

    float groundLevel;           // Livello di Collisione col Piano
    float deltaTime;             
    float currentTime;           
    float maxTime;               

    int numFragments;            // Numero di Frammenti attivi in Scena
    int resetSimulation;         
    float explosionRadius;       // Raggio dell'esplosione
    float padding;               
};

class ComputeShader {
private:
    GLuint m_computeProgram;
    GLuint m_fragmentsSSBO;      
    GLuint m_paramsUBO;         
    GLuint m_instanceSSBO;

    std::vector<FragmentData> m_fragmentsData;
    PhysicsParams m_physicsParams;

    bool m_useDoubleBuffering = true;
    int m_currentBufferIndex = 0;
    std::vector<FragmentData> m_fragmentsBufferA;
    std::vector<FragmentData> m_fragmentsBufferB;

    int m_numFragments;
    bool m_initialized;
    bool m_simulationActive;

    GLuint compileComputeShader(const std::string& source);
    std::string loadShaderFromFile(const std::string& filePath);
    void checkShaderErrors(GLuint shader, const std::string& type);

    void createBuffers();
    void updateBuffers();
    void setupFragmentData(const std::vector<glm::vec3>& centers, const std::vector<float>& radii);

public:
    ComputeShader();
    ~ComputeShader();

    bool initialize();
    bool loadFromFile(const std::string& shaderPath);
    bool loadFromSource(const std::string& shaderSource);

    void initializeFragments(const std::vector<glm::vec3>& fragmentCenters,
        const std::vector<float>& fragmentRadii);
    void initializeFragments(int numFragments, const glm::vec3& buildingCenter,
        const glm::vec3& buildingSize);

    GLuint getInstanceSSBO() const { return m_instanceSSBO; }

    void startExplosion(const glm::vec3& center, float force, float radius);
    void resetSimulation();
    void pauseSimulation();
    void resumeSimulation();

    void downloadFragmentDataFromGPU();
    void forceGPUDataSync();

    void update(float deltaTime);
    void dispatch();
    void synchronize();

    void setGravity(const glm::vec3& gravity) { m_physicsParams.gravity = gravity; }
    void setAirDamping(float damping) { m_physicsParams.airDamping = damping; }
    void setGroundLevel(float level);
    void setExplosionParameters(const glm::vec3& center, float force, float radius);

    std::vector<FragmentData> getFragmentData();
    std::vector<glm::mat4> getFragmentTransforms();
    const PhysicsParams& getPhysicsParams() const { return m_physicsParams; }
    PhysicsParams& getPhysicsParams() { return m_physicsParams; }

    void updateSingleFragment(int index, const FragmentData& fragmentData);
    void updateFragmentData(const std::vector<FragmentData>& fragmentData);
    void uploadFragmentDataToGPU(); 

    bool isInitialized() const { return m_initialized; }
    bool isSimulationActive() const { return m_simulationActive; }
    int getNumFragments() const { return m_numFragments; }
    float getSimulationProgress() const;

    bool hasFragmentData() const { return !m_fragmentsData.empty(); }
    const std::vector<FragmentData>& getFragmentDataRef() const { return m_fragmentsData; }

    void ensureBufferSize(int numFragments);

    void validateFragmentData() const;

    void cleanup();
};