#!/usr/bin/env ruby
# coding: utf-8

require 'mxmms/gui'
require 'mxmms/backend'

gui = Gui.new 100, 100

gui.set_playpause_handler do
  @backend.playback_toggle
end

gui.set_jump_pos_handler do |pos|
  @backend.jump_pos pos
end

gui.set_change_playlist_handler do |name|
  @backend.change_playlist name
end

@backend = Backend.new

@backend.set_playback_status_changed_handler do |status|
  gui.set_status status
end

@backend.set_playback_playtime_changed_handler do |msec|
  gui.set_playtime msec / 1000
end

@backend.set_playback_entry_changed_handler do |pos|
  gui.set_current_pos pos
end

@backend.set_playlist_loaded_handler do |name, list|
  gui.set_playlist name, list
end

@backend.set_playlist_changed_handler do |list|
  gui.set_playlist nil, list	# fixme: nil->name
end

@backend.set_playlist_list_retr_handler do |list|
  gui.set_playlist_list list
end

gui.main

#
