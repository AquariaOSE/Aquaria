#include "RenderBase.h"
#include "Base.h"


float *drawCircle(float *p, float radius, size_t vertices, const Vector& center)
{
	const float step = (2 * PI) / float(vertices);
	float a = 0;
	for(size_t i = 0; i < vertices; ++i, a += step)
	{
		*p++ = center.x + cosf(a)*radius;
		*p++ = center.y + sinf(a)*radius;
	}
	return p;
}

void sizePowerOf2Texture(int &v)
{
	int p = 8, use=0;
	do
	{
		use = 1 << p;
		p++;
	}
	while(v > use);

	v = use;
}


unsigned generateEmptyTexture(int quality)											// Create An Empty Texture
{
	GLuint txtnumber=0;											// Texture ID
	unsigned char *data;											// Stored Data

	// Create Storage Space For Texture Data (128x128x4)
	int size = (quality * quality) * 4;
	data = new unsigned char[size];

	memset(data, 0, size);	// Clear Storage Memory

	glGenTextures(1, &txtnumber);								// Create 1 Texture
	glBindTexture(GL_TEXTURE_2D, txtnumber);					// Bind The Texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, quality, quality, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, data);						// Build Texture Using Information In data

	delete [] data;												// Release data

	return txtnumber;											// Return The Texture ID
}
