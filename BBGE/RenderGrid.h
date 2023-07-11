#ifndef BBGE_RENDERGRID_H
#define BBGE_RENDERGRID_H

#include "Vector.h"
#include "DataStructures.h"

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
	GRID_INTERP = 3, // quad is in grid mode
};

class RenderGrid
{
public:
	RenderGrid(size_t w, size_t h);
	~RenderGrid();

	void init(size_t w, size_t h);
	void reset();
	void resetWithAlpha(float a);
	void update(float dt);
	void render(const RenderState& rs, const Vector& upperLeftTexCoords, const Vector& lowerRightTexCoords) const;
	void renderDebugPoints(const RenderState& rs) const;
	void setAlpha(size_t x, size_t y, float a);
	void setSegs(float dgox, float dgoy, float dgmx, float dgmy, float dgtm, bool dgo);
	void setStripPoints(bool vert, const Vector *points, size_t n);
	void setFromWavy(const Vector *wavy, size_t len, float width);

	size_t width() const { return grid.width(); }
	size_t height() const { return grid.height(); }
	size_t linearsize() const { return grid.linearsize(); }
	const Vector *data() const { return grid.data(); }
	Vector *data() { return grid.data(); }
	Array2d<Vector>& array2d() { return grid; }
	const Array2d<Vector>& array2d() const { return grid; }

	static void ResetWithAlpha(Vector* dst, size_t w, size_t h, float alpha);

protected:
	Array2d<Vector> grid;
	float gridTimer;
	float drawGridOffsetX;
	float drawGridOffsetY;
	float drawGridModX;
	float drawGridModY;
	float drawGridTimeMultiplier;
	bool drawGridOut;
public:
	unsigned char gridType;  // unsigned char to save space
	unsigned char drawOrder;

	void render_LRTB(const RenderState& rs, const Vector& upperLeftTexCoords, const Vector& lowerRightTexCoords) const;
	void render_LRBT(const RenderState& rs, const Vector& upperLeftTexCoords, const Vector& lowerRightTexCoords) const;
	void render_WithAlpha(const RenderState& rs, const Vector& upperLeftTexCoords, const Vector& lowerRightTexCoords) const;
};

#endif


