#pragma once

#include "IMNODES_NAMESPACE.h"
#include "imnodes_config_or_default.h"

namespace IMNODES_NAMESPACE
{
namespace Internal
{

// Copy-pasted from ImGui because we needed to change the ID type

// Helper: Key->Value storage
// Typically you don't have to worry about this since a storage is held within each Window.
// We use it to e.g. store collapse state for a tree (Int 0/1)
// This is optimized for efficient lookup (dichotomy into a contiguous buffer) and rare insertion
// (typically tied to user interactions aka max once a frame) You can use it as custom user storage
// for temporary values. Declare your own storage if, for example:
// - You want to manipulate the open/close state of a particular sub-tree in your interface (tree
// node uses Int 0/1 to store their state).
// - You want to store custom debug data easily without adding or editing structures in your code
// (probably not efficient, but convenient) Types are NOT stored, so it is up to you to make sure
// your Key don't collide with different types.
struct Storage
{
    // [Internal]
    struct ImGuiStoragePair
    {
        ID key;
        union
        {
            int   val_i;
            float val_f;
            void* val_p;
        };
        ImGuiStoragePair(ID _key, int _val_i)
        {
            key = _key;
            val_i = _val_i;
        }
        ImGuiStoragePair(ID _key, float _val_f)
        {
            key = _key;
            val_f = _val_f;
        }
        ImGuiStoragePair(ID _key, void* _val_p)
        {
            key = _key;
            val_p = _val_p;
        }
    };

    ImVector<ImGuiStoragePair> Data;

    // - Get***() functions find pair, never add/allocate. Pairs are sorted so a query is O(log N)
    // - Set***() functions find pair, insertion on demand if missing.
    // - Sorted insertion is costly, paid once. A typical frame shouldn't need to insert any new
    // pair.
    void            Clear() { Data.clear(); }
    IMGUI_API int   GetInt(ID key, int default_val = 0) const;
    IMGUI_API void  SetInt(ID key, int val);
    IMGUI_API bool  GetBool(ID key, bool default_val = false) const;
    IMGUI_API void  SetBool(ID key, bool val);
    IMGUI_API float GetFloat(ID key, float default_val = 0.0f) const;
    IMGUI_API void  SetFloat(ID key, float val);
    IMGUI_API void* GetVoidPtr(ID key) const; // default_val is NULL
    IMGUI_API void  SetVoidPtr(ID key, void* val);

    // - Get***Ref() functions finds pair, insert on demand if missing, return pointer. Useful if
    // you intend to do Get+Set.
    // - References are only valid until a new value is added to the storage. Calling a Set***()
    // function or a Get***Ref() function invalidates the pointer.
    // - A typical use case where this is convenient for quick hacking (e.g. add storage during a
    // live Edit&Continue session if you can't modify existing struct)
    //      float* pvar = ImGui::GetFloatRef(key); ImGui::SliderFloat("var", pvar, 0, 100.0f);
    //      some_var += *pvar;
    IMGUI_API int*   GetIntRef(ID key, int default_val = 0);
    IMGUI_API bool*  GetBoolRef(ID key, bool default_val = false);
    IMGUI_API float* GetFloatRef(ID key, float default_val = 0.0f);
    IMGUI_API void** GetVoidPtrRef(ID key, void* default_val = NULL);

    // Use on your own storage if you know only integer are being stored (open/close all tree nodes)
    IMGUI_API void SetAllInt(int val);

    // For quicker full rebuild of a storage (instead of an incremental one), you may add all your
    // contents and then sort once.
    IMGUI_API void BuildSortByKey();
};

} // namespace Internal
} // namespace IMNODES_NAMESPACE