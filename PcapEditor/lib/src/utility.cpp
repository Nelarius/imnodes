#include <utility.hpp>

namespace utility
{
     void impl::add(const impl::Entry &entry) {
        // log::info("Registered new data processor node type: [{}]: ", entry.category, entry.name);

        getEntries().push_back(entry);
    }

    void addSeparator() {
        getEntries().push_back({ "", "", [] { return nullptr; } });
    }

    std::vector<impl::Entry> &getEntries() {
        static std::vector<impl::Entry> nodes;

        return nodes;
    }   

    std::string hexStr(u8 *data, u32 len)
    {
        std::stringstream ss;
        ss << std::hex;

        for( int i(0) ; i < len; ++i )
            ss << std::setw(2) << std::setfill('0') << (int)data[i];

        return ss.str();
    }
} // namespace name