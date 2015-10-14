all:
	gem build mxmms.gemspec

install: all
	gem install ./mxmms-`ruby -e 'require "./lib/mxmms/version"; print VERSION'`.gem
