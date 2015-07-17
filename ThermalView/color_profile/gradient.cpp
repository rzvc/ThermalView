/* The MIT License (MIT)
 *
 * Copyright (c) 2015 Razvan C. Cojocariu (code@dumb.ro)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "color_profile/gradient.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <boost/algorithm/string/trim.hpp>


using namespace std;


static const size_t MAX_GRADIENT_HEIGHT = 1000;



//////////////////////////////////////////////////////////////////////////

static char * hex_chars = "0123456789ABCDEF";

#define HEX_BYTE(b) ( std::string() + hex_chars[(b) >> 4] + hex_chars[(b) & 0x0F] )


std::string GradientProfile::gpRGB::getHex() const
{
	return HEX_BYTE(r) + HEX_BYTE(g) + HEX_BYTE(b);
}

//////////////////////////////////////////////////////////////////////////

static unsigned char from_hex(char c)
{
	if (c <= '9')
		return c - '0';
	else
		return 10 + (c - 'A');
}

//////////////////////////////////////////////////////////////////////////


// Regular constructor
GradientProfile::GradientProfile(const std::string & file, const std::string & name, const Pattern & pattern, uint16_t granularity)
	: ColorProfile(name, TYPE_GRADIENT), m_file(file), m_pattern(pattern), m_granularity(granularity)
{
	createProfile();
}

// Constructor from file
GradientProfile::GradientProfile(const std::string & file)
	: ColorProfile("", TYPE_GRADIENT), m_file(file)
{
	ifstream f(file);

	if (f.is_open())
	{
		f >> m_name;
		f >> m_granularity;

		while (!f.eof())
		{
			float	rank;
			string	str;

			f >> rank;
			f >> str;

			transform(str.begin(), str.end(), str.begin(), toupper);

			// Figure out the RGB value
			gpRGB rgb;

			if (regex_match(str, regex("[0-9A-Fa-f]{6}")))
			{
				rgb.r = from_hex(str[0]) * 16 + from_hex(str[1]);
				rgb.g = from_hex(str[2]) * 16 + from_hex(str[3]);
				rgb.b = from_hex(str[4]) * 16 + from_hex(str[5]);
			}
			else
				throw std::exception(("Invalid color value " + str).c_str());

			m_pattern.push_back(Pattern::value_type(rank, rgb));
		}

		// We need at least two data points
		assert(m_pattern.size() >= 2);

		f.close();

		createProfile();
	}
	else
		throw std::exception(("Failed to open file '" + file + "'").c_str());
}

void GradientProfile::createProfile()
{
	size_t pos = 1;	// The current position

	// Initial data
	gpRGB start_color = m_pattern[0].second;
	gpRGB stop_color = m_pattern[1].second;

	float stop_ratio = m_pattern[1].first;

	// Calculate the ratios
	size_t first_value = 0;
	size_t last_value = m_granularity * stop_ratio;

	size_t span = last_value - first_value;

	float r_mul = (static_cast<float>(stop_color.r) - start_color.r) / span;
	float g_mul = (static_cast<float>(stop_color.g) - start_color.g) / span;
	float b_mul = (static_cast<float>(stop_color.b) - start_color.b) / span;


	// Now let's fill it
	for (size_t i = 0; i < m_granularity; ++i)
	{
		if (i > last_value)
		{
			if (++pos == m_pattern.size())
				break;

			start_color = stop_color;
			stop_color = m_pattern[pos].second;

			stop_ratio = m_pattern[pos].first;

			first_value = i - 1;
			last_value = (m_granularity - 1) * stop_ratio;

			span = last_value - first_value;

			r_mul = (static_cast<float>(stop_color.r) - start_color.r) / span;
			g_mul = (static_cast<float>(stop_color.g) - start_color.g) / span;
			b_mul = (static_cast<float>(stop_color.b) - start_color.b) / span;
		}

		uint8_t r = start_color.r + (i - first_value) * r_mul;
		uint8_t g = start_color.g + (i - first_value) * g_mul;
		uint8_t b = start_color.b + (i - first_value) * b_mul;

		m_rgb.push_back(gpRGB(r, g, b));
	}
}

wxImage GradientProfile::getImage(const ThermalFrame & frame, uint16_t min_val, uint16_t max_val) const
{
	wxImage img(206, 156);

	uint16_t real_span = max_val - min_val;

	for (size_t y = 0; y < 156; ++y)
	{
		for (size_t x = 0; x < 206; ++x)
		{
			size_t pixel = y * 206 + x;
			uint16_t val = frame.m_pixels[pixel];

			if (val < min_val)
				val = min_val;

			if (val > max_val)
				val = max_val;

			// The value should be something between 0 and 255, so we have to map the difference between min_val and max_val to 0 to 255
			if (real_span != 0)
				val = ((val - min_val) * (m_rgb.size() - 1)) / real_span;
			else
				val = m_rgb.size() / 2;

			img.SetRGB(x, y, m_rgb[val].r, m_rgb[val].g, m_rgb[val].b);
		}
	}
	
	return img;
}

wxImage GradientProfile::getGradient() const
{
	size_t max_height = std::max(m_rgb.size(), MAX_GRADIENT_HEIGHT);
	
	wxImage img(15, max_height);
	
	for (size_t y = 0; y < max_height; ++y)
	{
		size_t val = (y * (m_rgb.size() - 1)) / max_height;
		
		for (size_t x = 0; x < 15; ++x)
			img.SetRGB(x, max_height - y - 1, m_rgb[val].r, m_rgb[val].g, m_rgb[val].b);
	}
	
	return img;
}

const GradientProfile::Pattern & GradientProfile::getPattern() const
{
	return m_pattern;
}

uint16_t GradientProfile::getGranularity() const
{
	return m_rgb.size();
}

const string & GradientProfile::getFile() const
{
	return m_file;
}

bool GradientProfile::save() const
{
	ofstream f(m_file);

	if (f.is_open())
	{
		f << m_name << endl;

		f.precision(2);

		for (const auto & p : m_pattern)
			f << p.first << " " << p.second.getHex() << endl;

		f.close();

		return true;
	}
	else
		return false;
}