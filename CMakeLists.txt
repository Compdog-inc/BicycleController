cmake_minimum_required (VERSION 3.8)

project ("BicycleController")

# Add source to this project's executable.
add_executable (BicycleController
"BicycleController.cpp" "BicycleController.h" 
"ioutils.cpp" "ioutils.h"
"staticlist.h"
"framework.h"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET BicycleController PROPERTY CXX_STANDARD 20)
endif()

set(F_CPU 8000000UL)
set(MCU atmega328p)
set(PROG_TYPE usbasp)

set(L_FUSE 0xC2)
set(H_FUSE 0xD0)
set(E_FUSE 0xFF)
set(LOCK_BIT 0xFF)

add_definitions(
    -DF_CPU=${F_CPU}
    -mmcu=${MCU}
    -B="C:/Program Files/avr8-gnu-toolchain/packs/gcc/dev/atmega328p/"
)

set(CMAKE_EXE_LINKER_FLAGS -mmcu=${MCU})

target_include_directories(BicycleController PUBLIC
  "C:/Program Files/avr8-gnu-toolchain/avr/include/"
  E:/source/include/avr/
)

target_link_libraries(BicycleController "E:/source/include/avr/shared/lib/libusart.a")

add_compile_options(
    -mmcu=${MCU}
    -std=c++17
    -Os
    -Wall
    -Wno-main
    -Wundef
    -pedantic
    -Wstrict-prototypes
    -Werror
    -Wfatal-errors
    -Wl,--relax,--gc-sections
    -g
    -gdwarf-2
    -funsigned-char
    -funsigned-bitfields
    -fpack-struct
    -fshort-enums
    -ffunction-sections
    -fdata-sections
    -fno-split-wide-types
    -fno-tree-scev-cprop
)

set_target_properties(BicycleController PROPERTIES OUTPUT_NAME BicycleController.elf)

add_custom_target(strip ALL avr-strip BicycleController.elf DEPENDS BicycleController)
add_custom_target(hex ALL avr-objcopy -R .eeprom -O ihex BicycleController.elf BicycleController.hex DEPENDS strip)

add_custom_target(eeprom avr-objcopy -j .eeprom  --set-section-flags=.eeprom="alloc,load"  --change-section-lma .eeprom=0 -O ihex BicycleController.elf BicycleController.eep DEPENDS strip)

add_custom_target(upload avrdude  -c ${PROG_TYPE} -p ${MCU} -U flash:w:BicycleController.hex DEPENDS hex)
add_custom_target(upload_eeprom avrdude -c ${PROG_TYPE} -p ${MCU}  -U eeprom:w:BicycleController.eep DEPENDS eeprom)

add_custom_target(fuses avrdude -c ${PROG_TYPE} -p ${MCU}  -U lfuse:w:${L_FUSE}:m -U hfuse:w:${H_FUSE}:m -U efuse:w:${E_FUSE}:m -U lock:w:${LOCK_BIT}:m )

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "BicycleController.hex;BicycleController.eeprom;BicycleController.lst")