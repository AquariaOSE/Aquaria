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


RenderGrid::RenderGrid()
	: indexbuf(GPUBUF_STATIC | GPUBUF_INDEXBUF), vbo(GPUBUF_DYNAMIC | GPUBUF_VERTEXBUF), trisToDraw(0)
	, needVBOUpdate(false),  drawOrder(GRID_DRAW_DEFAULT)
{
	tc.setStandard();
}

RenderGrid::~RenderGrid()
{
}

void RenderGrid::dropBuffers()
{
	vbo.dropBuffer();
	indexbuf.dropBuffer();
}

void RenderGrid::init(size_t w, size_t h)
{
	assert(w > 1 && h > 1);
	grid.init(w, h);
	setDrawOrder((GridDrawOrder)drawOrder, true);
	reset();
	Vector *dg = grid.data();
	for(size_t i = 0; i < grid.linearsize(); ++i)
		dg[i].z = 1.0f;

	updateVBO();
}

void RenderGrid::init(size_t w, size_t h, const TexCoordBox& tc)
{
	this->tc = tc;
	this->init(w, h);
}


void RenderGrid::reset()
{
	ResetGrid(grid.data(), grid.width(), grid.height());
	needVBOUpdate = true;
}

void RenderGrid::resetWithAlpha(float a)
{
	ResetWithAlpha(grid.data(), grid.width(), grid.height(), a);
	needVBOUpdate = true;
}

void RenderGrid::setAlpha(size_t x, size_t y, float a)
{
	if (x < grid.width() && y < grid.height())
		grid(x, y).z = a;
	needVBOUpdate = true;
}

void RenderGrid::setDrawOrder(GridDrawOrder ord, bool force)
{
	if(!force && drawOrder == ord)
		return;
	drawOrder = ord;
	trisToDraw = indexbuf.initGridIndices_Triangles(grid.width(), grid.height(), ord == GRID_DRAW_LRBT, GPUACCESS_HOSTCOPY);
}

void RenderGrid::setTexCoords(const TexCoordBox& tc)
{
	this->tc = tc;
	needVBOUpdate = true;
}


void RenderGrid::render(const RenderState& rs) const
{
	switch(drawOrder)
	{
		case GRID_DRAW_WORLDMAP:
			if(rs.alpha != 1 || rs.color != Vector(1,1,1))
			{
				render_WithAlpha(rs);
				break;
			}
			// else fall through

		default:
			render_Indexed(rs);

	}
}

void RenderGrid::renderDebugPoints(const RenderState& rs) const
{
	(void)rs; // unused yet
	glPointSize(2);
	glColor3f(1,0,0);
	vbo.apply();
	glDrawArrays(GL_POINTS, 0, grid.linearsize());
}

void RenderGrid::updateVBO()
{
	const float percentX = tc.u2 - tc.u1;
	const float percentY = tc.v2 - tc.v1;

	const float baseX = tc.u1;
	const float baseY = tc.v1;

	const size_t W = grid.width();
	const size_t H = grid.height();

	// NOTE: These are used to avoid repeated expensive divide operations,
	// but they may cause rounding error of around 1 part per million,
	// which could in theory cause minor graphical glitches with broken
	// OpenGL implementations.  --achurch
	const float incX = percentX / float(W-1);
	const float incY = percentY / float(H-1);

	do
	{
		float *p = (float*)vbo.beginWrite(GPUBUFTYPE_VEC2_TC, W*H * (2*2) * sizeof(float), GPUACCESS_DEFAULT);

		float v = baseY;
		for (size_t y = 0; y < H; y++, v += incY)
		//for (size_t y = H; y --> 0; v += incY)
		{
			float u = baseX;
			const Vector *row = grid.row(y);
			for (size_t x = 0; x < W; x++, u += incX)
			{
				*p++ = row->x;
				*p++ = row->y;
				++row;
				*p++ = u;
				*p++ = v;
			}
		}
	}
	while(!vbo.commitWrite());

	needVBOUpdate = false;
}

void RenderGrid::updateVBOIfNecessary()
{
	if(needVBOUpdate)
		updateVBO();
}

void RenderGrid::render_Indexed(const RenderState& rs) const
{
	(void)rs;
	// can't render this here when color/alpha is modulated AND we have colors as part of the vertex data;
	// old opengl simply doesn't support this
	assert(drawOrder != GRID_DRAW_WORLDMAP || (rs.color == Vector(1,1,1) && rs.alpha == 1));
	vbo.apply();
	indexbuf.drawElements(GL_TRIANGLES, trisToDraw);
}

void RenderGrid::render_WithAlpha(const RenderState& rs) const
{
	const float percentX = fabsf(tc.u2 - tc.u1);
	const float percentY = fabsf(tc.v1 - tc.v2);

	const float baseX =
		(tc.u2 < tc.u1)
		? tc.u2 : tc.u1;
	const float baseY =
		(tc.v2 < tc.v1)
		? tc.v2 : tc.v1;

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


// -------------------------------------


DynamicRenderGrid::DynamicRenderGrid()
	: RenderGrid()
	, gridTimer(0)
	, drawGridOffsetX(0), drawGridOffsetY(0), drawGridModX(0), drawGridModY(0), drawGridTimeMultiplier(0)
	, drawGridOut(false), gridType(GRID_WAVY)
{
}

DynamicRenderGrid::~DynamicRenderGrid()
{
}

void DynamicRenderGrid::update(float dt)
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
		// always update vbo now
	}
	else if(!needVBOUpdate)
		return;

	updateVBO();
}

void DynamicRenderGrid::setSegs(float dgox, float dgoy, float dgmx, float dgmy, float dgtm, bool dgo)
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

void DynamicRenderGrid::setStripPoints(bool vert, const Vector* points, size_t n)
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

	needVBOUpdate = true;
}

void DynamicRenderGrid::setFromWavy(const float* wavy, size_t len, float width)
{
	const size_t NX = grid.width() - 1;
	const size_t H = grid.height();

	const float iw = 1.0f / width;
	for (size_t y = 0; y < H; y++)
	{
		const size_t wavy_y = (H - y)-1;
		if (wavy_y < len)
		{
			const float tmp = wavy[wavy_y] * iw;
			Vector * const row = grid.row(y);
			for (size_t x = 0; x < NX; x++)
			{
				row[x].x = tmp - 0.5f;
				row[x+1].x = tmp + 0.5f;
			}
		}
	}
	needVBOUpdate = true;
}
