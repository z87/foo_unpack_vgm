#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdint.h>
#include <stdio.h>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <foobar2000/SDK/foobar2000.h>
#include <foobar2000/helpers/readers.h>
#include <foobar2000/helpers/readers_lite.h>

#include "foo_unpack_vgm.h"
#include "resource.h"

//CRC64 ECMA (0x00600340 & 0x00F0D50B Polynomials)
// non-standard values from antique software
// Matt Dillon's Diablo nntp server (~1999)
// XPSP1/NT/admin/wmi/wbem/winmgmt/wbemcomn/crc64.h
static uint64_t crc64(const std::string_view& p_string) {
    static const uint32_t m_CrcTable[] = 
        {
            0x00000000, 0x00000000, 0x00600340, 0x00F0D50B, 0x00C00680, 0x01E1AA16, 0x00A005C0, 0x01117F1D,
            0x01800D00, 0x03C3542C, 0x01E00E40, 0x03338127, 0x01400B80, 0x0222FE3A, 0x012008C0, 0x02D22B31,
            0x03001A00, 0x0786A858, 0x03601940, 0x07767D53, 0x03C01C80, 0x0667024E, 0x03A01FC0, 0x0697D745,
            0x02801700, 0x0445FC74, 0x02E01440, 0x04B5297F, 0x02401180, 0x05A45662, 0x022012C0, 0x05548369,
            0x06003400, 0x0F0D50B0, 0x06603740, 0x0FFD85BB, 0x06C03280, 0x0EECFAA6, 0x06A031C0, 0x0E1C2FAD,
            0x07803900, 0x0CCE049C, 0x07E03A40, 0x0C3ED197, 0x07403F80, 0x0D2FAE8A, 0x07203CC0, 0x0DDF7B81,
            0x05002E00, 0x088BF8E8, 0x05602D40, 0x087B2DE3, 0x05C02880, 0x096A52FE, 0x05A02BC0, 0x099A87F5,
            0x04802300, 0x0B48ACC4, 0x04E02040, 0x0BB879CF, 0x04402580, 0x0AA906D2, 0x042026C0, 0x0A59D3D9,
            0x0C006800, 0x1E1AA160, 0x0C606B40, 0x1EEA746B, 0x0CC06E80, 0x1FFB0B76, 0x0CA06DC0, 0x1F0BDE7D,
            0x0D806500, 0x1DD9F54C, 0x0DE06640, 0x1D292047, 0x0D406380, 0x1C385F5A, 0x0D2060C0, 0x1CC88A51,
            0x0F007200, 0x199C0938, 0x0F607140, 0x196CDC33, 0x0FC07480, 0x187DA32E, 0x0FA077C0, 0x188D7625,
            0x0E807F00, 0x1A5F5D14, 0x0EE07C40, 0x1AAF881F, 0x0E407980, 0x1BBEF702, 0x0E207AC0, 0x1B4E2209,
            0x0A005C00, 0x1117F1D0, 0x0A605F40, 0x11E724DB, 0x0AC05A80, 0x10F65BC6, 0x0AA059C0, 0x10068ECD,
            0x0B805100, 0x12D4A5FC, 0x0BE05240, 0x122470F7, 0x0B405780, 0x13350FEA, 0x0B2054C0, 0x13C5DAE1,
            0x09004600, 0x16915988, 0x09604540, 0x16618C83, 0x09C04080, 0x1770F39E, 0x09A043C0, 0x17802695,
            0x08804B00, 0x15520DA4, 0x08E04840, 0x15A2D8AF, 0x08404D80, 0x14B3A7B2, 0x08204EC0, 0x144372B9,
            0x1800D000, 0x3C3542C0, 0x1860D340, 0x3CC597CB, 0x18C0D680, 0x3DD4E8D6, 0x18A0D5C0, 0x3D243DDD,
            0x1980DD00, 0x3FF616EC, 0x19E0DE40, 0x3F06C3E7, 0x1940DB80, 0x3E17BCFA, 0x1920D8C0, 0x3EE769F1,
            0x1B00CA00, 0x3BB3EA98, 0x1B60C940, 0x3B433F93, 0x1BC0CC80, 0x3A52408E, 0x1BA0CFC0, 0x3AA29585,
            0x1A80C700, 0x3870BEB4, 0x1AE0C440, 0x38806BBF, 0x1A40C180, 0x399114A2, 0x1A20C2C0, 0x3961C1A9,
            0x1E00E400, 0x33381270, 0x1E60E740, 0x33C8C77B, 0x1EC0E280, 0x32D9B866, 0x1EA0E1C0, 0x32296D6D,
            0x1F80E900, 0x30FB465C, 0x1FE0EA40, 0x300B9357, 0x1F40EF80, 0x311AEC4A, 0x1F20ECC0, 0x31EA3941,
            0x1D00FE00, 0x34BEBA28, 0x1D60FD40, 0x344E6F23, 0x1DC0F880, 0x355F103E, 0x1DA0FBC0, 0x35AFC535,
            0x1C80F300, 0x377DEE04, 0x1CE0F040, 0x378D3B0F, 0x1C40F580, 0x369C4412, 0x1C20F6C0, 0x366C9119,
            0x1400B800, 0x222FE3A0, 0x1460BB40, 0x22DF36AB, 0x14C0BE80, 0x23CE49B6, 0x14A0BDC0, 0x233E9CBD,
            0x1580B500, 0x21ECB78C, 0x15E0B640, 0x211C6287, 0x1540B380, 0x200D1D9A, 0x1520B0C0, 0x20FDC891,
            0x1700A200, 0x25A94BF8, 0x1760A140, 0x25599EF3, 0x17C0A480, 0x2448E1EE, 0x17A0A7C0, 0x24B834E5,
            0x1680AF00, 0x266A1FD4, 0x16E0AC40, 0x269ACADF, 0x1640A980, 0x278BB5C2, 0x1620AAC0, 0x277B60C9,
            0x12008C00, 0x2D22B310, 0x12608F40, 0x2DD2661B, 0x12C08A80, 0x2CC31906, 0x12A089C0, 0x2C33CC0D,
            0x13808100, 0x2EE1E73C, 0x13E08240, 0x2E113237, 0x13408780, 0x2F004D2A, 0x132084C0, 0x2FF09821,
            0x11009600, 0x2AA41B48, 0x11609540, 0x2A54CE43, 0x11C09080, 0x2B45B15E, 0x11A093C0, 0x2BB56455,
            0x10809B00, 0x29674F64, 0x10E09840, 0x29979A6F, 0x10409D80, 0x2886E572, 0x10209EC0, 0x28763079,
            0x3001A000, 0x786A8580, 0x3061A340, 0x789A508B, 0x30C1A680, 0x798B2F96, 0x30A1A5C0, 0x797BFA9D,
            0x3181AD00, 0x7BA9D1AC, 0x31E1AE40, 0x7B5904A7, 0x3141AB80, 0x7A487BBA, 0x3121A8C0, 0x7AB8AEB1,
            0x3301BA00, 0x7FEC2DD8, 0x3361B940, 0x7F1CF8D3, 0x33C1BC80, 0x7E0D87CE, 0x33A1BFC0, 0x7EFD52C5,
            0x3281B700, 0x7C2F79F4, 0x32E1B440, 0x7CDFACFF, 0x3241B180, 0x7DCED3E2, 0x3221B2C0, 0x7D3E06E9,
            0x36019400, 0x7767D530, 0x36619740, 0x7797003B, 0x36C19280, 0x76867F26, 0x36A191C0, 0x7676AA2D,
            0x37819900, 0x74A4811C, 0x37E19A40, 0x74545417, 0x37419F80, 0x75452B0A, 0x37219CC0, 0x75B5FE01,
            0x35018E00, 0x70E17D68, 0x35618D40, 0x7011A863, 0x35C18880, 0x7100D77E, 0x35A18BC0, 0x71F00275,
            0x34818300, 0x73222944, 0x34E18040, 0x73D2FC4F, 0x34418580, 0x72C38352, 0x342186C0, 0x72335659,
            0x3C01C800, 0x667024E0, 0x3C61CB40, 0x6680F1EB, 0x3CC1CE80, 0x67918EF6, 0x3CA1CDC0, 0x67615BFD,
            0x3D81C500, 0x65B370CC, 0x3DE1C640, 0x6543A5C7, 0x3D41C380, 0x6452DADA, 0x3D21C0C0, 0x64A20FD1,
            0x3F01D200, 0x61F68CB8, 0x3F61D140, 0x610659B3, 0x3FC1D480, 0x601726AE, 0x3FA1D7C0, 0x60E7F3A5,
            0x3E81DF00, 0x6235D894, 0x3EE1DC40, 0x62C50D9F, 0x3E41D980, 0x63D47282, 0x3E21DAC0, 0x6324A789,
            0x3A01FC00, 0x697D7450, 0x3A61FF40, 0x698DA15B, 0x3AC1FA80, 0x689CDE46, 0x3AA1F9C0, 0x686C0B4D,
            0x3B81F100, 0x6ABE207C, 0x3BE1F240, 0x6A4EF577, 0x3B41F780, 0x6B5F8A6A, 0x3B21F4C0, 0x6BAF5F61,
            0x3901E600, 0x6EFBDC08, 0x3961E540, 0x6E0B0903, 0x39C1E080, 0x6F1A761E, 0x39A1E3C0, 0x6FEAA315,
            0x3881EB00, 0x6D388824, 0x38E1E840, 0x6DC85D2F, 0x3841ED80, 0x6CD92232, 0x3821EEC0, 0x6C29F739,
            0x28017000, 0x445FC740, 0x28617340, 0x44AF124B, 0x28C17680, 0x45BE6D56, 0x28A175C0, 0x454EB85D,
            0x29817D00, 0x479C936C, 0x29E17E40, 0x476C4667, 0x29417B80, 0x467D397A, 0x292178C0, 0x468DEC71,
            0x2B016A00, 0x43D96F18, 0x2B616940, 0x4329BA13, 0x2BC16C80, 0x4238C50E, 0x2BA16FC0, 0x42C81005,
            0x2A816700, 0x401A3B34, 0x2AE16440, 0x40EAEE3F, 0x2A416180, 0x41FB9122, 0x2A2162C0, 0x410B4429,
            0x2E014400, 0x4B5297F0, 0x2E614740, 0x4BA242FB, 0x2EC14280, 0x4AB33DE6, 0x2EA141C0, 0x4A43E8ED,
            0x2F814900, 0x4891C3DC, 0x2FE14A40, 0x486116D7, 0x2F414F80, 0x497069CA, 0x2F214CC0, 0x4980BCC1,
            0x2D015E00, 0x4CD43FA8, 0x2D615D40, 0x4C24EAA3, 0x2DC15880, 0x4D3595BE, 0x2DA15BC0, 0x4DC540B5,
            0x2C815300, 0x4F176B84, 0x2CE15040, 0x4FE7BE8F, 0x2C415580, 0x4EF6C192, 0x2C2156C0, 0x4E061499,
            0x24011800, 0x5A456620, 0x24611B40, 0x5AB5B32B, 0x24C11E80, 0x5BA4CC36, 0x24A11DC0, 0x5B54193D,
            0x25811500, 0x5986320C, 0x25E11640, 0x5976E707, 0x25411380, 0x5867981A, 0x252110C0, 0x58974D11,
            0x27010200, 0x5DC3CE78, 0x27610140, 0x5D331B73, 0x27C10480, 0x5C22646E, 0x27A107C0, 0x5CD2B165,
            0x26810F00, 0x5E009A54, 0x26E10C40, 0x5EF04F5F, 0x26410980, 0x5FE13042, 0x26210AC0, 0x5F11E549,
            0x22012C00, 0x55483690, 0x22612F40, 0x55B8E39B, 0x22C12A80, 0x54A99C86, 0x22A129C0, 0x5459498D,
            0x23812100, 0x568B62BC, 0x23E12240, 0x567BB7B7, 0x23412780, 0x576AC8AA, 0x232124C0, 0x579A1DA1,
            0x21013600, 0x52CE9EC8, 0x21613540, 0x523E4BC3, 0x21C13080, 0x532F34DE, 0x21A133C0, 0x53DFE1D5,
            0x20813B00, 0x510DCAE4, 0x20E13840, 0x51FD1FEF, 0x20413D80, 0x50EC60F2, 0x20213EC0, 0x501CB5F9
        };

    uint32_t dwCrc1 = 0xFAC432B1;
    uint32_t dwCrc2 = 0x0CD5E44A;

    for (uint32_t i = 0; i < p_string.size(); i++) {
        uint32_t dwIndex = (dwCrc1 >> 24) & 255;

        dwCrc1 = ((dwCrc1 << 8) & 0xFFFFFFFF) ^ (dwCrc2 >> 24) ^ m_CrcTable[dwIndex * 2];
        dwCrc2 = (dwCrc2 << 8) ^ (byte)p_string[i] ^ m_CrcTable[dwIndex * 2 + 1];
    }
    return (uint64_t)dwCrc2 << 32 | dwCrc1;
}

typedef struct {
    std::string path;
    size_t offset;
    size_t size;
    bool hash;
    bool bnk_read;
    std::vector<file_view_t> bnk_files;
} big_file_t;


/***** BNK *****/
class unpack_tdu_bnk : public archive_impl {
    friend class unpack_tdu_big;
public:
    unpack_tdu_bnk() {};
    ~unpack_tdu_bnk() = default;

    void archive_list(const char* path, const service_ptr_t<file>& p_reader, archive_callback& p_out, bool p_want_readers) override;
    bool is_our_archive(const char* path) override { return strcmp(pfc::extract_ext_v2(path), "bnk") == 0; };

    static const GUID g_get_guid() { return plugin_guid; };
    static const char* g_get_name() { return "unpack_tdu2_bnk"; }

protected:
    const char* get_archive_type() override { return m_type; }
    t_filestats2 get_stats2_in_archive(const char* p_archive, const char* p_file, unsigned s2flags, abort_callback& p_abort) override;
    void open_archive(service_ptr_t<file>& p_out, const char* archive, const char* file, abort_callback& p_abort) override;

private:
    static constexpr const char* m_type = "tdu2bnk";
    pfc::string8 m_filename;
    std::vector<file_view_t> m_files;
    std::vector<pfc::string> m_skip_ext;

    void read_toc(const char* path, const service_ptr_t<file>& p_reader, abort_callback& p_abort);
    const file_view_t* get_file(const char* path) {
        for (size_t i = 0; i < m_files.size(); i++) {
            if (m_files[i].name == path) return &m_files[i];
        }
        return nullptr;
    }
};

void unpack_tdu_bnk::read_toc(const char* path, const service_ptr_t<file>& p_reader, abort_callback& p_abort) {
    if (m_filename == path) return;
    m_filename = path;
    m_files.clear();

    if (!path) return;

    service_ptr_t<file> reader = p_reader;
    if (reader.is_empty()) {
        filesystem::g_open_read(reader, path, p_abort);
    }

    constexpr const uint32_t BANK = MAKEFOURCC('B', 'A', 'N', 'K');
    constexpr const uint32_t KNAB = MAKEFOURCC('K', 'N', 'A', 'B');

    uint32_t (*read_u32)(file_ptr, abort_callback&) = NULL;

    reader->seek(0x08, p_abort);
    const uint32_t version = reader->read_bendian_t<uint32_t>(p_abort);
    if (version == BANK) {
        read_u32 = [](file_ptr reader, abort_callback& p_abort)->uint32_t { return reader->read_lendian_t<uint32_t>(p_abort); };
    }
    else if (version == KNAB) {
        read_u32 = [](file_ptr reader, abort_callback& p_abort)->uint32_t { return reader->read_bendian_t<uint32_t>(p_abort); };
    }
    else {
        FB2K_console_formatter() << "TDU2 BNK error: file signature mismatch in " << path;
        throw exception_io_data();
    }

    reader->skip(28, p_abort);
    const uint32_t PackedCount = read_u32(reader, p_abort);
    reader->skip(4, p_abort); // Year
    const uint32_t SizeSectionAddr = read_u32(reader, p_abort);
    reader->skip(4, p_abort); // TypeMappingSectionAddr
    const uint32_t TreeSectionAddr = read_u32(reader, p_abort);
    const uint32_t OrderSectionAddr = read_u32(reader, p_abort);

    m_files.reserve(PackedCount);

    // SizeSection
    struct size_entry_t {
        uint32_t offset;
        uint32_t size;
    };
    std::vector<size_entry_t> sizeSection;
    sizeSection.reserve(PackedCount);

    reader->seek((t_filesize)SizeSectionAddr + 8, p_abort); // skip GenericSectionHeader
    for (uint32_t i = 0; i < PackedCount; i++) {
        size_entry_t entry = {};
        entry.offset = read_u32(reader, p_abort);
        entry.size = read_u32(reader, p_abort);
        sizeSection.emplace_back(entry);
        reader->skip(12, p_abort); // u64 magic, u32 const 0x10
    }

    // TreeSection
    reader->seek((t_filesize)TreeSectionAddr, p_abort); // GenericSectionHeader
    uint32_t treeSize = read_u32(reader, p_abort);
    if (treeSize < 6) {
        // bigfile_EU_1.big|0x32f2e2f54d3d711.bnk
        FB2K_console_formatter() << "TDU2 BNK warning: tree section is too short in " << path;
        return;
    }
    reader->skip(4, p_abort); // checksum

    std::vector<pfc::string> fileList = {};
    fileList.reserve(PackedCount);

    const std::function<void(const pfc::string&, const char*)> read_folder = [&](const pfc::string& parentPath, const char* ext) {
        uint8_t nameLength = reader->read_object_t<uint8_t>(p_abort);
        if (!nameLength) {
            FB2K_console_formatter() << "TDU2 BNK warning: 0 length file name in " << parentPath;
            return;
        }
        pfc::string name;
        if (!ext) {
            nameLength = 256 - nameLength;
            uint16_t children = reader->read_object_t<uint8_t>(p_abort);
            reader->read_string_ex(name, nameLength, p_abort);
            uint8_t flag = 0;
            if (name[0] < 0x20) {
                flag = name[0];
                name.remove_chars(0, 1);
                name.add_byte(reader->read_object_t<uint8_t>(p_abort));
            }
            if (flag == 0x02) {
                // bigfile_EU_1.big|euro/bnk/interior/icasvip.bnk
                children += 128;
            }
            else if (flag > 0x02) {
                FB2K_console_formatter() << "TDU2 BNK error: magic byte " << flag << " in " << parentPath << "/" << name;
                throw exception_io_data();
            }
            if (name[0] == '.') {
                for (uint16_t i = 0; i < children; i++) {
                    read_folder(parentPath, name);
                }
            }
            else {
                if (!parentPath.empty()) name = parentPath + "\\" + name;
                for (uint8_t i = 0; i < children; i++) {
                    read_folder(name, NULL);
                }
            }
        }
        else {
            reader->read_string_ex(name, nameLength, p_abort);
            name = parentPath + "\\" + name + ext;
            fileList.emplace_back(name);
        }
    };
    t_filesize pos = 8;
    // bigfile_EU_1.big|euro/bnk/avatar/clothes/pnj/w_pn_b_skirt_host_rpt.bnk
    // 2 root sections and 0x00
    while (pos + 1 < treeSize) {
        read_folder("", NULL);
        pos = reader->get_position(p_abort) - TreeSectionAddr + 8;
    }

    if (fileList.size() != PackedCount) {
        FB2K_console_formatter() << "TDU2 BNK error: file count mismatch in " << path;
        throw exception_io_data();
    }

    // OrderSection
    std::vector<uint16_t> orderSection = {};
    orderSection.reserve(PackedCount);

    reader->seek((t_filesize)OrderSectionAddr + 8, p_abort); // skip GenericSectionHeader
    for (uint32_t i = 0; i < PackedCount; i++) {
        const uint16_t order =
            PackedCount <= 256 ? reader->read_object_t<uint8_t>(p_abort) :
            version == BANK ? reader->read_lendian_t<uint16_t>(p_abort) :
            reader->read_bendian_t<uint16_t>(p_abort);
        orderSection.emplace_back(order);
    }

    uint32_t skipCounter = 0;
    for (uint32_t i = 0; i < PackedCount; i++) {
        const pfc::string ext = pfc::extract_ext_v2(fileList[i].c_str());
        if (strcmp(ext.c_str(), "wav")) {
            continue;
        }
        const auto& sizeEntry = sizeSection[orderSection[i]];
        m_files.emplace_back(file_view_t{ fileList[i].c_str(), sizeEntry.offset, sizeEntry.size });
    }
}

void unpack_tdu_bnk::archive_list(const char* path, const service_ptr_t<file>& p_reader, archive_callback& p_out, bool p_want_readers) {
    service_ptr_t<file> reader = p_reader;
    if (reader.is_empty()) {
        filesystem::g_open_read(reader, path, p_out);
    }
    read_toc(path, reader, p_out);

    pfc::string8 url;
    for (const auto& entry : m_files) {
        make_unpack_path(url, path, entry.name.c_str());
        t_filestats stats {};
        stats.m_size = entry.size;
        service_ptr_t<file> fileReader = {};
        if (p_want_readers) {
            fileReader = createFileLimited(reader, entry.offset, entry.size, p_out);
        }
        if (!p_out.on_entry(this, url, stats, fileReader)) {
            break;
        }
    }
}

void unpack_tdu_bnk::open_archive(service_ptr_t<file>& p_out, const char* archive, const char* p_file, abort_callback& p_abort) {
    if (!archive || !p_file) // shouldn't be possible
        throw exception_io_data();

    service_ptr_t<file> reader;
    filesystem::g_open_read(reader, archive, p_abort);
    read_toc(archive, reader, p_abort);

    const auto entry = get_file(p_file);
    if (entry) {
        p_out = createFileLimited(reader, entry->offset, entry->size, p_abort);
        return;
    }

    throw exception_io_data();
}

t_filestats2 unpack_tdu_bnk::get_stats2_in_archive(const char* p_archive, const char* p_file, unsigned s2flags, abort_callback& p_abort) {
    t_filestats2 stats = {};
    read_toc(p_archive, NULL, p_abort);
    if (m_filename == p_archive) {
        const auto entry = get_file(p_file);
        if (entry) stats.m_size = entry->size;
    }
    return stats;
}

static archive_factory_t<unpack_tdu_bnk> g_unpack_tdu_bnk_factory;

/***** BIG + MAP *****/
class reader_bigfile : public reader_limited {
private:
    inline static const UCHAR XOR_KEY[] = { 0xD7, 0xA8, 0xE2, 0xD4 };
public:
    static file::ptr g_create(file::ptr base, t_filesize offset, t_filesize size, abort_callback& abort) {
        service_ptr_t<reader_bigfile> r = new service_impl_t<reader_bigfile>();
        if (offset + size < offset) throw pfc::exception_overflow();
        r->init(base, offset, offset + size, abort);
        return r;
    }

    t_size read(void* p_buffer, t_size p_bytes, abort_callback& p_abort) override {
        t_size pos = get_position(p_abort);
        t_size bytes_read = reader_limited::read(p_buffer, p_bytes, p_abort);
        for (uint32_t i = 0; i < bytes_read; i++) {
            ((uint8_t*)p_buffer)[i] ^= XOR_KEY[(pos + i) % 4];
        }
        return bytes_read;
    }
};

class unpack_tdu_big : public archive_impl {
public:
    unpack_tdu_big(){};
    ~unpack_tdu_big() = default;

    void archive_list(const char* path, const service_ptr_t<file>& p_reader, archive_callback& p_out, bool p_want_readers) override;
    bool is_our_archive(const char* path) override;

    static const GUID g_get_guid() { return plugin_guid; };
    static const char* g_get_name() { return "unpack_tdu2_big"; }

protected:
    const char* get_archive_type() override { return m_type; }
    t_filestats2 get_stats2_in_archive(const char* p_archive, const char* p_file, unsigned s2flags, abort_callback& p_abort) override;
    void open_archive(service_ptr_t<file>& p_out, const char* archive, const char* file, abort_callback& p_abort) override;

private:
    static constexpr const char* m_type = "tdu2";
    pfc::string8 m_filename;
    std::vector<big_file_t> m_files;
    std::optional<HashTable<uint64_t>> hashes;

    void read_toc(const char* path, const service_ptr_t<file>& p_reader, abort_callback& p_abort);
    const big_file_t* get_file(const char* path) {
        for (uint32_t i = 0; i < m_files.size(); i++) {
            if (m_files[i].path == path) return &m_files[i];
        }
        return nullptr;
    }
};

bool unpack_tdu_big::is_our_archive(const char* path) {
    const auto ext = pfc::string_extension(path);
    return (ext == "big" || ext == "map");
}

void unpack_tdu_big::read_toc(const char* path, const service_ptr_t<file>& p_reader, abort_callback& p_abort) {
    if (!path) return;

    pfc::string map_path = path;
    const auto ext_offset = pfc::find_extension_offset(path);
    if (ext_offset != SIZE_MAX && strcmp(path + ext_offset, ".map")) {
        map_path = pfc::string(path, ext_offset) + ".map";
    }

    if (m_filename == map_path) return;
    m_filename = map_path;
    m_files.clear();

    service_ptr_t<file> reader = {};
    filesystem::g_open_read(reader, map_path, p_abort);

    constexpr const uint32_t XMBF = MAKEFOURCC('X', 'M', 'B', 'F');
    uint32_t version = reader->read_lendian_t<uint32_t>(p_abort);
    if (version != XMBF) throw exception_io_data();
    reader->skip(12, p_abort);
    uint32_t dwSubHeaderOffset = reader->read_bendian_t<uint32_t>(p_abort);

    // SubHeader
    reader->seek((t_filesize)dwSubHeaderOffset + 4, p_abort);
    uint32_t dwTotalFiles = reader->read_bendian_t<uint32_t>(p_abort);
    uint32_t dwSubHeaderSize = reader->read_bendian_t<uint32_t>(p_abort);
    uint32_t dwNumRecords = reader->read_bendian_t<uint32_t>(p_abort);
    uint32_t dwRecordsOffset = reader->read_bendian_t<uint32_t>(p_abort);
    if (!dwTotalFiles)
        throw exception_io_data();

    m_files.reserve(dwTotalFiles);

    if (!hashes) hashes.emplace("TDU2names.txt", crc64);

    reader->seek((t_filesize)dwSubHeaderOffset + dwSubHeaderSize, p_abort);
    for (uint32_t i = 0; i < dwTotalFiles; i++) {
        big_file_t file = {};
        const uint64_t hash = reader->read_bendian_t<uint64_t>(p_abort);
        const auto decoded_name = hashes->table.find(hash);
        if (decoded_name != hashes->table.end()) {
            file.path = decoded_name->second;
        }
        else {
            char buffer[19] = {};
            sprintf_s(buffer, "0x%016llx", hash);
            file.path = buffer;
            file.hash = true;
        }
        m_files.emplace_back(file);
    }

    reader->seek((t_filesize)dwRecordsOffset + dwSubHeaderOffset, p_abort);
    for (uint32_t i = 0; i < dwTotalFiles; i++) {
        m_files[i].size = reader->read_bendian_t<uint32_t>(p_abort);
        m_files[i].offset = reader->read_bendian_t<uint64_t>(p_abort);
    }
}

void unpack_tdu_big::archive_list(const char* path, const service_ptr_t<file>& p_reader, archive_callback& p_out, bool p_want_readers) {
    service_ptr_t<file> map_reader, big_reader;
    pfc::string map_path, big_path;

    const auto ext_offset = pfc::find_extension_offset(path);
    if (ext_offset == SIZE_MAX)
        throw exception_io_no_handler_for_path();

    if (strcmp(path + ext_offset, ".map")) {
        map_path = pfc::string(path, ext_offset) + ".map";
        big_path = path;
        big_reader = p_reader;
    }
    else {
        map_path = path;
        big_path = pfc::string(path, ext_offset) + ".big";
        map_reader = p_reader;
    }

    read_toc(map_path, map_reader, p_out);
    if (m_files.empty()) return;

    if (big_reader.is_empty()) {
        filesystem::g_open_read(big_reader, big_path, p_out);
    }

    constexpr const uint32_t RIFF = MAKEFOURCC('R', 'I', 'F', 'F');
    constexpr const uint32_t BANK = 0x40000000;
    constexpr const uint32_t KNAB = 0x40;

    auto& bnk_parser = g_unpack_tdu_bnk_factory.get_static_instance();
    for (auto& entry : m_files) {
        pfc::string8 url;
        make_unpack_path(url, big_path, entry.path.c_str());
        t_filestats stats {};
        stats.m_size = entry.size;
        service_ptr_t<file> file_reader;
        if (p_want_readers) {
            file_reader = reader_bigfile::g_create(big_reader, entry.offset, entry.size, p_out);
        }
        if (entry.hash) {
            if (p_want_readers) {
                uint32_t sign = file_reader->read_lendian_t<uint32_t>(p_out);
                switch (sign) {
                case RIFF:
                    entry.path += ".wav";
                    url += ".wav";
                    break;
                case BANK:
                case KNAB:
                    entry.path += ".bnk";
                    url += ".bnk";
                    break;
                default:
                    continue;
                }
            }
        }
        if (strcmp(pfc::extract_ext_v2(url.c_str()), "bnk")) {
            if (!p_out.on_entry(this, url, stats, file_reader)) {
                break;
            }
        }
        else {
            p_out.check();
            if (!entry.bnk_read) {
                bnk_parser.archive_list(url, file_reader, p_out, p_want_readers);
                entry.bnk_files = bnk_parser.m_files;
            }
            else {
                for (const auto& bnk_file : entry.bnk_files) {
                    pfc::string8 bnk_url;
                    bnk_parser.make_unpack_path(bnk_url, url, bnk_file.name.c_str());
                    if (!p_out.on_entry(this, url, stats, file_reader)) {
                        break;
                    }
                }
            }
        }
    }
}

void unpack_tdu_big::open_archive(service_ptr_t<file>& p_out, const char* archive, const char* p_file, abort_callback& p_abort) {
    if (!archive || !p_file) // shouldn't be possible
        throw exception_io_data();

    read_toc(archive, NULL, p_abort);

    const auto entry = get_file(p_file);
    if (entry) {
        service_ptr_t<file> reader;
        filesystem::g_open_read(reader, archive, p_abort);
        p_out = reader_bigfile::g_create(reader, entry->offset, entry->size, p_abort);
        return;
    }
    throw exception_io_data();
}

t_filestats2 unpack_tdu_big::get_stats2_in_archive(const char* p_archive, const char* p_file, unsigned s2flags, abort_callback& p_abort) {
    t_filestats2 stats = {};
    read_toc(p_archive, NULL, p_abort);
    if (m_filename == p_archive) {
        const auto entry = get_file(p_file);
        if (entry) stats.m_size = entry->size;
    }
    return stats;
}

// foobar plugin defs
static archive_factory_t<unpack_tdu_big> g_unpack_tdu_big_factory;
