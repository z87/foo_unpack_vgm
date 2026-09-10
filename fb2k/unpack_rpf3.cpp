#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdint.h>
#include <optional>
#include <string>
#include <vector>

#include <foobar2000/SDK/foobar2000.h>
#include <foobar2000/helpers/readers_lite.h>
#include <rpf-archive/rpf_archive.h>

#include "foo_unpack_vgm.h"

static uint32_t joaat(const std::string_view& str) {
    uint32_t value = 0, temp;
    uint32_t index = 0;

    for (; index < str.length(); index++) {
        unsigned char v = std::tolower(str[index]);
        temp = v;
        temp = temp + value;
        value = temp << 10;
        temp += value;
        value = temp >> 6;
        value = value ^ temp;
    }

    temp = value << 3;
    temp = value + temp;
    uint32_t temp2 = temp >> 11;
    temp = temp2 ^ temp;
    temp2 = temp << 15;

    value = temp2 + temp;

    if (value < 2) value += 2;

    return value;
};

class unpack_rpf3 : public archive_impl {
public:
    unpack_rpf3() {};
    ~unpack_rpf3() = default;

    void archive_list(const char* path, const service_ptr_t<file>& p_reader, archive_callback& p_out, bool p_want_readers) override;
    bool is_our_archive(const char* path) override;

    static const GUID g_get_guid() { return plugin_guid; };
    static const char* g_get_name() { return "unpack_rpf3"; }

protected:
    const char* get_archive_type() override { return m_type; }
    t_filestats2 get_stats2_in_archive(const char* p_archive, const char* p_file, unsigned s2flags, abort_callback& p_abort) override;
    void open_archive(service_ptr_t<file>& p_out, const char* archive, const char* file, abort_callback& p_abort) override;

private:
    static constexpr const char* m_type = "rpf3";
    static inline unsigned char AES_KEY[32]{};
    pfc::string8 m_filename;
    std::vector<file_view_t> m_files;
    std::optional<HashTable<uint32_t>> hashes;

    void read_toc(const char* path, const service_ptr_t<file>& p_reader, abort_callback& p_abort);

    const file_view_t* get_file(const char* name) {
        for (uint32_t i = 0; i < m_files.size(); i++) {
            if (m_files[i].name == name) return &m_files[i];
        }
        return nullptr;
    }
};

bool unpack_rpf3::is_our_archive(const char* path) {
    if (pfc::string_extension(path) != "rpf") return false;
    constexpr const uint32_t RPF3 = MAKEFOURCC('R', 'P', 'F', '3');
    file_ptr reader;
    filesystem::g_open_read(reader, path, fb2k::noAbort);
    const uint32_t version = reader->read_lendian_t<uint32_t>(fb2k::noAbort);
    return version == RPF3;
}

void unpack_rpf3::read_toc(const char* path, const service_ptr_t<file>& p_reader, abort_callback& p_abort) {
    if (m_filename == path || !path) return;
    m_filename = "";
    m_files.clear();

    service_ptr_t<file> reader = p_reader;
    if (reader.is_empty()) {
        filesystem::g_open_read(reader, path, p_abort);
    }

    constexpr const uint32_t RPF3 = MAKEFOURCC('R', 'P', 'F', '3');
    const uint32_t version = reader->read_lendian_t<uint32_t>(p_abort);
    const uint32_t toc_size = reader->read_lendian_t<uint32_t>(p_abort);
    const uint32_t entry_count = reader->read_lendian_t<uint32_t>(p_abort);
    reader->skip_object(4, p_abort);
    const uint32_t encrypted = reader->read_lendian_t<uint32_t>(p_abort);

    if (version != RPF3 || entry_count == 0 || toc_size < 16 || entry_count > toc_size / 16) {
        throw exception_io_data();
    }

    reader->seek(0, p_abort);
    std::vector<uint8_t> toc((size_t)0x800 + toc_size);
    reader->read_object(toc.data(), toc.size(), p_abort);

    if (encrypted && !AES_KEY[0] && !read_keystring(cfg_rpf3key, AES_KEY))
        throw exception_vgm_key();

    rpf_gta_keys* keys{};
    if (rpf_keys_load(AES_KEY, false, &keys) != RPF_STATUS_OK) {
        FB2K_console_formatter() << rpf_get_last_error();
        throw exception_vgm_key();
    }

    rpf_archive_handle* arc = {};
    auto rc = rpf_archive_open(toc.data(), toc.size(), pfc::filename_ext_v2(path), keys, &arc);
    rpf_keys_close(keys);
    if (rc != RPF_STATUS_OK) {
        FB2K_console_formatter() << rpf_get_last_error();
        return;
    }

    if (!hashes) hashes.emplace("RPF3names.txt", joaat);

    rpf_archive_entry entry = {};
    const auto cnt = rpf_archive_entry_count(arc);
    for (int i = 0; i < cnt; i++) {
        rc = rpf_archive_entry_get(arc, i, &entry);
        if (rc != 0) {
            FB2K_console_formatter() << "RPF3 error: could not get archive entry #" << i;
            continue;
        }
        if (entry.kind != RPF_ENTRY_KIND_BINARY_FILE) {
            continue;
        }
        std::string name;
        const auto decoded_name = hashes->table.find(pfc::atohex<uint32_t>(entry.name, strlen(entry.name)));
        if (decoded_name != hashes->table.end()) {
            name = decoded_name->second;
        }
        else {
            name = entry.name;
        }
        name += ".ivaud";
        m_files.emplace_back(file_view_t{ name, entry.offset, entry.uncompressed_size });
    }

    rpf_archive_close(arc);

    m_filename = path;
}

void unpack_rpf3::archive_list(const char* path, const service_ptr_t<file>& p_reader, archive_callback& p_out, bool p_want_readers) {
    service_ptr_t<file> reader = p_reader;
    if (reader.is_empty()) {
        filesystem::g_open_read(reader, path, p_out);
    }
    read_toc(path, reader, p_out);

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

void unpack_rpf3::open_archive(service_ptr_t<file>& p_out, const char* archive, const char* p_file, abort_callback& p_abort) {
    if (!archive || !p_file || strcmp(pfc::extract_ext_v2(p_file), "ivaud"))
        throw exception_io_data();

    service_ptr_t<file> reader;
    filesystem::g_open_read(reader, archive, p_abort);
    read_toc(archive, reader, p_abort);

    const auto entry = get_file(p_file);
    if (!entry)
        throw exception_io_data();

    p_out = createFileLimited(reader, entry->offset, entry->size, p_abort);
}

t_filestats2 unpack_rpf3::get_stats2_in_archive(const char* p_archive, const char* p_file, unsigned s2flags, abort_callback& p_abort) {
    t_filestats2 stats = {};
    read_toc(p_archive, NULL, p_abort);
    if (m_filename == p_archive) {
        const auto entry = get_file(p_file);
        if (entry) stats.m_size = entry->size;
    }
    return stats;
}

// foobar plugin defs
static archive_factory_t<unpack_rpf3> g_unpack_rpf3_factory;
