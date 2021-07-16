{
  'targets': [
    {
      'target_name': 'raw',
      'sources': [
        'src/raw.cc'
      ],
      "include_dirs" : [
        "<!(node -e \"require('nan')\")"
      ],
      'conditions' : [
        ['OS=="win"', {
          'libraries' : ['ws2_32.lib']
        }],
        ['OS=="mac"', {
          'xcode_settings': {
              'OTHER_CPLUSPLUSFLAGS': ['-std=c++11', '-stdlib=libc++'],
              'OTHER_LDFLAGS': ['-stdlib=libc++'],
              'MACOSX_DEPLOYMENT_TARGET': '10.7',
              'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
          },
        }],
      ]
    }
  ]
}
