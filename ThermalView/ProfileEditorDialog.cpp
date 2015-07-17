#include "ProfileEditorDialog.h"
#include <string>
#include <regex>
#include <boost/lexical_cast.hpp>
#include <boost/scope_exit.hpp>

using namespace std;


#define ERROR_COLOR wxColor(0xff, 0xf0, 0xf0)

#define NO_ON_CHANGE() \
	if (m_no_on_change) \
		return; \
	\
	m_no_on_change = true; \
	\
	BOOST_SCOPE_EXIT(&m_no_on_change) \
	{ \
		m_no_on_change = false; \
	} \
	BOOST_SCOPE_EXIT_END


//////////////////////////////////////////////////////////////////////////
// Utility functions
//////////////////////////////////////////////////////////////////////////

static std::string float_to_str(float val)
{
	int fract = static_cast<int>((val - static_cast<int>(val)) * 100);

	std::string int_str = to_string(static_cast<int>(val));
	std::string fract_str = to_string(fract);

	if (fract_str.size() == 1)
		fract_str.insert(fract_str.begin(), '0');

	return int_str + "." + fract_str;
}

static unsigned char from_hex(char c)
{
	if (c <= '9')
		return c - '0';
	else
		return 10 + (c - 'A');
}


//////////////////////////////////////////////////////////////////////////
// The Dialog implementation
//////////////////////////////////////////////////////////////////////////

ProfileEditorDialog::ProfileEditorDialog(wxWindow* parent)
    : ProfileEditorDialogBase(parent)
{
	// Due to a bug in wxCrafter, the following code is not generated automatically in ProfileEditorDialogBase
	m_text_rank->SetMaxLength(4);
	m_text_color->SetMaxLength(6);

	// Because wxCrafter doesn't see this event, we have to hook it up manually
	Connect(wxEVT_SHOW, wxShowEventHandler(ProfileEditorDialog::OnShow), 0, this);

	m_org_txt_bg = m_text_color->GetBackgroundColour();

	m_no_on_change = false;
}

ProfileEditorDialog::~ProfileEditorDialog()
{
}


//////////////////////////////////////////////////////////////////////////
// Public methods
//////////////////////////////////////////////////////////////////////////

void ProfileEditorDialog::SetPattern(const GradientProfile::Pattern & pattern)
{
	m_pattern = pattern;

	m_lb_pattern->Clear();
	ElementClear();

	// Populate the listbox
	for (const auto & itr : m_pattern)
		m_lb_pattern->Append(float_to_str(itr.first) + " - " + itr.second.getHex());
}

void ProfileEditorDialog::SetGranularity(uint16_t granularity)
{
	m_granularity = granularity;

	m_text_granularity->SetValue(to_string(granularity));

	SetGranularityError(false);
}

const GradientProfile::Pattern & ProfileEditorDialog::GetPattern() const
{
	return m_pattern;
}

uint16_t ProfileEditorDialog::GetGranularity() const
{
	return m_granularity;
}

bool ProfileEditorDialog::IsPreview() const
{
	return IsVisible() && m_check_preview->IsChecked();
}

//////////////////////////////////////////////////////////////////////////
// Private methods
//////////////////////////////////////////////////////////////////////////

// Clear the part of the view that displays the element
void ProfileEditorDialog::ElementClear()
{
	m_text_rank->SetValue("");
	m_text_color->SetValue("");

	SetRankError(false);
	SetColorError(false);
}

// Load the currently selected element and update the view
void ProfileEditorDialog::ElementUpdate()
{
	int sel = m_lb_pattern->GetSelection();

	if (sel == wxNOT_FOUND)
	{
		ElementClear();
		return;
	}

	auto color = m_pattern[sel].second;

	m_text_rank->SetValue(float_to_str(m_pattern[sel].first));
	m_text_color->SetValue(color.getHex());

	SetRankError(false);
	SetColorError(false);

	// Set the color
	m_colour_picker->SetColour(wxColor(color.r, color.g, color.b));
}


void ProfileEditorDialog::ElementSetColor(GradientProfile::gpRGB rgb, bool update_textbox, bool update_picker)
{
	int sel = m_lb_pattern->GetSelection();

	if (sel == wxNOT_FOUND)
		return;

	// Update the actual element
	m_pattern[sel].second = rgb;

	// Textbox?
	if (update_textbox)
		m_text_color->SetValue(rgb.getHex());

	// Color picker?
	if (update_picker)
		m_colour_picker->SetColour(wxColor(rgb.r, rgb.g, rgb.b));

	// Listbox
	m_lb_pattern->SetString(sel, float_to_str(m_pattern[sel].first) + " - " + m_pattern[sel].second.getHex());


	// Clear error
	SetColorError(false);


	// Fire update event if we're in preview mode
	if (m_check_preview->IsChecked())
		onUpdated();
}

void ProfileEditorDialog::ElementSetRank(float rank)
{
	typedef GradientProfile::Pattern::value_type PairType;

	// Get the selected item
	int sel = m_lb_pattern->GetSelection();

	if (sel == wxNOT_FOUND)
		return;

	// Set the value & save the color for use after the sort
	m_pattern[sel].first = rank;
	auto rgb = m_pattern[sel].second;

	// Clear the error
	SetRankError(false);

	// Sort values
	std::sort(m_pattern.begin(), m_pattern.end(), [](const PairType & a, const PairType & b) { return a.first < b.first; });

	// Update listbox
	m_lb_pattern->Clear();

	// Set the listbox
	for (const auto & itr : m_pattern)
	{
		int pos = m_lb_pattern->Append(float_to_str(itr.first) + " - " + itr.second.getHex());

		if (itr.first == rank && itr.second == rgb)
			m_lb_pattern->SetSelection(pos);
	}


	// Fire update event if we're in preview mode
	if (m_check_preview->IsChecked())
		onUpdated();
}

void ProfileEditorDialog::SetRankError(bool b)
{
	m_text_rank->SetBackgroundColour(b ? ERROR_COLOR : m_org_txt_bg);
	m_text_rank->Refresh();
}

void ProfileEditorDialog::SetColorError(bool b)
{
	m_text_color->SetBackgroundColour(b ? ERROR_COLOR : m_org_txt_bg);
	m_text_color->Refresh();
}

void ProfileEditorDialog::SetGranularityError(bool b)
{
	m_text_granularity->SetBackgroundColour(b ? ERROR_COLOR : m_org_txt_bg);
	m_text_granularity->Refresh();
}


//////////////////////////////////////////////////////////////////////////
// UI Stuff
//////////////////////////////////////////////////////////////////////////

void ProfileEditorDialog::OnShow(wxShowEvent& event)
{
	onUpdated();
}

void ProfileEditorDialog::OnCheck_previewCheckboxClicked(wxCommandEvent& event)
{
	onUpdated();
}

void ProfileEditorDialog::OnLb_patternListboxSelected(wxCommandEvent& event)
{
	NO_ON_CHANGE();

	ElementUpdate();
}

void ProfileEditorDialog::OnColour_pickerColourpickerChanged(wxColourPickerEvent& event)
{
	NO_ON_CHANGE();

	wxColor c = m_colour_picker->GetColour();

	ElementSetColor(GradientProfile::gpRGB(c.Red(), c.Green(), c.Blue()), true, false);
}

void ProfileEditorDialog::OnText_colorTextUpdated(wxCommandEvent& event)
{
	NO_ON_CHANGE();

	string str = m_text_color->GetValue();

	// If it's a proper color
	if (regex_match(str, regex("[0-9A-Fa-f]{3}|[0-9A-Fa-f]{6}")))
	{
		transform(str.begin(), str.end(), str.begin(), toupper);

		GradientProfile::gpRGB rgb;

		if (str.size() == 3)
		{
			rgb.r = from_hex(str[0]);
			rgb.r = rgb.r * 16 + rgb.r;

			rgb.g = from_hex(str[1]);
			rgb.g = rgb.g * 16 + rgb.g;

			rgb.b = from_hex(str[2]);
			rgb.b = rgb.b * 16 + rgb.b;
		}
		else
		{
			rgb.r = from_hex(str[0]) * 16 + from_hex(str[1]);
			rgb.g = from_hex(str[2]) * 16 + from_hex(str[3]);
			rgb.b = from_hex(str[4]) * 16 + from_hex(str[5]);
		}

		// Set the color
		ElementSetColor(rgb, false, true);
	}
	else
		SetColorError(true);
}

void ProfileEditorDialog::OnText_rankTextUpdated(wxCommandEvent& event)
{
	NO_ON_CHANGE();

	string str = m_text_rank->GetValue();

	float val = 0;

	// Try to get a float value from that text box
	try
	{
		val = boost::lexical_cast<float>(str);
	}
	catch (boost::bad_lexical_cast &)
	{
		SetRankError(true);
		return;
	}


	// If it's not in the allowed ranges, mark it as an error
	if (val > 1 || val < 0)
	{
		SetRankError(true);
		return;
	}


	ElementSetRank(val);
}

void ProfileEditorDialog::OnText_granularityTextUpdated(wxCommandEvent& event)
{
	NO_ON_CHANGE();

	string str = m_text_granularity->GetValue();

	// Try to get a float value from that text box
	try
	{
		m_granularity = boost::lexical_cast<uint16_t>(str);
	}
	catch (boost::bad_lexical_cast &)
	{
		SetGranularityError(true);
		return;
	}

	SetGranularityError(false);


	// Fire update event if we're in preview mode
	if (m_check_preview->IsChecked())
		onUpdated();
}

void ProfileEditorDialog::OnButton_addButtonClicked(wxCommandEvent& event)
{
	NO_ON_CHANGE();

	typedef GradientProfile::Pattern::value_type PairType;

	// Get current selection
	int sel = m_lb_pattern->GetSelection();

	if (sel == wxNOT_FOUND)
	{
		PairType item;
		item = PairType(1, GradientProfile::gpRGB(0, 0, 0));

		m_pattern.insert(m_pattern.begin() + sel, item);

		int pos = m_lb_pattern->Append("1.00 - 000000");

		m_lb_pattern->SetSelection(pos);
	}
	else
	{
		PairType item;
		item = m_pattern[sel];

		m_pattern.insert(m_pattern.begin() + sel, item);

		m_lb_pattern->Insert(m_lb_pattern->GetString(sel), sel + 1);

		m_lb_pattern->SetSelection(sel + 1);
	}

	ElementUpdate();
}

void ProfileEditorDialog::OnButton_remButtonClicked(wxCommandEvent& event)
{
	NO_ON_CHANGE();

	// Get current selection
	int sel = m_lb_pattern->GetSelection();

	if (sel == wxNOT_FOUND)
		return;

	if (m_pattern.size() == 2)
	{
		wxMessageBox("You cannot have less than two data points.");
		return;
	}

	m_pattern.erase(m_pattern.begin() + sel);
	m_lb_pattern->Delete(sel);

	ElementUpdate();
}

void ProfileEditorDialog::OnButton_applyButtonClicked(wxCommandEvent& event)
{
	onApply();
}

void ProfileEditorDialog::OnButton_saveButtonClicked(wxCommandEvent& event)
{
	onSave();
}

void ProfileEditorDialog::OnButton_save_asButtonClicked(wxCommandEvent& event)
{
	onSaveAs();
}
