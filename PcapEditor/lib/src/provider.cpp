#include <provider.hpp>
namespace PcapEditor
{
    void removeProvider(){
        for(auto p : s_providers){
            remove(p);
        }
    }

void registerProvider(){
        // Provider::utility::add<prv::FileProvider>("hex.builtin.provider.file", false);
        createProvider();
    }    
    bool isValid() {
        return !s_providers.empty();
    }
    Provider *get() {
        if (!isValid())
            return nullptr;

        return s_providers[s_currentProvider];
    }

    void createProvider(){
        Provider * provider = new Provider;
        addProvider(provider);
    }


    const std::vector<Provider *> &getProviders() {
        return s_providers;
    }

    void setCurrentProvider(u32 index) {
        if (index < s_providers.size()) {
            auto oldProvider  = get();
            s_currentProvider = index;
            // EventManager::post<EventProviderChanged>(oldProvider, get());
        }
    }

    void addProvider(Provider *provider) {
        s_providers.push_back(provider);
        setCurrentProvider(s_providers.size() - 1);

        // EventManager::post<EventProviderCreated>(provider);
    }

    void remove(Provider *provider) {
        auto it = std::find(s_providers.begin(), s_providers.end(), provider);

        s_providers.erase(it);

        if (it - s_providers.begin() == s_currentProvider)
            setCurrentProvider(0);

        delete provider;
    }
} // namespace PcapEditor

