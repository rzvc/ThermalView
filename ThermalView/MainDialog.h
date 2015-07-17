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

#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include "wxcrafter.h"
#include "thermal.h"
#include "frame.h"
#include "color_profile/color_profile.h"
#include "ProfileEditorDialog.h"

#include <mutex>


wxDECLARE_EVENT(ON_MSG_FRAME_READY, wxCommandEvent);

class MainDialog : public MainDialogBaseClass
{
public:
	typedef std::unique_ptr<ColorProfile> PColorProfile;
	typedef std::unique_ptr<GradientProfile> PGradientProfile;

	SeekThermal					m_thermal;				// The camera interface
	ThermalFrame				m_frame;				// Current frame on display
	ThermalFrame				m_frame_extra;			// Current frame on display after extra calibration
	wxImage						m_new_img;				// The new image
	
	std::recursive_mutex		m_mx;
	
	std::vector<double>			m_gain_cal;				// Gain calibration data - Frame ID 4
	std::vector<uint16_t>		m_unknown_gain;			// Unknown gain pixels - Frame ID 4
	std::vector<int>			m_offset_cal;			// Offset calibration - Frame ID 1

	std::vector<int>			m_extra_cal;			// Extra offset calibration
	bool						m_get_extra_cal;		// Do we have to fetch a good frame for it?
	bool						m_use_extra_cal;
	bool						m_first_after_cal;		// Marks the first frame after the offset calibration
	bool						m_get_one_after_cal;	// Do we want to stop after we get the first one after the calibration frame?


	wxImageResizeQuality		m_quality;				// The image quality
	
	
	std::vector<PColorProfile>	m_profiles;				// The color profile container
	int							m_sel_profile;			// The selected profile

	ProfileEditorDialog			m_profile_editor;		// The profile editor dialog
	PColorProfile				m_preview_profile;		// The preview profile, generated from the profile editor data
	bool						m_use_preview_profile;	// Indicates that we should use the preview profile
	
	
	bool						m_got_image;			// Indicates that we receive at least one image

	bool						m_auto_range;
	int							m_manual_min;
	int							m_manual_max;
	

    MainDialog(wxWindow* parent);
    virtual ~MainDialog();

	// Seek Thermal events
	void OnConnectionStatusChange();
	void OnStreamingStatusChange();
	void OnNewFrame(const std::vector<uint16_t> & data);

	// Profile Editor events
	void OnProfileEditorUpdate();
	void OnProfileEditorApply();
	void OnProfileEditorSave();
	void OnProfileEditorSaveAs();
	
	// Main thread events
	void OnMsgConnectionStatusChange(wxCommandEvent &);
	void OnMsgStreamingStatusChange(wxCommandEvent &);
	void OnMsgFrameReady(wxCommandEvent &);
	
private:
	void UpdateFrame();
	
protected:
    virtual void OnButton_connectButtonClicked(wxCommandEvent& event);
    virtual void OnButton_stopButtonClicked(wxCommandEvent& event);
    virtual void OnButton_take_oneButtonClicked(wxCommandEvent& event);
    virtual void OnButton_get_calButtonClicked(wxCommandEvent& event);
    virtual void OnCheck_use_extra_calCheckboxClicked(wxCommandEvent& event);
	virtual void OnLb_interpolationChoiceSelected(wxCommandEvent& event);
	virtual void OnLb_profileChoiceSelected(wxCommandEvent& event);
	virtual void OnButton_edit_profileButtonClicked(wxCommandEvent& event);
	virtual void OnButton_saveButtonClicked(wxCommandEvent& event);
	virtual void OnCheck_auto_rangeCheckboxClicked(wxCommandEvent& event);
	virtual void OnSlider_lowScrollChanged(wxScrollEvent& event);
	virtual void OnSlider_highScrollChanged(wxScrollEvent& event);
};
#endif // MAINDIALOG_H
