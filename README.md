日本語は README を参照してください。

# mxmms - Masm's XMMS2 Client

Many apps have small play and pause buttons,
so I create an app with a large button.

The title, playtime, and status is shown on the button.

## Requirements

 - XMMS2 0.8 DrO_o
 - MATE

## Installation

```sh
make -C src all
sudo cp src/mxmms /usr/lib/mate-applets/
sudo cp org.mate.panel.applet.MxmmsAppletFactory.service /usr/share/dbus-1/services/
sudo cp org.mate.panel.MxmmsApplet.mate-panel-applet /usr/share/mate-panel/applets/
```

## Usage

Add an applet to the panel.

The shown applet is a button, so you can click it to play/pause.

You can click it with the right button to show the menu. Next and
Previous jump to the beginning of a music. Previous jumps to the
beginning of the music if more than 3 seconds have passed since the
beginning of the music, otherwise to the beginning of the previous
music.

You can select Others... to do other operation. In Title List page,
you can jump to another music by double click on the music title. In
Playlists page, you can switch to another playlist by double click on
the playlist name. Clicking the radio buttons do not work.

## Bugs

This program does not behave properly when you change the orientation
of the panel.

## License

GPL3. See COPYING.

## Author

Yuuki Harano &lt;masm@masm11.ddo.jp&gt;
