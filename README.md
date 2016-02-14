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

## License

GPL3. See COPYING.

## Author

Yuuki Harano &lt;masm@masm11.ddo.jp&gt;
