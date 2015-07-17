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

#pragma once

#include "color_profile.h"


class GradientProfile : public ColorProfile
{
public:
	struct gpRGB
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		
		gpRGB(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
		gpRGB() {}

		std::string getHex() const;

		bool operator == (const gpRGB & other) const
		{
			return r == other.r && g == other.g && b == other.b;
		}
	};

	typedef std::vector< std::pair<float, gpRGB> > Pattern;
	
protected:
	std::string			m_file;			// The file where it should be stored
	std::vector<gpRGB>	m_rgb;			// The gradient
	Pattern				m_pattern;		// The pattern that was used to create this profile
	uint16_t			m_granularity;	// How many points should the gradient have
	
public:
	GradientProfile(const std::string & file, const std::string & name, const Pattern & pattern, uint16_t granularity = 512);
	GradientProfile(const std::string & file);

	wxImage getImage(const ThermalFrame & frame, uint16_t min_val, uint16_t max_val) const override;
	wxImage getGradient() const override;

	const Pattern & getPattern() const;
	uint16_t getGranularity() const;

	const std::string & getFile() const;

	bool save() const;

private:
	void createProfile();	// Creates the profile based on the given pattern and granularity
};