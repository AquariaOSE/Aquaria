#include "RenderGrid.h"
#include "RenderBase.h"
#include "RenderState.h"


static void ResetGrid(Vector* dst, size_t w, size_t h)
{
	assert(w > 1 && h > 1);
	const float xMulF = 1.0f / (float)(w-1);
	const float yMulF = 1.0f / (float)(h-1);

	for (size_t y = 0; y < h; y++)
	{
		const float yval = float(y)*yMulF-0.5f;
		for (size_t x = 0; x < w; x++)
		{
			dst->x = float(x)*xMulF-0.5f;
			dst->y = yval;
			++dst;
		}
	}
}

void RenderGrid::ResetWithAlpha(Vector* dst, size_t w, size_t h, float alpha)
{
	assert(w > 1 && h > 1);
	const float xMulF = 1.0f / (float)(w-1);
	const float yMulF = 1.0f / (float)(h-1);

	for (size_t y = 0; y < h; y++)
	{
		const float yval = float(y)*yMulF-0.5f;
		for (size_t x = 0; x < w; x++)
		{
			dst->x = float(x)*xMulF-0.5f;
			dst->y = yval;
			dst->z = alpha;
			++dst;
		}
	}
}



RenderGrid::RenderGrid(size_t w, size_t h)
	: gridTimer(0)
	, drawGridOffsetX(0), drawGridOffsetY(0), drawGridModX(0), drawGridModY(0), drawGridTimeMultiplier(0)
	, drawGridOut(false), gridType(GRID_WAVY), drawOrder(GRID_DRAW_DEFAULT)
{
	init(w, h);
}

RenderGrid::~RenderGrid()
{
}

void RenderGrid::init(size_t w, size_t h)
{
	assert(w > 1 && h > 1);
	grid.init(w, h);
	reset();
	Vector *dg = grid.data();
	for(size_t i = 0; i < grid.linearsize(); ++i)
		dg[i].z = 1.0f;

}


void RenderGrid::reset()
{
	ResetGrid(grid.data(), grid.width(), grid.height());
}

void RenderGrid::resetWithAlpha(float a)
{
	ResetWithAlpha(grid.data(), grid.width(), grid.height(), a);
}

void RenderGrid::update(float dt)
{
	if (gridType == GRID_WAVY)
	{
		gridTimer += dt * drawGridTimeMultiplier;
		reset();
		const size_t w = grid.width();
		const size_t h = grid.height();

		size_t nx = w;
		if(drawGridOut)
			nx /= 2;

		for (size_t y = 0; y < h; y++)
		{
			Vector * const row = grid.row(y);
			const float xoffset = y * drawGridOffsetX;
			const float addx = sinf(gridTimer+xoffset)*drawGridModX;

			size_t x;
			for (x = 0; x < nx; x++)
				row[x].x -= addx;
			for (; x < w; x++)
				row[x].x += addx;

			if(const float dgmy = drawGridModY)
				for (x = 0; x < w; x++)
				{
					float yoffset = x * drawGridOffsetY;
					row[x].y += cosf(gridTimer+yoffset)*dgmy;
				}
		}
	}
}

void RenderGrid::setAlpha(size_t x, size_t y, float a)
{
	if (x < grid.width() && y < grid.height())
		grid(x, y).z = a;
}

void RenderGrid::setSegs(float dgox, float dgoy, float dgmx, float dgmy, float dgtm, bool dgo)
{
	drawGridOffsetX = dgox;
	drawGridOffsetY = dgoy;
	drawGridModX = dgmx;
	drawGridModY = dgmy;
	drawGridTimeMultiplier = dgtm;
	drawGridOut = dgo;
	gridTimer = 0;
	gridType = GRID_WAVY;
}

void RenderGrid::setStripPoints(bool vert, const Vector* points, size_t n)
{
	reset();

	const float mul = float(n);

	if (!vert) // horz
	{
		const size_t xmax = std::min(grid.width(), n);
		for (size_t y = 0; y < grid.height(); y++)
		{
			Vector *row = grid.row(y);
			for (size_t x = 0; x < xmax; x++)
				row[x] += points[x] * mul;
		}
	}
	else
	{
		const size_t ymax = std::min(grid.height(), n);
		for (size_t x = 0; x < grid.width(); x++)
			for (size_t y = 0; y < ymax; y++)
				grid(x, y) += points[y] * mul;
	}
}

void RenderGrid::setFromWavy(const Vector* wavy, size_t len, float width)
{
	const size_t NX = grid.width() - 1;
	const size_t H = grid.height();

	const float iw = 1.0f / width;
	for (size_t y = 0; y < H; y++)
	{
		const size_t wavy_y = (H - y)-1;
		if (wavy_y < len)
		{
			const float tmp = wavy[wavy_y].x * iw;
			Vector * const row = grid.row(y);
			for (size_t x = 0; x < NX; x++)
			{
				row[x].x = tmp - 0.5f;
				row[x+1].x = tmp + 0.5f;
			}
		}
	}
}


void RenderGrid::render(const RenderState& rs, const Vector& upperLeftTexCoords, const Vector& lowerRightTexCoords) const
{
	switch(drawOrder)
	{
		case GRID_DRAW_LRTB:
			render_LRTB(rs, upperLeftTexCoords, lowerRightTexCoords);
			break;

		case GRID_DRAW_LRBT:
			render_LRBT(rs, upperLeftTexCoords, lowerRightTexCoords);
			break;

		case GRID_DRAW_WORLDMAP:
			render_WithAlpha(rs, upperLeftTexCoords, lowerRightTexCoords);
			break;
	}
}

void RenderGrid::renderDebugPoints(const RenderState& rs) const
{
	(void)rs; // unused

	glPointSize(2);
	glColor3f(1,0,0);
	glBegin(GL_POINTS);
		const size_t NX = grid.width()-1;
		const size_t NY = grid.height()-1;
		for (size_t y = 0; y < NY; y++)
		{
			for (size_t x = 0; x < NX; x++)
			{
				glVertex2f(grid(x,y).x,     grid(x,y).y);
				glVertex2f(grid(x,y+1).x,   grid(x,y+1).y);
				glVertex2f(grid(x+1,y+1).x, grid(x+1,y+1).y);
				glVertex2f(grid(x+1,y).x,   grid(x+1,y).y);
			}
		}
	glEnd();
}

void RenderGrid::render_LRTB(const RenderState& rs, const Vector& upperLeftTexCoords, const Vector& lowerRightTexCoords) const
{
	const float percentX = lowerRightTexCoords.x - upperLeftTexCoords.x;
	const float percentY = lowerRightTexCoords.y - upperLeftTexCoords.y;

	const float baseX = upperLeftTexCoords.x;
	const float baseY = upperLeftTexCoords.y;

	const size_t NX = grid.width()-1;
	const size_t NY = grid.height()-1;

	// NOTE: These are used to avoid repeated expensive divide operations,
	// but they may cause rounding error of around 1 part per million,
	// which could in theory cause minor graphical glitches with broken
	// OpenGL implementations.  --achurch
	const float incX = percentX / float(NX);
	const float incY = percentY / float(NY);

	glColor4f(rs.color.x, rs.color.y, rs.color.z, rs.alpha);

	glBegin(GL_QUADS);
	float v0 = baseY;
	float v1 = v0 + incY;
	for (size_t y = 0; y < NY; y++, v0 = v1, v1 += incY)
	{
		float u0 = baseX;
		float u1 = u0 + incX;
		const Vector *row0 = grid.row(y);
		const Vector *row1 = grid.row(y+1);
		for (size_t x = 0; x < NX; x++, u0 = u1, u1 += incX)
		{
			const Vector dg00 = row0[x];
			const Vector dg01 = row1[x];
			const Vector dg10 = row0[x+1];
			const Vector dg11 = row1[x+1];

			glTexCoord2f(u0, v0);
			glVertex2f(dg00.x,		dg00.y);

			glTexCoord2f(u0, v1);
			glVertex2f(dg01.x,		dg01.y);

			glTexCoord2f(u1, v1);
			glVertex2f(dg11.x,		dg11.y);

			glTexCoord2f(u1, v0);
			glVertex2f(dg10.x,		dg10.y);
		}
	}
	glEnd();
}

void RenderGrid::render_LRBT(const RenderState& rs, const Vector& upperLeftTexCoords, const Vector& lowerRightTexCoords) const
{
	const float percentX = lowerRightTexCoords.x - upperLeftTexCoords.x;
	const float percentY = upperLeftTexCoords.y - lowerRightTexCoords.y;

	const float baseX = upperLeftTexCoords.x;
	const float baseY = lowerRightTexCoords.y;

	const size_t NX = grid.width()-1;
	const size_t NY = grid.height()-1;

	// NOTE: These are used to avoid repeated expensive divide operations,
	// but they may cause rounding error of around 1 part per million,
	// which could in theory cause minor graphical glitches with broken
	// OpenGL implementations.  --achurch
	const float incX = percentX / float(NX);
	const float incY = percentY / float(NY);

	glColor4f(rs.color.x, rs.color.y, rs.color.z, rs.alpha);

	glBegin(GL_QUADS);
	float v0 = baseY;
	float v1 = v0 + incY;
	for (size_t y = NY; y --> 0; v0 = v1, v1 += incY)
	{
		float u0 = baseX;
		float u1 = u0 + incX;
		const Vector *row0 = grid.row(y+1);
		const Vector *row1 = grid.row(y);
		for (size_t x = 0; x < NX; x++, u0 = u1, u1 += incX)
		{
			const Vector dg00 = row0[x];
			const Vector dg01 = row1[x];
			const Vector dg10 = row0[x+1];
			const Vector dg11 = row1[x+1];

			glTexCoord2f(u0, v0);
			glVertex2f(dg00.x,		dg00.y);

			glTexCoord2f(u0, v1);
			glVertex2f(dg01.x,		dg01.y);

			glTexCoord2f(u1, v1);
			glVertex2f(dg11.x,		dg11.y);

			glTexCoord2f(u1, v0);
			glVertex2f(dg10.x,		dg10.y);
		}
	}
	glEnd();
}

void RenderGrid::render_WithAlpha(const RenderState& rs, const Vector& upperLeftTexCoords, const Vector& lowerRightTexCoords) const
{
	const float percentX = fabsf(lowerRightTexCoords.x - upperLeftTexCoords.x);
	const float percentY = fabsf(upperLeftTexCoords.y - lowerRightTexCoords.y);

	const float baseX =
		(lowerRightTexCoords.x < upperLeftTexCoords.x)
		? lowerRightTexCoords.x : upperLeftTexCoords.x;
	const float baseY =
		(lowerRightTexCoords.y < upperLeftTexCoords.y)
		? lowerRightTexCoords.y : upperLeftTexCoords.y;

	const size_t NX = grid.width()-1;
	const size_t NY = grid.height()-1;

	// NOTE: These are used to avoid repeated expensive divide operations,
	// but they may cause rounding error of around 1 part per million,
	// which could in theory cause minor graphical glitches with broken
	// OpenGL implementations.  --achurch
	const float incX = percentX / float(NX);
	const float incY = percentY / float(NY);

	const Vector c = rs.color;
	const float alpha = rs.alpha;

	glBegin(GL_QUADS);
	float v0 = 1 - percentY + baseY;
	float v1 = v0 + incY;
	for (size_t y = 0; y < NY; y++, v0 = v1, v1 += incY)
	{
		float u0 = baseX;
		float u1 = u0 + incX;
		const Vector *row0 = grid.row(y);
		const Vector *row1 = grid.row(y+1);
		for (size_t x = 0; x < NX; x++, u0 = u1, u1 += incX)
		{
			const Vector dg00 = row0[x];
			const Vector dg01 = row1[x];
			const Vector dg10 = row0[x+1];
			const Vector dg11 = row1[x+1];

			if (dg00.z != 0 || dg01.z != 0 || dg10.z != 0 || dg11.z != 0)
			{
				glColor4f(c.x, c.y, c.z, alpha*dg00.z);
				glTexCoord2f(u0, v0);
				glVertex2f(dg00.x, dg00.y);

				glColor4f(c.x, c.y, c.z, alpha*dg01.z);
				glTexCoord2f(u0, v1);
				glVertex2f(dg01.x, dg01.y);

				glColor4f(c.x, c.y, c.z, alpha*dg11.z);
				glTexCoord2f(u1, v1);
				glVertex2f(dg11.x, dg11.y);

				glColor4f(c.x, c.y, c.z, alpha*dg10.z);
				glTexCoord2f(u1, v0);
				glVertex2f(dg10.x, dg10.y);
			}
		}
	}
	glEnd();
}

