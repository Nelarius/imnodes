#include <imnodes.h>
#include <imnodes_internal.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <UnitTest++/UnitTest++.h>

// A suitable tolerance for screen-space coordinate magnitudes
const float flt_tolerance = 1E-4;

SUITE(test_internals)
{
    TEST(calculate_panning_offset_to_node)
    {
        const ImRect editor_canvas_rect(ImVec2(0.f, 00.f), ImVec2(100.f, 100.f));
        const ImRect node_rect(ImVec2(25.f, 25.f), ImVec2(50.f, 50.f));

        // Canvas center: (50, 50)
        // Node center: (37.5, 37.5)
        // Vector required to translate (37.5, 37.5) -> (50, 50) = (12.5, 12.5)
        const ImVec2 expected = ImVec2(12.5f, 12.5f);
        const ImVec2 actual = ImNodes::CalculatePanningOffsetToNode(
            editor_canvas_rect.GetCenter(), node_rect.GetCenter());

        CHECK_ARRAY_CLOSE(&expected.x, &actual.x, 2, flt_tolerance);
    }
}
