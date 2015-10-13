#!/usr/bin/env ruby
# coding: utf-8

require 'mxmms/gui'
require 'mxmms/backend'

gui = Gui.new 100, 100

gui.set_playpause_handler do
  @backend.playback_toggle
end

gui.set_change_music_handler do |id|
  print "change " + id.to_s + "\n"
end

@backend = Backend.new

@backend.set_playback_status_changed_handler do |status|
  gui.set_status status
end

@backend.set_playback_playtime_changed_handler do |msec|
  gui.set_playtime msec / 1000
end

@backend.set_playback_entry_changed_handler do |id, artist, title|
  # fixme: id だけ渡せば良さそう。
  gui.set_title id, title, artist
end

@backend.set_playlist_loaded_handler do |list|
  gui.set_playlist list
end

@backend.set_playlist_changed_handler do |list|
  gui.set_playlist list
end

gui.main

#
