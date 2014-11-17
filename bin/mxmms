#!/usr/bin/env ruby
# coding: utf-8

# mxmms - Masm's XMMS2 Client
# Copyright (C) 2014 Yuuki Harano
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

require 'gtk3'
require 'xmmsclient'
require 'xmmsclient_glib'
require_relative 'mxmms/playlist'
require_relative 'mxmms/playlists'

@playing = false

@geometry = nil
@width = 48
@height = 48
@autostart = false

i = 0
while i < ARGV.length
  case ARGV[i]
  when '--geometry'
    @geometry = ARGV[i + 1]
    i += 2
  when '--width'
    @width = ARGV[i + 1].to_i
    i += 2
  when '--height'
    @height = ARGV[i + 1].to_i
    i += 2
  when '--autostart'
    @autostart = true
    i += 1
  else
    $stderr.puts('unknown args.')
    exit 1
  end
end

def set_title(id)
  @xmms.medialib_get_info(id).notifier do |res|
    title = nil
    artist = nil
      
    if res
      r = res[:title]
      if r
        r.each_pair { |x, y| title = y }
      end
      
      r = res[:artist]
      if r
        r.each_pair { |x, y| artist = y }
      end
    end
    
    if title
      if artist
        @title.set_text("#{artist} - #{title}")
      else
        @title.set_text("#{title}")
      end
    else
      if artist
        @title.set_text("#{artist} - No Title")
      else
        @title.set_text('No Title')
      end
    end
    
    true
  end
end

def set_status(status)
  # 0: 停止/daemon いない
  # 1: 再生
  # 2: 一時停止
  case status
  when 0
    @icon.set_icon_name('gtk-media-stop')
    @playing = false
  when 1
    @icon.set_icon_name('gtk-media-play')
    @playing = true
  else
    @icon.set_icon_name('gtk-media-pause')
    @playing = false
  end
  @layout.move(@icon, @width - @icon.allocation.width, @height - @icon.allocation.height)
end

def set_playtime(msec)
  sec = msec / 1000
  mm = sec / 60
  ss = sec % 60
  @playtime.set_text(sprintf('%d:%02d', mm, ss))
  @layout.move(@playtime, 0, @height - @playtime.allocation.height)
end

def connect_xmms
  return true if @xmms
  
  begin
    @xmms = Xmms::Client.new('mxmms')
    @xmms.connect(ENV['XMMS_PATH'])
  rescue
    if @autostart
      system('xmms2-launcher')
    end
    @xmms = nil
    return true
  end

  @xmms.broadcast_medialib_entry_changed.notifier do |res|
    set_title(res)
    Playlist.set_active_menuitem(res)
    true
  end
  
  @xmms.broadcast_playback_status.notifier do |res|
    set_status(res)
    true
  end

  @xmms.broadcast_playlist_loaded.notifier do |res|
    Playlist.update_list(@xmms, @menuitem_music)
    true
  end

  @xmms.broadcast_playlist_changed.notifier do |res|
    Playlist.update_list(@xmms, @menuitem_music)
    true
  end
  
  @xmms.signal_playback_playtime.notifier do |res|
    set_playtime(res)
    true
  end

  @xmms.on_disconnect do
    @xmms = nil
  end
  
  Playlist.update_list(@xmms, @menuitem_music)
  Playlists.update_list(@xmms, @menuitem_list)
  
  @xmms.playback_current_id.notifier do |res|
    set_title(res)
    Playlist.set_active_menuitem(res)
    true
  end
  
  @xmms.playback_status.notifier do |res|
    set_status(res)
    true
  end
  
  @xmms.playback_playtime.notifier do |res|
    set_playtime(res)
    true
  end
  
  @xmms.add_to_glib_mainloop

  true
end

toplevel = Gtk::Window.new
toplevel.set_wmclass('mxmms', 'MXmms')

#

menu = Gtk::Menu.new

item = Gtk::MenuItem.new('Previous')
item.signal_connect('activate') do
  if @xmms
    @xmms.playback_playtime.notifier do |res|
      if res < 3000
        @xmms.playlist_set_next_rel(-1).notifier do |res|
          @xmms.playback_tickle.notifier do |res|
          end
        end
      else
        @xmms.playback_tickle.notifier do |res|
        end
      end
    end
  end
end
menu.append(item)
item.show

item = Gtk::MenuItem.new('Next')
item.signal_connect('activate') do
  if @xmms
    @xmms.playlist_set_next_rel(1).notifier do |res|
      @xmms.playback_tickle.notifier do |res|
      end
    end
  end
end
menu.append(item)
item.show

@menuitem_list = Gtk::MenuItem.new('Playlists')
menu.append(@menuitem_list)
@menuitem_list.show

@menuitem_music = Gtk::MenuItem.new('Music')
menu.append(@menuitem_music)
@menuitem_music.show

#

button = Gtk::Button.new
button.set_size_request(@width, @height)
button.signal_connect('clicked') do
  if @xmms
    if @playing
      @xmms.playback_pause.notifier do |res|
      end
    else
      @xmms.playback_start.notifier do |res|
      end
    end
  end
end
button.signal_connect('button-press-event') do |w, ev|
  if ev.button == 3
    menu.popup nil, nil, ev.button, ev.time
  end
  false
end
toplevel.add(button)

#

@layout = Gtk::Layout.new
button.add(@layout)

#

@title = Gtk::Label.new('.')
@layout.put(@title, @width, 0)

title_x = @width
GLib::Timeout.add(50) do
  title_x -= 1
  if title_x < -@title.allocation.width
    title_x = @width
  end
  @layout.move(@title, title_x, 0)
  true
end

#

@playtime = Gtk::Label.new('0:00')
@layout.put(@playtime, 0, 0)

#

@icon = Gtk::Image.new(:icon_name => 'gtk-media-pause', :size => :small_toolbar)
@layout.put(@icon, 0, 0)

#

button.show_all
if @geometry
  toplevel.parse_geometry(@geometry)
end
toplevel.show

#

connect_xmms
GLib::Timeout.add(1000) do
  connect_xmms
end

Gtk.main
