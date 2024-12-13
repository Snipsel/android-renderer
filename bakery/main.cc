#include<assimp/cimport.h>
#include<assimp/material.h>
#include<assimp/scene.h>
#include<assimp/postprocess.h>
#define STB_IMAGE_IMPLEMENTATION
#include"stb_image.h"

void load_mesh(char const * inpath){
    stbi_set_flip_vertically_on_load(true);
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

    static char outpath[1<<10];

    aiString albedo_path = {};
    int albedo_ext_index=0;
    int albedo_width=0, albedo_height=0;
    if(scene->mMaterials[mesh.mMaterialIndex]->GetTexture(aiTextureType_BASE_COLOR, mesh.mMaterialIndex, &albedo_path)==AI_SUCCESS){
        static char albedo_in_path[1<<10];
        snprintf(albedo_in_path, sizeof(albedo_in_path), "%.*s/%s", parent_index, inpath, albedo_path.data);

        albedo_ext_index = albedo_path.length-1;
        while(albedo_ext_index>=0 && albedo_path.data[albedo_ext_index]!='.')
            --albedo_ext_index;

        FILE* in_albedo = fopen(albedo_in_path, "rb");
        if(!in_albedo){
            fprintf(stderr, "file not found: \"%s\"\n", albedo_in_path);
            exit(1);
        }
        uint8_t* pixels = stbi_load_from_file(in_albedo, &albedo_width, &albedo_height, NULL, 4);
        fclose(in_albedo);

        auto const out_albedo_size = albedo_width*albedo_height*4;
        snprintf(outpath, sizeof(outpath), "assets/%.*s", albedo_ext_index, albedo_path.data);
        FILE* out_albedo_file = fopen(outpath, "wb");
        fwrite(pixels, out_albedo_size, 1, out_albedo_file);
        fclose(out_albedo_file);
        stbi_image_free(pixels);
    }

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


    printf("Mesh %s{\n", mesh.mName.data);
    printf("    .filename      = \"%s\",\n",   mesh.mName.data);
    printf("    .vertex_count  = %u,\n",       mesh.mNumVertices);
    printf("    .index_count   = %u,\n",       mesh.mNumFaces*3);
    printf("    .albedo        = \"%.*s\",\n", albedo_ext_index, albedo_path.data);
    printf("    .albedo_width  = %u,\n",       albedo_width);
    printf("    .albedo_height = %u,\n",       albedo_height);
    printf("};\n");

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
    fclose(outfile);

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

