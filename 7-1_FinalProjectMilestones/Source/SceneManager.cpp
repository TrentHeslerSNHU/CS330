///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables and defines
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	// create the shape meshes object
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// free the allocated objects
	m_pShaderManager = NULL;
	if (NULL != m_basicMeshes)
	{
		delete m_basicMeshes;
		m_basicMeshes = NULL;
	}

	// free the allocated OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL, 
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename, 
		&width, 
		&height, 
		&colorChannels, 
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the 
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}
	
	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL &material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** The code in the methods BELOW is for preparing and     ***/
/*** rendering the 3D replicated scenes.                    ***/
/**************************************************************/

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
		/*** STUDENTS - add the code BELOW for loading the textures that ***/
		/*** will be used for mapping to objects in the 3D scene. Up to  ***/
		/*** 16 textures can be loaded per scene. Refer to the code in   ***/
		/*** the OpenGL Sample for help.                                 ***/

		bool bReturn = false;

		//Apparently, pictures can't be perfectly square?
		bReturn = CreateGLTexture(
			"Textures/Coffee.jpg",
			"coffee");

		//https://www.flickr.com/photos/qubodup/2951312694/in/pool-pdtextures/
		bReturn = CreateGLTexture(
			"Textures/WhiteBrick.jpg",
			"white_brick");

		//https://publicdomaintextures.com/elementor-112/stone09/
		bReturn = CreateGLTexture(
			"Textures/Plaster.jpg",
			"plaster");

		bReturn = CreateGLTexture(
			"Textures/Wood.jpg",
			"wood");

		//Picture I took myself
		bReturn = CreateGLTexture(
			"Textures/macbook.jpg",
			"macbook");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	metalMaterial.ambientStrength = 0.3f;
	metalMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	metalMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	metalMaterial.shininess = 22.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL liquidMaterial;
	liquidMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	liquidMaterial.ambientStrength = 0.3f;
	liquidMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	liquidMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	liquidMaterial.shininess = 95.0;
	liquidMaterial.tag = "liquid";

	m_objectMaterials.push_back(liquidMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light 
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	//Window light
	m_pShaderManager->setVec3Value("lightSources[0].position", -12.0f, 15.0f, -7.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.4f);

	//Notepad light
	m_pShaderManager->setVec3Value("lightSources[1].position", 5.0, 2.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.2f);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	DrawBackdrop();
	DrawCoffee();
	DrawNotepad();
	DrawPen();
	DrawSmartphone();
	DrawLaptop();
}

void SceneManager::DrawBackdrop() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("wood");
	SetTextureUVScale(1.0, 3.0);
	SetShaderMaterial("wood");


	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 1.0f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 10.0f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("white_brick");
	SetTextureUVScale(2.5, 2.5);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 1.0f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-15.0f, 10.0f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	//SetShaderTexture("white_brick");
	//SetTextureUVScale(2.5, 2.5);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
}

void SceneManager::DrawLaptop() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	float itemScale = 1.0;
	glm::vec3 itemOrigin = { -1.0f, 0.12f, 5.0f };

	//Laptop Body
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 4.0f), (itemScale * 0.2f), (itemScale * 5.0f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] + 0.0f), itemScale * (itemOrigin[1] + 0.0f), itemScale * (itemOrigin[2] + 0.0f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1);
	//SetShaderTexture("macbook");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//Keyboard
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 2.0f), (itemScale * 0.2f), (itemScale * 2.5f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] + 0.0f), itemScale * (itemOrigin[1] + 0.101f), itemScale * (itemOrigin[2] + 0.0f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 1);
	SetShaderTexture("macbook");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");


	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	//Laptop Lid
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 0.1f), (itemScale * 4.0f), (itemScale * 5.0f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 25.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] - 2.8f), itemScale * (itemOrigin[1] + 1.9f), itemScale * (itemOrigin[2] + 0.0f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1);
	//SetShaderTexture("plaster");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//Laptop screen
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 0.01f), (itemScale * 3.9f), (itemScale * 4.8f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 25.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] - 2.75f), itemScale * (itemOrigin[1] + 1.9f), itemScale * (itemOrigin[2] + 0.0f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.01, 0.01, 0.01, 1);
	//SetShaderTexture("plaster");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}

void SceneManager::DrawCoffee() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	float itemScale = 1.25;
	glm::vec3 itemOrigin = { -1.75f, 0.16f, -0.5f };

	// -- MUG BASE --
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 0.8f), (itemScale * 1.25f), (itemScale * 0.8f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] + 0.0f), itemScale * (itemOrigin[1] - 0.15f), itemScale * (itemOrigin[2] + 0.0f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("plaster");
	SetTextureUVScale(2.0, 1.0);
	SetShaderMaterial("glass");


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// -- COFFEE LIQUID --
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 0.7f), (itemScale * 0.1f), (itemScale * 0.7f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] + 0.0f), itemScale * (itemOrigin[1] + 1.05f), itemScale * (itemOrigin[2] + 0.0f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.55, 0.27, 0.08, 1);
	SetShaderTexture("coffee");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("liquid");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// -- MUG LIP --
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 0.72f), (itemScale * 0.72f), (itemScale * 0.72f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] + 0.0f), itemScale * (itemOrigin[1] + 1.1f), itemScale * (itemOrigin[2] + 0.0f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("plaster");
	SetTextureUVScale(1.0, 0.1);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	// -- MUG HANDLE --
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 0.25f), (itemScale * 0.5f), (itemScale * 0.37f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] + -1.0f), itemScale * (itemOrigin[1] + 0.5f), itemScale * (itemOrigin[2] + 0.0f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("plaster");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/
}

void SceneManager::DrawNotepad() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	float itemScale = 2.0;
	glm::vec3 itemOrigin = { 2.5f, 0.0f, -1.5f };
	
	//Paper Roll
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 0.2f), (itemScale * 2.0f), (itemScale * 0.2f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] + 0.0f), itemScale * (itemOrigin[1] + 0.25f), itemScale * (itemOrigin[2] + 0.0f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("plaster");
	SetTextureUVScale(2.0, 1.0);
	SetShaderMaterial("wood");


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	
	//Paper Pad
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 0.1f), (itemScale * 2.0f), (itemScale * 2.0f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] - 0.51f), itemScale * (itemOrigin[1] + 0.1f), itemScale * (itemOrigin[2] + 1.25f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("plaster");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}

void SceneManager::DrawPen() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	float itemScale = 1.25;
	glm::vec3 itemOrigin = { 2.0f, 0.2f, 0.75f };

	//DrawCylinder
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 0.1f), (itemScale * 1.0f), (itemScale * 0.1f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] + 0.0f), itemScale * (itemOrigin[1] + 0.15f), itemScale * (itemOrigin[2] + 0.0f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.01, 0.01, 0.01, 1);
	//SetShaderTexture("plaster");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//DrawTorus //(Pen grip)
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 0.11f), (itemScale * 0.11f), (itemScale * 1.25f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] + 0.0f), itemScale * (itemOrigin[1] + 0.15f), itemScale * (itemOrigin[2] + 0.2f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.02, 0.02, 0.02, 1);
	//SetShaderTexture("plaster");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");


	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	//DrawCone
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 0.1f), (itemScale * 0.2f), (itemScale * 0.1f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = -90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] + 0.0f), itemScale * (itemOrigin[1] + 0.15f), itemScale * (itemOrigin[2] + 0.0f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.01, 0.01, 0.01, 1);
	//SetShaderTexture("plaster");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");


	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/

	//DrawCylinder
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 0.05f), (itemScale * 0.1f), (itemScale * 0.05f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] + 0.0f), itemScale * (itemOrigin[1] + 0.15f), itemScale * (itemOrigin[2] + 1.0f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.01, 0.01, 0.01, 1);
	//SetShaderTexture("plaster");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//DrawBox
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 0.05f), (itemScale * 0.2f), (itemScale * 0.05f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] - 0.1f), itemScale * (itemOrigin[1] + 0.15f), itemScale * (itemOrigin[2] + 0.85f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.01, 0.01, 0.01, 1);
	//SetShaderTexture("plaster");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}

void SceneManager::DrawSmartphone() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	float itemScale = 1.75;
	glm::vec3 itemOrigin = { 2.5f, 0.075f, 2.5f };

	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3((itemScale * 1.0f), (itemScale * 0.075f), (itemScale * 1.5f));

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(itemScale * (itemOrigin[0] + 0.0f), itemScale * (itemOrigin[1] + 0.0f), itemScale * (itemOrigin[2] + 0.0f));

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.01, 0.01, 0.01, 1);
	//SetShaderTexture("plaster");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}