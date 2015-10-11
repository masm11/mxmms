all:
	gem build mxmms.gemspec

install: all
	gem install ./mxmms-1.0.5.gem
