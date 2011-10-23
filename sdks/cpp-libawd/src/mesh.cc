#include <stdlib.h>
#include <cstdio>

#include "platform.h"

#include "mesh.h"
#include "util.h"




AWDSubMesh::AWDSubMesh() :
    AWDAttrElement()
{
    this->num_streams = 0;
    this->first_stream = NULL;
    this->last_stream = NULL;
    this->next = NULL;
}

AWDSubMesh::~AWDSubMesh()
{
    AWDDataStream *cur;

    cur = this->first_stream;
    while (cur) {
        AWDDataStream *next = cur->next;
        cur->next = NULL;
        delete cur;
        cur = next;
    }

    this->first_stream = NULL;
    this->last_stream = NULL;
}


unsigned int
AWDSubMesh::get_num_streams()
{
    return this->num_streams;
}


AWDDataStream *
AWDSubMesh::get_stream_at(unsigned int idx)
{
    if (idx < this->num_streams) {
        unsigned int cur_idx;
        AWDDataStream *cur;

        cur_idx = 0;
        cur = this->first_stream;
        while (cur) {
            if (cur_idx == idx)
                return cur;

            cur_idx++;
            cur = cur->next;
        }
    }

    return NULL;
}


void 
AWDSubMesh::add_stream(AWD_mesh_str_type type, AWD_str_ptr data, awd_uint32 num_elements)
{
    AWDMeshDataStream *str;

    str = new AWDMeshDataStream((awd_uint8)type, data, num_elements);

    if (this->first_stream == NULL) {
        this->first_stream = str;
    }
    else {
        this->last_stream->next = str;
    }

    this->num_streams++;
    this->last_stream = str;
    this->last_stream->next = NULL;
}


awd_uint32
AWDSubMesh::calc_streams_length(bool wide_geom)
{
    awd_uint32 len;
    AWDDataStream *str;

    len = 0;
    str = this->first_stream;
    while (str) {
        len += 5 + str->get_length(wide_geom);
        str = str->next;
    }

    return len;
}


awd_uint32
AWDSubMesh::calc_sub_length(bool wide_geom, bool wide_mtx)
{
    awd_uint32 len;

    len = 4; // Sub-mesh header
    len += this->calc_streams_length(wide_geom);
    len += this->calc_attr_length(true,true, wide_geom, wide_mtx);

    return len;
}


void
AWDSubMesh::write_sub(int fd, bool wide_geom, bool wide_mtx)
{
    AWDDataStream *str;
    awd_uint32 sub_len;

    // Verify byte-order
    sub_len = UI32(this->calc_streams_length(wide_geom));

    // Write sub-mesh header
    write(fd, &sub_len, sizeof(awd_uint32));

    this->properties->write_attributes(fd, wide_geom, wide_mtx);

    str = this->first_stream;
    while(str) {
        str->write_stream(fd, wide_geom);
        str = str->next;
    }

    this->user_attributes->write_attributes(fd, wide_geom, wide_mtx);
}






AWDMeshData::AWDMeshData(const char *name, awd_uint16 name_len) :
    AWDBlock(MESH_DATA),
    AWDNamedElement(name, name_len),
    AWDAttrElement() 
{
    this->first_sub = NULL;
    this->last_sub = NULL;
    this->bind_mtx = NULL;
    this->num_subs = 0;
}

AWDMeshData::~AWDMeshData()
{
    AWDSubMesh *cur;

    cur = this->first_sub;
    while (cur) {
        AWDSubMesh *next = cur->next;
        cur->next = NULL;
        delete cur;
        cur = next;
    }

    if (this->bind_mtx) {
        free(this->bind_mtx);
        this->bind_mtx = NULL;
    }

    this->first_sub = NULL;
    this->last_sub = NULL;
}


void 
AWDMeshData::add_sub_mesh(AWDSubMesh *sub)
{
    if (this->first_sub == NULL) {
        this->first_sub = sub;
    }
    else {
        this->last_sub->next = sub;
    }
    
    this->num_subs++;
    this->last_sub = sub;
}


unsigned int
AWDMeshData::get_num_subs()
{
    return this->num_subs;
}


AWDSubMesh *
AWDMeshData::get_sub_at(unsigned int idx)
{
    if (idx < this->num_subs) {
        unsigned int cur_idx;
        AWDSubMesh *cur;

        cur_idx = 0;
        cur = this->first_sub;
        while (cur) {
            if (cur_idx == idx)
                return cur;

            cur_idx++;
            cur = cur->next;
        }
    }

    return NULL;
}



awd_float64 *
AWDMeshData::get_bind_mtx()
{
    return this->bind_mtx;
}


void
AWDMeshData::set_bind_mtx(awd_float64 *bind_mtx)
{
    this->bind_mtx = bind_mtx;
}


awd_uint32
AWDMeshData::calc_body_length(bool wide_geom, bool wide_mtx)
{
    AWDSubMesh *sub;
    awd_uint32 mesh_len;

    // Calculate length of entire mesh 
    // data (not block header)
    mesh_len = sizeof(awd_uint16); // Num subs
    mesh_len += sizeof(awd_uint16) + this->get_name_length();
    mesh_len += this->calc_attr_length(true,true, wide_geom, wide_mtx);
    sub = this->first_sub;
    while (sub) {
        mesh_len += sub->calc_sub_length(wide_geom, wide_mtx);
        sub = sub->next;
    }

    return mesh_len;
}


void
AWDMeshData::write_body(int fd, bool wide_geom, bool wide_mtx)
{
    awd_uint16 num_subs_be;
    AWDSubMesh *sub;

    // Write name and sub count
    num_subs_be = UI16(this->num_subs);
    awdutil_write_varstr(fd, this->get_name(), this->get_name_length()); 
    write(fd, &num_subs_be, sizeof(awd_uint16));

    // Write list of optional properties
    this->properties->write_attributes(fd, wide_geom, wide_mtx);

    // Write all sub-meshes
    sub = this->first_sub;
    while (sub) {
        sub->write_sub(fd, wide_geom, wide_mtx);
        sub = sub->next;
    }
    
    // Write list of user attributes
    this->user_attributes->write_attributes(fd, wide_geom, wide_mtx);
}





AWDMeshInst::AWDMeshInst(const char *name, awd_uint16 name_len, AWDMeshData *data) :
    AWDSceneBlock(MESH_INSTANCE, name, name_len, NULL)
{
    this->set_data(data);
    this->materials = new AWDBlockList();
}


AWDMeshInst::AWDMeshInst(const char *name, awd_uint16 name_len, AWDMeshData *data, awd_float64 *mtx) :
    AWDSceneBlock(MESH_INSTANCE, name, name_len, mtx)
{
    this->set_data(data);
    this->materials = new AWDBlockList();
}


AWDMeshInst::~AWDMeshInst()
{
}


void
AWDMeshInst::add_material(AWDMaterial *material)
{
    this->materials->force_append(material);
}


AWDMeshData *
AWDMeshInst::get_data()
{
    return this->data;
}


void
AWDMeshInst::set_data(AWDMeshData *data)
{
    this->data = data;
}


awd_uint32
AWDMeshInst::calc_body_length(bool wide_geom, bool wide_mtx)
{
    return 8 + MTX4_SIZE(wide_mtx) + sizeof(awd_uint16) + (this->materials->get_num_blocks() * sizeof(awd_baddr))
        + sizeof(awd_uint16) + this->get_name_length() + this->calc_attr_length(true,true, wide_geom,wide_mtx);
}



void
AWDMeshInst::write_body(int fd, bool wide_geom, bool wide_mtx)
{
    AWDBlock *block;
    AWDBlockIterator *it;
    awd_baddr data_addr;
    awd_uint16 num_materials;

    this->write_scene_common(fd, wide_mtx);

    // Write mesh data address
    data_addr = UI32(this->data->get_addr());
    write(fd, &data_addr, sizeof(awd_uint32));

    // Write materials list. First write material count, and then
    // iterate over materials block list and write all addresses
    printf("material count: %d\n", this->materials->get_num_blocks());
    num_materials = UI16((awd_uint16)this->materials->get_num_blocks());
    write(fd, &num_materials, sizeof(awd_uint16));
    it = new AWDBlockIterator(this->materials);
    while ((block = it->next()) != NULL) {
        awd_baddr addr = UI32(block->get_addr());
        write(fd, &addr, sizeof(awd_baddr));
    }

    this->properties->write_attributes(fd, wide_geom, wide_mtx);
    this->user_attributes->write_attributes(fd, wide_geom, wide_mtx);
}
