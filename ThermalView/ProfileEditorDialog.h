#ifndef PROFILEEDITORDIALOG_H
#define PROFILEEDITORDIALOG_H

#include "wxcrafter.h"
#include "color_profile/gradient.h"

#include <boost/signals2.hpp>


class ProfileEditorDialog : public ProfileEditorDialogBase
{
private:
	wxColor m_org_txt_bg;

	GradientProfile::Pattern m_pattern;
	uint16_t m_granularity;

	bool m_no_on_change;		// Helper var that prevents us from handling on change events when updating data

public:
    ProfileEditorDialog(wxWindow* parent);
    virtual ~ProfileEditorDialog();

	void SetPattern(const GradientProfile::Pattern & pattern);
	void SetGranularity(uint16_t granularity);

	const GradientProfile::Pattern & GetPattern() const;
	uint16_t GetGranularity() const;

	bool IsPreview() const;

	boost::signals2::signal<void()> onUpdated;
	boost::signals2::signal<void()> onApply;
	boost::signals2::signal<void()> onSave;
	boost::signals2::signal<void()> onSaveAs;

private:
	// Methods related to the part of the view that is responsible for the selected element

	void ElementClear();		// Clear the element view
	void ElementUpdate();		// Load the currently selected element

	void ElementSetColor(GradientProfile::gpRGB rgb, bool update_textbox, bool update_picker);
	void ElementSetRank(float rank);

	void SetRankError(bool b);
	void SetColorError(bool b);
	void SetGranularityError(bool b);

protected:
    virtual void OnButton_saveButtonClicked(wxCommandEvent& event);
    virtual void OnButton_save_asButtonClicked(wxCommandEvent& event);
    virtual void OnButton_applyButtonClicked(wxCommandEvent& event);
    virtual void OnButton_addButtonClicked(wxCommandEvent& event);
    virtual void OnButton_remButtonClicked(wxCommandEvent& event);
    virtual void OnCheck_previewCheckboxClicked(wxCommandEvent& event);
	virtual void OnShow(wxShowEvent& event);
    virtual void OnText_granularityTextUpdated(wxCommandEvent& event);
    virtual void OnText_rankTextUpdated(wxCommandEvent& event);
    virtual void OnText_colorTextUpdated(wxCommandEvent& event);
    virtual void OnColour_pickerColourpickerChanged(wxColourPickerEvent& event);
    virtual void OnLb_patternListboxSelected(wxCommandEvent& event);
};
#endif // PROFILEEDITORDIALOG_H
