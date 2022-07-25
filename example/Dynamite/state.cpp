#include "state.h" 

struct BlockNames names;

void State::init() {
    names.dsp_names = m_wrapper.get_dsp_list();
    names.control_names = m_wrapper.get_control_list();
}