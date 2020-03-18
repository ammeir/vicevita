# VICEVita

Description:

VICEVita is a port of the Versatile Commodore Emulator (VICE) for the Sony PlayStation Vita.
It features a completely new UI with touch keyboard, savestates and control mappings.


Compiling:

-Install VitaSDK toolchain: http://vitasdk.org
-If you are on Windows, install MSYS2 command shell: http://msys2.org
-Clone vicevita repo into a folder somewhere.
-Compile and package:
  cmake <your vicevita repo folder> -DBUILD_TYPE=Release
  make

  For a debug version replace Release with Debug.
