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
  
  def initialize
    GLib::Timeout.add(1000) do
      connect_xmms
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
  
  def set_playlist_changed_handler(&handler)
    @playlist_changed_handler = handler
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
    
    # artist, title
    @xmms.broadcast_medialib_entry_changed.notifier do |res|
      get_title_from_id(res) do |id, artist, title|
        @playback_entry_changed_handler.call id, artist, title
      end
      true
    end
    
    @xmms.playback_current_id.notifier do |res|
      get_title_from_id(res) do |id, artist, title|
        @playback_entry_changed_handler.call id, artist, title
      end
      true
    end
    
    # playlist が切り替わった
    @xmms.broadcast_playlist_loaded.notifier do |res|
      # fixme: playlist menu の active を変更
      p res  # encoding がおかしい気がする...
      get_playlist do |list|
        @playlist_loaded_handler.call list
      end
      true
    end
    
    # playlist が編集された
    @xmms.broadcast_playlist_changed.notifier do |res|
      p res
      get_playlist do |list|
        @playlist_changed_handler.call list
      end
      true
    end
    
    get_playlist do |list|
      @playlist_loaded_handler.call list
    end
    
    
    @xmms.add_to_glib_mainloop
    true
  end

  # id から該当曲の artist と title を取得する。
  def get_title_from_id(id, &block)
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
      
      block.call id, artist, title
      true
    end
  end
  
  # get_playlist から使われるメソッド。
  def get_playlist_iter(list, ids, id, artist, title, block)
    entry = {
      :id => id,
      :artist => artist,
      :title => title,
    }
    list << entry
    
    if ids.size == 0
      block.call list
    else
      id = ids.shift
      get_title_from_id(id) do |id, artist, title|
        get_playlist_iter list, ids, id, artist, title, block
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
        get_title_from_id(id) do |id, artist, title|
          get_playlist_iter list, ids, id, artist, title, block
        end
      end
      
      true
    end
  end

end