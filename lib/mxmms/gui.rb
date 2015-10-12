#!/usr/bin/env ruby

# mxmms - Masm's XMMS2 Client
# Copyright (C) 2014,2015 Yuuki Harano
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

class Gui

  def initialize(width, height)
    @x = 0
    
    @window = Gtk::Window.new
    @window.set_wmclass 'mxmms', 'MXmms'
    @window.decorated = false
    @window.type_hint = :dock
    
    @button = Gtk::Button.new
    @window.add @button
    @button.set_size_request width, height
    
    @layout = Gtk::Layout.new
    @button.add @layout
    
    @title = Gtk::Label.new '.'
    @layout.put @title, 0, 0
    
    @playtime = Gtk::Label.new '0:00'
    @layout.put @playtime, 0, 0
    
    @icon = Gtk::Image.new :icon_name => 'gtk-media-pause',
                           :size => :small_toolbar
    @layout.put @icon, 0, 0
    
    @window.show_all
    
    GLib::Timeout.add(50) do
      @x -= 1
      if @x < -@title.allocation.width
        @x = @layout.allocation.width
      end
      @layout.move @title, @x, 0
      true
    end
    
    set_title nil, nil
    set_status 0
    set_playtime 0
    
    @button.signal_connect('clicked') do
      @playpause_handler.call
    end
    
    @button.signal_connect('button-press-event') do |w, ev|
      if ev.button == 3
        print "button3 pressed\n"
      end
      false
    end
    
  end
  
  def set_title title, artist
    if title
      if artist
        str = "#{artist} - #{title}"
      else
        str = "#{title}"
      end
    else
      if artist
        str = "#{artist} - No Title"
      else
        str = 'No Title'
      end
    end
    
    @title.set_text str
    @x = 0
  end
  
  def set_status status
    case status
    when 0
      @icon.set_icon_name 'gtk-media-stop'
    when 1
      @icon.set_icon_name 'gtk-media-play'
    else
      @icon.set_icon_name 'gtk-media-pause'
    end
    @layout.move @icon, @layout.allocation.width - @icon.allocation.width, @layout.allocation.height - @icon.allocation.height
  end
  
  def set_playtime sec
    mm = sec / 60
    ss = sec % 60
    @playtime.set_text sprintf('%d:%02d', mm, ss)
    @layout.move @playtime, 0, @layout.allocation.height - @playtime.allocation.height
  end
  
  def set_playlist list
    list.each do |e|
      print "#{e[:id]} #{e[:artist]} #{e[:title]}\n"
    end
  end
  
  def set_playlist_list
  end
  
  def set_playpause_handler(&handler)
    @playpause_handler = handler
  end
  
  def set_change_music_handler(&handler)
    @change_music_handler = handler
  end
  
  def set_change_playlist_handler
  end

  def set_next_handler
  end

  def set_prev_handler
  end

  def main
    Gtk.main
  end
end
