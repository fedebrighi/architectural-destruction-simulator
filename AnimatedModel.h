#pragma once

#include "Model.h"

struct aiNode;
struct aiScene;
struct aiMesh;

class AnimatedModel : public Model {
private:
    void SetVertexBoneDataToDefault(Vertex& vertex);
    void SetVertexBoneData(Vertex& vertex, int boneID, float weight);
    void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);
    void CreateMixamoBones(); // Crea ossa standard Mixamo nel caso fossero mancanti
    void FixBoneWeightsPostConstruction();

public:
    // Carica un modello animato
    AnimatedModel(const std::string& path);

    // Distruttore
    virtual ~AnimatedModel() = default;

    void DebugBoneMapping();

protected:
    std::unique_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene) override;
    void processNode(aiNode* node, const aiScene* scene) override;
};