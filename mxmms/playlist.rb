#!/usr/bin/env ruby

module Playlist
  @parent_menuitem = nil
  @ids = []
  @id_to_menuitem = {}
  @current_id = nil
  @pos = 0
  
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
      end
    end
  end
  
  def update_list(xmms, parent_menuitem)
    @parent_menuitem = parent_menuitem
    @id_to_menuitem = {}
    
    @submenu = Gtk::Menu.new
    @first_menuitem = nil
    @pos = 0
    
    pl = xmms.playlist
    pl.entries.notifier do |res|
      @ids = res
      
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
