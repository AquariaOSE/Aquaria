#include "RenderBase.h"
#include "Base.h"


void drawCircle(float radius, int stepSize)
{
	glBegin(GL_POLYGON);
		for(int i=0;i < 360; i+=stepSize)
		{
			const float degInRad = i*(PI/180.0f);
			glVertex3f(cosf(degInRad)*radius, sinf(degInRad)*radius,0.0);
		}
	glEnd();
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
