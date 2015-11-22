日本語は README を参照してください。

# mxmms - Masm's XMMS2 Client

Many apps have small play and pause buttons,
so I create an app with a large button.

The title, playtime, and status is shown on the button.

## Installation

At first, you need xmms2 ruby binding, so install it.

After that, do:

```sh
make install
```

## Usage

Run:

```sh
mxmms
```

The shown window is a button, so you can click it to play/pause.

When you click it with the right button, a menu pops up and
you can change music or playlist, or seek.

For seek, seekbar is shown, and you can seek with it.
When you completed, the seekbar disappears when you click outside the app.

The following options are available:

 - --geometry &lt;position&gt;
 - --width &lt;width&gt;
 - --height &lt;height&gt;
 - --autostart

## Other Settings

mxmms reads and executes ~/.mxmms/rc.rb, so you can do a few configuration.

Configure playlist format:

```ruby
class Config
  def music_list_handler(pos, id, artist, title)
    "#{pos + 1}. #{artist} - #{title}"
  end
end
```

Configure strings on the main button.

```ruby
class Config
  def main_title_handler(pos, id, artist, title)
    "#{artist} - #{title}"
  end
end
```

These two methods accept arguments:

 - pos: position in the playlist. 0 for first music.
 - id: music id.
 - artist: artist or nil.
 - title: music title or nil.

Configure repeat mode.

```ruby
GLib::Timeout.add(2000) do
  @backend.set_repeat_mode :all
  false
end
```

You can pass one of :none, :one, and :all.
I'm sorry for the timing issue.

## License

GPL3. See COPYING.

## Author

Yuuki Harano &lt;masm@masm11.ddo.jp&gt;
