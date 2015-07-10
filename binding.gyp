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
        }]
      ]
    }
  ]
}
