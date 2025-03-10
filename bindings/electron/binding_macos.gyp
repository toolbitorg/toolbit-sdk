#  Toolbit SDK
#  Copyright (C) 2020 ohamxa <toolbitorg@gmail.com>
#
#  This program is distributed in the hope that it will be useful, but WITHOUT
#  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
#  more details.
{
  "targets": [
    {
      "target_name": "toolbit",
      'include_dirs': [
        '../node_modules/node-addon-api',
      ],
      "sources": [ "toolbit_wrap.cxx", "tbi_core.cpp", "tbi_device.cpp", "tbi_service.cpp", "attribute.cpp", "tbit.cpp", "hid.c", "chopper.cpp", "choppy.cpp", "dmm.cpp", "tbi_device_manager.cpp", "adc.cpp", "adc_hw.cpp", "gpio_hw.cpp", "i2c_hw.cpp", "pin.cpp"],
      "libraries": [ "-lstdc++ -framework IOKit -framework Carbon"],
      "conditions": [
        ['OS=="mac"', {
          'xcode_settings': {
            'OTHER_CFLAGS': [ '' ],
            'OTHER_CPLUSPLUSFLAGS': [ '-std=c++17' ],
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
          }
        }]
      ]
    }
  ]
}
