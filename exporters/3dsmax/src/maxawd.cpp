//**************************************************************************/
// Copyright (c) 1998-2007 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Appwizard generated plugin
// AUTHOR: 
//***************************************************************************/

#include "awd/awd.h"
#include "awd/platform.h"
#include "maxawd.h"

#define MaxAWDExporter_CLASS_ID	Class_ID(0xa8e047f2, 0x81e112c0)





class MaxAWDExporterClassDesc : public ClassDesc2 
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) 	{ return new MaxAWDExporter(); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID SuperClassID() 				{ return SCENE_EXPORT_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return MaxAWDExporter_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() 			{ return _T("MaxAWDExporter"); }		// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2* GetMaxAWDExporterDesc() { 
	static MaxAWDExporterClassDesc MaxAWDExporterDesc;
	return &MaxAWDExporterDesc; 
}





INT_PTR CALLBACK MaxAWDExporterOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
	static MaxAWDExporter *imp = NULL;

	switch(message) {
		case WM_INITDIALOG:
			imp = (MaxAWDExporter *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));
			return TRUE;

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return 1;
	}
	return 0;
}


//--- Utilities ------------------------------------------------------------
static void SerializeMatrix3(Matrix3 &mtx, double *output)
{
	Point3 row;
	
	row = mtx.GetRow(0);
	output[0] = row.x;
	output[1] = -row.z;
	output[2] = -row.y;

	row = mtx.GetRow(2);
	output[3] = -row.x;
	output[4] = row.z;
	output[5] = row.y;

	row = mtx.GetRow(1);
	output[6] = -row.x;
	output[7] = row.z;
	output[8] = row.y;

	row = mtx.GetRow(3);
	output[9] = -row.x;
	output[10] = row.z;
	output[11] = row.y;
}


static int IndexOfSkinMod(Object *obj, IDerivedObject **derivedObject)
{
	if (obj != NULL && obj->SuperClassID() == GEN_DERIVOB_CLASS_ID) {
		int i;

		IDerivedObject *derived = (IDerivedObject *)obj;

		for (i=0; i < derived->NumModifiers(); i++) {
			Modifier *mod = derived->GetModifier(i);

			void *skin = mod->GetInterface(I_SKIN);
			if (skin != NULL) {
				*derivedObject = derived;
				return i;
			}
		}
	}
	
	return -1;
}

//--- MaxAWDExporter -------------------------------------------------------
MaxAWDExporter::MaxAWDExporter()
{

}

MaxAWDExporter::~MaxAWDExporter() 
{

}

int MaxAWDExporter::ExtCount()
{
	return 1;
}

const TCHAR *MaxAWDExporter::Ext(int n)
{		
	return _T("AWD");
}

const TCHAR *MaxAWDExporter::LongDesc()
{
	return _T("Away3D AWD File");
}
	
const TCHAR *MaxAWDExporter::ShortDesc() 
{
	return _T("Away3D");
}

const TCHAR *MaxAWDExporter::AuthorName()
{			
	return _T("Away3D");
}

const TCHAR *MaxAWDExporter::CopyrightMessage() 
{	
	return _T("Copyright 2012 The Away3D Team");
}

const TCHAR *MaxAWDExporter::OtherMessage1() 
{		
	return _T("");
}

const TCHAR *MaxAWDExporter::OtherMessage2() 
{		
	return _T("");
}

unsigned int MaxAWDExporter::Version()
{				
	return 100;
}

void MaxAWDExporter::ShowAbout(HWND hWnd)
{			
	// Optional
}

BOOL MaxAWDExporter::SupportsOptions(int ext, DWORD options)
{
	#pragma message(TODO("Decide which options to support.  Simply return true for each option supported by each Extension the exporter supports."))
	return TRUE;
}


int	MaxAWDExporter::DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts, DWORD options)
{
	/*
	if(!suppressPrompts)
		DialogBoxParam(hInstance, 
				MAKEINTRESOURCE(IDD_PANEL), 
				GetActiveWindow(), 
				MaxAWDExporterOptionsDlgProc, (LPARAM)this);
	*/

	int fd = open(name, _O_TRUNC | _O_CREAT | _O_BINARY | _O_RDWR, _S_IWRITE);

	awd = new AWD(UNCOMPRESSED, 0);

	INode *root = i->GetRootNode();
	ExportNode(root, NULL);

	awd->flush(fd);

	close(fd);

	// Export worked
	return TRUE;
}


void MaxAWDExporter::ExportNode(INode *node, AWDSceneBlock *parent)
{
	int i;
	int skinIdx;
	int numChildren;
	IDerivedObject *derivedObject;
	ObjectState os;
	Object *obj;

	AWDSceneBlock *awdParent = NULL;

	derivedObject = NULL;
	skinIdx = IndexOfSkinMod(node->GetObjectRef(), &derivedObject);
	if (skinIdx >= 0) {
		// Flatten all modifiers up to but not including
		// the skin modifier.
		os = derivedObject->Eval(0, skinIdx + 1);
	}
	else {
		// Flatten entire modifier stack
		os = node->EvalWorldState(0);
	}
	
	obj = os.obj;
	if (obj) {
		SClass_ID scid = obj->SuperClassID();
		Class_ID cid = obj->ClassID();

		if (obj->CanConvertToType(triObjectClassID)) {
			TriObject *triObject = (TriObject*)obj->ConvertToType(0, triObjectClassID);
			if (triObject != NULL) {
				AWDMeshInst *awdMesh;

				awdMesh = ExportTriObject(triObject, node);

				if (parent) {
					parent->add_child(awdMesh);
				}
				else {
					awd->add_scene_block(awdMesh);
				}

				// Store the new block as parent to be used for
				// blocks that represent children of this Max node.
				awdParent = awdMesh;

				// If conversion created a new object, dispose it
				if (triObject != obj) 
					triObject->DeleteMe();
			}

			if (derivedObject != NULL && skinIdx >= 0) {
				// TODO: Export actual skin
			}
		}
	}

	numChildren = node->NumberOfChildren();
	for (i=0; i<numChildren; i++) {
		ExportNode(node->GetChildNode(i), awdParent);
	}
}


AWDMeshInst * MaxAWDExporter::ExportTriObject(TriObject *obj, INode *node)
{
	int i;
	int numVerts, numTris;
	AWD_str_ptr vertData;
	AWD_str_ptr indexData;

	Mesh& mesh = obj->GetMesh();

	// Calculate offset matrix from the object TM (which includes geometry
	// offset) and the node TM (which doesn't.) This will be used to transform
	// all vertices into node space.
	Matrix3 offsMtx = node->GetObjectTM(0) * Inverse(node->GetNodeTM(0));

	numVerts = mesh.getNumVerts();
	vertData.v = malloc(3 * numVerts * sizeof(double));

	for (i=0; i<numVerts; i++) {
		// Transform vertex into node space
		Point3& vtx = offsMtx * mesh.getVert(i);
		vertData.f64[i*3+0] = vtx.x;
		vertData.f64[i*3+1] = vtx.y;
		vertData.f64[i*3+2] = vtx.z;
	}

	numTris = mesh.getNumFaces();
	indexData.v = malloc(3 * numTris * sizeof(int));

	for (i=0; i<numTris; i++) {
		Face& face = mesh.faces[i];
		DWORD *inds = face.getAllVerts();

		indexData.ui32[i*3+0] = inds[0];
		indexData.ui32[i*3+1] = inds[2];
		indexData.ui32[i*3+2] = inds[1];
	}

	AWDSubGeom *sub = new AWDSubGeom();
	sub->add_stream(VERTICES, AWD_FIELD_FLOAT32, vertData, numVerts*3);
	sub->add_stream(TRIANGLES, AWD_FIELD_UINT16, indexData, numTris*3);

	char *name = node->GetName();

	// TODO: Use another name for the geometry
	AWDTriGeom *geom = new AWDTriGeom(name, strlen(name));
	geom->add_sub_mesh(sub);
	awd->add_mesh_data(geom);

	Matrix3 mtx = node->GetNodeTM(0) * Inverse(node->GetParentTM(0));
	double *mtxData = (double *)malloc(12*sizeof(double));
	SerializeMatrix3(mtx, mtxData);

	// Export material
	AWDMaterial *awdMtl = ExportNodeMaterial(node);

	// Export instance
	AWDMeshInst *inst = new AWDMeshInst(name, strlen(name), geom, mtxData);
	inst->add_material(awdMtl);
	
	return inst;
}


AWDMaterial *MaxAWDExporter::ExportNodeMaterial(INode *node) 
{
	Mtl *mtl = node->GetMtl();

	if (mtl == NULL) {
		AWDMaterial *awdMtl = new AWDMaterial(AWD_MATTYPE_COLOR, "", 0);
		awdMtl->color = node->GetWireColor();

		awd->add_material(awdMtl);

		return awdMtl;
	}
	else {
		AWDMaterial *awdMtl;
		const MSTR &name = mtl->GetName();
		
		awdMtl = NULL;

		if (mtl->IsSubClassOf(Class_ID(DMTL_CLASS_ID, 0))) {
			StdMat *stdMtl = (StdMat *)mtl;
		}

		int i;

		for (i=0; i<mtl->NumSubTexmaps(); i++) {
			Texmap *tex = mtl->GetSubTexmap(i);

			// If there is a texture, AND that texture is a plain bitmap
			if (tex != NULL && tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0)) {
				MSTR slotName = mtl->GetSubTexmapSlotName(i);
				const MSTR diff = _M("Diffuse Color");

				if (slotName == diff) {
					AWDBitmapTexture *awdDiffTex;
					
					awdDiffTex = ExportBitmapTexture((BitmapTex *)tex);

					awdMtl = new AWDMaterial(AWD_MATTYPE_TEXTURE, name.data(), name.length());
					awdMtl->set_texture(awdDiffTex);
				}
			}
		}

		// If no material was created during the texture search loop, this
		// is a plain color material.
		if (awdMtl == NULL) 
			awdMtl = new AWDMaterial(AWD_MATTYPE_COLOR, name.data(), name.Length());

		awd->add_material(awdMtl);

		return awdMtl;
	}
}


AWDBitmapTexture * MaxAWDExporter::ExportBitmapTexture(BitmapTex *tex)
{
	AWDBitmapTexture *awdTex;
	MSTR name;
	char *path;

	name = tex->GetName();
	path = tex->GetMapName();

	// TODO: Deal differently with embedded textures
	awdTex = new AWDBitmapTexture(EXTERNAL, name.data(), name.length());
	awdTex->set_url(path, strlen(path));

	awd->add_texture(awdTex);

	return awdTex;
}
