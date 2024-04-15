#ifndef BBGE_RENDERGRID_H
#define BBGE_RENDERGRID_H

#include "Vector.h"
#include "DataStructures.h"
#include "VertexBuffer.h"
#include "Texture.h" // TexCoordBox

struct RenderState;

enum GridDrawOrder
{
	GRID_DRAW_LRTB = 0, // the default. ignores grid.z
	GRID_DRAW_LRBT = 1, // Y axis inverted
	GRID_DRAW_WORLDMAP = 2, // LRTB order, uses grid.z as alpha

	GRID_DRAW_DEFAULT = GRID_DRAW_LRTB
};

enum GridType
{
	GRID_UNDEFINED = 0,
	GRID_WAVY	= 1,
	GRID_STRIP	= 2, // quad is in strip mode
	GRID_INTERP = 3  // quad is in grid mode
};

// simple render grid, must be manually uploaded to GPU if changed
class RenderGrid
{
public:
	RenderGrid();
	~RenderGrid();
	void dropBuffers();

	void init(size_t w, size_t h);
	void init(size_t w, size_t h, const TexCoordBox& tc);
	void reset01();
	void reset();
	void resetWithAlpha(float a);
	void render(const RenderState& rs) const;
	void renderDebugPoints(const RenderState& rs) const;
	void setAlpha(size_t x, size_t y, float a);
	void setDrawOrder(GridDrawOrder ord, bool force = false);
	inline GridDrawOrder getDrawOrder() const { return GridDrawOrder(drawOrder); }
	void setTexCoords(const TexCoordBox& tc);
	const TexCoordBox& getTexCoords() const { return tc; }

	bool empty() const { return !(width() | height()); }
	size_t width() const { return grid.width(); }
	size_t height() const { return grid.height(); }
	size_t linearsize() const { return grid.linearsize(); }
	const Vector *data() const { return grid.data(); }
	Vector *dataRW() { this->needVBOUpdate = true; return grid.data(); }
	Array2d<Vector>& array2d() { return grid; }
	const Array2d<Vector>& array2d() const { return grid; }
	const DynamicGPUBuffer& getVBO() const { return vbo; }
	void updateVBO();
	void updateVBOIfNecessary();

	static void ResetWithAlpha(Vector* dst, size_t w, size_t h, float alpha);

protected:
	DynamicGPUBuffer indexbuf, vbo;
	size_t trisToDraw;
	Array2d<Vector> grid;
	TexCoordBox tc;

	void render_Indexed(const RenderState& rs) const;
	void render_WithAlpha(const RenderState& rs) const;

public:
	bool needVBOUpdate;
	GridDrawOrder drawOrder;
};

// supports animation and automatic upload
class DynamicRenderGrid : public RenderGrid
{
public:
	DynamicRenderGrid();
	~DynamicRenderGrid();

	void update(float dt);

	void setSegs(float dgox, float dgoy, float dgmx, float dgmy, float dgtm, bool dgo);
	void setStripPoints(bool vert, const Vector *points, size_t n);
	void setFromWavy(const float *wavy, size_t len, float width);

protected:
	float gridTimer;
	float drawGridOffsetX;
	float drawGridOffsetY;
	float drawGridModX;
	float drawGridModY;
	float drawGridTimeMultiplier;
	bool drawGridOut;
public:
	unsigned char gridType;  // unsigned char to save space
};

#endif


