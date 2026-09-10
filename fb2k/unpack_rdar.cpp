#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdint.h>
#include <optional>
#include <string>
#include <vector>

#include <foobar2000/SDK/foobar2000.h>
#include <foobar2000/helpers/readers_lite.h>
#include <kraken/src/kraken.h>

#include "foo_unpack_vgm.h"

#define FNV1A64_BASIS 0xcbf29ce484222325
#define FNV1A64_PRIME 0x00000100000001b3

static constexpr uint64_t fnv1a64(const std::string_view& str) {
	uint64_t out = FNV1A64_BASIS;

	for (size_t i = 0; i < str.size(); i++)
		out = (out ^ str[i]) * FNV1A64_PRIME;

	return out;
}

class unpack_rdar : public archive_impl {
public:
    unpack_rdar() {};
    ~unpack_rdar() = default;

    void archive_list(const char* path, const service_ptr_t<file>& p_reader, archive_callback& p_out, bool p_want_readers) override;
    bool is_our_archive(const char* path) override;

    static const GUID g_get_guid() { return plugin_guid; };
    static const char* g_get_name() { return "unpack_rdar"; }

protected:
    const char* get_archive_type() override { return m_type; }
    t_filestats2 get_stats2_in_archive(const char* p_archive, const char* p_file, unsigned s2flags, abort_callback& p_abort) override;
    void open_archive(service_ptr_t<file>& p_out, const char* archive, const char* file, abort_callback& p_abort) override;

private:
    static constexpr const char* m_type = "rdar";
    pfc::string8 m_filename;
    std::vector<file_view_t> m_files;
    std::optional<HashTable<uint64_t>> hashes;

    void read_toc(const char* path, const service_ptr_t<file>& p_reader, abort_callback& p_abort);

    const file_view_t* get_file(const char* name) {
        for (uint32_t i = 0; i < m_files.size(); i++) {
            if (m_files[i].name == name) return &m_files[i];
        }
        return nullptr;
    }
};

bool unpack_rdar::is_our_archive(const char* path) {
    if (pfc::string_extension(path) != "archive") return false;
    constexpr const uint32_t RDAR = MAKEFOURCC('R', 'D', 'A', 'R');
    file_ptr reader;
    filesystem::g_open_read(reader, path, fb2k::noAbort);
    const uint32_t version = reader->read_lendian_t<uint32_t>(fb2k::noAbort);
    return version == RDAR;
}

void unpack_rdar::read_toc(const char* path, const service_ptr_t<file>& p_reader, abort_callback& p_abort) {
    if (m_filename == path || !path) return;
    m_filename = "";
    m_files.clear();

    service_ptr_t<file> reader = p_reader;
    if (reader.is_empty()) {
        filesystem::g_open_read(reader, path, p_abort);
    }

    constexpr const uint32_t RDAR = MAKEFOURCC('R', 'D', 'A', 'R');
    const uint32_t version = reader->read_lendian_t<uint32_t>(p_abort);
    reader->skip_object(4, p_abort);
    const uint64_t index_position = reader->read_lendian_t<uint64_t>(p_abort);

    if (version != RDAR || index_position == 0) {
        throw exception_io_data();
    }

    if (!hashes) {
        const auto path = pfc::io::path::combine(pfc::io::path::getParent(core_api::get_my_full_path()), "2077names.kark");
        if (!filesystem::g_exists(path, fb2k::noAbort))
            return;

        file_ptr reader{};
        filesystem::g_open_read(reader, path, fb2k::noAbort);
        const auto kark_size = reader->get_size(fb2k::noAbort);
        std::vector<uint8_t> kark(kark_size);
        reader->read(kark.data(), kark_size, fb2k::noAbort);

        uint32_t names_size = pfc::decode_little_endian<uint32_t>(kark.data() + 4);
        std::string names;
        names.resize(names_size);

        Kraken_Decompress(kark.data() + 8, kark_size - 8, (byte*)names.data(), names_size);
        std::vector<uint8_t>().swap(kark); // free()

        hashes.emplace(names, fnv1a64);
    }

    // Index
    reader->seek(index_position + 16, p_abort); // file_table_offset, file_table_size, crc
    uint32_t file_entry_count = reader->read_lendian_t<uint32_t>(p_abort);
    uint32_t file_segment_count = reader->read_lendian_t<uint32_t>(p_abort);
    reader->skip_object(4, p_abort); // resource_dependency_count

    m_files.reserve(file_entry_count);

    // FileEntry
    for (; file_entry_count > 0; file_entry_count--) {
        uint64_t name_hash_64 = reader->read_lendian_t<uint64_t>(p_abort);
        std::string name{};
        bool hash = false;
        const auto decoded_name = hashes->table.find(name_hash_64);
        if (decoded_name != hashes->table.end()) {
            if (strcmp(pfc::extract_ext_v2(decoded_name->second.data()), "wem")) {
                reader->skip_object(48, p_abort);
                continue;
            }
            name = decoded_name->second;
        }
        else {
            name.resize(18);
            sprintf_s(name.data(), 19, "0x%016llx", name_hash_64);
            hash = true;
        }
        reader->skip_object(8, p_abort); // timestamp
        uint32_t num_inline_buffer_segments = reader->read_lendian_t<uint32_t>(p_abort);
        if (num_inline_buffer_segments != 0) {
            FB2K_console_formatter() << "RDAR error: num_inline_buffer_segments " << num_inline_buffer_segments << " " << name.c_str();
            reader->skip_object(36, p_abort); // resource_dependencies_start, resource_dependencies_end, sha1_hash
            continue;
        }
        uint32_t segments_start = reader->read_lendian_t<uint32_t>(p_abort);
        uint32_t segments_end = reader->read_lendian_t<uint32_t>(p_abort);
        if (segments_end != segments_start + 1) {
            FB2K_console_formatter() << "RDAR info: multi-segment file " << name.c_str();
        }
        reader->skip_object(28, p_abort); // resource_dependencies_start, resource_dependencies_end, sha1_hash

        m_files.emplace_back(file_view_t{ name, segments_start, 0, hash });
    }
    //hashes.reset();
    m_files.shrink_to_fit();

    typedef struct {
        uint64_t offset;
        uint32_t z_size;
        uint32_t size;
    } file_segment_t;
    std::vector<file_segment_t> segments(file_segment_count);

    // FileSegment
    for (uint32_t i = 0; i < file_segment_count; i++) {
        segments[i].offset = reader->read_lendian_t<uint64_t>(p_abort);
        segments[i].z_size = reader->read_lendian_t<uint32_t>(p_abort);
        segments[i].size = reader->read_lendian_t<uint32_t>(p_abort);
    }

    constexpr const uint32_t RIFF = MAKEFOURCC('R', 'I', 'F', 'F');

    for (auto it = m_files.begin(); it != m_files.end();) {
        const auto& segment = segments[it->offset];
        if (segment.z_size != segment.size) {
            FB2K_console_formatter() << "RDAR error: compressed segment " << it->name.c_str();
            it = m_files.erase(it);
            continue;
        }
        it->offset = segment.offset;
        it->size = segment.size;
        if (it->size > 4 && it->hash) { // strncmp(it->name.c_str(), "0x", 2) == 0) {
            reader->seek(it->offset, p_abort);
            if (reader->read_lendian_t<uint32_t>(p_abort) == RIFF) {
                it->name += ".wem";
            }
        }
        it++;
    }

    m_filename = path;
}

void unpack_rdar::archive_list(const char* path, const service_ptr_t<file>& p_reader, archive_callback& p_out, bool p_want_readers) {
    service_ptr_t<file> reader = p_reader;
    if (reader.is_empty()) {
        filesystem::g_open_read(reader, path, p_out);
    }
    read_toc(path, reader, p_out);

    for (const auto& entry: m_files) {
        pfc::string8 url;
        make_unpack_path(url, path, entry.name.c_str());
        t_filestats stats = {};
        stats.m_size = entry.size;
        service_ptr_t<file> file_reader;
        if (p_want_readers) {
            file_reader = createFileLimited(reader, entry.offset, entry.size, p_out);
        }
        if (!p_out.on_entry(this, url, stats, file_reader)) {
            break;
        }
    }
}

void unpack_rdar::open_archive(service_ptr_t<file>& p_out, const char* archive, const char* p_file, abort_callback& p_abort) {
    if (!archive || !p_file) // || strcmp(pfc::extract_ext_v2(p_file), "wem"))
        throw exception_io_data();

    service_ptr_t<file> reader;
    filesystem::g_open_read(reader, archive, p_abort);
    read_toc(archive, reader, p_abort);

    const auto entry = get_file(p_file);
    if (!entry)
        throw exception_io_data();

    p_out = createFileLimited(reader, entry->offset, entry->size, p_abort);
}

t_filestats2 unpack_rdar::get_stats2_in_archive(const char* p_archive, const char* p_file, unsigned s2flags, abort_callback& p_abort) {
    t_filestats2 stats = {};
    read_toc(p_archive, NULL, p_abort);
    if (m_filename == p_archive) {
        const auto entry = get_file(p_file);
        if (entry) stats.m_size = entry->size;
    }
    return stats;
}

// foobar plugin defs
static archive_factory_t<unpack_rdar> g_unpack_rdar_factory;
