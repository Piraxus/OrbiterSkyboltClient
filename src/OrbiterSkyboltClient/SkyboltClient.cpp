/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "OpenGlContext.h"

#define ORBITER_MODULE
#include "OrbiterAPI.h"

#include "ModelFactory.h"
#include "OrbiterEntityFactory.h"
#include "ObjectUtil.h"
#include "OrbiterVisibilityCategory.h"
#include "OsgSketchpad.h"
#include "OverlayPanelFactory.h"
#include "SkyboltClient.h"
#include "SkyboltParticleStream.h"

#include <SkyboltEngine/EngineRoot.h>
#include <SkyboltEngine/EngineRootFactory.h>
#include <SkyboltEngine/SimVisBinding/CameraSimVisBinding.h>
#include <SkyboltSim/Entity.h>
#include <SkyboltSim/World.h>
#include <SkyboltSim/CameraController/CameraControllerSelector.h>
#include <SkyboltSim/Components/CameraComponent.h>
#include <SkyboltSim/Components/CameraControllerComponent.h>
#include <SkyboltVis/Camera.h>
#include <SkyboltVis/OsgImageHelpers.h>
#include <SkyboltVis/OsgStateSetHelpers.h>
#include <SkyboltVis/OsgTextureHelpers.h>
#include <SkyboltVis/RenderTarget/RenderTargetSceneAdapter.h>
#include <SkyboltVis/RenderTarget/RenderTexture.h>
#include <SkyboltVis/RenderTarget/Viewport.h>
#include <SkyboltVis/RenderTarget/ViewportHelpers.h>
#include <SkyboltSim/Spatial/Geocentric.h>
#include <SkyboltSim/System/SimStepper.h>
#include <SkyboltSim/System/System.h>
#include <SkyboltVis/Window/EmbeddedWindow.h>
#include <SkyboltCommon/MapUtility.h>
#include <SkyboltCommon/File/OsDirectories.h>
#include <SkyboltCommon/Json/ReadJsonFile.h>

#include <osgDB/ReadFile>

using namespace skybolt;

namespace oapi {

HINSTANCE g_hInst;
SkyboltClient* g_client = nullptr;

DLLCLBK void InitModule(HINSTANCE hDLL)
{
	g_hInst = hDLL;
	g_client = new SkyboltClient(hDLL);
	if (!oapiRegisterGraphicsClient(g_client)) {
		delete g_client;
		g_client = 0;
	}
}

DLLCLBK void ExitModule(HINSTANCE hDLL)
{
	if (g_client)
	{
		oapiUnregisterGraphicsClient(g_client);
		delete g_client;
		g_client = nullptr;
	}
}

SkyboltClient::SkyboltClient(HINSTANCE hInstance) :
	GraphicsClient(hInstance)
{
}

SkyboltClient::~SkyboltClient()
{
}

bool SkyboltClient::clbkInitialise()
{
	return GraphicsClient::clbkInitialise();
	// Don't create engine here because clbkInitialise is called by DllMain()
	// where it's illegal to create threads. We will delay creation of the engine
	// until the first frame needs to be rendered, where threads can be safely created.
	// See https://stackoverflow.com/questions/1688290/creating-a-thread-in-dllmain
}

static osg::ref_ptr<osg::Texture2D> readAlbedoTexture(const std::string& filename)
{
	auto image = osgDB::readImageFile(filename);
	image->setInternalTextureFormat(vis::toSrgbInternalFormat(image->getInternalTextureFormat()));
	osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D(image);
	texture->setFilter(osg::Texture::FilterParameter::MIN_FILTER, osg::Texture::FilterMode::LINEAR_MIPMAP_LINEAR);
	texture->setFilter(osg::Texture::FilterParameter::MAG_FILTER, osg::Texture::FilterMode::LINEAR);
	return texture;
}

#define LOAD_TEXTURE_BIT_DONT_LOAD_MIPMAPS 4

SURFHANDLE SkyboltClient::clbkLoadTexture(const char *fname, DWORD flags)
{
	char cpath[256];
	if (TexturePath(fname, cpath))
	{
		auto texture = readAlbedoTexture(std::string(cpath));

		if (flags & LOAD_TEXTURE_BIT_DONT_LOAD_MIPMAPS) // interpret this flag as disabling mipmaps
		{
			texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
			texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
			texture->setUseHardwareMipMapGeneration(false);

		}

		SURFHANDLE handle = (SURFHANDLE)texture.get();
		mTextures[handle] = texture;
		return handle;
	}
	return NULL;
}

SURFHANDLE SkyboltClient::clbkLoadSurface(const char *fname, DWORD attrib)
{
	return NULL;
}

void SkyboltClient::clbkReleaseTexture(SURFHANDLE hTex)
{
}

int SkyboltClient::clbkVisEvent(OBJHANDLE hObj, VISHANDLE vis, DWORD msg, DWORD_PTR context)
{
	return -2;
}

MESHHANDLE SkyboltClient::clbkGetMesh(VISHANDLE vis, UINT idx)
{
	return NULL;
}

ParticleStream* SkyboltClient::clbkCreateParticleStream(PARTICLESTREAMSPEC *pss)
{
	auto entityFinder = [this](OBJHANDLE vessel) {
		return findOptional(mEntities, vessel).get_value_or(nullptr);
	};

	auto destructionAction = [this](SkyboltParticleStream* stream) {
		mParticleStreams.erase(stream);
	};

	auto stream = new SkyboltParticleStream(this, pss, *mEngineRoot->entityFactory, mEngineRoot->simWorld.get(), entityFinder, destructionAction);
	mParticleStreams.insert(stream);
	return stream;
}

ParticleStream* SkyboltClient::clbkCreateExhaustStream(PARTICLESTREAMSPEC *pss,
	OBJHANDLE hVessel, const double *lvl, const VECTOR3 *ref, const VECTOR3 *dir)
{
	auto stream = clbkCreateParticleStream(pss);
	stream->Attach(hVessel, ref, dir, lvl);
	return stream;
}

ParticleStream* SkyboltClient::clbkCreateExhaustStream(PARTICLESTREAMSPEC *pss,
	OBJHANDLE hVessel, const double *lvl, const VECTOR3 &ref, const VECTOR3 &dir)
{
	auto stream = clbkCreateParticleStream(pss);
	stream->Attach(hVessel, ref, dir, lvl);
	return stream;
}

ParticleStream* SkyboltClient::clbkCreateReentryStream(PARTICLESTREAMSPEC *pss,
	OBJHANDLE hVessel)
{
	return new ParticleStream(this, pss);
	//auto stream = clbkCreateParticleStream(pss);
	//stream->Attach(hVessel);
	//return stream;
}

ScreenAnnotation *SkyboltClient::clbkCreateAnnotation()
{
	return nullptr;
}

LRESULT SkyboltClient::RenderWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return GraphicsClient::RenderWndProc(hWnd, uMsg, wParam, lParam);
}

INT_PTR SkyboltClient::LaunchpadVideoWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return false;
}

bool SkyboltClient::clbkFullscreenMode() const
{
	return false;
}

void SkyboltClient::clbkGetViewportSize(DWORD *width, DWORD *height) const
{
	*width = mWindow->getWidth();
	*height = mWindow->getHeight();
}

bool SkyboltClient::clbkGetRenderParam(DWORD prm, DWORD *value) const
{
	return false;
}

void SkyboltClient::clbkRender2DPanel(SURFHANDLE *hSurf, MESHHANDLE hMesh, MATRIX3 *T, bool additive)
{
	float alpha = 1.0;
	clbkRender2DPanel(hSurf, hMesh, T, alpha, additive);
}

void SkyboltClient::clbkRender2DPanel(SURFHANDLE *hSurf, MESHHANDLE hMesh, MATRIX3 *T, float alpha, bool additive)
{
	osg::ref_ptr<osg::Geode> geode = mOverlayPanelFactory->createOverlayPanel(hSurf, hMesh, T, alpha, additive);
	mPanelGroup->addChild(geode);
}

static osg::ref_ptr<osg::Texture2D> createOrbiterRenderTexture(int width, int height)
{
	osg::ref_ptr<osg::Texture2D> texture = vis::createRenderTexture(width, height);
	texture->setResizeNonPowerOfTwoHint(false);
	texture->setInternalFormat(GL_RGBA);
	texture->setFilter(osg::Texture2D::FilterParameter::MIN_FILTER, osg::Texture2D::FilterMode::LINEAR);
	texture->setFilter(osg::Texture2D::FilterParameter::MAG_FILTER, osg::Texture2D::FilterMode::LINEAR);
	texture->setNumMipmapLevels(0);
	texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
	texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

	return texture;
}

static osg::ref_ptr<osg::Camera> createSketchpadCamera(osg::ref_ptr<osg::Texture> outputTexture)
{
	assert(!outputTextures.empty());
	osg::ref_ptr<osg::Camera> camera = new osg::Camera;
	camera->setClearMask(0); // Sketchpad will be cleared on request by clbkFillSurface()
	camera->setViewport(0, 0, outputTexture->getTextureWidth(), outputTexture->getTextureHeight());
	camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	camera->setRenderOrder(osg::Camera::PRE_RENDER);
	camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

	camera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0), outputTexture);
	// nanoVG requires stencil buffer
	camera->attach(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER, GL_DEPTH_STENCIL_EXT);

	return camera;
}

SURFHANDLE SkyboltClient::clbkCreateSurfaceEx(DWORD w, DWORD h, DWORD attrib)
{
	if (attrib & (OAPISURFACE_RENDERTARGET | OAPISURFACE_SKETCHPAD))
	{
		auto texture = createOrbiterRenderTexture(w, h);

		if (w == 0 || h == 0 || w > 4096 || h > 4096)
		{
			return nullptr;
			//throw std::runtime_error("Invalid render target size requested: " + std::to_string(w) + " x " + std::to_string(h));
		}

		osg::ref_ptr<osg::Camera> camera = createSketchpadCamera(texture);
		SURFHANDLE handle = texture.get();
		mTextures[handle] = texture;

		mEngineRoot->scene->_getGroup()->addChild(camera);

		if (attrib & OAPISURFACE_SKETCHPAD)
		{
			auto sketchpad = std::make_shared<OsgSketchpad>(camera, handle);
			mSketchpads[handle] = sketchpad;
		}

		return handle;
	}
	return NULL;
}

SURFHANDLE SkyboltClient::clbkCreateSurface(DWORD w, DWORD h, SURFHANDLE hTemplate)
{
	return clbkCreateSurfaceEx(w, h, OAPISURFACE_RENDERTARGET);
}

SURFHANDLE SkyboltClient::clbkCreateSurface(HBITMAP hBmp)
{
	return NULL;
}

bool SkyboltClient::clbkBlt(SURFHANDLE tgt, DWORD tgtx, DWORD tgty, SURFHANDLE src, DWORD flag) const
{
	auto texture = findOptional(mTextures, tgt);
	if (texture)
	{
		int w = (*texture)->getTextureWidth();
		int h = (*texture)->getTextureHeight();
		return clbkBlt(tgt, tgtx, tgty, src, 0, 0, w, h, flag);
	}
	return false;
}

bool SkyboltClient::clbkBlt(SURFHANDLE tgt, DWORD tgtx, DWORD tgty, SURFHANDLE src, DWORD srcx, DWORD srcy, DWORD w, DWORD h, DWORD flag) const
{
	auto srcTexture = findOptional(mTextures, src);
	auto dstTexture = findOptional(mTextures, tgt);
	if (!srcTexture || !dstTexture)
	{
		return false;
	}

	mTextureBlitter->blitTexture(**srcTexture, **dstTexture,
		Box2i(glm::ivec2(srcx, srcy), glm::ivec2(srcx + w, srcy + h)),
		Box2i(glm::ivec2(tgtx, tgty), glm::ivec2(tgtx + w, tgty + h)));
	return true;
}

bool SkyboltClient::clbkScaleBlt(SURFHANDLE tgt, DWORD tgtx, DWORD tgty, DWORD tgtw, DWORD tgth,
	SURFHANDLE src, DWORD srcx, DWORD srcy, DWORD srcw, DWORD srch, DWORD flag) const
{
	// TODO: support scaled texture blitting
	return false;
}

int SkyboltClient::clbkBeginBltGroup(SURFHANDLE tgt)
{
	return -2;
}

int SkyboltClient::clbkEndBltGroup()
{
	return -2;
}

static osg::Vec4f toOsgColor(DWORD color)
{
	int alpha = color & 0xFF;
	int blue = (color >> 8) & 0xFF;
	int green = (color >> 16) & 0xFF;
	int red = (color >> 24) & 0xFF;

	return osg::Vec4f(float(red) / 255, float(green) / 255, float(blue) / 255, float(alpha) / 255);
}

bool SkyboltClient::clbkFillSurface(SURFHANDLE surf, DWORD col) const
{
	auto i = mSketchpads.find(surf);
	if (i != mSketchpads.end())
	{
		auto sketchpad = i->second;
		sketchpad->fillBackground(toOsgColor(col));
		return true;
	}
	return false;
}

bool SkyboltClient::clbkFillSurface(SURFHANDLE surf, DWORD tgtx, DWORD tgty, DWORD w, DWORD h, DWORD col) const
{
	return false;
}

bool SkyboltClient::clbkCopyBitmap(SURFHANDLE pdds, HBITMAP hbm, int x, int y, int dx, int dy)
{
	return false;
}

Sketchpad* SkyboltClient::clbkGetSketchpad(SURFHANDLE surf)
{
	auto i = mSketchpads.find(surf);
	if (i != mSketchpads.end())
	{
		return i->second.get();
	}

	return nullptr;
}

Font* SkyboltClient::clbkCreateFont(int height, bool prop, const char* face, oapi::Font::Style style, int orientation) const
{
	auto font = std::make_shared<OsgFont>();
	font->name = face;
	font->heightPixels = std::abs(height); // note: windows API heights can be negative, indicating 'character height' instead of 'cell height'
	font->rotationRadians = -osg::DegreesToRadians(orientation * 0.1f);
	mFonts[font.get()] = font;
	return font.get();
}

void SkyboltClient::clbkReleaseFont(Font* font) const
{
	mFonts.erase(font);
}

Pen *SkyboltClient::clbkCreatePen(int style, int width, DWORD col) const
{
	auto pen = std::make_shared<OsgPen>();
	pen->style = OsgPen::toStyle(style);
	pen->width = width;
	pen->color = rgbIntToVec4iColor(col);
	mPens[pen.get()] = pen;
	return pen.get();
}

void SkyboltClient::clbkReleasePen(Pen *pen) const
{
	mPens.erase(pen);
}

Brush* SkyboltClient::clbkCreateBrush(DWORD col) const
{
	auto brush = std::make_shared<OsgBrush>();
	brush->color = rgbIntToVec4iColor(col);
	mBrushes[brush.get()] = brush;
	return brush.get();
}

void SkyboltClient::clbkReleaseBrush(Brush* brush) const
{
	mBrushes.erase(brush);
}

static void WriteLog(const std::string& str)
{
	oapiWriteLog(const_cast<char*>(str.c_str()));
}

HWND SkyboltClient::clbkCreateRenderWindow()
{
	HWND hWnd = GraphicsClient::clbkCreateRenderWindow();
	
	mGldc = GetDC(hWnd);
	createOpenGlContext(mGldc);

	{
		// Create engine
		file::Path settingsFilename = file::getAppUserDataDirectory("Skybolt").append("Settings.json");

		nlohmann::json settings;
		if (std::filesystem::exists(settingsFilename))
		{
			WriteLog(std::string("Reading Skybolt settings file '" + settingsFilename.string() + "'"));
			settings = readJsonFile(settingsFilename.string());
		}
		else
		{
			WriteLog(std::string("Settings file not found: '" + settingsFilename.string() + "'"));
		}

		try
		{
			mEngineRoot = EngineRootFactory::create({}, settings);
		}
		catch (const std::exception& e)
		{
			oapiWriteLogV(e.what());
		}

		auto textureProvider = [this](SURFHANDLE surface) {
			return findOptional(mTextures, surface).get_value_or(nullptr);
		};

		std::shared_ptr<ModelFactory> modelFactory;
		{
			ModelFactoryConfig config;
			config.surfaceHandleFromTextureIdProvider = [this](MESHHANDLE mesh, int textureId) {
				return getSurfaceHandleFromTextureId(mesh, textureId);
			};
			config.textureProvider = textureProvider;
			config.program = mEngineRoot->programs.getRequiredProgram("model");

			modelFactory = std::make_shared<ModelFactory>(config);
		}

		{
			OrbiterEntityFactoryConfig config;
			config.entityFactory = mEngineRoot->entityFactory.get();
			config.scene = mEngineRoot->scene;
			config.modelFactory = modelFactory;

			mEntityFactory = std::make_unique<OrbiterEntityFactory>(config);
		}

		mOverlayPanelFactory = std::make_unique<OverlayPanelFactory>(
			[this] { return osg::Vec2i(mWindow->getWidth(), mWindow->getHeight()); },
			textureProvider,
			[this](int mfdId) { return GetMFDSurface(mfdId); }
		);

		// Create camera
		mSimCamera = mEngineRoot->entityFactory->createEntity("Camera");
		auto cameraComponent = mSimCamera->getFirstComponentRequired<sim::CameraComponent>();
		cameraComponent->getState().fovY = 1.0;
		cameraComponent->getState().nearClipDistance = 0.2f;
		cameraComponent->getState().farClipDistance = 5e7;

		auto cameraController = static_cast<sim::CameraControllerSelector*>(mSimCamera->getFirstComponentRequired<sim::CameraControllerComponent>()->cameraController.get());
		cameraController->selectController("Null");

		mEngineRoot->simWorld->addEntity(mSimCamera);
	}

	mEngineRoot->simWorld->addEntity(mEngineRoot->entityFactory->createEntity("Stars"));
	mEngineRoot->simWorld->addEntity(mEngineRoot->entityFactory->createEntity("SunBillboard"));
	mEngineRoot->simWorld->addEntity(mEngineRoot->entityFactory->createEntity("MoonBillboard"));

	RECT rect;
	GetClientRect(hWnd, &rect);

	mWindow = std::make_unique<vis::EmbeddedWindow>(rect.right - rect.left, rect.bottom - rect.top);

	// Attach camera to window
	osg::ref_ptr<vis::RenderTarget> viewport = createAndAddViewportToWindow(*mWindow, mEngineRoot->programs.getRequiredProgram("compositeFinal"));
	viewport->setScene(std::make_shared<vis::RenderTargetSceneAdapter>(mEngineRoot->scene));
	viewport->setCamera(getVisCamera(*mSimCamera));

	// Setup blitter
	{
		osg::ref_ptr<osg::Camera> osgCamera = mWindow->getRenderTargets().front().target->getOsgCamera();
		mTextureBlitter = new TextureBlitter();
		osgCamera->addPreDrawCallback(mTextureBlitter);
	}

	// Create HUD panel overlay
	{
		mPanelGroup = new osg::Group();
		osg::ref_ptr<osg::Camera> osgCamera = mWindow->getRenderTargets().back().target->getOsgCamera();
		osgCamera->addChild(mPanelGroup);

		auto program = mEngineRoot->programs.getRequiredProgram("hudGeometry");
		auto stateSet = mPanelGroup->getOrCreateStateSet();
		stateSet->setAttribute(program);
		stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
		vis::makeStateSetTransparent(*stateSet, vis::TransparencyMode::Classic);
	}

	return hWnd;
}

void SkyboltClient::clbkDestroyRenderWindow(bool fastclose)
{
	mWindow.reset();
	mEntities.clear();
}

sim::Matrix3 toSkyboltMatrix3(const MATRIX3& m)
{
	// Convert from left handed row major to right handed column major
	sim::Vector3 x(m.m11, m.m31, m.m21);
	sim::Vector3 y(m.m12, m.m32, m.m22);
	sim::Vector3 z(m.m13, m.m33, m.m23);
	return sim::Matrix3(x, z, y);
}

OBJHANDLE g_earth = nullptr;

sim::Vector3 toSkyboltPosFromGlobal(const VECTOR3& gpos)
{
	if (g_earth)
	{
		sim::LatLonAlt lla;
		oapiGlobalToEqu(g_earth, gpos, &lla.lon, &lla.lat, &lla.alt);
		double radius = 6371000;
		lla.alt -= radius;
		return sim::llaToGeocentric(lla, radius);
	}
	return sim::Vector3(0, 0, 0);
}

sim::Quaternion toSkyboltOriFromGlobal(const MATRIX3& m)
{
	if (g_earth)
	{
		MATRIX3 mat;
		oapiGetRotationMatrix(g_earth, &mat);
		return sim::Quaternion(glm::transpose(toSkyboltMatrix3(mat)) * toSkyboltMatrix3(m)) * glm::angleAxis(math::halfPiD(), sim::Vector3(0, 0, 1)) * glm::angleAxis(math::piD(), sim::Vector3(1, 0, 0));
	}
	return sim::Quaternion();
}

static int getCameraVisibilityCategoryMask()
{
	if (oapiCameraInternal())
	{
		return (oapiCockpitMode() == COCKPIT_VIRTUAL ? OrbiterVisibilityCategory::virtualCockpitView : OrbiterVisibilityCategory::cockpitView);
	}
	else
	{
		return OrbiterVisibilityCategory::externalView;
	}
}

static void updateCamera(sim::Entity& camera)
{
	VECTOR3 gpos;
	oapiCameraGlobalPos(&gpos);

	setPosition(camera, toSkyboltPosFromGlobal(gpos));

	MATRIX3 mat;
	oapiCameraRotationMatrix(&mat);
	setOrientation(camera, sim::Quaternion(toSkyboltOriFromGlobal(mat)));

	auto component = camera.getFirstComponentRequired<sim::CameraComponent>();
	component->getState().fovY = float(oapiCameraAperture()) * 2.0f;

	auto visCamera = getVisCamera(camera);
	visCamera->setVisibilityCategoryMask(getCameraVisibilityCategoryMask());
}

static sim::EntityPtr createEntity(const OrbiterEntityFactory& factory, OBJHANDLE object)
{
	std::string name = getName(object);
	if (name == "Earth")
	{
		g_earth = object;
	}
	return factory.createEntity(object);
}

static void updateEntity(OBJHANDLE object, sim::Entity& entity)
{
	VECTOR3 gpos;
	oapiGetGlobalPos(object, &gpos);

	VECTOR3 lpos;
	oapiGlobalToLocal(g_earth, &gpos, &lpos);

	setPosition(entity, toSkyboltPosFromGlobal(gpos));

	MATRIX3 mat;
	oapiGetRotationMatrix(object, &mat);
	setOrientation(entity, sim::Quaternion(toSkyboltOriFromGlobal(mat)));
}

SURFHANDLE SkyboltClient::getSurfaceHandleFromTextureId(MESHHANDLE mesh, int id) const
{
	if (id < TEXIDX_MFD0)
	{
		return oapiGetTextureHandle(mesh, id + 1); // texture ID's start at 1 because 0 is reserved for no assigned texture.
	}
	else
	{
		return GetMFDSurface(id - TEXIDX_MFD0);
	}
}

void SkyboltClient::translateEntities()
{
	std::map<OBJHANDLE, skybolt::sim::EntityPtr> currentEntities;

	std::vector<OBJHANDLE> objects;
	int objectCount = oapiGetObjectCount();
	for (int i = 0; i < objectCount; ++i)
	{
		objects.push_back(oapiGetObjectByIndex(i));
	}

	if (g_earth)
	{
		objectCount = oapiGetBaseCount(g_earth);
		for (int i = 0; i < objectCount; ++i)
		{
			objects.push_back(oapiGetBaseByIndex(g_earth, i));
		}
	}

	for (const auto& object : objects)
	{
		sim::EntityPtr entity;
		auto it = mEntities.find(object);
		if (it == mEntities.end())
		{
			entity = createEntity(*mEntityFactory, object);
			if (entity)
			{
				if (object != g_earth)
					updateEntity(object, *entity);

				mEngineRoot->simWorld->addEntity(entity);
			}
		}
		else
		{
			entity = it->second;
			if (object != g_earth)
				updateEntity(object, *entity);
		}

		if (entity)
		{
			currentEntities[object] = entity;
		}
	}

	// TODO: remove entities
	std::swap(currentEntities, mEntities);
}

void SkyboltClient::clbkRenderScene()
{
	//return;
	/*
	while (GetRenderWindow())
	{
		MSG msg;
		BOOL result = PeekMessageA(&msg, GetRenderWindow(), 0, 0, PM_REMOVE);
		if (!result)
		{
			break;
		}
		RenderWndProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
	}
	*/
	mEngineRoot->scenario.startJulianDate = oapiGetSimMJD() + 2400000.5;
	updateCamera(*mSimCamera);
	translateEntities();

	// Update particles
	for (auto& stream : mParticleStreams)
	{
		stream->update();
	}

	// Step sim
	auto simStepper = std::make_shared<sim::SimStepper>(mEngineRoot->systemRegistry);

	sim::System::StepArgs args;
	args.dtSim = oapiGetSimStep();
	args.dtWallClock = args.dtSim;
	simStepper->step(args);

	// Update panels
	{
		// Remove previous panels
		mPanelGroup->removeChildren(0, mPanelGroup->getNumChildren());

		// Add new panels
		Render2DOverlay();
	}

	// Render
	mWindow->render();

	SwapBuffers(mGldc);
}

}; // namespace oapi
