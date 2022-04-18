/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#define OAPI_STATIC
#include "GraphicsAPI.h"
#include "TextureBlitter.h"

#include <SkyboltEngine/SkyboltEngineFwd.h>
#include <SkyboltSim/SkyboltSimFwd.h>
#include <SkyboltVis/SkyboltVisFwd.h>

#include <osg/Group>
#include <osg/Texture2D>

#include <memory>
#include <map>

class OrbiterEntityFactory;
class OrbiterModel;
class OsgSketchpad;
class OverlayPanelFactory;
class SkyboltParticleStream;
class VideoTab;

namespace oapi {

class SkyboltClient : public GraphicsClient
{
public:
	SkyboltClient (HINSTANCE hInstance);
	~SkyboltClient() override;

	bool clbkInitialise () override;

	void clbkRefreshVideoData() override;

	SURFHANDLE clbkLoadTexture (const char *fname, DWORD flags = 0) override;

	SURFHANDLE clbkLoadSurface (const char *fname, DWORD attrib) override;

	bool clbkSaveSurfaceToImage (SURFHANDLE surf, const char *fname,
		ImageFileFormat fmt, float quality=0.7f) override { return false; }

	void clbkReleaseTexture (SURFHANDLE hTex) override;

	bool clbkSetMeshTexture (DEVMESHHANDLE hMesh, DWORD texidx, SURFHANDLE tex) override { return false; }

	int clbkSetMeshMaterial (DEVMESHHANDLE hMesh, DWORD matidx, const MATERIAL *mat) override { return 2; }

	int clbkMeshMaterial (DEVMESHHANDLE hMesh, DWORD matidx, MATERIAL *mat) override { return 2; }

	bool clbkSetMeshProperty (DEVMESHHANDLE hMesh, DWORD property, DWORD value) override { return false; }

	int clbkVisEvent (OBJHANDLE hObj, VISHANDLE vis, DWORD msg, DWORD_PTR context) override;

	MESHHANDLE clbkGetMesh(VISHANDLE vis, UINT idx) override;

	int clbkGetMeshGroup (DEVMESHHANDLE hMesh, DWORD grpidx, GROUPREQUESTSPEC *grs) override { return -2; }

	int clbkEditMeshGroup (DEVMESHHANDLE hMesh, DWORD grpidx, GROUPEDITSPEC *ges) override { return -2; }

	void clbkPreOpenPopup () override {}

	bool clbkFilterElevation(OBJHANDLE hPlanet, int ilat, int ilng, int lvl, double elev_res, INT16* elev) override { return false; }

	ParticleStream *clbkCreateParticleStream (PARTICLESTREAMSPEC *pss) override;

	ParticleStream *clbkCreateExhaustStream (PARTICLESTREAMSPEC *pss,
		OBJHANDLE hVessel, const double *lvl, const VECTOR3 *ref, const VECTOR3 *dir) override;

	ParticleStream *clbkCreateExhaustStream (PARTICLESTREAMSPEC *pss,
		OBJHANDLE hVessel, const double *lvl, const VECTOR3 &ref, const VECTOR3 &dir) override;

	ParticleStream *clbkCreateReentryStream (PARTICLESTREAMSPEC *pss,
		OBJHANDLE hVessel) override;

	ScreenAnnotation *clbkCreateAnnotation () override;

	LRESULT RenderWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	INT_PTR LaunchpadVideoWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	bool clbkFullscreenMode () const override;

	void clbkGetViewportSize (DWORD *width, DWORD *height) const override;

	bool clbkGetRenderParam (DWORD prm, DWORD *value) const override;

	void clbkRender2DPanel (SURFHANDLE *hSurf, MESHHANDLE hMesh, MATRIX3 *T, bool additive = false) override;

	void clbkRender2DPanel (SURFHANDLE *hSurf, MESHHANDLE hMesh, MATRIX3 *T, float alpha, bool additive = false) override;

	SURFHANDLE clbkCreateSurfaceEx(DWORD w, DWORD h, DWORD attrib) override;

	SURFHANDLE clbkCreateSurface(DWORD w, DWORD h, SURFHANDLE hTemplate = NULL) override;

	SURFHANDLE clbkCreateTexture (DWORD w, DWORD h) override { return NULL; }

	SURFHANDLE clbkCreateSurface (HBITMAP hBmp) override;

	void clbkIncrSurfaceRef (SURFHANDLE surf) override {}

	bool clbkReleaseSurface (SURFHANDLE surf) override { return false; }

	bool clbkGetSurfaceSize (SURFHANDLE surf, DWORD *w, DWORD *h) override { *w = *h = 0; return false; }

	bool clbkSetSurfaceColourKey (SURFHANDLE surf, DWORD ckey) override { return false; }

	DWORD clbkGetDeviceColour (BYTE r, BYTE g, BYTE b) override { return ((DWORD)r << 16) + ((DWORD)g << 8) + (DWORD)b; }

	bool clbkBlt(SURFHANDLE tgt, DWORD tgtx, DWORD tgty, SURFHANDLE src, DWORD flag = 0) const override;

	bool clbkBlt(SURFHANDLE tgt, DWORD tgtx, DWORD tgty, SURFHANDLE src, DWORD srcx, DWORD srcy, DWORD w, DWORD h, DWORD flag = 0) const override;

	bool clbkScaleBlt(SURFHANDLE tgt, DWORD tgtx, DWORD tgty, DWORD tgtw, DWORD tgth,
		SURFHANDLE src, DWORD srcx, DWORD srcy, DWORD srcw, DWORD srch, DWORD flag = 0) const override;

	int clbkBeginBltGroup (SURFHANDLE tgt) override;

	int clbkEndBltGroup () override;

	bool clbkFillSurface(SURFHANDLE surf, DWORD col) const override;

	bool clbkFillSurface(SURFHANDLE surf, DWORD tgtx, DWORD tgty, DWORD w, DWORD h, DWORD col) const override;

	bool clbkCopyBitmap (SURFHANDLE pdds, HBITMAP hbm, int x, int y, int dx, int dy) override;

	Sketchpad *clbkGetSketchpad(SURFHANDLE surf) override;

	void clbkReleaseSketchpad (Sketchpad *sp) override {}

	Font *clbkCreateFont(int height, bool prop, const char *face, oapi::Font::Style style = oapi::Font::NORMAL, int orientation = 0) const override;

	void clbkReleaseFont(Font *font) const override;

	Pen *clbkCreatePen(int style, int width, DWORD col) const override;

	void clbkReleasePen(Pen *pen) const override;

	Brush *clbkCreateBrush(DWORD col) const override;

	void clbkReleaseBrush(Brush *brush) const override;

	HDC clbkGetSurfaceDC (SURFHANDLE surf) override { return NULL; }

	void clbkReleaseSurfaceDC (SURFHANDLE surf, HDC hDC) override {}

	bool clbkUseLaunchpadVideoTab () const override { return true; }

	HWND clbkCreateRenderWindow() override;

	void clbkPostCreation () override {}

	void clbkCloseSession (bool fastclose) override {}

	void clbkDestroyRenderWindow (bool fastclose) override;

	void clbkUpdate (bool running) override {}

	void clbkRenderScene () override;

	bool clbkDisplayFrame () override { return false; }

	bool clbkSplashLoadMsg (const char *msg, int line) override { return false; }

	void clbkStoreMeshPersistent(MESHHANDLE hMesh, const char *fname) override {}

	private:
		void updateVirtualCockpitTextures(OrbiterModel& model) const;
		void updateEntity(OBJHANDLE object, skybolt::sim::Entity& entity) const;
		void translateEntities();
		SURFHANDLE getSurfaceHandleFromTextureId(MESHHANDLE mesh, int id) const;

private:
	std::unique_ptr<skybolt::EngineRoot> mEngineRoot;
	std::unique_ptr<OrbiterEntityFactory> mEntityFactory;
	std::unique_ptr<OverlayPanelFactory> mOverlayPanelFactory;
	std::unique_ptr<skybolt::vis::EmbeddedWindow> mWindow;
	skybolt::sim::EntityPtr mSimCamera;
	std::unique_ptr<VideoTab> mVideoTab;
	std::shared_ptr<struct NVGcontext> m_nanoVgContext;

	osg::ref_ptr<osg::Group> mPanelGroup;
	std::map<OBJHANDLE, skybolt::sim::EntityPtr> mEntities;
	std::map<SURFHANDLE, osg::ref_ptr<osg::Texture2D>> mTextures;
	std::map<SURFHANDLE, std::shared_ptr<OsgSketchpad>> mSketchpads;
	std::set<SkyboltParticleStream*> mParticleStreams;
	osg::ref_ptr<TextureBlitter> mTextureBlitter;

	mutable std::map<oapi::Pen*, std::shared_ptr<oapi::Pen>> mPens;
	mutable std::map<oapi::Brush*, std::shared_ptr<oapi::Brush>> mBrushes;
	mutable std::map<oapi::Font*, std::shared_ptr<oapi::Font>> mFonts;

	HDC mGldc = nullptr;

}; // SkyboltClient

}; // namespace oapi
