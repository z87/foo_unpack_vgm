#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <foobar2000/helpers/foobar2000+atl.h>
#include <foobar2000/helpers/atl-misc.h>
#include <foobar2000/SDK/coreDarkMode.h>
#include <rpf-archive/rpf_archive.h>

#include "foo_unpack_vgm.h"
#include "resource.h"

class unpack_vgmPreferences : public CDialogImpl<unpack_vgmPreferences>, public preferences_page_instance {
public:
	unpack_vgmPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//dialog resource ID
	enum { IDD = IDD_CONFIG };
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(UsfPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_RPF3KEY, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_RPF7KEY, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_LOADKEY, BN_CLICKED, OnBtnClick)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	void OnBtnClick(UINT, int, CWindow);
	bool HasChanged();

	const preferences_page_callback::ptr m_callback;

	fb2k::CCoreDarkModeHooks m_darkhooks;
};

class unpack_vgm_prefs : public preferences_page_impl<unpack_vgmPreferences> {

public:
	const char* get_name();
	GUID get_guid();
	GUID get_parent_guid() {
		return guid_components;
	};
};

const char* unpack_vgm_prefs::get_name() {
	return "foo_unpack_vgm";
}

GUID unpack_vgm_prefs::get_guid() {
	static const GUID guid = { 0x5606402, 0x695e, 0x4334, { 0x9e, 0x2, 0x18, 0xe8, 0x47, 0xad, 0xc8, 0xa8 } };
	return guid;
}

BOOL unpack_vgmPreferences::OnInitDialog(CWindow, LPARAM) {
	uSetDlgItemText(m_hWnd, IDC_RPF3KEY, cfg_rpf3key);
	uSetDlgItemText(m_hWnd, IDC_RPF7KEY, cfg_rpf7key);

	m_darkhooks.AddDialogWithControls(m_hWnd);

	return TRUE;
}

t_uint32 unpack_vgmPreferences::get_state() {
	t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
	if (HasChanged())
		state |= preferences_state::changed;
	return state;
}

void unpack_vgmPreferences::reset() {
	uSetDlgItemText(m_hWnd, IDC_RPF3KEY, "");
	uSetDlgItemText(m_hWnd, IDC_RPF7KEY, "");
}

void unpack_vgmPreferences::apply() {
	pfc::string buf;
	buf = uGetDlgItemText(m_hWnd, IDC_RPF3KEY);
	if (buf != "" && strlen(buf) != 64) {
		uMessageBox(m_hWnd,
				"Invalid value for RPF3 AES key\n"
				"Must be a 64 character hex string",
				"Error",MB_OK|MB_ICONERROR);
		return;
	} else cfg_rpf3key = buf.get_ptr();

	buf = uGetDlgItemText(m_hWnd, IDC_RPF7KEY);
	if (buf != "" && strlen(buf) != 64) {
		uMessageBox(m_hWnd,
			"Invalid value for RPF7 AES key\n"
			"Must be a 64 character hex string",
			"Error", MB_OK | MB_ICONERROR);
		return;
	} else cfg_rpf7key = buf;
}

bool unpack_vgmPreferences::HasChanged() {
	pfc::string rpf3key(cfg_rpf3key);
	pfc::string rpf7key(cfg_rpf7key);

	if (rpf3key != uGetDlgItemText(m_hWnd, IDC_RPF3KEY)) return true;
	if (rpf7key != uGetDlgItemText(m_hWnd, IDC_RPF7KEY)) return true;

	return FALSE;
}

void unpack_vgmPreferences::OnEditChange(UINT, int, CWindow) {
	m_callback->on_state_changed();
}

void unpack_vgmPreferences::OnBtnClick(UINT, int, CWindow) {
	pfc::string exe_path;
	const char* mask = "GTA*.exe|GTA*.exe";
	rpf_gta_keys* keys;
	uint8_t aes_key[32];
	char hex[65] = {};

	uGetOpenFileName(m_hWnd, mask, 0, NULL, "Select a GTA IV or V exe", NULL, exe_path, false);

	if (exe_path.empty()) return;
	if (rpf_keys_extract_from_exe(exe_path, &keys) != RPF_STATUS_OK)
		goto fail;
	if (rpf_keys_get_aes_key(keys, aes_key) != RPF_STATUS_OK)
		goto fail;
	rpf_keys_close(keys);
	pfc::print_hex_raw(aes_key, 32, hex);

	if (pfc::string_filename(exe_path).contains("5"))
		uSetDlgItemText(m_hWnd, IDC_RPF7KEY, hex);
	else
		uSetDlgItemText(m_hWnd, IDC_RPF3KEY, hex);
	return;

fail:
	FB2K_console_formatter() << rpf_get_last_error();
	rpf_keys_close(keys);
}

static preferences_page_factory_t<unpack_vgm_prefs> g_unpack_vgm_preferences_page_factory;
