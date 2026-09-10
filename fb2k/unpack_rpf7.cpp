#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdint.h>
#include <functional>
#include <string>
#include <vector>

#include <foobar2000/SDK/foobar2000.h>
#include <foobar2000/helpers/readers_lite.h>
#include <rpf-archive/rpf_archive.h>

#include "foo_unpack_vgm.h"

class unpack_rpf7 : public archive_impl {
public:
    unpack_rpf7() {}
    ~unpack_rpf7() = default;

    void archive_list(const char* path, const service_ptr_t<file>& p_reader, archive_callback& p_out, bool p_want_readers) override;
    bool is_our_archive(const char* path) override;

    static const GUID g_get_guid() { return plugin_guid; };
    static const char* g_get_name() { return "unpack_rpf7"; }

protected:
    const char* get_archive_type() override { return m_type; }
    t_filestats2 get_stats2_in_archive(const char* p_archive, const char* p_file, unsigned s2flags, abort_callback& p_abort) override;
    void open_archive(service_ptr_t<file>& p_out, const char* archive, const char* file, abort_callback& p_abort) override;

private:
    static constexpr const char* m_type = "rpf7";
    static inline unsigned char AES_KEY[32]{};
    static inline uint32_t AWC_KEY[4]{};
    static inline fb2k::memBlockRef awckey_file_mem{};
    pfc::string8 m_filename;
    std::vector<file_view_t> m_files;

    void read_toc(const char* path, abort_callback& p_abort);

    const file_view_t* get_file(const char* name) {
        for (uint32_t i = 0; i < m_files.size(); i++) {
            if (m_files[i].name == name) return &m_files[i];
        }
        return nullptr;
    }
};

bool unpack_rpf7::is_our_archive(const char* path) {
    if (pfc::string_extension(path) != "rpf") return false;
    constexpr const uint32_t RPF7 = MAKEFOURCC('R', 'P', 'F', '7');
    file_ptr reader;
    filesystem::g_open_read(reader, path, fb2k::noAbort);
    const uint32_t version = reader->read_bendian_t<uint32_t>(fb2k::noAbort);
    return version == RPF7;
}

void unpack_rpf7::read_toc(const char* path, abort_callback& p_abort) {
    if (m_filename == path || !path) return;
    m_filename = "";
    m_files.clear();

    if (!AES_KEY[0] && !read_keystring(cfg_rpf7key, AES_KEY))
        throw exception_vgm_key();

    rpf_gta_keys* keys{};
    if (rpf_keys_load(AES_KEY, true, &keys) != RPF_STATUS_OK) {
        FB2K_console_formatter() << rpf_get_last_error();
        throw exception_vgm_key();
    }

    if (!AWC_KEY[0]) {
        if (rpf_keys_get_awc_key(keys, AWC_KEY) != RPF_STATUS_OK) {
            FB2K_console_formatter() << rpf_get_last_error();
            throw exception_vgm_key();
        }
        for (int i = 0; i < 4; i++) byte_order::order_native_to_be_t<uint32_t>(AWC_KEY[i]); // vgmstream reads BE
        awckey_file_mem = fb2k::makeMemBlock(AWC_KEY, 16);
    }

    t_filestats f_stats{};
    bool f_write;
    filesystem::g_get_stats(path, f_stats, f_write, p_abort);

    pfc::string system_path{};
    filesystem::g_get_native_path(path, system_path);

    auto h_file = CreateFileA(system_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    auto h_map = CreateFileMappingA(h_file, NULL, PAGE_READONLY, 0, f_stats.m_size, NULL);
    auto file_view = MapViewOfFile(h_map, FILE_MAP_READ, 0, 0, 0);

    rpf_archive_handle* arc = {};
    auto rc = rpf_archive_open((uint8_t*)file_view, f_stats.m_size, pfc::filename_ext_v2(path), keys, &arc);
    rpf_keys_close(keys);
    if (rc != RPF_STATUS_OK) {
        FB2K_console_formatter() << rpf_get_last_error();
        UnmapViewOfFile(file_view);
        CloseHandle(h_map);
        CloseHandle(h_file);
        return;
    }

    rpf_archive_entry entry = {};
    const auto cnt = rpf_archive_entry_count(arc);
    for (int i = 0; i < cnt; i++) {
        rc = rpf_archive_entry_get(arc, i, &entry);
        if (rc != 0) {
            FB2K_console_formatter() << rpf_get_last_error();
            continue;
        }
        if (entry.kind != RPF_ENTRY_KIND_BINARY_FILE || strcmp(pfc::extract_ext_v2(entry.name), "awc")) {
            continue;
        }
        //FB2K_console_formatter() << entry.kind << " " << entry.name << " @ " << entry.offset << " [" << entry.size << "]";
        m_files.emplace_back(file_view_t{ entry.name, entry.offset, entry.uncompressed_size });
    }

    rpf_archive_close(arc);

    UnmapViewOfFile(file_view);
    CloseHandle(h_map);
    CloseHandle(h_file);

    m_filename = path;
}

void unpack_rpf7::archive_list(const char* path, const service_ptr_t<file>& p_reader, archive_callback& p_out, bool p_want_readers) {
    read_toc(path, p_out);
    if (m_files.empty())
        return;

    service_ptr_t<file> reader = p_reader;
    if (p_want_readers && reader.is_empty()) {
        filesystem::g_open_read(reader, path, p_out);
    }

    for (int32_t i = 0; i < m_files.size(); i++) {
        pfc::string8 url;
        make_unpack_path(url, path, m_files[i].name.c_str());
        t_filestats stats = {};
        stats.m_size = m_files[i].size;
        service_ptr_t<file> file_reader;
        if (p_want_readers) {
            file_reader = createFileLimited(reader, m_files[i].offset, m_files[i].size, p_out);
        }
        if (!p_out.on_entry(this, url, stats, file_reader)) {
            break;
        }
    }
}

void unpack_rpf7::open_archive(service_ptr_t<file>& p_out, const char* archive, const char* p_file, abort_callback& p_abort) {
    if (!archive || !p_file)
        throw exception_io_data();

    const auto ext = pfc::extract_ext_v2(p_file);
    if (!strcmp(ext, "awckey")) {
        p_out = createFileWithMemBlock(awckey_file_mem);
        return;
    }
    else if (strcmp(ext, "awc")) {
        throw exception_io_data();
    }

    read_toc(archive, p_abort);

    const auto entry = get_file(p_file);
    if (!entry)
        throw exception_io_data();

    service_ptr_t<file> reader;
    filesystem::g_open_read(reader, archive, p_abort);
    p_out = createFileLimited(reader, entry->offset, entry->size, p_abort);
}

t_filestats2 unpack_rpf7::get_stats2_in_archive(const char* p_archive, const char* p_file, unsigned s2flags, abort_callback& p_abort) {
    t_filestats2 stats = {};

    if (!strcmp(pfc::extract_ext_v2(p_file), "awc")) {
        read_toc(p_archive, p_abort);
        if (m_filename == p_archive) {
            const auto entry = get_file(p_file);
            if (entry) stats.m_size = entry->size;
        }
    }
    return stats;
}

// foobar plugin defs
static archive_factory_t<unpack_rpf7> g_unpack_rpf7_factory;
