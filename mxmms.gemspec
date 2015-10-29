require './lib/mxmms/version'

Gem::Specification.new do |s|
  s.name	= 'mxmms'
  s.version	= VERSION
  s.summary	= "Masm's XMMS2 Client"
  s.description	= 'XMMS2 client with a large play/pause button.'
  s.authors	= ['Yuuki Harano']
  s.email	= 'masm@masm11.ddo.jp'
  s.files	= [ 'README', 'README.md', 'COPYING',
                    'bin/mxmms', 'lib/mxmms/version.rb',
                    'lib/mxmms/backend.rb', 'lib/mxmms/gui.rb' ]
  s.executables	<< 'mxmms'
  s.homepage	= 'https://github.com/masm11/mxmms'
  s.license	= 'GPL'
  s.add_runtime_dependency 'gtk3', '~> 2.2', '>= 2.2.3'
end
