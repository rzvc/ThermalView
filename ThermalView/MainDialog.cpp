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
#include <functional>

#include "color_profile/gray.h"
#include "color_profile/inverted_gray.h"
#include "color_profile/iron.h"
#include "color_profile/amber.h"

wxDEFINE_EVENT(ON_MSG_FRAME_READY, wxCommandEvent);
wxDEFINE_EVENT(ON_MSG_CONNECTION_STATUS_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(ON_MSG_STREAMING_STATUS_CHANGE, wxCommandEvent);

MainDialog::MainDialog(wxWindow* parent)
    : MainDialogBaseClass(parent)
{
	SetIcon(wxIcon("aaaFirstIcon", wxBITMAP_TYPE_ICO_RESOURCE));
	
	m_get_extra_cal		= false;
	m_use_extra_cal		= false;
	m_first_after_cal	= false;
	m_get_one_after_cal	= false;
	m_got_image			= false;
	m_auto_range		= true;
	m_manual_min		= 2000;
	m_manual_max		= 18000;
	

	m_profiles.push_back(std::unique_ptr<ColorProfile>(new GrayProfile()) );
	m_profiles.push_back(std::unique_ptr<ColorProfile>(new InvertedGrayProfile()) );
	m_profiles.push_back(std::unique_ptr<ColorProfile>(new IronProfile()) );
	m_profiles.push_back(std::unique_ptr<ColorProfile>(new AmberProfile()) );

	
	// Connect wx Events
	Bind(ON_MSG_FRAME_READY, &MainDialog::OnMsgFrameReady, this);
	Bind(ON_MSG_CONNECTION_STATUS_CHANGE, &MainDialog::OnMsgConnectionStatusChange, this);
	Bind(ON_MSG_STREAMING_STATUS_CHANGE, &MainDialog::OnMsgStreamingStatusChange, this);

	// Connect SeekThermal Events
	m_thermal.onNewFrame.connect(std::bind(&MainDialog::OnNewFrame, this, std::placeholders::_1));
	m_thermal.onConnecting.connect(std::bind(&MainDialog::OnConnectionStatusChange, this));
	m_thermal.onDisconnected.connect(std::bind(&MainDialog::OnConnectionStatusChange, this));
	m_thermal.onStreamingStart.connect(std::bind(&MainDialog::OnStreamingStatusChange, this));
	m_thermal.onStreamingStop.connect(std::bind(&MainDialog::OnStreamingStatusChange, this));


	// Populate the interpolation LB
	m_lb_interpolation->Append("Normal");
	m_lb_interpolation->Append("Bilinear");
	m_lb_interpolation->Append("Bicubic");

	m_lb_interpolation->SetSelection(0);
	m_quality = wxIMAGE_QUALITY_NORMAL;

	
	// Populate the color profile LB
	for (size_t i = 0; i < m_profiles.size(); ++i)
		m_lb_profile->Append(m_profiles[i]->getName());
		
	m_lb_profile->SetSelection(0);
	m_sel_profile = 0;
	
	
	// Set the gradient image
	m_gradient->setZoomType(wxImageView::ZOOM_STRETCH);
	m_gradient->setImage(m_profiles[m_sel_profile]->getGradient());
	

	// Populate the save sizes
	m_lb_sizes->Append("206 x 156");
	m_lb_sizes->Append("412 x 312");
	m_lb_sizes->Append("824 x 624");
	
	m_lb_sizes->SetSelection(0);
	
	
	// Try to connect to the camera
	if (m_thermal.connect())
		m_thermal.getStream();
}

MainDialog::~MainDialog()
{
	m_thermal.close();
}


//////////////////////////////////////////////////////////////////////////
// Events from the camera interface - may run from the worker thread
//////////////////////////////////////////////////////////////////////////
void MainDialog::OnConnectionStatusChange()
{
	// Trigger the event on the main thread
	QueueEvent(new wxCommandEvent(ON_MSG_CONNECTION_STATUS_CHANGE));
}

void MainDialog::OnStreamingStatusChange()
{
	// Trigger the event on the main thread
	QueueEvent(new wxCommandEvent(ON_MSG_STREAMING_STATUS_CHANGE));
}


//////////////////////////////////////////////////////////////////////////
// Forwarded events, that run on the UI thread
//////////////////////////////////////////////////////////////////////////
void MainDialog::OnMsgConnectionStatusChange(wxCommandEvent &)
{
	if (m_thermal.isOpen())
	{
		m_button_connect->SetLabel("Disconnect");
		m_button_stop->Enable();
		m_button_take_one->Enable();
		m_button_get_cal->Enable();
	}
	else
	{
		m_button_connect->SetLabel("Connect");
		m_button_stop->Enable(false);
		m_button_take_one->Enable(false);
		m_button_get_cal->Enable(false);
	}
}

void MainDialog::OnMsgStreamingStatusChange(wxCommandEvent &)
{
	if (m_thermal.isStreaming())
		m_button_stop->SetLabel("Stop");
	else
		m_button_stop->SetLabel("Start");
}

void MainDialog::OnMsgFrameReady(wxCommandEvent &)
{
	std::lock_guard<std::recursive_mutex> lck(m_mx);
	
	m_picture->setImage(m_new_img);

	m_slider_low->SetSelection(m_frame_extra.m_min_val, m_frame_extra.m_max_val);
	m_slider_high->SetSelection(m_frame_extra.m_min_val, m_frame_extra.m_max_val);

	if (m_auto_range)
	{
		m_slider_low->SetValue(m_frame_extra.m_min_val);
		m_slider_high->SetValue(m_frame_extra.m_max_val);
	}
	
	if (!m_got_image)
	{
		m_got_image = true;
		m_button_save->Enable();
	}
}


//////////////////////////////////////////////////////////////////////////
// UI events
//////////////////////////////////////////////////////////////////////////

// Connect/Disconnect
void MainDialog::OnButton_connectButtonClicked(wxCommandEvent& event)
{
	if (m_thermal.isOpen())
		m_thermal.close();
	else
	{
		if (m_thermal.connect())
			m_thermal.getStream();
		else
			wxMessageBox("Failed to connect to the USB device");
	}
}


// Start/Stop streaming
void MainDialog::OnButton_stopButtonClicked(wxCommandEvent& event)
{
	if (m_thermal.isStreaming())
		m_thermal.stopStreaming();
	else
		m_thermal.getStream();
}


// Take One
void MainDialog::OnButton_take_oneButtonClicked(wxCommandEvent& event)
{
	{
		std::lock_guard<std::recursive_mutex> lck(m_mx);
		
		m_get_one_after_cal = true;
	}
	
	m_thermal.getStream();
}


// Get Extra Cal
void MainDialog::OnButton_get_calButtonClicked(wxCommandEvent& event)
{
	std::lock_guard<std::recursive_mutex> lck(m_mx);
	
	m_get_extra_cal = true;
	
	// Set the checkbox, so we also get to use it
	m_check_use_extra_cal->SetValue(true);
	m_use_extra_cal = true;
}


// Use Extra Calibration
void MainDialog::OnCheck_use_extra_calCheckboxClicked(wxCommandEvent& event)
{
	std::lock_guard<std::recursive_mutex> lck(m_mx);
	
	m_use_extra_cal = m_check_use_extra_cal->IsChecked();
	
	UpdateFrame();
}


// Interpolation selector
void MainDialog::OnLb_interpolationChoiceSelected(wxCommandEvent& event)
{
	std::lock_guard<std::recursive_mutex> lck(m_mx);

	int sel = m_lb_interpolation->GetSelection();

	if (sel == wxNOT_FOUND)
		sel = -1;

	if (sel >= 0)
	{
		switch (sel)
		{
			case 1:
				m_quality = wxIMAGE_QUALITY_BILINEAR;
				break;

			case 2:
				m_quality = wxIMAGE_QUALITY_BICUBIC;
				break;

			default:
				m_quality = wxIMAGE_QUALITY_NORMAL;
				break;
		}

		m_picture->setQuality(m_quality);
	}
}


// Color Profile
void MainDialog::OnLb_profileChoiceSelected(wxCommandEvent& event)
{
	std::lock_guard<std::recursive_mutex> lck(m_mx);
	
	m_sel_profile = m_lb_profile->GetSelection();
	
	if (m_sel_profile == wxNOT_FOUND)
		m_sel_profile = -1;
		
	if (m_sel_profile >= 0)
		m_gradient->setImage(m_profiles[m_sel_profile]->getGradient());
	
	UpdateFrame();
}


// Save
void MainDialog::OnButton_saveButtonClicked(wxCommandEvent& event)
{
	wxFileDialog fd(this, "Save image", "", "", "PNG files (*.png)|*.png|JPEG files (*.jpg)|*.jpg|BMP files (*.bmp)|*.bmp", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	
	if (fd.ShowModal() == wxID_CANCEL)
		return;
	
	
	std::lock_guard<std::recursive_mutex> lck(m_mx);
	
	wxImage to_save;
	
	switch (m_lb_sizes->GetSelection())
	{
		case 0:
			to_save = m_new_img;
			break;
		
		case 1:
			to_save = m_new_img.Scale(412, 312, m_quality);
			break;
		
		default:
		case 2:
			to_save = m_new_img.Scale(824, 624, m_quality);
			break;
	}
	
	if ( !to_save.SaveFile(fd.GetPath()) )
		wxMessageBox("Failed to save file");
}


// Autorange checkbox
void MainDialog::OnCheck_auto_rangeCheckboxClicked(wxCommandEvent& event)
{
	std::lock_guard<std::recursive_mutex> lck(m_mx);

	m_auto_range = m_check_auto_range->IsChecked();

	if (!m_auto_range)
	{
		m_manual_min = m_slider_low->GetValue();
		m_manual_max = m_slider_high->GetValue();

		m_slider_low->Enable();
		m_slider_high->Enable();
	}
	else
	{
		m_slider_low->Disable();
		m_slider_high->Disable();
	}
}


// Low limit
void MainDialog::OnSlider_lowScrollChanged(wxScrollEvent& event)
{
	std::lock_guard<std::recursive_mutex> lck(m_mx);

	if (!m_auto_range)
	{
		m_manual_min = m_slider_low->GetValue();

		if (m_manual_min > m_manual_max)
		{
			m_manual_min = m_manual_max;
			m_slider_low->SetValue(m_manual_min);
		}

		UpdateFrame();
	}
}


// High limit
void MainDialog::OnSlider_highScrollChanged(wxScrollEvent& event)
{
	std::lock_guard<std::recursive_mutex> lck(m_mx);

	if (!m_auto_range)
	{
		m_manual_max = m_slider_high->GetValue();

		if (m_manual_max < m_manual_min)
		{
			m_manual_max = m_manual_min;
			m_slider_high->SetValue(m_manual_max);
		}

		UpdateFrame();
	}
}
