#include <foobar2000/SDK/foobar2000.h>

#include <cstring>
#include <exception>

namespace {
	class foo_contextmenu : public contextmenu_item_simple {
	public:
		unsigned get_num_items() override { return 1; }

		void get_item_name(unsigned, pfc::string_base& out) override {
			out = "Extract...";
		}

		GUID get_item_guid(unsigned) override {
			static const GUID guid = { 0x69e4a7d1, 0x9b2e, 0x4fd7, { 0xa4, 0x1c, 0x52, 0x7f, 0x0e, 0xc8, 0x33, 0x91 } };
			return guid;
		}

		bool get_item_description(unsigned, pfc::string_base& out) override {
			out = "Save the selected unpacked archive item to a file";
			return true;
		}

		bool context_get_display(unsigned, metadb_handle_list_cref data, pfc::string_base& out,
			unsigned& displayflags, const GUID&) override {
			displayflags = 0;
			if (data.get_count() != 1) return false;

			const char* path = data[0]->get_path();
			if (!path || !archive_impl::g_is_unpack_path(path)) return false;

			out = "Extract...";
			return true;
		}

		void context_command(unsigned, metadb_handle_list_cref data, const GUID&) override {
			if (data.get_count() != 1) return;

			const char* item_path = data[0]->get_path();
			if (!item_path || !archive_impl::g_is_unpack_path(item_path)) return;

			pfc::string8 archive_path, item_name;
			if (!archive_impl::g_parse_unpack_path(item_path, archive_path, item_name)) return;

			pfc::string8 suggested_name = pfc::io::path::getFileName(item_name);
			if (suggested_name.is_empty()) suggested_name = "extracted.bin";

			auto dialog = fb2k::fileDialog::get()->setupSave();
			dialog->setTitle("Extract archive item");
			dialog->setFileTypes("All files|*.*");
			dialog->setInitialValue(suggested_name);
			const pfc::string8 source_path(item_path);
			dialog->runSimple([source_path](fb2k::stringRef destination) {
				if (destination.is_empty() || destination->isEmpty()) return;

				try {
					file_ptr reader, writer;
					filesystem::g_open_read(reader, source_path, fb2k::noAbort);
					filesystem::g_open_write_new(writer, destination->c_str(), fb2k::noAbort);
					file::g_transfer_file(reader, writer, fb2k::noAbort);
				} catch (const std::exception& e) {
					FB2K_console_formatter() << "Extract failed: " << e.what();
				} catch (...) {
					FB2K_console_formatter() << "Extract failed due to an unknown error.";
				}
			});
		}
	};

	static contextmenu_item_factory_t<foo_contextmenu> g_foo_contextmenu_factory;
}
