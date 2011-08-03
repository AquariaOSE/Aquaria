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

void FFTNotes::load()
{
	data.clear();
	std::ifstream in("data/FFTNotes.txt");
	std::string read;
	while (std::getline(in, read))
	{
		std::istringstream is(read);
		int n=0, v=0;
		is >> n;
		while (is >> v)
		{
			data[v] = n+1;
		}
	}
	in.close();
}

int FFTNotes::getNoteFromFFT(int fft, int octave)
{
	if (fft == 0)
		return -1;
	int v = data[fft]-1;
	/*
	std::ostringstream os;
	os << "fftv: " << v;
	debugLog(os.str());
	*/
	if (v == (octave+1)*10)
	{
		v = 7;
	}
	if (v > (octave+1)*10 || v < octave*10)
	{
		v = -1;
	}
	if (v != -1)
		v -= octave*10; // : )
	return v;
}
