#!/usr/bin/env ruby

require 'gtk3'
require 'xmmsclient'
require 'xmmsclient_glib'

@playing = false

@geometry = nil
@width = 48
@height = 48

i = 0
while i < ARGV.length
  case ARGV[i]
  when '--geometry'
    @geometry = ARGV[i + 1]
    i += 2
  when '--width'
    @width = ARGV[i + 1].to_i
    i += 2
  when '--height'
    @height = ARGV[i + 1].to_i
    i += 2
  else
    $stderr.puts('unknown args.')
    exit 1
  end
end

def set_title(id)
  @xmms.medialib_get_info(id).notifier do |res|
    title = nil
    artist = nil
    
    r = res[:title]
    if r
      r.each_pair { |x, y| title = y }
    end
    
    r = res[:artist]
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
end

def set_status(status)
  if status == 2
    @icon.set_icon_name('gtk-media-pause')
    @playing = false
  else
    @icon.set_icon_name('gtk-media-play')
    @playing = true
  end
  @layout.move(@icon, @width - @icon.allocation.width, @height - @icon.allocation.height)
end

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
    set_title(res)
    true
  end
  
  @xmms.broadcast_playback_status.notifier do |res|
    set_status(res)
    true
  end
  
  @xmms.signal_playback_playtime.notifier do |res|
    sec = res / 1000
    mm = sec / 60
    ss = sec % 60
    @playtime.set_text(sprintf('%d:%02d', mm, ss))
    @layout.move(@playtime, 0, @height - @playtime.allocation.height)
    true
  end

  @xmms.on_disconnect do
    puts 'disconnected.'
    @xmms = nil
    connect_xmms
  end
  
  @xmms.playback_current_id.notifier do |res|
    set_title(res)
    true
  end
  
  @xmms.playback_status.notifier do |res|
    set_status(res)
    true
  end
  
  @xmms.add_to_glib_mainloop
end

toplevel = Gtk::Window.new
if @geometry
  toplevel.parse_geometry(@geometry)
end

#

button = Gtk::Button.new
button.set_size_request(@width, @height)
button.signal_connect('clicked') do
  if @playing
    @xmms.playback_pause.notifier do |res|
    end
  else
    @xmms.playback_start.notifier do |res|
    end
  end
end
toplevel.add(button)

#

@layout = Gtk::Layout.new
button.add(@layout)

#

@title = Gtk::Label.new('.')
@layout.put(@title, @width, 0)

title_x = @width
GLib::Timeout.add(50) do
  title_x -= 1
  if title_x < -@title.allocation.width
    title_x = @width
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
