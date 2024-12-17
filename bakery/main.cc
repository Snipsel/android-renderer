#include<assimp/cimport.h>
#include<assimp/material.h>
#include<assimp/scene.h>
#include<assimp/postprocess.h>
#define STB_IMAGE_IMPLEMENTATION
#include"stb_image.h"

struct vec2{ float x, y; };
struct vec3{ float x, y, z; };

struct Vertex{
    vec2 uv;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
};

struct Img{
    int width, height;
    uint8_t* pixels;
};

Img load_img(aiScene const& scene, aiTextureType texture_type, char const* inpath, int inpath_len){
    stbi_set_flip_vertically_on_load(true);
    aiMesh const& mesh = **scene.mMeshes;
    aiString path={};
    if(scene.mMaterials[mesh.mMaterialIndex]->GetTexture(texture_type, mesh.mMaterialIndex, &path)!=AI_SUCCESS)
        return Img{0,0,nullptr};
    static char in_path[1<<10];
    snprintf(in_path, sizeof(in_path), "%.*s/%s", inpath_len, inpath, path.data);

    FILE* in_albedo = fopen(in_path, "rb");
    if(!in_albedo){
        fprintf(stderr, "file not found: \"%s\"\n", in_path);
        exit(1);
    }
    Img ret = {0,0,nullptr};
    ret.pixels = stbi_load_from_file(in_albedo, &ret.width, &ret.height, NULL, 4);
    fclose(in_albedo);

    return ret;
}

void load_mesh(char const * inpath){
    aiScene const* const scene = aiImportFile(inpath,
        aiProcess_GenNormals             |
        aiProcess_CalcTangentSpace       |
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_PreTransformVertices   |
        aiProcess_GenUVCoords            |
        aiProcess_TransformUVCoords      );
    if(scene==nullptr){
       fprintf(stderr, "failed to load \"%s\"", inpath);
       exit(1);
    }
    if(scene->mNumMeshes != 1){
        fprintf(stderr, "%d meshes found, expected 1 mesh\n", scene->mNumMeshes);
        exit(1);
    }

    int parent_index = strlen(inpath)-1;
    while(parent_index>=0 && inpath[parent_index]!='/')
        --parent_index;

    aiMesh const& mesh = **scene->mMeshes;

    auto const out_pos_size    = mesh.mNumVertices*sizeof(vec3);
    auto const out_vert_size   = mesh.mNumVertices*sizeof(Vertex);
    auto const out_idx_size    = mesh.mNumFaces*3*sizeof(uint16_t);
    vec3*     const out_pos    = (vec3*)malloc(out_pos_size);
    Vertex*   const out_vert   = (Vertex*)malloc(out_vert_size);
    uint16_t* const out_idx    = (uint16_t*)malloc(out_idx_size);

    for(int i=0; i<mesh.mNumVertices; i++){
        out_pos[i] = (vec3){
            .x = mesh.mVertices[i].x,
            .y = mesh.mVertices[i].y,
            .z = mesh.mVertices[i].z,
        };
        out_vert[i] = {
            .uv = {
                .x = mesh.mTextureCoords[0][i].x,
                .y = mesh.mTextureCoords[0][i].y
            },
            .normal  = {
                .x = mesh.mNormals[i].x,
                .y = mesh.mNormals[i].y,
                .z = mesh.mNormals[i].z,
            },
            .tangent = {
                .x = mesh.mTangents[i].x,
                .y = mesh.mTangents[i].y,
                .z = mesh.mTangents[i].z
            },
            .bitangent = {
                .x = mesh.mBitangents[i].x,
                .y = mesh.mBitangents[i].y,
                .z = mesh.mBitangents[i].z
            },
        };
    }
    for(int i=0; i<mesh.mNumFaces; i++){
        out_idx[3*i+0] = (uint16_t)mesh.mFaces[i].mIndices[0];
        out_idx[3*i+1] = (uint16_t)mesh.mFaces[i].mIndices[1];
        out_idx[3*i+2] = (uint16_t)mesh.mFaces[i].mIndices[2];
    }

    Img const albedo = load_img(*scene, aiTextureType_BASE_COLOR, inpath, parent_index);
    Img const normal = load_img(*scene, aiTextureType_NORMALS,    inpath, parent_index);
    Img const metal_roughness = load_img(*scene, aiTextureType_DIFFUSE_ROUGHNESS, inpath, parent_index);

    // pack roughness into the alpha channel of albedo
    for(int i=0; i<albedo.width*albedo.height; i++){
        albedo.pixels[4*i+3] = metal_roughness.pixels[4*i+1];
    }

    // pack metalic into the alpha channel of normal
    for(int i=0; i<normal.width*normal.height; i++){
        normal.pixels[4*i+3] = metal_roughness.pixels[4*i+2];
    }

    static char outpath[1<<10];
    snprintf(outpath, sizeof(outpath), "assets/%s", mesh.mName.data);
    fprintf(stderr, "%s -> %s\n", inpath, outpath);
    FILE* outfile = fopen(outpath, "wb");
    if(!outfile){
        fprintf(stderr, "could not open \"%s\"\n", outpath);
        exit(1);
    }
    fwrite(out_pos,  out_pos_size,  1, outfile);
    fwrite(out_vert, out_vert_size, 1, outfile);
    fwrite(out_idx,  out_idx_size,  1, outfile);
    fwrite(albedo.pixels, 4*albedo.width*albedo.height, 1, outfile);
    fwrite(normal.pixels, 4*normal.width*normal.height, 1, outfile);
    fclose(outfile);

    printf("Mesh %s{\n", mesh.mName.data);
    printf("    .filename      = \"%s\",\n",  mesh.mName.data);
    printf("    .vertex_count  = %u,\n",      mesh.mNumVertices);
    printf("    .index_count   = %u,\n",    3*mesh.mNumFaces);
    printf("    .albedo_extent = {%u,%u},\n", albedo.width,  albedo.height);
    printf("    .normal_extent = {%u,%u},\n", normal.height, normal.height);
    printf("};\n");

    stbi_image_free(normal.pixels);
    stbi_image_free(albedo.pixels);
    free(out_idx);
    free(out_vert);
    free(out_pos);
    aiReleaseImport(scene);

}

int main(int argc, char** argv){
    for(int i=1; i<argc; i++){
        load_mesh(argv[i]);
    }
}

