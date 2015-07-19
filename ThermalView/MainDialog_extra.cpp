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

#include "MainDialog.h"


void MainDialog::OnNewFrame(const std::vector<uint16_t> & data)
{
	std::lock_guard<std::recursive_mutex> lck(m_mx);
	
	// Let's extract and process the data, while we're still running from the worker thread
	ThermalFrame frame(data);
	
	// See if it's a key frame
	if (frame.m_id != 3)
	{
		switch (frame.m_id)
		{
			// Gain calibration
			case 4:
				m_gain_cal = frame.getGainCalibration();
				m_unknown_gain = frame.getZeroPixels();
			break;

			// Offset calibration (every time the shutter is heard)
			case 1:
				frame.applyGainCalibration(m_gain_cal);
				frame.computeMinMax();
				
				m_offset_cal = frame.getOffsetCalibration();

				m_first_after_cal = true;
			break;
		}

		return;
	}
	
	
	// It's a regular frame, so let's process it
	
	frame.addBadPixels(frame.getZeroPixels());
	frame.addBadPixels(m_unknown_gain);

	frame.applyGainCalibration(m_gain_cal);
	frame.applyOffsetCalibration(m_offset_cal);

	frame.computeMinMax();

	frame.fixBadPixels();

	// If it's the first regular frame after the calibration frame
	if (m_first_after_cal)
	{
		m_first_after_cal = false;

		// Do we have to use it as an extra calibration frame?
		if (m_get_extra_cal)
		{
			m_get_extra_cal = false;
			m_extra_cal = frame.getOffsetCalibration();
		}
		
		// Do we have to stop streaming?
		if (m_get_one_after_cal)
		{
			m_get_one_after_cal = false;
			m_thermal.stopStreaming();
		}
	}
	
	// Set it as the current frame
	m_frame = frame;
	
	// Update the display
	UpdateFrame();
}



void MainDialog::UpdateFrame()
{
	// If we don't have a real profile selected, stop
	if ( m_sel_profile < 0 || m_sel_profile > static_cast<int>(m_profiles.size()) )
		return;

	// If we don't have any data, return
	if (m_frame.m_pixels.empty())
		return;
	
	// Make a copy, we don't want to alter it
	m_frame_extra = m_frame;
	
	
	// Handle extra calibration
	if (!m_extra_cal.empty() && m_use_extra_cal)
	{
		m_frame_extra.applyOffsetCalibration(m_extra_cal);
		m_frame_extra.computeMinMax();
	}


	const auto & profile = m_use_preview_profile ? m_preview_profile : m_profiles[m_sel_profile];

	// Update the image to be displayed
	if (m_auto_range)
		m_new_img = profile->getImage(m_frame_extra, m_frame_extra.m_min_val, m_frame.m_max_val);
	else
		m_new_img = profile->getImage(m_frame_extra, m_manual_min, m_manual_max);


	// Compute the new histogram
	ComputeHistogram();

		
	QueueEvent(new wxCommandEvent(ON_MSG_FRAME_READY));
}


void MainDialog::ComputeHistogram()
{
	// Compute the histogram
	std::vector<uint16_t> vect((m_frame_extra.m_max_val - m_frame_extra.m_min_val) + 1, 0);

	uint16_t max_val = 0;

	for (uint16_t v : m_frame_extra.m_pixels)
	{
		auto & elm = vect[v - m_frame_extra.m_min_val];

		++elm;

		if (elm > max_val)
			max_val = elm;
	}

	// Create the histogram image
	wxBitmap bmp(vect.size(), max_val + 1);
	wxMemoryDC dc(bmp);

	// Erase background
	dc.SetBrush( wxBrush(GetBackgroundColour(), wxBRUSHSTYLE_SOLID) );
	dc.SetPen( wxPen(GetBackgroundColour(), 0) );

	dc.DrawRectangle(0, 0, vect.size(), max_val + 1);

	// Paint the histogram
	dc.SetPen(wxPen(wxColour(0, 0, 0), 1));

	for (size_t i = 0; i < vect.size(); ++i)
	{
		uint16_t v = vect[i];

		if (v)
			dc.DrawLine(i, max_val, i, max_val - v);
	}

	dc.SelectObject(wxNullBitmap);

	// Convert to image
	m_new_historgram = bmp.ConvertToImage();
}