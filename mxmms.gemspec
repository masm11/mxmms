Gem::Specification.new do |s|
  s.name	= 'mxmms'
  s.version	= '1.0.0'
  s.summary	= "Masm's XMMS2 Client"
  s.authors	= ['Yuuki Harano']
  s.email	= 'masm@masm11.ddo.jp'
  s.files	= [ 'bin/mxmms',
                    'lib/mxmms/playlist.rb', 'lib/mxmms/playlists.rb' ]
  s.executables	<< 'mxmms'
  s.license	= 'GPL'
end
