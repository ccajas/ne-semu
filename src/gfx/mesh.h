enum meshProps 
{
    mesh_Normals   = 1,
    mesh_Colors    = 2,
    mesh_Texcoords = 4,
    mesh_Tangents  = 8
};

typedef struct Mesh_struct
{
    char name[128];
    /* Material and offset data */
    uint32_t materialID;
    uint32_t indexOffset, lineIndexOffset;
    uint8_t  meshProperties;
}
Mesh;