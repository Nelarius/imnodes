#pragma once
#include <node.hpp>
#include <list>
namespace PcapEditor{
   class Provider{
    public:
    
        Overlay *newOverlay() {
            return this->m_overlays.emplace_back(new Overlay());
        }

        void deleteOverlay(Overlay *overlay) {
            this->m_overlays.erase(std::find(this->m_overlays.begin(), this->m_overlays.end(), overlay));
            delete overlay;
        }

        const std::list<Overlay *> &getOverlays() {
            return this->m_overlays;
        }
    private:
        std::list<Overlay *> m_overlays;
    };
    static u32 s_currentProvider;
    static std::vector<Provider *> s_providers;
    
    bool isValid() ;
    Provider *get() ;

    void createProvider();
    void remove(Provider *provider);


    const std::vector<Provider *> &getProviders() ;

    void setCurrentProvider(u32 index) ;

    void addProvider(Provider *provider) ;

    void remove(Provider *provider) ;

void registerProvider();


}
// namespace Provider {

//             namespace impl {

//                 void addProviderName(const std::string &unlocalizedName);

//             }

//             template<hex::derived_from<hex::prv::Provider> T>
//             void add(const std::string &unlocalizedName, bool addToList = true) {
                
                    

//                 auto newProvider = new T();

//                 hex::ImHexApi::Provider::add(newProvider);

//                 if (provider != nullptr)
//                     *provider = newProvider;

//                 if (addToList)
//                     impl::addProviderName(unlocalizedName);
//             }

//             std::vector<std::string> &getEntries();

//             Provider *get();
//             const std::vector<Provider *> &getProviders();

//             void setCurrentProvider(u32 index);

//             bool isValid();

//             void add(Provider *provider);

//             template<hex::derived_from<Provider> T>
//             void add(auto &&...args) {
//                 add(new T(std::forward<decltype(args)>(args)...));
//             }

//             void remove(Provider *provider);

//         }