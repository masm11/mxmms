Gem::Specification.new do |s|
  s.name	= 'mxmms'
  s.version	= '1.0.0'
  s.summary	= "Masm's XMMS2 Client"
  s.description	= 'XMMS2 client with large play/pause button.'
  s.authors	= ['Yuuki Harano']
  s.email	= 'masm@masm11.ddo.jp'
  s.files	= [ 'README', 'COPYING',
                    'bin/mxmms',
                    'lib/mxmms/playlist.rb', 'lib/mxmms/playlists.rb' ]
  s.executables	<< 'mxmms'
  s.homepage	= 'https://github.com/masm11/mxmms'
  s.license	= 'GPL'
  s.add_runtime_dependency 'gtk3', '~> 2.2', '>= 2.2.3'
end
