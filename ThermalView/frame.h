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

#include <cstdint>
#include <vector>
#include <array>

class ThermalFrame
{
public:
	std::vector<uint16_t>						m_pixels;
	std::array<std::array<bool, 156>, 206>	m_bad_pixels;

	uint8_t m_id;

	uint16_t m_max_val;
	uint16_t m_min_val;
	uint16_t m_avg_val;

    ThermalFrame();
	ThermalFrame(const std::vector<uint16_t> & data);

	void computeMinMax();

	// Offset calibration
	std::vector<int> getOffsetCalibration() const;
	void applyOffsetCalibration(const std::vector<int> & calibration);


	// Gain calibration
	std::vector<double> getGainCalibration() const;
	void applyGainCalibration(const std::vector<double> & calibration);


	// Get the zero value pixels, that are not pattern pixels
	std::vector<uint16_t> getZeroPixels() const;
	
	// Add to the bad pixels matrix
	void addBadPixels(const std::vector<uint16_t> & pixels);

	// Fix bad pixels
	void fixBadPixels();
	
	// Fix the given pixels
	void fixPixels(const std::vector<uint16_t> & pixels, bool use_given_pixel = false);

	void subtract(const ThermalFrame & frame);
	void add(const ThermalFrame & frame);
};

bool is_pattern_pixel(int x, int y);
bool is_pattern_pixel(int pos);
