#!/usr/bin/env ruby
# coding: utf-8

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
require 'mxmms/version'

class Gui

  def initialize(geometry, width, height)
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
    if geometry
      @window.parse_geometry geometry
    end
    
    GLib::Timeout.add(50) do
      @x -= 1
      if @x < -@title.allocation.width
        @x = @layout.allocation.width
      end
      @layout.move @title, @x, 0
      true
    end
    
    # set_current_pos 0
    set_status 0
    set_playtime 0
    
    @button.signal_connect('clicked') do
      @playpause_handler.call
    end
    
    @button.signal_connect('button-press-event') do |w, ev|
      if ev.button == 3
        @menu.popup nil, nil, ev.button, ev.time
      end
      false
    end
    
    @menu = Gtk::Menu.new
    
    menuitem = Gtk::MenuItem.new 'Previous'
    menuitem.signal_connect('activate') do
      if @current_playtime < 3
        skip -1
      else
        skip 0
      end
    end
    @menu.append menuitem
    menuitem.show
    
    menuitem = Gtk::MenuItem.new 'Next'
    menuitem.signal_connect('activate') do
      skip 1
    end
    @menu.append menuitem
    menuitem.show
    
    @menuitem_list = Gtk::MenuItem.new 'Playlists'
    @menu.append @menuitem_list
    @menuitem_list.show
    
    @menuitem_music = Gtk::MenuItem.new 'Musics'
    @menu.append @menuitem_music
    @menuitem_music.show
    
    menuitem = Gtk::MenuItem.new 'About'
    menuitem.signal_connect('activate') do
      about = Gtk::AboutDialog.new
      about.program_name = 'MXMMS'
      about.version = VERSION
      about.authors = ['Yuuki Harano <masm@masm11.ddo.jp>']
      about.copyright = 'Copyright (C) 2014,2015 Yuuki Harano'
      about.comments = 'XMMS2 client with a large play/pause button.'
      about.license = 'GNU General Public License Version 3.'
      about.wrap_license = true
      about.transient_for = @window
      about.modal = true
      about.signal_connect('response') do
        about.destroy
        about = nil
      end
      about.show
    end
    @menu.append menuitem
    menuitem.show
  end
  
  def set_current_pos(pos)
p "set_current_pos: pos=#{pos}"
    artist = nil
    title = nil
    if @playlist && @playlist[pos]
      artist = @playlist[pos][:artist]
      title = @playlist[pos][:title]
    end
    
    if title
      if artist
        str = "#{artist} - #{title}"
      else
        str = "#{title}"
      end
    else
      str = 'No Title'
    end
    
    @current_pos = pos
    @title.set_text str
    @x = 0
    
    if @playlist && @playlist[pos]
      @playlist[pos][:menuitem].active = true
    end
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
    @current_playtime = sec
    mm = sec / 60
    ss = sec % 60
    @playtime.set_text sprintf('%d:%02d', mm, ss)
    @layout.move @playtime, 0, @layout.allocation.height - @playtime.allocation.height
  end
  
  @playlist = []
  def set_playlist(name, list)
    first_item = nil
    submenu = Gtk::Menu.new
    pos = 0
    
    list.each do |e|
      print "#{pos}: #{e[:id]}: #{e[:artist]} #{e[:title]}\n"
      item = Gtk::RadioMenuItem.new first_item, e[:title] || 'No Title'
      first_item = item unless first_item
      e[:menuitem] = item
      item.show
      submenu.add item
      item.active = (pos == @current_pos)
      item.signal_connect('activate', pos) do |w, pos|
        if w.active? && pos != @current_pos
          print "jump: pos=#{pos}\n"
          @jump_pos_handler.call pos
        end
      end
      pos += 1
    end
    
    if @playlist_list[name]
      @playlist_list[name].active = true
    end
    
    @menuitem_music.set_submenu submenu
    @playlist = list
p "set current_playlist: #{name}"
    @current_playlist = name
    set_current_pos @current_pos
  end
  
  def skip(dir)
    newpos = @current_pos
    case dir
    when -1
      newpos -= 1
      if newpos < 0
        newpos = @playlist.length - 1
      end

    when 1
      newpos += 1
      if newpos >= @playlist.length
        newpos = 0
      end

    end

    if newpos >= 0 && newpos < @playlist.length
      @jump_pos_handler.call newpos
    end
  end

  @playlist_list = {}
  @submenu_list = nil
  def set_playlist_list(list)
    need_update = false
    new_size = list.size
    old_size = @playlist_list ? @playlist_list.size : 0
    if new_size == old_size
      # 個数が同じなら、同じかもしれない。中身を逐一確認する。
      list.each do |e|
        need_update = true unless @playlist_list[e]
      end
    else
      # 個数が違うなら、更新する
      need_update = true
    end
    return unless need_update

    @playlist_list = {}
    first_item = nil
    submenu = Gtk::Menu.new
    
    list.each do |e|
      print "#{e}\n"
      item = Gtk::RadioMenuItem.new first_item, e || 'No Title'
      first_item = item unless first_item
      item.show
      submenu.add item
      item.active = (e == @current_playlist)
      item.signal_connect('activate', e) do |w, name|
p "activate: #{w.active?}, #{name}, #{@current_playlist}"
        # 起動時、@current_playlist が取得できていない段階で呼ばれ、
        # playlist が切り替わったものと判断されてしまう。
        # それを避けるため、@current_playlist が nil でないことも
        # 条件に入れておく。
        if w.active? && @current_playlist && name != @current_playlist
          # print "jump: pos=#{pos}\n"
p 'call change_playlist_handler'
          @change_playlist_handler.call name
        end
      end
      @playlist_list[e] = item
    end
    
    if @list_submenu
      @list_submenu.destroy
    end
    @menuitem_list.set_submenu submenu
    @list_submenu = submenu
    # set_current_pos @current_pos
  end
  
  def set_playpause_handler(&handler)
    @playpause_handler = handler
  end
  
  def set_jump_pos_handler(&handler)
    @jump_pos_handler = handler
  end
  
  def set_change_playlist_handler(&handler)
    @change_playlist_handler = handler
  end

  def main
    Gtk.main
  end
end
