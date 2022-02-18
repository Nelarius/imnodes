FIND_PATH(PcapPlusPlus_INCLUDE_DIR pcapplusplus/Layer.h)
# message("[+] pcapplusplus include dir is : ${pcapplusplus_include_dir}")
FIND_PATH(PcapPlusPlus_DIR NAMES lib/libCommon++.a lib/libPacket++.a)

IF( PcapPlusPlus_INCLUDE_DIR AND pcapplusplus_dir)
    set(PcapPlusPlus_FOUND TRUE)
    set(PcapPlusPlus_LIBS_DIR ${pcapplusplus_dir}/lib)
    # message("[+] pcapplusplus libs dir is : ${PcapPlusPlus_LIBS_DIR}")
ENDIF()
