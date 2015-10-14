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

module Playlists
  @parent_menuitem = nil
  @names = []
  @name_to_menuitem = {}
  @current_list = nil

  def update_list(xmms, parent_menuitem)
    @parent_menuitem = parent_menuitem
    @name_to_menuitem = {}

    @submenu = Gtk::Menu.new
    @first_menuitem = nil

    xmms.playlist_list.notifier do |res|
      @names = res.sort
      for name in @names
        item = Gtk::RadioMenuItem.new(@first_menuitem, name)
        @first_menuitem = item unless @first_menuitem
        item.show
        @submenu.add(item)
        @name_to_menuitem[name] = item
        item.active = (name == @current_list)
        item.signal_connect('activate', name) do |w, name|
          if w.active? && name != @current_list
            pl = xmms.playlist(name)
            pl.load.notifier do |res|
              true
            end
          end
        end
      end
      @parent_menuitem.set_submenu @submenu
      true
    end
  end
  
  def set_active_menuitem(name)
    @current_list = name
    item = @name_to_menuitem[name]
    item.active = true if item
  end

  module_function :update_list
  module_function :set_active_menuitem
end
