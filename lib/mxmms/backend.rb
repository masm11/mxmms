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
      get_title_from_id(res) do |artist, title|
        @playback_entry_changed_handler.call artist, title
      end
      true
    end
    
    @xmms.playback_current_id.notifier do |res|
      get_title_from_id(res) do |artist, title|
        @playback_entry_changed_handler.call artist, title
      end
      true
    end
    
    
    @xmms.add_to_glib_mainloop
    true
  end

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
      
      block.call artist, title
    end
  end

end
