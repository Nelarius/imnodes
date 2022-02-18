#pragma once
#include <vector>
#include <string_view>
#include <fmt/format.h>
#include <fmt/chrono.h>

#include <node.hpp>
#include <sstream>
#include <iomanip>

namespace utility {

    template<typename... Args>
    inline std::string format(std::string_view format, Args... args) {
        return fmt::format(fmt::runtime(format), args...);
    }

    template<typename... Args>
    inline void print(std::string_view format, Args... args) {
        fmt::print(fmt::runtime(format), args...);
    }

    template<typename T>
    std::vector<u8> toBytes(T value) {
        std::vector<u8> bytes(sizeof(T));
        std::memcpy(bytes.data(), &value, sizeof(T));

        return bytes;
    }


    // template<class _Dp, class _Bp>
    // concept derived_from =
    //     __is_base_of(_Bp, _Dp) && __is_convertible_to(const volatile _Dp *, const volatile _Bp *);

    template< class Derived, class Base >
    concept derived_from =
    std::is_base_of_v<Base, Derived> &&
    std::is_convertible_v<const volatile Derived*, const volatile Base*>;





    namespace impl {

        using CreatorFunction = std::function<PcapEditor::Node *()>;

        struct Entry {
            std::string category;
            std::string name;
            CreatorFunction creatorFunction;
        };

            void add(const Entry &entry);

    }
    
    template<derived_from<PcapEditor::Node> T, typename... Args>
    void add(const std::string &unlocalizedCategory, const std::string &unlocalizedName, Args &&...args) {
        add(impl::Entry { unlocalizedCategory.c_str(), unlocalizedName.c_str(), [=] {
                                auto node = new T(std::forward<Args>(args)...);
                                node->setUnlocalizedName(unlocalizedName);
                                return node;
                            } });
    }

    void addSeparator() ;
    std::vector<impl::Entry> &getEntries() ; 



    std::string hexStr(u8 *data, u32 len);

}