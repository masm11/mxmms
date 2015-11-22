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

require 'xmmsclient'
require 'xmmsclient_glib'

class Backend
  
  def initialize autostart
    @autostart = autostart
    
    GLib::Timeout.add(1000) do
      connect_xmms
      true
    end
  end
  
  def playback_toggle
    if @last_playback_status == 1
      @xmms.playback_pause.notifier do |res|
      end
    else
      @xmms.playback_start.notifier do |res|
      end
    end
  end
  
  def seek(sec)
    @xmms.playback_seek_ms(sec.to_i * 1000).notifier do |res|
    end
  end

  def set_playback_status_changed_handler(&handler)
    @playback_status_changed_handler = handler
  end
  
  def set_playback_playtime_changed_handler(&handler)
    @playback_playtime_changed_handler = handler
  end
  
  def set_playback_entry_changed_handler(&handler)
    @playback_entry_changed_handler = handler
  end
  
  def set_playlist_loaded_handler(&handler)
    @playlist_loaded_handler = handler
  end
  
  def set_playlist_list_retr_handler(&handler)
    @playlist_list_retr_handler = handler
  end
  
  def set_repeat_mode_handler(&handler)
    @repeat_mode_handler = handler
  end

  def jump_pos(pos)
    @xmms.playlist_set_next(pos).notifier do |res|
      @xmms.playback_tickle.notifier do |res|
      end
    end
  end
  
  def change_playlist(name)
    p 'change_playlist: ' + name
    @xmms.playlist(name).load.notifier do |res|
      # プレイリストを切り替えたら先頭から再生
      jump_pos 0
      true
    end
  end

  def set_repeat_mode(mode)
    case mode
    when :none
      one = '0'
      all = '0'
    when :one
      one = '1'
      all = '0'
    when :all
      one = '0'
      all = '1'
    end
    @xmms.config_set_value('playlist.repeat_one', one).notifier do |res|
      @xmms.config_set_value('playlist.repeat_all', all).notifier do |res|
      end
    end
  end

  private
  
  def connect_xmms
    return if @xmms
    
    begin
      @xmms = Xmms::Client.new 'mxmms'
      @xmms.connect(ENV['XMMS_PATH'])
    rescue
      if @autostart
        system('xmms2-launcher')
      end
      @xmms = nil
      return true
    end
    
    @xmms.on_disconnect do
      @xmms = nil
    end
    
    # 再生/停止/一時停止
    @xmms.broadcast_playback_status.notifier do |res|
      @last_playback_status = res
      @playback_status_changed_handler.call(res)
      true
    end
    
    @xmms.playback_status.notifier do |res|
      @last_playback_status = res
      @playback_status_changed_handler.call(res)
      true
    end
    
    # 再生時間
    @xmms.signal_playback_playtime.notifier do |res|
      @playback_playtime_changed_handler.call(res)
      true
    end
    
    @xmms.playback_playtime.notifier do |res|
      @playback_playtime_changed_handler.call(res)
      true
    end
    
    # 曲が変わった。
    @xmms.broadcast_medialib_entry_changed.notifier do |res|
      @xmms.playlist.current_pos.notifier do |res|
        @playback_entry_changed_handler.call res[:position]  # 0-base
      end
      true
    end
    
    @xmms.playlist.current_pos.notifier do |res|
      @playback_entry_changed_handler.call res[:position]
      true
    end
    
    # playlist が編集された
    @xmms.broadcast_playlist_changed.notifier do |res|
      # どの playlist が編集されたかは置いといて、
      # 現在の playlist を取得する。
      get_playlist do |list|
        @playlist_loaded_handler.call nil, list
      end
      true
    end

    # playlist が切り替わった
    @xmms.broadcast_playlist_loaded.notifier do |res|
      name = res
      p name
      get_playlist do |list|
        @playlist_loaded_handler.call name, list
      end
      true
    end
    
    @xmms.playlist_current_active.notifier do |res|
      name = res
      get_playlist do |list|
        @playlist_loaded_handler.call name, list
      end
    end
    
    get_playlist_list do |list|
      @playlist_list_retr_handler.call list
    end
    # playlist list を取得
    GLib::Timeout.add(1000) do
      rv = false
      if @xmms
        get_playlist_list do |list|
          @playlist_list_retr_handler.call list
        end
        rv = true
      end
      rv
    end
    
    # config が変わった
    @xmms.broadcast_config_value_changed.notifier do |res|
      get_repeat_mode do |mode|
        @repeat_mode_handler.call mode
      end
    end
    
    get_repeat_mode do |mode|
      @repeat_mode_handler.call mode
    end
    
    @xmms.add_to_glib_mainloop
    true
  end

  # id から該当曲の artist と title を取得する。
  def get_title_from_id(id, &block)
    @xmms.medialib_get_info(id).notifier do |res|
      title = nil
      artist = nil
      duration = 0
      
      if res
        r = res[:title]
        if r
          r.each_pair { |x, y| title = y }
        end
        
        r = res[:artist]
        if r
          r.each_pair { |x, y| artist = y }
        end
        
        r = res[:duration]
        if r
          r.each_pair { |x, y| duration = y }
        end
      end
      
      block.call id, artist, title, duration
      true
    end
  end
  
  # get_playlist から使われるメソッド。
  def get_playlist_iter(list, ids, id, artist, title, duration, block)
    entry = {
      :id => id,
      :artist => artist,
      :title => title,
      :duration => duration,
    }
    list << entry
    
    if ids.size == 0
      block.call list
    else
      id = ids.shift
      get_title_from_id(id) do |id, artist, title, duration|
        get_playlist_iter list, ids, id, artist, title, duration, block
      end
    end
  end
  
  # playlist を取得する。
  # 取得が終わったら、callback として block が呼ばれる。
  def get_playlist(&block)
    @xmms.playlist.entries.notifier do |res|
      ids = res
      list = []
      
      if ids.size == 0
        block.call list
      else
        id = ids.shift
        get_title_from_id(id) do |id, artist, title, duration|
          get_playlist_iter list, ids, id, artist, title, duration, block
        end
      end
      
      true
    end
  end

  def get_playlist_list(&block)
    @xmms.playlist_list.notifier do |res|
      block.call res.sort
      true
    end
  end

  def get_repeat_mode(&block)
    @xmms.config_get_value('playlist.repeat_one').notifier do |res|
      if res != '0'
        block.call :one
      else
        @xmms.config_get_value('playlist.repeat_all').notifier do |res|
          if res != '0'
            block.call :all
          else
            block.call :none
          end
          true
        end
      end
      true
    end
  end

end
