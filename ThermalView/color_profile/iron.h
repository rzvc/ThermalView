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

#include "gradient.h"


class IronProfile : public GradientProfile
{
public:
	IronProfile()
		: GradientProfile(
			"Iron",
			std::vector< std::pair<float, pcRGB> >
			{
				std::make_pair(0.0, pcRGB(0x00, 0x00, 0x00)),
				std::make_pair(0.06, pcRGB(0x00, 0x00, 0x66)),
				std::make_pair(0.2, pcRGB(0x7c, 0x00, 0x9d)),
				std::make_pair(0.6, pcRGB(0xf0, 0x70, 0x00)),
				std::make_pair(0.8, pcRGB(0xff, 0xcc, 0x00)),
				std::make_pair(0.9, pcRGB(0xff, 0xf0, 0x60)),
				std::make_pair(1.0, pcRGB(0xff, 0xff, 0xff))
			}
		)
	{
	}
};