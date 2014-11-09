#!/usr/bin/env ruby

require 'gtk3'
require 'xmmsclient'
require 'xmmsclient_glib'

def connect_xmms
  begin
    @xmms = Xmms::Client.new('mxmms')
    @xmms.connect(ENV['XMMS_PATH'])
  rescue
    @xmms = nil
    Kernel.sleep(1)
    retry
  end
  
  @xmms.broadcast_medialib_entry_changed.notifier do |res|
    @xmms.medialib_get_info(res).notifier do |res2|
      title = nil
      artist = nil
      
      r = res2[:title]
      if r
        r.each_pair { |x, y| title = y }
      end
      
      r = res2[:artist]
      if r
        r.each_pair { |x, y| artist = y }
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
    true
  end
  
  @xmms.broadcast_playback_status.notifier do |res|
    if res == 2
      @icon.set_icon_name('gtk-media-pause')
    else
      @icon.set_icon_name('gtk-media-play')
    end
    @layout.move(@icon, 48 - @icon.allocation.width, 48 - @icon.allocation.height)
    
    true
  end
  
  @xmms.signal_playback_playtime.notifier do |res|
    sec = res / 1000
    mm = sec / 60
    ss = sec % 60
    @playtime.set_text(sprintf('%d:%02d', mm, ss))
    @layout.move(@playtime, 0, 48 - @playtime.allocation.height)
    true
  end

  @xmms.on_disconnect do
    puts 'disconnected.'
    @xmms = nil
    connect_xmms
  end
  
  @xmms.add_to_glib_mainloop
end

toplevel = Gtk::Window.new

@layout = Gtk::Layout.new
@layout.set_size_request(48, 48)
toplevel.add(@layout)

#

@title = Gtk::Label.new('.')
@layout.put(@title, 0, 0)

title_x = 0
GLib::Timeout.add(50) do
  title_x -= 1
  if title_x < -@title.allocation.width
    title_x = 48
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

toplevel.show_all

connect_xmms

Gtk.main
