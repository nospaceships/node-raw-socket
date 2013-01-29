{
  'targets': [
    {
      'target_name': 'raw',
      'sources': [
        'src/raw.cc'
      ],
      'conditions' : [
        ['OS=="win"', {
          'libraries' : ['ws2_32.lib']
        }]
      ]
    }
  ]
}
