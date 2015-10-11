#!/usr/bin/env ruby

require 'mxmms/gui'
require 'mxmms/backend'

gui = Gui.new 100, 100

gui.set_playpause_handler do
  print "clicked\n"
end

gui.set_change_music_handler do |id|
  print "change " + id.to_s + "\n"
end

backend = Backend.new

backend.set_playback_status_changed_handler do |status|
  gui.set_status status
end

gui.main

#
