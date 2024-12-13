#include<assimp/cimport.h>
#include<assimp/material.h>
#include<assimp/scene.h>
#include<assimp/postprocess.h>
#define STB_IMAGE_IMPLEMENTATION
#include"stb_image.h"

struct Extent{ int width, height; };

Extent write_img(aiScene const& scene, aiTextureType texture_type, char const* inpath, int inpath_len, FILE* outfile){
    stbi_set_flip_vertically_on_load(true);
    aiMesh const& mesh = **scene.mMeshes;
    aiString albedo_path = {};
    if(scene.mMaterials[mesh.mMaterialIndex]->GetTexture(texture_type, mesh.mMaterialIndex, &albedo_path)!=AI_SUCCESS)
        return Extent{0,0};
    static char albedo_in_path[1<<10];
    snprintf(albedo_in_path, sizeof(albedo_in_path), "%.*s/%s", inpath_len, inpath, albedo_path.data);

    FILE* in_albedo = fopen(albedo_in_path, "rb");
    if(!in_albedo){
        fprintf(stderr, "file not found: \"%s\"\n", albedo_in_path);
        exit(1);
    }
    Extent ret = {0,0};
    uint8_t* pixels = stbi_load_from_file(in_albedo, &ret.width, &ret.height, NULL, 4);
    fclose(in_albedo);

    fwrite(pixels, 4*ret.width*ret.height, 1, outfile);
    stbi_image_free(pixels);
    return ret;
}

void load_mesh(char const * inpath){
    aiScene const* const scene = aiImportFile(inpath,
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

    auto const out_pos_size    = mesh.mNumVertices*3*sizeof(float);
    auto const out_uv_size     = mesh.mNumVertices*2*sizeof(float);
    auto const out_idx_size    = mesh.mNumFaces*3*sizeof(uint16_t);
    float*    const out_pos    = (float*)malloc(out_pos_size);
    float*    const out_uv     = (float*)malloc(out_uv_size);
    uint16_t* const out_idx    = (uint16_t*)malloc(out_idx_size);


    for(int i=0; i<mesh.mNumVertices; i++){
        out_pos[3*i+0] = mesh.mVertices[i].x;
        out_pos[3*i+1] = mesh.mVertices[i].y;
        out_pos[3*i+2] = mesh.mVertices[i].z;
        out_uv[2*i+0]  = mesh.mTextureCoords[0][i].x;
        out_uv[2*i+1]  = mesh.mTextureCoords[0][i].y;
    }
    for(int i=0; i<mesh.mNumFaces; i++){
        out_idx[3*i+0] = (uint16_t)mesh.mFaces[i].mIndices[0];
        out_idx[3*i+1] = (uint16_t)mesh.mFaces[i].mIndices[1];
        out_idx[3*i+2] = (uint16_t)mesh.mFaces[i].mIndices[2];
    }

    fprintf(stderr, "faces: %d\n", mesh.mNumFaces);

    static char outpath[1<<10];
    snprintf(outpath, sizeof(outpath), "assets/%s", mesh.mName.data);
    fprintf(stderr, "%s -> %s\n", inpath, outpath);
    FILE* outfile = fopen(outpath, "wb");
    if(!outfile){
        fprintf(stderr, "could not open \"%s\"\n", outpath);
        exit(1);
    }
    fwrite(out_pos, out_pos_size, 1, outfile);
    fwrite(out_uv,  out_uv_size,  1, outfile);
    fwrite(out_idx, out_idx_size, 1, outfile);
    Extent albedo = write_img(*scene, aiTextureType_BASE_COLOR, inpath, parent_index, outfile);
    fclose(outfile);

    printf("Mesh %s{\n", mesh.mName.data);
    printf("    .filename      = \"%s\",\n", mesh.mName.data);
    printf("    .vertex_count  = %u,\n",     mesh.mNumVertices);
    printf("    .index_count   = %u,\n",     3*mesh.mNumFaces);
    printf("    .albedo_width  = %u,\n",     albedo.width);
    printf("    .albedo_height = %u,\n",     albedo.height);
    printf("};\n");

    free(out_idx);
    free(out_uv);
    free(out_pos);
    aiReleaseImport(scene);

}

int main(int argc, char** argv){
    for(int i=1; i<argc; i++){
        load_mesh(argv[i]);
    }
}

