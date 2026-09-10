#ifndef _FOO_UNPACK_VGM_
#define _FOO_UNPACK_VGM_

inline constexpr GUID plugin_guid = { 0x8e05611c, 0x7d9e, 0x4bda, { 0x9e, 0x19, 0x6f, 0x6e, 0xc9, 0x31, 0x87, 0xc1 } };
extern cfg_string cfg_rpf3key;
extern cfg_string cfg_rpf7key;

bool read_keystring(const char* str, uint8_t* key);

PFC_DECLARE_EXCEPTION(exception_vgm_key, exception_io, "Decryption key could not be loaded");

typedef struct {
    std::string name;
    size_t offset;
    size_t size;
    bool hash;
} file_view_t;

template<typename hash_t, typename hash_func_t = hash_t(*)(const std::string_view&)>
class HashTable {
private:
    std::string m_data;

    // skip double hashing by unordered_map (fnv1a64)
    struct Identity {
        size_t operator()(const hash_t k) const noexcept { return k; }
    };

    void build_table(const hash_func_t hash_func) {
        size_t start = 0;
        size_t pos = 0;
        std::string_view token;
        while (pos < m_data.size()) {
            if (m_data[pos] == '\n' || m_data[pos] == '\r' || m_data[pos] == '\0') {
                if (pos > start) {
                    m_data[pos] = '\0';
                    token = std::string_view(m_data.data() + start, pos - start);
                    table.emplace(hash_func(token), token);
                }
                start = pos + 1;
            }
            pos++;
        }
    }

public:
    std::unordered_map<hash_t, std::string_view, Identity> table = {};

    HashTable(std::string& data, const hash_func_t hash_func) {
        m_data = std::move(data);
        build_table(hash_func);
    }
    HashTable(const char* resource, const hash_func_t hash_func) {
        const auto path = pfc::io::path::combine(pfc::io::path::getParent(core_api::get_my_full_path()), resource);
        if (!filesystem::g_exists(path, fb2k::noAbort))
            return;

        file_ptr reader{};
        filesystem::g_open_read(reader, path, fb2k::noAbort);
        const auto size = reader->get_size(fb2k::noAbort);
        m_data.resize(size);
        reader->read(m_data.data(), size, fb2k::noAbort);

        build_table(hash_func);
    }
    ~HashTable() = default;
};

#endif
