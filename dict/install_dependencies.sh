apt install -y \
        libsdl2-dev\
        gcc-10                \
        g++-10                \


# pcapPlusPlus
wget https://github.com/seladb/PcapPlusPlus/releases/download/v21.11/pcapplusplus-21.11-ubuntu-20.04-gcc-9.tar.gz
tar xvf pcapplusplus-21.11-ubuntu-20.04-gcc-9.tar.gz
cd pcapplusplus-21.11-ubuntu-20.04-gcc-9/
chmod +x .
./install


sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 90 --slave /usr/bin/g++ g++ /usr/bin/g++-10 --slave /usr/bin/gcov gcov /usr/bin/gcov-10 # 指定gcc10 优先级为90
 sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 80 --slave /usr/bin/g++ g++ /usr/bin/g++-9 --slave /usr/bin/gcov gcov /usr/bin/gcov-9 ## gcc9 为80
