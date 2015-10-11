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
  
  def set_playback_status_changed_handler(&handler)
    @playback_status_changed_handler = handler
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
      @playback_status_changed_handler.call(res)
      true
    end
    
    @xmms.add_to_glib_mainloop
    true
  end

end
