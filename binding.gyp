{
  'targets': [
    {
      'target_name': 'raw',
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'xcode_settings': { 'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'CLANG_CXX_LIBRARY': 'libc++',
        'MACOSX_DEPLOYMENT_TARGET': '10.7',
      },
      'msvs_settings': {
        'VCCLCompilerTool': { 'ExceptionHandling': 1 },
      },
      'sources': [
        'src/raw.cc'
      ],
      "include_dirs" : [
      ],
      'conditions' : [
        ['OS=="win"', {
          'libraries' : ['ws2_32.lib']
        }]
      ]
    }
  ]
}
