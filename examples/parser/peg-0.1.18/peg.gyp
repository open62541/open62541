{
  'targets': [
    {
      'target_name': 'peg',
      'type': 'executable',
      'msvs_guid': '5ECEC9E5-8F23-47B6-93E0-C3B328B3BE65',
      'sources': [
        'peg.c',
        'tree.c',
        'compile.c',
      ],
      'conditions': [
        ['OS=="win"', {
          'include_dirs': [
            'win',
          ],
          'sources': [
            'win/getopt.c',
          ],
        }],
      ],
    },
    {
      'target_name': 'leg',
      'type': 'executable',
      'msvs_guid': '5ECEC9E5-8F23-47B6-93E0-C3B328B3BE66',
      'sources': [
        'leg.c',
        'tree.c',
        'compile.c',
      ],
      'conditions': [
        ['OS=="win"', {
          'include_dirs': [
            'win',
          ],
          'sources': [
            'win/getopt.c',
          ],
        }],
      ],
    },
  ],

  'target_defaults': {
    'configurations': {
      'Debug': {
        'defines': [
          'DEBUG',
        ],
      },
      'Release': {
        'defines': [
          'NDEBUG',
        ],
      },
    },
  },

  # define default project settings
  'conditions': [
    ['OS=="win"', {
      'target_defaults': {
        'defines': [
          'WIN32',
          '_WINDOWS',
        ],
        'msvs_settings': {
          'VCLinkerTool': {
            'GenerateDebugInformation': 'true',
            # SubSystem values:
            #   0 == not set
            #   1 == /SUBSYSTEM:CONSOLE
            #   2 == /SUBSYSTEM:WINDOWS
            'SubSystem': '1',
          },
        },
      },
    }],
  ],
}
