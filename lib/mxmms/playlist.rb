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

module Playlist
  @parent_menuitem = nil
  @ids = []
  @id_to_menuitem = {}
  @current_id = nil
  @pos = 0
  @updating = false
  @need_update = false
  
  def update_list_iter(res, xmms)
    if xmms
      title = nil
      id = nil
      if res
        r = res[:title]
        if r
          r.each_pair { |x, y| title = y }
        end
        
        r = res[:id]
        if r
          r.each_pair { |x, y| id = y }
        end
      end
      
      item = Gtk::RadioMenuItem.new(@first_menuitem, title || 'No Title')
      @first_menuitem = item unless @first_menuitem
      item.show
      @submenu.add(item)
      @id_to_menuitem[id.to_s] = item
      item.active = (id == @current_id)
      item.signal_connect('activate', @pos, id) do |w, pos, id|
        if w.active? && id != @current_id
          xmms.playlist_set_next(pos).notifier do |res|
            xmms.playback_tickle.notifier do |res|
            end
          end
        end
      end
      @pos += 1
      
      id = @ids.shift
      if id
        xmms.medialib_get_info(id).notifier do |res|
          Playlist.update_list_iter res, xmms
          true
        end
      else
        @parent_menuitem.set_submenu @submenu
        
        @updating = false
        if @need_update
          @need_update = false
          update_list xmms, @parent_menuitem
        end
      end
    end
  end
  
  def update_list(xmms, parent_menuitem)
    if @updating
      @need_update = true
      return
    end
    @updating = true
    
    @parent_menuitem = parent_menuitem
    @id_to_menuitem = {}
    
    @submenu = Gtk::Menu.new
    @first_menuitem = nil
    @pos = 0
    
    pl = xmms.playlist
    pl.entries.notifier do |res|
      @ids = res
      
      if @ids.size == 0
        @updating = false
        next
      end
      
      id = @ids.shift
      if id
        xmms.medialib_get_info(id).notifier do |res|
          Playlist.update_list_iter res, xmms
          true
        end
      end
      true
    end
  end
  
  def set_active_menuitem(id)
    @current_id = id
    item = @id_to_menuitem[id.to_s]
    item.active = true if item
  end

  module_function :update_list
  module_function :update_list_iter
  module_function :set_active_menuitem
end
