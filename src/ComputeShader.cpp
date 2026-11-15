#include "ComputeShader.h" 
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/quaternion.hpp>     


ComputeShader::ComputeShader()
    : m_computeProgram(0),
    m_fragmentsSSBO(0),
    m_instanceSSBO(0),     
    m_paramsUBO(0),
    m_numFragments(0),
    m_initialized(false),
    m_simulationActive(false) {

    m_physicsParams.explosionCenter = glm::vec3(0.0f, 0.0f, 0.0f);
    m_physicsParams.explosionForce = 20.0f;
    m_physicsParams.gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    m_physicsParams.airDamping = 0.1f;
    m_physicsParams.groundLevel = -1.05f;
    m_physicsParams.deltaTime = 0.016f;
    m_physicsParams.currentTime = 0.0f;
    m_physicsParams.maxTime = 10.0f;
    m_physicsParams.explosionRadius = 5.0f;
    m_physicsParams.resetSimulation = 0;
    m_physicsParams.numFragments = 0;

    m_physicsParams.padding = 0.0f; 
}

ComputeShader::~ComputeShader() {
    cleanup();
}

bool ComputeShader::initialize() {
    std::cout << "Inizializzazione ComputeShader (caricamento da file)..." << std::endl;

    if (loadFromFile("compute_physics.glsl")) {
        std::cout << "Compute shader caricato con successo dal file!" << std::endl;
        createBuffers(); 
        m_initialized = true;
        return true;
    }

    std::cerr << "Errore durante il caricamento di compute_physics.glsl!" << std::endl;
    std::cerr << "Assicurati che il file sia nella directory principale del progetto." << std::endl;
    return false;
}

std::string ComputeShader::loadShaderFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "ComputeShader: Impossibile aprire il file: " << filePath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

bool ComputeShader::loadFromFile(const std::string& shaderPath) {
    std::string source = loadShaderFromFile(shaderPath);
    if (source.empty()) {
        std::cerr << "ComputeShader: Errore durante il caricamento dello shader dal file: " << shaderPath << std::endl;
        return false;
    }
    return loadFromSource(source);
}

bool ComputeShader::loadFromSource(const std::string& shaderSource) {
    if (m_computeProgram) {
        glDeleteProgram(m_computeProgram);
        m_computeProgram = 0;
    }

    GLuint computeShader = compileComputeShader(shaderSource);
    if (computeShader == 0) return false;

    m_computeProgram = glCreateProgram();
    glAttachShader(m_computeProgram, computeShader);
    glLinkProgram(m_computeProgram);

    GLint success;
    glGetProgramiv(m_computeProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetProgramInfoLog(m_computeProgram, 1024, nullptr, infoLog);
        std::cerr << "Compute shader linking fallito:\n" << infoLog << std::endl;
        glDeleteShader(computeShader);
        glDeleteProgram(m_computeProgram);
        m_computeProgram = 0;
        return false;
    }

    glDeleteShader(computeShader);
    std::cout << "Compute shader compilato e linkato con successo!" << std::endl;
    return true;
}

GLuint ComputeShader::compileComputeShader(const std::string& source) {
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "Compilazione del compute shader fallita:\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

void ComputeShader::checkShaderErrors(GLuint shader, const std::string& type) {
    GLint success;
    GLchar infoLog[1024];

    if (type == "PROGRAM") {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR: " << infoLog << std::endl;
        }
    }
    else {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
        }
    }
}

void ComputeShader::createBuffers() {
    if (m_paramsUBO) {
        glDeleteBuffers(1, &m_paramsUBO);
    }

    glGenBuffers(1, &m_paramsUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_paramsUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(PhysicsParams), &m_physicsParams, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_paramsUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0); // Scollega

    std::cout << "ComputeShader: Buffers creati con successo" << std::endl;
}

// Funzione helper 
void ComputeShader::setupFragmentData(const std::vector<glm::vec3>& centers, const std::vector<float>& radii) {
    //l'implementazione è stata spostata in initializeFragments
}

void ComputeShader::initializeFragments(const std::vector<glm::vec3>& fragmentCenters,
    const std::vector<float>& fragmentRadii) {

    if (fragmentCenters.size() != fragmentRadii.size()) {
        std::cerr << "ComputeShader: I vettori di centri e raggi devono avere la stessa dimensione" << std::endl;
        return;
    }

    m_numFragments = static_cast<int>(fragmentCenters.size());
    m_fragmentsData.clear();
    m_fragmentsData.reserve(m_numFragments);

    std::cout << "ComputeShader: Inizializzazione di " << m_numFragments << " frammenti" << std::endl;

    // Crea i dati dei frammenti
    for (int i = 0; i < m_numFragments; ++i) {
        FragmentData fragment = {};
        fragment.position = fragmentCenters[i];
        fragment.initialPosition = fragmentCenters[i];
        fragment.velocity = glm::vec3(0.0f);
        fragment.angularVelocity = glm::vec3(0.0f);
        fragment.mass = 1.0f;
        fragment.boundingRadius = fragmentRadii[i];
        fragment.restitution = 0.3f;  // Coefficiente di restituzione realistico
        fragment.orientation = glm::vec4(0, 0, 0, 1);
        fragment.force = glm::vec3(0.0f);
        fragment.padding1 = 0.0f;
        fragment.structuralIntegrity = 1.0f;

        m_fragmentsData.push_back(fragment);
    }

    // Crea/ricrea il buffer dei frammenti (SSBO binding 1)
    if (m_fragmentsSSBO) {
        glDeleteBuffers(1, &m_fragmentsSSBO);
    }
    glGenBuffers(1, &m_fragmentsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_fragmentsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        m_numFragments * sizeof(FragmentData),
        m_fragmentsData.data(), GL_DYNAMIC_DRAW);

    // Verifica buffer creation
    GLint bufferSize;
    glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &bufferSize);

    // Collega al binding point 1 come specificato nello shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_fragmentsSSBO);

    // Verifica binding
    GLint boundBuffer;
    glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 1, &boundBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Scollega

    if (m_instanceSSBO) {
        glDeleteBuffers(1, &m_instanceSSBO);
    }

    struct InstanceTransform {
        glm::mat4 modelMatrix;
        glm::vec3 velocity;
        float explosionTime;
    };

    size_t instanceBufferSize = m_numFragments * sizeof(InstanceTransform);

    glGenBuffers(1, &m_instanceSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, instanceBufferSize, nullptr, GL_DYNAMIC_DRAW);

    std::vector<InstanceTransform> initialTransforms(m_numFragments);
    for (int i = 0; i < m_numFragments; ++i) {
        initialTransforms[i].modelMatrix = glm::translate(glm::mat4(1.0f), fragmentCenters[i]);
        initialTransforms[i].velocity = glm::vec3(0.0f);
        initialTransforms[i].explosionTime = -1.0f;
    }

    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, instanceBufferSize, initialTransforms.data());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_instanceSSBO);

    GLint instanceBufferCheck;
    glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &instanceBufferCheck);
    if (instanceBufferCheck > 0) {
        std::cout << "✅ Instance SSBO creato con successo (" << instanceBufferCheck << " bytes)" << std::endl;
    }
    else {
        std::cerr << "❌ Errore nella creazione dell'Instance SSBO!" << std::endl;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Scollega


    m_physicsParams.numFragments = m_numFragments;
    updateBuffers();

    std::cout << "ComputeShader: Inizializzazione completata con " << m_numFragments
        << " frammenti e instance buffer" << std::endl;
}

void ComputeShader::initializeFragments(int numFragments, const glm::vec3& buildingCenter,
    const glm::vec3& buildingSize) {

    if (numFragments <= 0) {
        std::cerr << "ComputeShader: Conteggio frammenti non valido: " << numFragments << std::endl;
        return;
    }

    std::vector<glm::vec3> centers;
    std::vector<float> radii;
    centers.reserve(numFragments);
    radii.reserve(numFragments);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-0.5f, 0.5f); // Per offset casuali
    std::uniform_real_distribution<float> radiusDis(0.1f, 0.3f); // Per raggi casuali

    for (int i = 0; i < numFragments; ++i) {
        glm::vec3 randomOffset = glm::vec3(dis(gen), dis(gen), dis(gen)) * buildingSize;
        centers.push_back(buildingCenter + randomOffset);
        radii.push_back(radiusDis(gen));
    }

    initializeFragments(centers, radii); 
}

void ComputeShader::startExplosion(const glm::vec3& center, float force, float radius) {

    setExplosionParameters(center, force, radius);
    m_physicsParams.currentTime = 0.0f; 
    m_physicsParams.resetSimulation = 0; 
    m_simulationActive = true;
    updateBuffers(); 

    std::cout << "DEBUG: Explosion started - Force: " << m_physicsParams.explosionForce
        << ", Radius: " << m_physicsParams.explosionRadius
        << ", Center: (" << m_physicsParams.explosionCenter.x
        << ", " << m_physicsParams.explosionCenter.y
        << ", " << m_physicsParams.explosionCenter.z << ")" << std::endl;
}

void ComputeShader::setGroundLevel(float level) {
    m_physicsParams.groundLevel = level;

    if (m_paramsUBO) {
        glBindBuffer(GL_UNIFORM_BUFFER, m_paramsUBO);
        glBufferSubData(GL_UNIFORM_BUFFER,
            offsetof(PhysicsParams, groundLevel),
            sizeof(float),
            &level);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

// Resetto la simulazione.
void ComputeShader::resetSimulation() {
    std::cout << "ComputeShader: Reset della simulazione" << std::endl;

    m_physicsParams.resetSimulation = 1; 
    m_physicsParams.currentTime = 0.0f;
    m_simulationActive = false; 

    updateBuffers();
    dispatch();
    synchronize(); 

    downloadFragmentDataFromGPU();

    m_physicsParams.resetSimulation = 0;
    updateBuffers();
}

// Mette in pausa la simulazione.
void ComputeShader::pauseSimulation() {
    m_simulationActive = false;
    std::cout << "ComputeShader: Simulazione in pausa" << std::endl;
}

// Riprende la simulazione.
void ComputeShader::resumeSimulation() {
    m_simulationActive = true;
    std::cout << "ComputeShader: Simulazione ripresa" << std::endl;
}

// Aggiorna i parametri di tempo e li invia alla GPU.
void ComputeShader::update(float deltaTime) {
    if (!m_simulationActive || !m_initialized) return;

    // Convalida e limita il delta time
    if (deltaTime <= 0.0f || deltaTime > 0.1f) {
        deltaTime = 1.0f / 60.0f; // Default a 60 FPS
    }

    m_physicsParams.deltaTime = deltaTime;
    m_physicsParams.currentTime += deltaTime;

    // Aggiorna solo il buffer dei parametri GPU (UBO)
    glBindBuffer(GL_UNIFORM_BUFFER, m_paramsUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PhysicsParams), &m_physicsParams);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void ComputeShader::dispatch() {
    if (!m_initialized || m_numFragments == 0) return;

    glUseProgram(m_computeProgram);

    // BIND TUTTI I BUFFER
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_paramsUBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_fragmentsSSBO);

    if (m_instanceSSBO) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_instanceSSBO);
    }

    GLuint numGroups = (m_numFragments + 63) / 64;
    glDispatchCompute(numGroups, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
}

// Sincronizza le operazioni della GPU.
void ComputeShader::synchronize() {
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
}

// Imposta i parametri dell'esplosione.
void ComputeShader::setExplosionParameters(const glm::vec3& center, float force, float radius) {
    m_physicsParams.explosionCenter = center;
    m_physicsParams.explosionForce = force;
    m_physicsParams.explosionRadius = radius;
}

// Aggiorna i buffer (specificamente l'UBO dei parametri fisici).
void ComputeShader::updateBuffers() {
    if (!m_initialized) return;

    glBindBuffer(GL_UNIFORM_BUFFER, m_paramsUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PhysicsParams), &m_physicsParams);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// Restituisce una copia dei dati dei frammenti dalla CPU.
std::vector<FragmentData> ComputeShader::getFragmentData() {
    if (!m_initialized) {
        std::cerr << "ComputeShader: Impossibile ottenere i dati dei frammenti - non inizializzato" << std::endl;
        return {};
    }
    return m_fragmentsData;
}

// Restituisce le matrici di trasformazione dei frammenti basate sui dati CPU.
std::vector<glm::mat4> ComputeShader::getFragmentTransforms() {
    static glm::vec3 lastPos0 = glm::vec3(0.0f);
    if (!m_fragmentsData.empty()) {
        glm::vec3 currentPos0 = m_fragmentsData[0].position;
        if (glm::length(currentPos0 - lastPos0) > 0.001f) {
            lastPos0 = currentPos0;
        }
    }

    std::vector<glm::mat4> transforms;
    transforms.reserve(m_fragmentsData.size());

    for (const auto& fragment : m_fragmentsData) {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), fragment.position);

        transforms.push_back(transform);
    }

    return transforms;
}

// Restituisce il progresso della simulazione (0.0 a 1.0).
float ComputeShader::getSimulationProgress() const {
    if (m_physicsParams.maxTime <= 0.0f) return 0.0f;
    return std::min(1.0f, m_physicsParams.currentTime / m_physicsParams.maxTime);
}

// Aggiorna i dati di un singolo frammento nella CPU.
void ComputeShader::updateSingleFragment(int index, const FragmentData& fragmentData) {
    if (!m_initialized || index < 0 || index >= static_cast<int>(m_fragmentsData.size())) {
        std::cerr << "ComputeShader: Indice frammento non valido per l'aggiornamento: " << index << std::endl;
        return;
    }
    // Aggiorna i dati lato CPU
    m_fragmentsData[index] = fragmentData;
}

// Aggiorna tutti i dati dei frammenti nella CPU.
// Poi chiama uploadFragmentDataToGPU() per sincronizzare.
void ComputeShader::updateFragmentData(const std::vector<FragmentData>& fragmentData) {
    if (!m_initialized) {
        std::cerr << "ComputeShader: Impossibile aggiornare i dati dei frammenti - non inizializzato" << std::endl;
        return;
    }

    if (fragmentData.size() != m_fragmentsData.size()) {
        std::cerr << "ComputeShader: Dimensione dei dati dei frammenti non corrispondente" << std::endl;
        return;
    }

    // Aggiorna i dati lato CPU
    m_fragmentsData = fragmentData;

    // Carica sulla GPU
    uploadFragmentDataToGPU();
}

// Carica i dati dei frammenti dalla CPU alla GPU (SSBO).
void ComputeShader::uploadFragmentDataToGPU() {
    if (!m_initialized || m_fragmentsData.empty()) return;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_fragmentsSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
        m_fragmentsData.size() * sizeof(FragmentData),
        m_fragmentsData.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

}

// Scarica i dati dei frammenti dalla GPU (SSBO) alla CPU.
void ComputeShader::downloadFragmentDataFromGPU() {
    if (!m_initialized || m_fragmentsData.empty()) return;

    glm::vec3 oldPos0 = m_fragmentsData.empty() ? glm::vec3(0.0f) : m_fragmentsData[0].position;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_fragmentsSSBO);

    void* bufferData = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
        m_fragmentsData.size() * sizeof(FragmentData),
        GL_MAP_READ_BIT); 

    if (bufferData) {
        memcpy(m_fragmentsData.data(), bufferData, m_fragmentsData.size() * sizeof(FragmentData));
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        glm::vec3 newPos0 = m_fragmentsData.empty() ? glm::vec3(0.0f) : m_fragmentsData[0].position;
        float posChange = glm::length(newPos0 - oldPos0);
    }
    else {
        std::cerr << "ComputeShader: Errore durante il mapping del buffer dei frammenti per la lettura" << std::endl;
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cout << "OpenGL error during buffer mapping: " << error << std::endl;
        }
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

// Assicura che la dimensione del buffer dei frammenti sia sufficiente.
void ComputeShader::ensureBufferSize(int numFragments) {
    if (numFragments <= 0) return;

    if (static_cast<int>(m_fragmentsData.size()) != numFragments) {
        m_fragmentsData.resize(numFragments);
        m_numFragments = numFragments;
        m_physicsParams.numFragments = numFragments;

        // Ricrea i buffer con la nuova dimensione se già inizializzato
        if (m_initialized) {
            // Elimina il vecchio SSBO e ricrealo con la nuova dimensione
            if (m_fragmentsSSBO) {
                glDeleteBuffers(1, &m_fragmentsSSBO);
            }
            glGenBuffers(1, &m_fragmentsSSBO);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_fragmentsSSBO);
            glBufferData(GL_SHADER_STORAGE_BUFFER,
                m_numFragments * sizeof(FragmentData),
                m_fragmentsData.data(), GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_fragmentsSSBO);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            // Aggiorna l'UBO con il nuovo numero di frammenti
            updateBuffers();
        }
    }
}

// Valida i dati dei frammenti (per debug).
void ComputeShader::validateFragmentData() const {
    std::cout << "=== VALIDAZIONE COMPUTE SHADER ===" << std::endl;
    std::cout << "Inizializzato: " << (m_initialized ? "SI" : "NO") << std::endl;
    std::cout << "Conteggio Frammenti: " << m_numFragments << std::endl;
    std::cout << "Dimensione Dati Frammenti (CPU): " << m_fragmentsData.size() << std::endl;
    std::cout << "Simulazione Attiva: " << (m_simulationActive ? "SI" : "NO") << std::endl;

    if (!m_fragmentsData.empty()) {
        // Valida i primi pochi frammenti
        for (size_t i = 0; i < std::min(size_t(3), m_fragmentsData.size()); ++i) {
            const auto& frag = m_fragmentsData[i];
            std::cout << "Frammento " << i << ": pos("
                << frag.position.x << ", " << frag.position.y << ", " << frag.position.z
                << "), vel(" << glm::length(frag.velocity) << ")" << std::endl;
        }
    }
    std::cout << "=================================" << std::endl;
}

// Pulisce le risorse OpenGL.
void ComputeShader::cleanup() {
    std::cout << "ComputeShader: Pulizia in corso..." << std::endl;

    if (m_computeProgram) {
        glDeleteProgram(m_computeProgram);
        m_computeProgram = 0;
    }

    if (m_fragmentsSSBO) {
        glDeleteBuffers(1, &m_fragmentsSSBO);
        m_fragmentsSSBO = 0;
    }

    if (m_paramsUBO) {
        glDeleteBuffers(1, &m_paramsUBO);
        m_paramsUBO = 0;
    }

    m_fragmentsData.clear();
    m_initialized = false;
    m_simulationActive = false;
    m_numFragments = 0;
}

void ComputeShader::forceGPUDataSync() {
    // Lasciato per compatibilità
}