/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "DSQ.h"
#include "ttvfs_stdio.h"
#include <fstream>
#include "stb_image_resize.h"

#if MAPVIS_SUBDIV % 8 != 0
	#error MAPVIS_SUBDIV must be a multiple of 8
#endif
const unsigned int rowSize = MAPVIS_SUBDIV / 8;
const unsigned int dataSize = MAPVIS_SUBDIV * rowSize;

// Used by dataToString(), stringToData()
static const char base64Chars[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

WorldMapTile::WorldMapTile()
{
	revealed = false;
	prerevealed = false;
	scale = scale2 = 1;
	layer = 0;
	index = -1;
	stringIndex = 0;
	dirty = true;
}

WorldMapTile::~WorldMapTile()
{
}

void WorldMapTile::markVisited(int left, int top, int right, int bottom)
{
	if(visited.empty())
		visited.init(rowSize, MAPVIS_SUBDIV);

	if (left < 0)
		left = 0;
	if (top < 0)
		top = 0;
	if (right > MAPVIS_SUBDIV - 1)
		right = MAPVIS_SUBDIV - 1;
	if (bottom > MAPVIS_SUBDIV - 1)
		bottom = MAPVIS_SUBDIV - 1;

	// Accumulate bits that were changed
	unsigned changed = 0;

	for (int y = top; y <= bottom; y++)
	{
		unsigned char *row = visited.row(y);
		for (int x = left; x <= right; x++)
		{
			unsigned char& val = row[x/8];
			unsigned char add = 1 << (x%8);
			changed |= (val ^ add) & add;
			val |= add;
		}
	}

	if(changed)
	{
		//debugLog("worldmap[" + name + "] visited something new");
		dirty = true; // Only mark as dirty if we've uncovered something new
	}
}

// Convert the data array to string format for saving.
void WorldMapTile::dataToString(std::ostringstream &os) const
{
	if (visited.empty())
	{
		os << "0 0";
		return;
	}

#ifdef AQUARIA_SAVE_MAPVIS_RAW

	char *outbuf = new char[((dataSize+2)/3)*4 + 1];
	char *ptr = outbuf;
	const unsigned char * const data = visited.data();

	unsigned int i;
	for (i = 0; i+3 <= dataSize; i += 3, ptr += 4)
	{
		ptr[0] = base64Chars[(               data[i+0]>>2) & 0x3F];
		ptr[1] = base64Chars[(data[i+0]<<4 | data[i+1]>>4) & 0x3F];
		ptr[2] = base64Chars[(data[i+1]<<2 | data[i+2]>>6) & 0x3F];
		ptr[3] = base64Chars[(data[i+2]<<0               ) & 0x3F];
	}
	if (i < dataSize)
	{
		ptr[0] = base64Chars[(data[i+0]>>2) & 0x3F];
		if (i+1 < dataSize)
		{
			ptr[1] = base64Chars[(data[i+0]<<4 | data[i+1]>>4) & 0x3F];
			ptr[2] = base64Chars[(data[i+1]<<2) & 0x3F];
		} else {
			ptr[1] = base64Chars[(data[i+0]<<4) & 0x3F];
			ptr[2] = '=';
		}
		ptr[3] = '=';
		ptr += 4;
	}
	*ptr = 0;

	os << MAPVIS_SUBDIV << " b " // "b" for bitmap
	   << (dataSize==0 ? "====" : outbuf);  // Always write a non-empty string
	delete[] outbuf;

#else  // !AQUARIA_SAVE_MAPVIS_RAW

	unsigned int count = 0;
	std::ostringstream tempStream;

	for (unsigned int y = 0; y < MAPVIS_SUBDIV; y++)
	{
		const unsigned char *row = visited.row(y);
		for (unsigned int x = 0; x < MAPVIS_SUBDIV; x += 8)
		{
			if(unsigned char dataByte = row[x/8])
			{
				for (unsigned int x2 = 0; x2 < 8; x2++)
				{
					if (dataByte & (1 << x2))
					{
						tempStream << " " << (x+x2) << " " << y;
						count++;
					}
				}
			}
		}
	}

	os << MAPVIS_SUBDIV << " " << count << tempStream.str();

#endif
}

// Parse a string from a save file and store in the data array.
void WorldMapTile::stringToData(SimpleIStringStream &is)
{
	dirty = true;
	visited.init(rowSize, MAPVIS_SUBDIV);

	int subdiv;
	std::string countOrType;
	is >> subdiv >> countOrType;
	if (subdiv != MAPVIS_SUBDIV)
	{
		if (subdiv != 0)
		{
			std::ostringstream os;
			os << "subdiv " << subdiv << " != MAPVIS_SUBDIV "
			   << MAPVIS_SUBDIV << ", can't load data!";
			debugLog(os.str());
		}

		// Skip over the data so we can still load the next tile's data
		if (countOrType == "b")
		{
			std::string encodedData;
			is >> encodedData;
		}
		else
		{
			int count = 0;
			std::istringstream is2(countOrType);
			is2 >> count;
			for (int i = 0; i < count; i++)
			{
				int x, y;
				is >> x >> y;
			}
		}
		return;
	}

	if (countOrType == "b")  // Raw bitmap (base64-encoded)
	{
		std::string encodedData = "";
		is >> encodedData;
		const char *in = encodedData.c_str();
		unsigned char *out = visited.data();
		unsigned char * const top = out + dataSize;
		while (in[0] != 0 && in[1] != 0 && out < top)
		{
			unsigned char ch0, ch1, ch2, ch3;
			const char *temp;
			temp = strchr(base64Chars, in[0]);
				ch0 = temp ? temp - base64Chars : 0;
			temp = strchr(base64Chars, in[1]);
				ch1 = temp ? temp - base64Chars : 0;
			if (in[2] != 0)
			{
				temp = strchr(base64Chars, in[2]);
					ch2 = temp ? temp - base64Chars : 0;
				temp = strchr(base64Chars, in[3]);
					ch3 = temp ? temp - base64Chars : 0;
			}
			else
			{
				ch2 = ch3 = 0;
			}
			*out++ = ch0<<2 | ch1>>4;
			if (out >= top || in[2] == 0 || in[2] == '=')
				break;
			*out++ = ch1<<4 | ch2>>2;
			if (out >= top || in[3] == 0 || in[3] == '=')
				break;
			*out++ = ch2<<6 | ch3>>0;
			in += 4;
		}
	}
	else  // List of coordinate pairs
	{
		visited.fill(0);

		int count = 0;
		std::istringstream is2(countOrType);
		is2 >> count;

		for (int i = 0; i < count; i++)
		{
			int x, y;
			is >> x >> y;
			if (x >= 0 && x < MAPVIS_SUBDIV && y >= 0 && y < MAPVIS_SUBDIV)
				visited(x/8, y) |= 1 << (x%8);
		}
	}
}

ImageData WorldMapTile::generateAlphaImage(size_t w, size_t h)
{
	ImageData ret = {0};
	if(visited.empty())
		return ret;

	const unsigned char exploredAlpha = 0xff;
	const unsigned char unexploredAlpha = prerevealed ? WORLDMAP_REVEALED_BUT_UNEXPLORED_ALPHA : 0;

	// convert visited data to a proper 1-channel image
	Array2d<unsigned char> tmp(MAPVIS_SUBDIV, MAPVIS_SUBDIV);
	unsigned char *dst = tmp.data();
	for(size_t y = 0; y < MAPVIS_SUBDIV; ++y)
	{
		const unsigned char * const src = visited.row(y);
		for(size_t x = 0; x < MAPVIS_SUBDIV; x += 8)
		{
			unsigned c = src[x/8];
			for(size_t bit = 0; bit < 8; ++bit)
				*dst++ = (c & (1 << bit)) ? exploredAlpha : unexploredAlpha;
		}
	}

	std::ostringstream os;
	os << "worldmap[" << name << "] Generate alpha from vis data, resize to (" << w << ", " << h << ")";
	debugLog(os.str());

	ret.pixels = (unsigned char*)malloc(w * h);
	if(ret.pixels)
	{
		ret.w = w;
		ret.h = h;
		ret.channels = 1;

		stbir_resize_uint8_generic(
			tmp.data(), MAPVIS_SUBDIV, MAPVIS_SUBDIV, 0,
			ret.pixels, w, h, 0,
			1, STBIR_ALPHA_CHANNEL_NONE, 0,
			STBIR_EDGE_CLAMP, STBIR_FILTER_TRIANGLE, STBIR_COLORSPACE_LINEAR, NULL
		);
	}

	return ret;
}

// If there was a shader to use one texture for colors and another for alpha this wouldn't be needed
// TODO: If we ever get this far, do this with shaders. Software resize sucks so bad...
bool WorldMapTile::updateDiscoveredTex()
{
	int w, h;
	size_t sz;
	const unsigned char * texbuf = originalTex->getBufferAndSize(&w, &h, &sz);
	std::vector<unsigned char> tmp(sz);
	{
		const ImageData aimg = generateAlphaImage(w, h);
		if(!aimg.pixels)
			return false;
		const unsigned char *ap = aimg.pixels;
		unsigned char *dst = &tmp[0];
		for(size_t i = 0; i < sz; i += 4)
		{
			*dst++ = *texbuf++;
			*dst++ = *texbuf++;
			*dst++ = *texbuf++;
			*dst++ = (*texbuf++ * *ap++) >> 8; // inaccurate fixed-point multiplication; ends up 0xfe as the highest possible value but whatev
		}
		free(aimg.pixels);
	}

	ImageData up;
	up.pixels = &tmp[0];
	up.channels = 4;
	up.w = w;
	up.h = h;

	Texture *tex = generatedTex.content();
	if(!tex)
		generatedTex = tex = new Texture();

	tex->upload(up, true);
	return true;
}



WorldMap::WorldMap()
{
	gw=gh=0;
}

void WorldMap::load()
{
	if (!dsq->mod.isActive())
		_load("data/worldmap.txt");
	else
		_load(dsq->mod.getPath() + "worldmap.txt");
}

void WorldMap::_load(const std::string &file)
{
	worldMapTiles.clear();

	std::string line;

	InStream in(file.c_str());

	while (std::getline(in, line))
	{
		WorldMapTile t;
		std::istringstream is(line);
		is >> t.index >> t.stringIndex >> t.name >> t.layer >> t.scale >> t.gridPos.x >> t.gridPos.y >> t.prerevealed >> t.scale2;
		t.revealed = t.prerevealed;
		stringToUpper(t.name);
		worldMapTiles.push_back(t);
	}
}

void WorldMap::save()
{
	std::string fn;

	if (dsq->mod.isActive())
		fn = dsq->mod.getPath() + "worldmap.txt";
	else
		fn = "data/worldmap.txt";

	std::ofstream out(fn.c_str());

	if (out)
	{
		for (size_t i = 0; i < worldMapTiles.size(); i++)
		{
			WorldMapTile *t = &worldMapTiles[i];
			out << t->index << " " << t->stringIndex << " " << t->name << " " << t->layer << " " << t->scale << " " << t->gridPos.x << " " << t->gridPos.y << " " << t->prerevealed << " " << t->scale2 << std::endl;
		}
		dsq->screenMessage(stringbank.get(2019) + " " + fn);
	}
	else
	{
		dsq->screenMessage(stringbank.get(2020) + " " + fn);
	}
}

void WorldMap::revealMap(const std::string &name)
{
	WorldMapTile *t = getWorldMapTile(name);
	if (t)
	{
		t->revealed = true;
	}
}

void WorldMap::revealMapIndex(int index)
{
	WorldMapTile *t = getWorldMapTileByIndex(index);
	if (t)
	{
		t->revealed = true;
	}
}

WorldMapTile *WorldMap::getWorldMapTile(const std::string &name)
{
	std::string n = name;
	stringToUpper(n);
	for (size_t i = 0; i < worldMapTiles.size(); i++)
	{
		if (worldMapTiles[i].name == n)
		{
			return &worldMapTiles[i];
		}
	}
	return 0;
}

WorldMapTile *WorldMap::getWorldMapTileByIndex(int index)
{
	for (size_t i = 0; i < worldMapTiles.size(); i++)
	{
		if (worldMapTiles[i].index == index)
		{
			return &worldMapTiles[i];
		}
	}
	return 0;
}
