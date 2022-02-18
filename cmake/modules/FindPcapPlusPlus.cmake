FIND_PATH(PcapPlusPlus_INCLUDE_DIR 
    NAMES pcapplusplus/Layer.h 
    PATHS  /usr/local/)
message("[+] pcapplusplus include dir is : ${PcapPlusPlus_INCLUDE_DIR}")
FIND_PATH(PcapPlusPlus_DIR 
    NAMES lib/libCommon++.a lib/libPacket++.a  lib/libPcap++
    PATHS /usr/local/)
message("[+] pcapplusplus include dir is : ${PcapPlusPlus_DIR}")

IF( PcapPlusPlus_INCLUDE_DIR AND PcapPlusPlus_DIR)
    set(PcapPlusPlus_FOUND TRUE)
    set(PcapPlusPlus_LIBS_DIR ${PcapPlusPlus_DIR}/lib)
    # message("[+] pcapplusplus libs dir is : ${PcapPlusPlus_LIBS_DIR}")
ENDIF()
