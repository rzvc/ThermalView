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
	
	// Make a copy, we don't want to alter it
	ThermalFrame frame = m_frame;
	
	
	// Handle extra calibration
	if (!m_extra_cal.empty() && m_use_extra_cal)
	{
		frame.applyOffsetCalibration(m_extra_cal);
		frame.computeMinMax();
	}


	m_new_img = m_profiles[m_sel_profile]->getImage(frame);
	
		
	QueueEvent(new wxCommandEvent(ON_MSG_FRAME_READY));
}